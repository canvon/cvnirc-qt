#include "irccorecontext.h"

#include "irccore.h"
#include <stdexcept>


IRCCoreContext::IRCCoreContext(IRCProtoClient *ircProtoClient, IRCCoreContext::Type type, const QString &outgoingTarget, QObject *parent) :
    QObject(parent), _ircProtoClient(ircProtoClient), _type(type), _outgoingTarget(outgoingTarget)
{
    if (_ircProtoClient == nullptr)
        throw std::invalid_argument("IRCCoreContext ctor: IRC protocol client can't be null");

    if (type == Type::Server) {
        connect(ircProtoClient, &IRCProtoClient::connectionStateChanged, this, &IRCCoreContext::handle_connectionStateChanged);
        connect(ircProtoClient, &IRCProtoClient::notifyUser, this, &IRCCoreContext::handle_notifyUser);
        connect(ircProtoClient, &IRCProtoClient::sendingLine, this, &IRCCoreContext::handle_sendingLine);
        connect(ircProtoClient, &IRCProtoClient::receivedLine, this, &IRCCoreContext::handle_receivedLine);
    }

    connect(ircProtoClient, &IRCProtoClient::receivedMessage, this, &IRCCoreContext::receiveIRCProtoMessage);
}

bool IRCCoreContext::operator ==(const IRCCoreContext &other)
{
    if (_ircProtoClient != other._ircProtoClient)
        return false;
    if (_type != other._type)
        return false;
    if (_outgoingTarget != other._outgoingTarget)
        return false;

    return true;
}

IRCProtoClient *IRCCoreContext::ircProtoClient()
{
    return _ircProtoClient;
}

const IRCProtoClient *IRCCoreContext::ircProtoClient() const
{
    return _ircProtoClient;
}

IRCCoreContext::Type IRCCoreContext::type() const
{
    return _type;
}

const QString &IRCCoreContext::outgoingTarget() const
{
    return _outgoingTarget;
}

QString IRCCoreContext::disambiguator() const
{
    auto *irc = dynamic_cast<IRCCore *>(parent());
    bool needConnectionDisambiguation;
    if (irc == nullptr) {
        qDebug() << Q_FUNC_INFO << "Can't get IRCCore via QObject parent!";
        needConnectionDisambiguation = true;  // Better safe than sorry.
    }
    else {
        // Only disambiguate connection when we have multiple protocol clients.
        needConnectionDisambiguation = irc->ircProtoClients().length() > 1;
    }

    QString info;
    switch (_type) {
    case IRCCoreContext::Type::Server:
        info = "(Server)";
        break;
    case IRCCoreContext::Type::Channel:
        if (_outgoingTarget.isEmpty())
            info = "(Channel unknown)";
        else
            info = _outgoingTarget;  // Be compact, as channel name should be starting with a disambiguating character anyhow.
        break;
    case IRCCoreContext::Type::Query:
        if (_outgoingTarget.isEmpty())
            info = "(Query unknown)";
        else
            info = "Q:" + _outgoingTarget;
        break;
    }

    if (needConnectionDisambiguation) {
        // TODO: Change to connection tag later when we have them.
        // Otherwise, two connections to the same server
        // will look the same, that is, will not be disambiguated.
        QString serverName = _ircProtoClient->hostRequestedLast();
        if (serverName.isEmpty())
            serverName = "???";
        info.prepend(serverName + "/");
    }

    return info;
}

void IRCCoreContext::requestFocus()
{
    focusWanted(this);
}

