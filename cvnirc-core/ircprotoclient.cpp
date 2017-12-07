#include "ircprotoclient.h"

#include <QMetaEnum>
#include <QtNetwork>

using namespace cvnirc::core::IRCProto;

IRCProtoClient::IRCProtoClient(QObject *parent) : QObject(parent),
    socket(new QTcpSocket(this)),
    socketReadBuf(10*1024, '\0'),
    _connectionState(ConnectionState::Disconnected)
{
    // Exclude normal printable characters from escaping in rawLine signal arguments.
    for (unsigned char c = 0; c < 128; c++) {
        if (QChar(c).isPrint())
            _rawLineWhitelist.append(c);
    }

    _loadMsgArgTypes();
    _loadMsgTypeVocabIn();

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
#ifdef CVN_HAVE_Q_ENUM
               QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(err) +
#else
               QString::number(err) +
#endif
               ": " + socket->errorString());
    _setConnectionState(ConnectionState::Disconnected);
}

void IRCProtoClient::sendRaw(const QString &line)
{
    sendQueue.push_back(line);
    processOutgoingData();
}

void IRCProtoClient::processOutgoingData()
{
    // FIXME: This method has to be invoked by a timer, too!
    // Or else we risk sending data only at the next read, e.g.,
    // on the next PING...
    //
    // Stop sending queued messages when there is a "traditional"
    // receive buffer's worth of data already queued
    // in the Qt write buffer.
    while (sendQueue.size() > 0 &&
           socket->state() == QAbstractSocket::ConnectedState &&
           socket->bytesToWrite() < 512)
    {
        QString rawLine = sendQueue.front();
        sendingLine(rawLine);
        socket->write((rawLine + "\r\n").toUtf8());
        sendQueue.pop_front();
    }
}

void IRCProtoClient::processIncomingData()
{
    processOutgoingData();

    // Grow and read into socketReadBuf.
    qint64 ret = 0;
    while (socketReadBuf.resize(socketReadBufUsed + 10*1024),
           (ret = socket->read(socketReadBuf.data() + socketReadBufUsed,
                               socketReadBuf.size() - socketReadBufUsed)
            ) > 0) {
        socketReadBufUsed += ret;

        // Look for completely received lines.
        int iCrLf;
        while ((iCrLf = socketReadBuf.indexOf("\r\n")) >= 0) {
            int len = iCrLf + 2;
            QByteArray rawLineBytesCrLf = socketReadBuf.left(len);  // Extract line.
            socketReadBuf.remove(0, len);  // Remove line from buffer.
            socketReadBufUsed -= len;
            if (socketReadBufUsed < 0) {
                socketReadBufUsed = 0;
                notifyUser("Internal error: Socket buffer used had become negative; "
                           "corrected, but this should not happen!");
                return;
            }

            if (rawLineBytesCrLf.contains('\0')) {
                notifyUser("Protocol error: Server sent a NUL byte "
                           "(detected in extracted line): Aborting connection.");
                socket->abort();
                return;
            }

            // TODO: Move this over to parse(), somehow. (?)
            // N.B.: From the change 2017-12-01 on, rawLineBytes won't be used here anymore after the check!
            QByteArray rawLineBytes(rawLineBytesCrLf);
            rawLineBytes.chop(2);  // Strip CR/LF.
            if (rawLineBytes.contains('\r') || rawLineBytes.contains('\n')) {
                notifyUser("Protocol error: Server seems to have broken line-termination! "
                           "(Stray CR or LF found in extracted line.) Aborting connection.");
                socket->abort();
                return;
            }

            // Interpret message.
            MessageOnNetwork raw { rawLineBytesCrLf };
            receivedRaw(raw);
        }

        if (socketReadBufUsed == 0)
            // Skip additional sanity checks, as no buffered data is left.
            continue;  // Continue with next read attempt.

        // Still no complete line after 1 MiB?
        if (socketReadBufUsed >= 1*1024*1024) {
            notifyUser("Protocol error: Server sends data which either is "
                       "an extremely large line, or garbage: Aborting connection.");
            socket->abort();
            return;
        }

        // TODO: Test for: Still buffer contents with no complete line after (some minutes)?

        const QByteArray leftover(socketReadBuf.left(socketReadBufUsed));

        if (leftover.contains('\0')) {
            notifyUser("Protocol error: Server sent a NUL byte "
                       "(detected after extracting all lines from buffer): Aborting connection.");
            socket->abort();
            return;
        }

        // (A stray '\r' could actually happen when the CR/LF message framing
        // line terminator is split between two reads. But who uses CR alone
        // as line terminator nowadays, anyhow. The perhaps more commonly
        // to be expected case might be a server that sends LF only, which this
        // will then lead to connection abort.)
        //
        if (/* leftover.contains('\r') || */ leftover.contains('\n')) {
            notifyUser("Protocol error: Server seems to have broken line-termination! "
                       "(Stray LF found after extracting all lines from buffer.) Aborting connection.");
            socket->abort();
            return;
        }
    }

    if (ret < 0) {
        notifyUser("Error reading from network socket, aborting connection.");
        socket->abort();
        return;
    }
}

