#include "ircprotoclient.h"

#include <QMetaEnum>
#include <QtNetwork>

IRCProtoClient::IRCProtoClient(QObject *parent) : QObject(parent),
    socket(new QTcpSocket(this)),
    socketReadBuf(10*1024),
    socketReadBufUsed(0),
    _connectionState(ConnectionState::Disconnected)
{
    // Set up signals & slots.
    connect(socket, &QAbstractSocket::connected,
            this, &IRCProtoClient::handle_socket_connected);
    typedef void (QAbstractSocket::*error_signal_type)(QAbstractSocket::SocketError);
    connect(socket, static_cast<error_signal_type>(&QAbstractSocket::error),
            this, &IRCProtoClient::handle_socket_error);
    connect(socket, &QIODevice::readyRead, this, &IRCProtoClient::processIncomingData);
}

void IRCProtoClient::disconnectFromIRCServer(const QString &quitMsg)
{
    if (connectionState() == ConnectionState::Disconnected) {
        notifyUser("Already disconnected.");
        return;
    }

    if (connectionState() >= ConnectionState::Registering) {
        if (quitMsg.isNull()) {
            notifyUser("Sending quit request to server...");
            sendRaw("QUIT");
        }
        else {
            notifyUser("Sending quit request (message: " + quitMsg + ") to server...");
            sendRaw("QUIT :" + quitMsg);
        }
    }

    // Make sure nothing stays queued from the old connection,
    // or it would probably be misdirected to a new connection...
    sendQueue.clear();

    notifyUser("Aborting connection...");
    socket->abort();
    _setConnectionState(ConnectionState::Disconnected);
}

void IRCProtoClient::disconnectFromIRCServer()
{
    return disconnectFromIRCServer(QString());
}

void IRCProtoClient::connectToIRCServer(const QString &host, const QString &port, const QString &user, const QString &nick)
{
    // TODO: Do sanity checks on user-supplied data.

    if (connectionState() != ConnectionState::Disconnected)
        disconnectFromIRCServer();

    notifyUser("Setting host:port to request next to " + host + ":" + port +
               ", user to " + user + ", nick to " + nick + ".");
    _hostRequestNext = host;
    _portRequestNext = port;
    _userRequestNext = user;
    _nickRequestNext = nick;

    reconnectToIRCServer();
}

void IRCProtoClient::reconnectToIRCServer()
{
    if (connectionState() != ConnectionState::Disconnected)
        disconnectFromIRCServer();

    QString host = _hostRequestNext, port = _portRequestNext;
    // Clear these to avoid suggesting wrong destination
    // to signal connectionStateChanged handlers.
    _hostRequestedLast.clear();
    _portRequestedLast.clear();
    notifyUser("(Re)Connecting to " + host + ":" + port);
    _setConnectionState(ConnectionState::Connecting);
    socket->connectToHost(host, port.toShort());
    // Now that we are actually connecting to there,
    // update these again and emit signal about it.
    _hostRequestedLast = host;
    _portRequestedLast = port;
    hostPortRequestedLastChanged();
}

void IRCProtoClient::handle_socket_connected()
{
    _setConnectionState(ConnectionState::Registering);

    QString user = _userRequestNext;
    notifyUser("Registering as user " + user + "...");
    // "USER" USERNAME HOSTNAME SERVERNAME REALNAME
    // TODO: Allow setting realname.
    sendRaw("USER " + user + " * * :a cvnirc-qt user");
    _userRequestedLast = user;
    userRequestedLastChanged();

    QString nick = _nickRequestNext;
    notifyUser("Requesting nick " + nick + "...");
    sendRaw("NICK " + nick);
    _nickRequestedLast = nick;
    nickRequestedLastChanged();
}

void IRCProtoClient::handle_socket_error(QAbstractSocket::SocketError err)
{
    notifyUser(QString("Socket error ") +
               QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(err) +
               ": " + socket->errorString());
    _setConnectionState(ConnectionState::Disconnected);
}

void IRCProtoClient::sendRaw(const QString &line)
{
    sendQueue.push_back(line);

    if (socket->state() == QAbstractSocket::ConnectedState)
        processOutgoingData();
}

void IRCProtoClient::processOutgoingData()
{
    while (sendQueue.size() > 0) {
        QString rawLine = sendQueue.front();
        sendingLine(rawLine);
        socket->write((rawLine + "\r\n").toUtf8());
        sendQueue.pop_front();
    }
}