void IRCCoreContext::receiveIRCProtoMessage(IRCProto::Incoming *in)
{
    if (in == nullptr)
        throw std::invalid_argument("IRC core context, slot receiveIRCProtoMessage(): Incoming can't be null");

    std::shared_ptr<IRCProto::Message> msg = in->inMessage;
    if (!msg)
        throw std::invalid_argument("IRC core context, slot receiveIRCProtoMessage(): Incoming message can't be null");

    // TODO: Uncomment again when it's been made sure that
    // multi-target messages are only marked handled
    // if all targets have been handled...
    //if (msg.handled)
    //    return;

    auto *core = dynamic_cast<IRCCore *>(parent());

    if (msg->args.isEmpty())
        throw std::invalid_argument("IRC core context, receive IRC proto message: Incoming message can't miss first argument (the command name)");

    auto commandArg = std::dynamic_pointer_cast<IRCProto::CommandNameMessageArg>(msg->args.front());
    if (!commandArg)
        throw std::invalid_argument("IRC core context, receive IRC proto message: Incoming message first argument is not a command name argument");

    if (commandArg->commandUpper == "JOIN") {
        int argCount = msg->args.length();
        if (!(argCount >= 2 && argCount <= 3))
            throw std::runtime_error("IRC core context, receive IRC proto message: Invalid argument count after processing (" + std::to_string(argCount) + ")");

        auto channelsArg = std::dynamic_pointer_cast<IRCProto::ListMessageArg<IRCProto::ChannelTargetMessageArg>>(msg->args[1]);
        if (!channelsArg)
            throw std::runtime_error("IRC core context, receive IRC proto message: Invalid argument type at index 1");  // TODO: Give type information if possible.

        for (std::shared_ptr<IRCProto::ChannelTargetMessageArg> channelArg : channelsArg->list) {
            const QString channel = channelArg->channel;

            if (_type == Type::Server) {
                if (core == nullptr)
                    throw std::runtime_error("IRCCoreContext: A Server context needs to know its parent!");

                bool created = false;
                auto *context = core->createOrGetContext(_ircProtoClient, Type::Channel, channel, &created);
                if (context == nullptr)
                    throw std::runtime_error("IRCCoreContext: Create-or-get other context failed");

                if (created)
                    context->receiveIRCProtoMessage(in);
            }
            else if (_type == Type::Channel && channel == _outgoingTarget) {
                notifyUser("Joined channel " + channel +
                           (!msg->origin.prefix.isEmpty() ? ": " + msg->origin.prefix : ""),
                           this);
            }
        }

        // TODO: Only mark as handled if all channels have been handled somewhere...
        // (This may have been on another channel context than (if it is one) this one.)
        in->handled = true;
    }
    else if (commandArg->commandUpper == "PRIVMSG" ||
             commandArg->commandUpper == "NOTICE") {
        int argCount = msg->args.length();
        if (argCount != 3)
            throw std::runtime_error("IRC core context, receive IRC proto message: Invalid argument count after processing (" + std::to_string(argCount) + ")");

        auto targetsArg = std::dynamic_pointer_cast<IRCProto::ListMessageArg<IRCProto::TargetMessageArg>>(msg->args[1]);
        if (!targetsArg)
            throw std::runtime_error("IRC core context, receive IRC proto message: Invalid argument type at index 1");  // TODO: Give type information if possible.

        auto chatterDataArg = std::dynamic_pointer_cast<IRCProto::ChatterDataMessageArg>(msg->args[2]);
        if (!chatterDataArg)
            throw std::runtime_error("IRC core context, receive IRC proto message: Invalid argument type at index 2");

        bool isNotice = commandArg->commandUpper == "NOTICE";
        QString senderNick = msg->origin.type == IRCProto::MessageOrigin::Type::LinkServer ?
            "LinkServer" :  // TODO: Make sure this does not collide with a valid nick name!
            IRCProtoClient::nickUserHost2nick(msg->origin.prefix);

        for (std::shared_ptr<IRCProto::TargetMessageArg> targetArg : targetsArg->list) {
            if (!targetArg)
                throw std::runtime_error("IRC core context, receive IRC proto message: At least one target was null");

            auto channelTargetArg = std::dynamic_pointer_cast<IRCProto::ChannelTargetMessageArg>(targetArg);
            auto nickTargetArg = std::dynamic_pointer_cast<IRCProto::NickTargetMessageArg>(targetArg);
            bool isChannel = channelTargetArg != nullptr;

            Type contextType = isChannel ? Type::Channel : Type::Query;
            QString returnPath = isChannel ? channelTargetArg->channel : senderNick;

            if (_type == Type::Server) {
                bool created = false;
                auto *context = core->createOrGetContext(_ircProtoClient, contextType, returnPath, &created);
                if (context == nullptr)
                    throw std::runtime_error("IRCCoreContext: Create-or-get other context failed");

                if (created)
                    context->receiveIRCProtoMessage(in);

                continue;
            }

            if (_type != contextType || returnPath != _outgoingTarget)
                continue;

            QString sourceTyped = (isNotice ? "-" : "<") + senderNick + (isNotice ? "-" : ">");

            notifyUser(sourceTyped + " " + chatterDataArg->chatterData, this);
        }

        // TODO: Only mark as handled if all channels have been handled somewhere
        in->handled = true;
    }
}

void IRCCoreContext::sendChatMessage(const QString &line)
{
    if (_outgoingTarget.isEmpty()) {
        notifyUser("Error: This context does not have an outgoing target. Can't send a chat message here!", this);
        return;
    }

    // TODO: Use nick *taken* last, when we have support to track this.
    //
    notifyUser("<" + _ircProtoClient->nickRequestedLast() + "> " + line, this);
    _ircProtoClient->sendRaw("PRIVMSG " + _outgoingTarget + " :" + line);
}

void IRCCoreContext::handle_connectionStateChanged()
{
    connectionStateChanged(this);
}

void IRCCoreContext::handle_notifyUser(const QString &line)
{
    notifyUser(line, this);
}

void IRCCoreContext::handle_sendingLine(const QString &rawLine)
{
    sendingLine(rawLine, this);
}

void IRCCoreContext::handle_receivedLine(const QString &rawLine)
{
    receivedLine(rawLine, this);
}