void IRCProtoClient::receivedRaw(const MessageOnNetwork &raw)
{
    IRCProto::Incoming in(
        std::make_shared<MessageOnNetwork>(raw),
        std::make_shared<MessageAsTokens>(raw.parse())
    );
    receivedLine(raw.bytes.toPercentEncoding(_rawLineWhitelist));

#if 0  // TODO: Ensure the same functionality is provided via parse().
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
#endif

    const QByteArray     &prefix(in.inTokens->prefix);
    const QByteArrayList &tokens(in.inTokens->mainTokens);

    // Ignore empty lines silently.
    if (prefix.isNull() && tokens.isEmpty())
        return;

    if (!(tokens.size() >= 1)) {
        notifyUser("Protocol error, disconnecting: Received line with a prefix token only!");
        disconnectFromIRCServer("Protocol error");
        return;
    }

    // FIXME: Implement
    const QString command = tokens[0];

    in.inMessageType = _msgTypeVocabIn.messageType(command);
    if (!in.inMessageType) {
        notifyUser("Received unrecognized command \"" + command + "\"");
        return;
    }

    try {
        in.inMessage = in.inMessageType->fromMessageAsTokens(*in.inTokens);
    }
    catch (const std::exception &ex) {
        notifyUser("Error processing command \"" + command + "\": " + ex.what());
    }

#if 0
    // FIXME: Remove
    const QByteArray &commandOrig(tokens.front());
    const QByteArray  commandUpper = commandOrig.toUpper();

    if (commandUpper == "PING") {
        if (!(tokens.size() == 2)) {
            notifyUser("Protocol error, disconnecting: Received PING message with unexpected token count " + QString::number(tokens.size()));
            disconnectFromIRCServer("Protocol error");
            return;
        }
        in.inMessage = std::make_shared<PingPongMessage>(Message::MsgType::Ping, tokens[1]);
    }
    else if (commandUpper == "001") {
        in.inMessage = std::make_shared<NumericMessage>(Message::MsgType::Welcome, 1);
    }
    else if (commandUpper == "JOIN") {
        if (!(tokens.size() >= 2 && tokens.size() <= 3)) {
            notifyUser("Protocol error, ignoring: Received JOIN message with unexpected token count " + QString::number(tokens.size()));
            return;
        }

        QByteArrayList channelsBytes = tokens[1].split(',');
        QByteArrayList keysBytes = tokens.size() >= 3 ? tokens[2].split(',') : QByteArrayList();

        QStringList channels;
        for (QByteArray channelBytes : channelsBytes) {
            channels.append(QString(channelBytes));
        }

        QStringList keys;
        for (QByteArray keyBytes : keysBytes) {
            keys.append(QString(keyBytes));
        }

        in.inMessage = std::make_shared<JoinMessage>(Message::MsgType::Join, channels, keys);
    }
    else if (commandUpper == "PRIVMSG" || commandUpper == "NOTICE") {
        if (!(tokens.size() == 3)) {
            notifyUser("Protocol error, ignoring: Received " + commandUpper + " message with unexpected token count " + QString::number(tokens.size()));
            return;
        }

        auto msgType = IRCProtoMessage::MsgType::Unknown;
        if (commandUpper == "PRIVMSG")
            msgType = IRCProtoMessage::MsgType::PrivMsg;
        else if (commandUpper == "NOTICE")
            msgType = IRCProtoMessage::MsgType::Notice;

        in.inMessage = std::make_shared<ChatterMessage>(msgType, tokens[1], tokens[2]);
    }
    else {
        in.inMessage = std::make_shared<Message>();
    }
#endif

    if (in.inMessage) {
        receivedMessageAutonomous(&in);
        receivedMessage(&in);

        if (!in.handled)
            notifyUser("Unhandled IRC protocol message: " + command);
#if 0
            notifyUser("Unhandled IRC protocol message (type " + QString::number((int)in.inMessage->msgType) +
#ifdef CVN_HAVE_Q_ENUM
                       ": " + QMetaEnum::fromType<Message::MsgType>().valueToKey((int)in.inMessage->msgType) +
#endif
                       "): " + commandOrig);
#endif

        // This should not be necessary anymore, the smart pointers should
        // take care of everything. (?)
        //delete msg;
    }
}

