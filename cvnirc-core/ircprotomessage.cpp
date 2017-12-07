#include "ircprotomessage.h"

#include <stdexcept>

namespace cvnirc   {
namespace core     {  // cvnirc::core
namespace IRCProto {  // cvnirc::core::IRCProto

MessageAsTokens MessageOnNetwork::parse() const
{
    QByteArray line(bytes);
    // Remove message framing (line terminator).
    // TODO: Make this a requirement! But for a transition period, parse() doesn't throw, so it's optional.
    if (line.endsWith("\r\n"))
        line.chop(2);

    QByteArrayList parsedTokens;
    QByteArray token;
    bool quoteStarted = false;

    for (char c : line)
    {
        if (quoteStarted) {
            token.append(c);
            continue;
        }

        switch (c) {
        case ' ':
            // Space.
            if (token.length() == 0) {
                // Skip.
                continue;
            }

            // Otherwise, break token.
            parsedTokens.push_back(token);
            token.clear();
            break;
        case ':':
            if (parsedTokens.size() == 0 || token.length() > 0) {
                // Append to token.
                token.append(c);
            }
            else {
                // Ignore character, start quote.
                quoteStarted = true;
            }
            break;
        default:
            // Append to token.
            token.append(c);
            break;
        }
    }

    // Cater for last token.
    if (token.length() > 0 || quoteStarted)
        parsedTokens.push_back(token);


    QByteArray prefix;

    if (!parsedTokens.isEmpty() && parsedTokens.front().startsWith(':')) {
        prefix = parsedTokens.front();
        prefix.remove(0, 1);  // Strip prefix identifier from identified prefix.
        parsedTokens.pop_front();
    }

    return MessageAsTokens { prefix, parsedTokens };
}

MessageOnNetwork MessageAsTokens::pack() const
{
    throw std::logic_error("MessageAsTokens::pack(): Not implemented");
}

TokensReader::TokensReader(const QByteArrayList &tokens) :
    _remainingTokens(tokens)
{

}

TokensReader::TokensReader(const MessageAsTokens &msgTokens) :
    _remainingTokens(msgTokens.mainTokens)
{

}

const QByteArrayList &TokensReader::remainingTokens() const
{
    return _remainingTokens;
}

bool TokensReader::atEnd() const
{
    return _remainingTokens.isEmpty();
}

bool TokensReader::isByteAvailable() const
{
    return _remainingTokens.length() >= 1 && _remainingTokens.front().length() >= 1;
}

bool TokensReader::isTokenAvailable() const
{
    return _remainingTokens.length() >= 1;
}

char TokensReader::takeByte()
{
    if (!isByteAvailable())
        throw std::runtime_error("Message reader, take byte: No byte available");

    QByteArray &firstToken(_remainingTokens.front());
    char c = firstToken[0];
    firstToken.remove(0, 1);
    return c;
}

QByteArray TokensReader::takeToken()
{
    if (!isTokenAvailable())
        throw std::runtime_error("Message reader, take token: No token available");

    return _remainingTokens.takeFirst();
}


Incoming::Incoming(Incoming::raw_ptr inRaw, Incoming::tokens_ptr inTokens, messageType_ptr inMessageType, Incoming::message_ptr inMessage) :
    inRaw(inRaw),
    inTokens(inTokens),
    inMessageType(inMessageType),
    inMessage(inMessage)
{

}


MessageArgType::MessageArgType(const QString &name, const std::function<fromTokens_fun> &fromTokens_call) :
    _name(name), _fromTokens_call(fromTokens_call)
{

}

MessageArgType::~MessageArgType()
{

}

const QString &MessageArgType::name() const
{
    return _name;
}

const std::function<MessageArgType::fromTokens_fun> &MessageArgType::fromTokens_call() const
{
    return _fromTokens_call;
}

ConstMessageArgType::ConstMessageArgType(
        const QString &name, std::shared_ptr<const MessageArg> constArg,
        const std::function<fromTokens_fun> &fromTokens_call) :
    MessageArgType(name, [this](TokensReader *reader) { return fromTokens(reader); }),
    _constArg(constArg), _origFromTokens_call(fromTokens_call)
{

}

ConstMessageArgType::ConstMessageArgType(
        const QString &name, std::shared_ptr<const MessageArg> constArg,
        const MessageArgType &msgArgType) :
    ConstMessageArgType(name, constArg, msgArgType.fromTokens_call())
{

}

std::shared_ptr<const MessageArg> ConstMessageArgType::constArg()
{
    return _constArg;
}

const std::function<MessageArgType::fromTokens_fun> &ConstMessageArgType::origFromTokens_call() const
{
    return _origFromTokens_call;
}

MessageArgType::messageArg_ptr ConstMessageArgType::fromTokens(TokensReader *reader) const
{
    std::shared_ptr<MessageArg> arg = _origFromTokens_call(reader);
    if (!(*arg == *_constArg))
        throw std::runtime_error("Const message arg type: The retrieved message arg isn't equal to the const/reference arg");

    return arg;
}

OptionalMessageArgType::OptionalMessageArgType(
        const QString &name, const std::function<MessageArgType::fromTokens_fun> &fromTokens_call) :
    MessageArgType(name, [this](TokensReader *reader) { return fromTokens(reader); }),
    _origFromTokens_call(fromTokens_call)
{

}

const std::function<MessageArgType::fromTokens_fun> &OptionalMessageArgType::origFromTokens_call() const
{
    return _origFromTokens_call;
}

MessageArgType::messageArg_ptr OptionalMessageArgType::fromTokens(TokensReader *reader) const
{
    if (reader->atEnd())
        return nullptr;

    return _origFromTokens_call(reader);
}

MessageArg::~MessageArg()
{

}

CommandNameMessageArg::CommandNameMessageArg(const QString &commandOrig) :
    commandOrig(commandOrig),
    commandUpper(this->commandOrig.toUpper())
{

}

bool CommandNameMessageArg::operator ==(const MessageArg &other) const
{
    const auto *myTypeOther = dynamic_cast<const CommandNameMessageArg*>(&other);
    if (myTypeOther == nullptr)
        return false;

    return commandUpper == myTypeOther->commandUpper;
}

NumericCommandNameMessageArg::NumericCommandNameMessageArg(const QString &commandOrig) :
    CommandNameMessageArg(commandOrig)
{
    if (commandUpper.length() != 3)
        throw std::invalid_argument("Numeric command name message arg, ctor: Invalid numeric: Length is not 3");

    if (!(commandUpper[0].isDigit() && commandUpper[1].isDigit() && commandUpper[2].isDigit()))
        throw std::invalid_argument("Numeric command name message arg, ctor: Invalid numeric: Must be 3 digits");

    QString copy = commandUpper;
    if (copy.startsWith("0"))
        copy.remove(0, 1);
    if (copy.startsWith("0"))
        copy.remove(0, 1);

    bool ok = false;
    numeric = copy.toInt(&ok);
    if (!ok)
        throw std::invalid_argument("Numeric command name message arg, ctor: Invalid numeric: Conversion to int failed");
}

bool NumericCommandNameMessageArg::operator ==(const MessageArg &other) const
{
    const auto *myTypeOther = dynamic_cast<const NumericCommandNameMessageArg*>(&other);
    if (myTypeOther == nullptr)
        return false;

    return numeric == myTypeOther->numeric;
}

SourceMessageArg::SourceMessageArg(const QString &source) :
    source(source)
{

}

bool SourceMessageArg::operator ==(const MessageArg &other) const
{
    const auto *myTypeOther = dynamic_cast<const SourceMessageArg*>(&other);
    if (myTypeOther == nullptr)
        return false;

    return source == myTypeOther->source;
}

ChannelMessageArg::ChannelMessageArg(const QString &channel) :
    channel(channel)
{

}

bool ChannelMessageArg::operator ==(const MessageArg &other) const
{
    const auto *myTypeOther = dynamic_cast<const ChannelMessageArg*>(&other);
    if (myTypeOther == nullptr)
        return false;

    return channel == myTypeOther->channel;
}

ChannelListMessageArg::ChannelListMessageArg()
{

}

ChannelListMessageArg::ChannelListMessageArg(const QList<std::shared_ptr<ChannelMessageArg> > &channelList) :
    channelList(channelList)
{

}

bool ChannelListMessageArg::operator ==(const MessageArg &other) const
{
    const auto *myTypeOther = dynamic_cast<const ChannelListMessageArg*>(&other);
    if (myTypeOther == nullptr)
        return false;

    int len = channelList.length();
    int otherLen = myTypeOther->channelList.length();
    if (len != otherLen)
        return false;

    for (int i = 0; i < len; i++) {
        if (channelList[i]->channel != myTypeOther->channelList[i]->channel)
            return false;
    }
    return true;
}

MessageType::MessageType(const QString &name, const QList<MessageType::msgArgType_ptr> &argTypes) :
    _name(name), _argTypes(argTypes)
{

}

const QString &MessageType::name() const
{
    return _name;
}

const QList<MessageType::msgArgType_ptr> &MessageType::argTypes() const
{
    return _argTypes;
}

QList<Message::msgArg_ptr> MessageType::argsFromMessageAsTokens(const MessageAsTokens &msgTokens) const
{
    QList<Message::msgArg_ptr> ret;
    TokensReader reader(msgTokens);

    try {
        for (std::shared_ptr<MessageArgType> argType : _argTypes) {
            ret.append(argType->fromTokens_call()(&reader));
        }
    }
    catch (const std::exception &ex) {
        throw std::runtime_error(std::string("Message type \"") + qPrintable(_name) + "\": Message tokens failed to convert to type");
    }

    return ret;
}

std::shared_ptr<Message> MessageType::fromMessageAsTokens(const MessageAsTokens &msgTokens) const
{
    MessageOrigin origin { MessageOrigin::Type::SeePrefix, QString(msgTokens.prefix) };
    QList<Message::msgArg_ptr> args = argsFromMessageAsTokens(msgTokens);
    return std::make_shared<Message>(origin, args);
}

void MessageTypeVocabulary::registerMessageType(const QString &commandName, std::shared_ptr<MessageType> msgType)
{
    _map.insert(commandName.toUpper(), msgType);
}

std::shared_ptr<MessageType> MessageTypeVocabulary::messageType(const QString &commandName)
{
    return _map.value(commandName.toUpper());
}


#if 0
Message::Message() :
    msgType(MsgType::Unknown)
{

}

Message::Message(Message::MsgType msgType) :
    msgType(msgType)
{

}
#endif

Message::Message(const MessageOrigin &origin, const QList<Message::msgArg_ptr> args) :
    origin(origin), args(args)
{

}

Message::~Message()
{

}

#if 0
PingPongMessage::PingPongMessage(MsgType msgType, const QString &target) :
    Message(msgType),
    target(target)
{
    switch (msgType) {
    case MsgType::Ping:
    case MsgType::Pong:
        break;
    default:
        throw std::invalid_argument("PingPongMessage ctor: Invalid msgType " + std::to_string((int)msgType));
    }
}

NumericMessage::NumericMessage(MsgType msgType, int numeric) :
    Message(msgType),
    numeric(numeric)
{
    if (numeric < 0 || numeric > 999)
        throw std::invalid_argument("NumericMessage ctor: Numeric " + std::to_string(numeric) + " out of range");
}

JoinMessage::JoinMessage(MsgType msgType, const QStringList &channels, const QStringList &keys) :
    Message(msgType),
    channels(channels),
    keys(keys)
{
    if (msgType != MsgType::Join)
        throw std::invalid_argument("JoinMessage ctor: Invalid msgType " + std::to_string((int)msgType));
}

ChatterMessage::ChatterMessage(MsgType msgType, const QString &target, const QString &chatterData) :
    Message(msgType),
    target(target),
    chatterData(chatterData)
{
    switch (msgType) {
    case MsgType::PrivMsg:
    case MsgType::Notice:
        break;
    default:
        throw std::invalid_argument("ChatterMessage ctor: Invalid msgType " + std::to_string((int)msgType));
    }
}
#endif

}  // namespace cvnirc::core::IRCProto
}  // namespace cvnirc::core
}  // namespace cvnirc
