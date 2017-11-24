#include "irccorecontext.h"

#include "irccore.h"


IRCCoreContext::IRCCoreContext(IRCProtoClient *ircProtoClient, IRCCoreContext::Type type, const QString &outgoingTarget, QObject *parent) :
    QObject(parent), _ircProtoClient(ircProtoClient), _type(type), _outgoingTarget(outgoingTarget)
{
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

void IRCCoreContext::receiveIRCProtoMessage(IRCProtoMessage &msg)
{
    // TODO: Uncomment again when it's been made sure that
    // multi-target messages are only marked handled
    // if all targets have been handled...
    //if (msg.handled)
    //    return;

    auto *core = dynamic_cast<IRCCore *>(parent());

    switch (msg.msgType) {
    case IRCProtoMessage::MsgType::Join:
        {
            auto &joinMsg(static_cast<JoinIRCProtoMessage &>(msg));

            for (QString channel : joinMsg.channels) {
                if (_type == Type::Server) {
                    if (core == nullptr)
                        throw std::runtime_error("IRCCoreContext: A Server context needs to know its parent!");

                    bool created = false;
                    auto *context = core->createOrGetContext(_ircProtoClient, Type::Channel, channel, &created);
                    if (context == nullptr)
                        throw std::runtime_error("IRCCoreContext: Create-or-get other context failed");

                    if (created)
                        context->receiveIRCProtoMessage(msg);
                }
                else if (_type == Type::Channel && channel == _outgoingTarget) {
                    notifyUser("Joined channel " + channel +
                               (joinMsg.prefix.count() > 0 ? ": " + joinMsg.prefix : ""),
                               this);
                }
            }

            // TODO: Only mark as handled if all channels have been handled somewhere...
            // (This may have been on another channel context than (if it is one) this one.)
            joinMsg.handled = true;
        }
        break;
    case IRCProtoMessage::MsgType::PrivMsg:
    case IRCProtoMessage::MsgType::Notice:
        {
            auto &chatterMsg(static_cast<ChatterIRCProtoMessage &>(msg));
            bool isNotice = chatterMsg.msgType == IRCProtoMessage::MsgType::Notice;
            QString senderNick = IRCProtoClient::nickUserHost2nick(chatterMsg.prefix);

            // TODO: Recognize other types of channels.
            bool isChannel = chatterMsg.target.startsWith("#");
            Type contextType = isChannel ? Type::Channel : Type::Query;
            QString returnPath = isChannel ? chatterMsg.target : senderNick;

            if (_type == Type::Server) {
                bool created = false;
                auto *context = core->createOrGetContext(_ircProtoClient, contextType, returnPath, &created);
                if (context == nullptr)
                    throw std::runtime_error("IRCCoreContext: Create-or-get other context failed");

                if (created)
                    context->receiveIRCProtoMessage(msg);

                break;
            }

            if (_type != contextType || returnPath != _outgoingTarget)
                break;

            QString sourceTyped = (isNotice ? "-" : "<") + senderNick + (isNotice ? "-" : ">");

            notifyUser(sourceTyped + " " + chatterMsg.chatterData, this);
            chatterMsg.handled = true;
        }
        break;
    default:
        break;
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