void IRCProtoClient::receivedMessageAutonomous(Incoming *in)
{
    if (in == nullptr)
        throw std::invalid_argument("IRC protocol client, receivedMessageAutonomous(): Incoming can't be null");

    std::shared_ptr<Message> msg = in->inMessage;
    if (!msg)
        throw std::invalid_argument("IRC protocol client, receivedMessageAutonomous(): Incoming message can't be null");

    if (msg->args.isEmpty())
        throw std::invalid_argument("IRC protocol client, receivedMessageAutonomous(): Incoming message can't miss first argument (the command name)");

    auto commandArg = std::dynamic_pointer_cast<CommandNameMessageArg>(msg->args.front());
    if (!commandArg)
        throw std::invalid_argument("IRC protocol client, receivedMessageAutonomous(): Incoming message first argument is not a command name argument");

    auto numericArg = std::dynamic_pointer_cast<NumericCommandNameMessageArg>(commandArg);

    if (commandArg->commandUpper == "PING") {
        if (msg->args.length() < 2)
            throw std::invalid_argument("IRC protocol client, receivedMessageAutonomous(): Incoming message misses second argument, the ping source");

        auto sourceArg = std::dynamic_pointer_cast<SourceMessageArg>(msg->args[1]);
        if (!sourceArg)
            throw std::invalid_argument("IRC protocol client, receivedMessageAutonomous(): Incoming message second argument is not a source argument");

        sendRaw("PONG :" + sourceArg->source);
        in->handled = true;
    }
    else if (numericArg && numericArg->numeric == 1) {
        if (connectionState() != ConnectionState::Registering) {
            notifyUser("Protocol error, disconnecting: Got random Welcome/001 message");
            disconnectFromIRCServer("Protocol error");
            return;
        }

        notifyUser("Got welcome message; we're connected, now");
        _setConnectionState(ConnectionState::Connected);
        in->handled = true;
    }

#if 0
    switch (msg->msgType) {
    case Message::MsgType::Ping:
        {
            auto &pingMsg(static_cast<PingPongMessage &>(*msg));
            sendRaw("PONG :" + pingMsg.target);
            in->handled = true;
        }
        break;
    case Message::MsgType::Welcome:
        if (connectionState() != ConnectionState::Registering) {
            notifyUser("Protocol error, disconnecting: Got random Welcome/001 message");
            disconnectFromIRCServer("Protocol error");
            return;
        }

        notifyUser("Got welcome message; we're connected, now");
        _setConnectionState(ConnectionState::Connected);
        in->handled = true;
        break;
    default:
        break;
    }
#endif
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
    if (_verboseLevel >= 1)
        notifyUser("Setting host to request next to \"" + host + "\".");
    _hostRequestNext = host;
}

void IRCProtoClient::setPortRequestNext(const QString &port)
{
    if (_verboseLevel >= 1)
        notifyUser("Setting port to request next to \"" + port + "\".");
    _portRequestNext = port;
}