void IRCProtoClient::processIncomingData()
{
    processOutgoingData();

    if (socketReadBufUsed >= socketReadBuf.size()) {
        notifyUser("Socket read buffer size exceeded, aborting connection.");
        socket->abort();
        return;
    }

    qint64 ret = 0;
    while ((ret = socket->read(socketReadBuf.data() + socketReadBufUsed,
                               socketReadBuf.size() - socketReadBufUsed)
            ) > 0) {
        socketReadBufUsed += ret;

        while (socketReadBufUsed > 0) {
            int state = 0;
            socketReadBuf_type::const_iterator usedEnd = socketReadBuf.cbegin() + socketReadBufUsed;
            socketReadBuf_type::const_iterator iter;

            for (iter = socketReadBuf.cbegin();
                 iter < usedEnd;
                 iter++)
            {
                switch (state) {
                case 0:
                    if (*iter == '\r') {
                        state++;
                        continue;
                    }
                    break;
                case 1:
                    if (*iter == '\n') {
                        state++;
                    }
                    else {
                        notifyUser("Protocol error: Server seems to have broken line-termination! Aborting connection.");
                        socket->abort();
                        return;
                    }
                    break;
                }

                if (state == 2) {
                    state = 0;
                    qint64 lineLen = iter - socketReadBuf.cbegin() - 1;
                    socketReadBuf[lineLen] = '\0';
                    QString line(socketReadBuf.data());

                    if (socketReadBufUsed >= lineLen + 2) {
                        socketReadBuf_type::iterator writeIter = socketReadBuf.begin();
                        socketReadBuf_type::const_iterator readIter = iter + 1;
                        while (usedEnd - readIter > 0)
                            *writeIter++ = *readIter++;
                        socketReadBufUsed -= lineLen + 2;
                    }

                    // Interpret messages.
                    receivedRaw(line);
                    break;
                }
            }

            if (iter >= usedEnd)
                break;
        }
    }

    if (ret < 0) {
        notifyUser("Error reading from network socket, aborting connection.");
        socket->abort();
        return;
    }
}

void IRCProtoClient::receivedRaw(const QString &rawLine)
{
    receivedLine(rawLine);

    std::vector<QString> tokens = IRCProtoMessage::splitRawLine(rawLine);
    if (!(tokens.size() >= 1)) {
        notifyUser("Protocol error, disconnecting: Received raw line with no tokens!");
        disconnectFromIRCServer("Protocol error");
        return;
    }

    IRCProtoMessage *msg = nullptr;
    QString prefix;

    if (tokens[0].length() >= 1 && tokens[0][0] == ':') {
        prefix = tokens[0];
        prefix.remove(0, 1);  // Strip prefix identifier from identified prefix.
        tokens.erase(tokens.begin());
    }

    if (!(tokens.size() >= 1)) {
        notifyUser("Protocol error, disconnecting: Received line with a prefix token only!");
        disconnectFromIRCServer("Protocol error");
        return;
    }

    if (tokens[0] == "PING") {
        if (!(tokens.size() == 2)) {
            notifyUser("Protocol error, disconnecting: Received PING message with unexpected token count " + QString::number(tokens.size()));
            disconnectFromIRCServer("Protocol error");
            return;
        }
        msg = new PingPongIRCProtoMessage(rawLine, prefix, tokens, IRCProtoMessage::MsgType::Ping, tokens[1]);
    }
    else if (tokens[0] == "001") {
        msg = new NumericIRCProtoMessage(rawLine, prefix, tokens, IRCProtoMessage::MsgType::Welcome, 1);
    }
    else if (tokens[0] == "JOIN") {
        if (!(tokens.size() >= 2 && tokens.size() <= 3)) {
            notifyUser("Protocol error, ignoring: Received JOIN message with unexpected token count " + QString::number(tokens.size()));
            return;
        }

        JoinIRCProtoMessage::channels_type channels = tokens[1].split(',');
        JoinIRCProtoMessage::keys_type keys = tokens.size() >= 3 ? tokens[2].split(',') : QStringList();
        msg = new JoinIRCProtoMessage(rawLine, prefix, tokens, IRCProtoMessage::MsgType::Join, channels, keys);
    }
    else if (tokens[0] == "PRIVMSG" || tokens[0] == "NOTICE") {
        if (!(tokens.size() == 3)) {
            notifyUser("Protocol error, ignoring: Received " + tokens[0] + " message with unexpected token count " + QString::number(tokens.size()));
            return;
        }

        auto msgType = IRCProtoMessage::MsgType::Unknown;
        if (tokens[0] == "PRIVMSG")
            msgType = IRCProtoMessage::MsgType::PrivMsg;
        else if (tokens[0] == "NOTICE")
            msgType = IRCProtoMessage::MsgType::Notice;

        msg = new ChatterIRCProtoMessage(rawLine, prefix, tokens, msgType, tokens[1], tokens[2]);
    }
    else {
        msg = new IRCProtoMessage(rawLine, prefix, tokens);
    }

    if (msg != nullptr) {
        receivedMessageAutonomous(*msg);
        receivedMessage(*msg);

        if (!msg->handled)
            notifyUser("Unhandled IRC protocol message (type " + QString::number((int)msg->msgType) +
                       ": " + QMetaEnum::fromType<IRCProtoMessage::MsgType>().valueToKey((int)msg->msgType) +
                       "): " + msg->mainTokens[0]);

        delete msg;
    }
}

