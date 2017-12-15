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

    const QString command = tokens[0];

    in.inMessageType = _msgTypeVocabIn.messageType(command);
    if (!in.inMessageType) {
        notifyUser("Received unrecognized command \"" + command + "\"");
        return;
    }

    try {
        // TODO: Somehow pass in how to decode raw prefix from QByteArray to QString...
        // Might be relevant if network and user use different encodings,
        // like latin1 vs. UTF-8, or it might even strange things like KOI8-R
        // or what's it alled, or Shift_JIS or something be involved.
        //
        // Note: This would most likely be implemented via letting the lambdas
        // in _loadMsgArgTypes() capture "this" and call a reencoding method
        // of this IRCProtoClient for the conversion from QByteArray to QString.
        //
        in.inMessage = in.inMessageType->fromMessageAsTokens(*in.inTokens);
    }
    catch (const std::exception &ex) {
        notifyUser("Error processing command \"" + command + "\": " + ex.what());
    }

    if (in.inMessage) {
        receivedMessageAutonomous(&in);
        receivedMessage(&in);

        if (!in.handled)
            notifyUser("Unhandled IRC protocol message: " + command);
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
    _msgArgTypesHolder.originType = std::make_shared<MessageOriginType>("origin", [](const QByteArray &prefixBytes) {
        return QString(prefixBytes);  // TODO: Reencode from network to user encoding.
    }, MessageOrigin::Type::LinkServer);

    _msgArgTypesHolder.commandNameType = std::make_shared<MessageArgType<CommandNameMessageArg>>("command", [](TokensReader *reader) {
        return std::make_shared<CommandNameMessageArg>(QString(reader->takeToken()));
    });
    _msgArgTypesHolder.numericCommandNameType = std::make_shared<MessageArgType<NumericCommandNameMessageArg>>("numeric", [](TokensReader *reader) {
        return std::make_shared<NumericCommandNameMessageArg>(QString(reader->takeToken()));
    });

    auto unrecognizedType = std::make_shared<MessageArgType<UnrecognizedMessageArg>>("unrecognized", [](TokensReader *reader) {
        return std::make_shared<UnrecognizedMessageArg>(reader->takeToken());
    });
    _msgArgTypesHolder.unrecognizedType = unrecognizedType;
    _msgArgTypesHolder.unrecognizedArgListType = std::make_shared<MessageArgType<ListMessageArg<UnrecognizedMessageArg>>>("unrecognizedArgs",
        [unrecognizedType](TokensReader *reader) {
            auto ret = std::make_shared<ListMessageArg<UnrecognizedMessageArg>>();
            while (!reader->atEnd())
                ret->list.append(
                    unrecognizedType->fromTokens_call()(reader)
                );
            return ret;
        });

    _msgArgTypesHolder.sourceType = std::make_shared<MessageArgType<SourceMessageArg>>("source", [](TokensReader *reader) {
        return std::make_shared<SourceMessageArg>(QString(reader->takeToken()));
    });

    _msgArgTypesHolder.targetType = std::make_shared<MessageArgType<TargetMessageArg>>("target",
        [this](TokensReader *reader) -> std::shared_ptr<TargetMessageArg> {
            QByteArray token = reader->takeToken();
            if (isChannel(token))
                return std::make_shared<ChannelTargetMessageArg>(QString(token));
            else
                return std::make_shared<NickTargetMessageArg>(QString(token));
        });
    _msgArgTypesHolder.targetListType = make_commalist("targets", _msgArgTypesHolder.targetType);

    _msgArgTypesHolder.channelType = std::make_shared<MessageArgType<ChannelTargetMessageArg>>("channel", [](TokensReader *reader) {
        return std::make_shared<ChannelTargetMessageArg>(QString(reader->takeToken()));
    });
    _msgArgTypesHolder.channelListType = make_commalist("channels", _msgArgTypesHolder.channelType);

    _msgArgTypesHolder.keyType = std::make_shared<MessageArgType<KeyMessageArg>>("key", [](TokensReader *reader) {
        return std::make_shared<KeyMessageArg>(QString(reader->takeToken()));
    });
    _msgArgTypesHolder.keyListType = make_commalist("keys", _msgArgTypesHolder.keyType);

    _msgArgTypesHolder.chatterDataType = std::make_shared<MessageArgType<ChatterDataMessageArg>>("chatterData", [](TokensReader *reader) {
        return std::make_shared<ChatterDataMessageArg>(QString(reader->takeToken()));
    });
}

void IRCProtoClient::_loadMsgTypeVocabIn()
{
    _msgTypeVocabIn.registerMessageType("PING", MessageType::make_shared("PingType", _msgArgTypesHolder.originType, {
        make_const_fwd("PingCommandType", _msgArgTypesHolder.commandNameType, "PING"),
        _msgArgTypesHolder.sourceType,
        //OptionalMessageArgType("[server2]", _msgArgTypesHolder.FIXME),
    }));

    _msgTypeVocabIn.registerMessageType("001", MessageType::make_shared("WelcomeType", _msgArgTypesHolder.originType, {
        make_const_fwd("WelcomeNumericType", _msgArgTypesHolder.numericCommandNameType, "001"),
        _msgArgTypesHolder.unrecognizedArgListType,
    }));

    _msgTypeVocabIn.registerMessageType("JOIN", MessageType::make_shared("JoinChannelType", _msgArgTypesHolder.originType, {
        make_const_fwd("JoinChannelCommandType", _msgArgTypesHolder.commandNameType, "JOIN"),
        _msgArgTypesHolder.channelListType,
        make_optional("[keys]", _msgArgTypesHolder.keyListType),
    }));

    auto chatterMsgType = MessageType::make_shared("ChatterMessageType", _msgArgTypesHolder.originType, {
        _msgArgTypesHolder.commandNameType,
        _msgArgTypesHolder.targetListType,
        _msgArgTypesHolder.chatterDataType,
    });
    _msgTypeVocabIn.registerMessageType("PRIVMSG", chatterMsgType);
    _msgTypeVocabIn.registerMessageType("NOTICE",  chatterMsgType);
}

bool IRCProtoClient::isChannel(const QByteArray &token)
{
    // TODO: Use information from 001 "Welcome" message or the like
    //       to determine what's a channel and what's not.
    return token.startsWith('#');
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