void IRCProtoClient::setUserRequestNext(const QString &user)
{
    if (_verboseLevel >= 1)
        notifyUser("Setting user to request next to \"" + user + "\".");
    _userRequestNext = user;
}

void IRCProtoClient::setNickRequestNext(const QString &nick)
{
    if (_verboseLevel >= 1)
        notifyUser("Setting nick to request next to \"" + nick + "\".");
    _nickRequestNext = nick;
}

int IRCProtoClient::verboseLevel() const
{
    return _verboseLevel;
}

void IRCProtoClient::setVerboseLevel(int newVerboseLevel)
{
    if (_verboseLevel >= 2)
        notifyUser("Changing IRC protocol client verbose level from " + QString::number(_verboseLevel) + " to " + QString::number(newVerboseLevel));
    _verboseLevel = newVerboseLevel;
    if (_verboseLevel >= 2)
        notifyUser("Verbose level of IRC protocol client now is " + QString::number(_verboseLevel));
}

const QByteArray &IRCProtoClient::rawLineWhitelist() const
{
    return _rawLineWhitelist;
}

void IRCProtoClient::setRawLineWhitelist(const QByteArray &newRawLineWhitelist)
{
    _rawLineWhitelist = newRawLineWhitelist;
}

void IRCProtoClient::_setConnectionState(ConnectionState newState)
{
    if (_connectionState == newState)
        return;

    _connectionState = newState;
    connectionStateChanged();
}

void IRCProtoClient::_loadMsgArgTypes()
{
    _msgArgTypesHolder.commandNameType = std::make_shared<MessageArgType<CommandNameMessageArg>>("command", [](TokensReader *reader) {
        return std::make_shared<CommandNameMessageArg>(QString(reader->takeToken()));
    });

    _msgArgTypesHolder.sourceType = std::make_shared<MessageArgType<SourceMessageArg>>("source", [](TokensReader *reader) {
        return std::make_shared<SourceMessageArg>(QString(reader->takeToken()));
    });

    _msgArgTypesHolder.channelType = std::make_shared<MessageArgType<ChannelMessageArg>>("channel", [](TokensReader *reader) {
        return std::make_shared<ChannelMessageArg>(QString(reader->takeToken()));
    });
    _msgArgTypesHolder.channelListType = std::make_shared<MessageArgType<ChannelListMessageArg>>("channels", [this](TokensReader *reader) {
        auto ret = std::make_shared<ChannelListMessageArg>();
        QByteArrayList channelsBytes = reader->takeToken().split(',');
        TokensReader innerReader(channelsBytes);
        while (!innerReader.atEnd()) {
            ret->channelList.append(std::dynamic_pointer_cast<ChannelMessageArg>(
                    _msgArgTypesHolder.channelType->fromTokens_call()(&innerReader)
            ));
        }
        return ret;
    });
}

void IRCProtoClient::_loadMsgTypeVocabIn()
{
#if 1
    _msgTypeVocabIn.registerMessageType("PING", std::make_shared<MessageType>("PingType", QList<std::shared_ptr<MessageArgTypeBase>> {
        std::make_shared<ConstMessageArgType<MessageArgType<CommandNameMessageArg>>>("PingCommandType",
            std::make_shared<CommandNameMessageArg>("PING"),
            *_msgArgTypesHolder.commandNameType),
        _msgArgTypesHolder.sourceType,
        //OptionalMessageArgType("[server2]", *_msgArgTypesHolder.FIXME),
    }));
#else
    QList<std::shared_ptr<MessageArgTypeBase>> argTypes;
    std::shared_ptr<MessageArgType<CommandNameMessageArg>> commandArg =
        std::make_shared<ConstMessageArgType<MessageArgType<CommandNameMessageArg>>>("PingCommandType",
            std::make_shared<CommandNameMessageArg>("PING"),
            *_msgArgTypesHolder.commandNameType);
    argTypes.append(commandArg);
    argTypes.append(_msgArgTypesHolder.sourceType);
    //argTypes.append(OptionalMessageArgType("[server2]", *_msgArgTypesHolder.FIXME));
    _msgTypeVocabIn.registerMessageType("PING", std::make_shared<MessageType>("PingType", argTypes));
#endif
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