void IRCProtoClient::receivedMessageAutonomous(IRCProtoMessage &msg)
{
    switch (msg.msgType) {
    case IRCProtoMessage::MsgType::Ping:
        {
            auto &pingMsg(static_cast<PingPongIRCProtoMessage &>(msg));
            sendRaw("PONG :" + pingMsg.target);
            pingMsg.handled = true;
        }
        break;
    case IRCProtoMessage::MsgType::Welcome:
        if (connectionState() != ConnectionState::Registering) {
            notifyUser("Protocol error, disconnecting: Got random Welcome/001 message");
            disconnectFromIRCServer("Protocol error");
            return;
        }

        notifyUser("Got welcome message; we're connected, now");
        _setConnectionState(ConnectionState::Connected);
        msg.handled = true;
        break;
    default:
        break;
    }
}

IRCProtoClient::ConnectionState IRCProtoClient::connectionState() const
{
    return _connectionState;
}

const QString &IRCProtoClient::hostRequestedLast() const
{
    return _hostRequestedLast;
}

const QString &IRCProtoClient::portRequestedLast() const
{
    return _portRequestedLast;
}

const QString &IRCProtoClient::userRequestedLast() const
{
    return _userRequestedLast;
}

const QString &IRCProtoClient::nickRequestedLast() const
{
    return _nickRequestedLast;
}
const QString &IRCProtoClient::hostRequestNext() const
{
    return _hostRequestNext;
}

const QString &IRCProtoClient::portRequestNext() const
{
    return _portRequestNext;
}

const QString &IRCProtoClient::userRequestNext() const
{
    return _userRequestNext;
}

const QString &IRCProtoClient::nickRequestNext() const
{
    return _nickRequestNext;
}

void IRCProtoClient::setHostRequestNext(const QString &host)
{
    notifyUser("Setting host to request next to \"" + host + "\".");
    _hostRequestNext = host;
}

void IRCProtoClient::setPortRequestNext(const QString &port)
{
    notifyUser("Setting port to request next to \"" + port + "\".");
    _portRequestNext = port;
}

void IRCProtoClient::setUserRequestNext(const QString &user)
{
    notifyUser("Setting user to request next to \"" + user + "\".");
    _userRequestNext = user;
}

void IRCProtoClient::setNickRequestNext(const QString &nick)
{
    notifyUser("Setting nick to request next to \"" + nick + "\".");
    _nickRequestNext = nick;
}

void IRCProtoClient::_setConnectionState(ConnectionState newState)
{
    if (_connectionState == newState)
        return;

    _connectionState = newState;
    connectionStateChanged();
}

QString IRCProtoClient::nickUserHost2nick(const QString &nickUserHost)
{
    QString tmp = nickUserHost;
    int posBang = tmp.indexOf('!', 1);
    int posAt   = tmp.indexOf('@', posBang > 0 ? posBang  + 1 : 1);
    int truncLen = posAt > 0 ? posAt : 0;
    if (posBang > 0 && posBang < truncLen)
        truncLen = posBang;
    if (truncLen < 1)
        // Failed. Just use the full information.
        return tmp;
    tmp.truncate(truncLen);
    return tmp;
}
