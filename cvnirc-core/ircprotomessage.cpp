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


MessageOrigin MessageOrigin::fromPrefix(const QString &prefix, Type onNull)
{
    return { prefix.isNull() ? onNull : Type::SeePrefix, prefix };
}

MessageOrigin MessageOrigin::fromPrefixBytes(const QByteArray &prefixBytes, const std::function<MessageOrigin::decode_fun> &decoder, MessageOrigin::Type onNull)
{
    return fromPrefix(decoder(prefixBytes), onNull);
}

MessageOriginType::MessageOriginType(const QString &name, MessageOriginType::decoder_type decoder, MessageOrigin::Type onNullPrefix) :
    _name(name), _decoder(decoder), _onNullPrefix(onNullPrefix)
{

}

const QString &MessageOriginType::name() const
{
    return _name;
}

MessageOriginType::decoder_type MessageOriginType::decoder() const
{
    return _decoder;
}

MessageOrigin::Type MessageOriginType::onNullPrefix() const
{
    return _onNullPrefix;
}

MessageOrigin MessageOriginType::fromPrefixBytes(const QByteArray &prefixBytes) const
{
    return MessageOrigin::fromPrefixBytes(prefixBytes, _decoder, _onNullPrefix);
}

MessageArgTypeBase::MessageArgTypeBase(const QString &name) :
    _name(name)
{

}

MessageArgTypeBase::~MessageArgTypeBase()
{

}

const QString &MessageArgTypeBase::name() const
{
    return _name;
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

UnrecognizedMessageArg::UnrecognizedMessageArg(const QByteArray &token) :
    token(token)
{

}

bool UnrecognizedMessageArg::operator ==(const MessageArg &other) const
{
    const auto *myTypeOther = dynamic_cast<const UnrecognizedMessageArg*>(&other);
    if (myTypeOther == nullptr)
        return false;

    return token == myTypeOther->token;
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

ChannelTargetMessageArg::ChannelTargetMessageArg(const QString &channel) :
    channel(channel)
{

}

QString ChannelTargetMessageArg::targetToString() const
{
    return channel;
}

bool ChannelTargetMessageArg::operator ==(const MessageArg &other) const
{
    const auto *myTypeOther = dynamic_cast<const ChannelTargetMessageArg*>(&other);
    if (myTypeOther == nullptr)
        return false;

    return channel == myTypeOther->channel;
}

NickTargetMessageArg::NickTargetMessageArg(const QString &nick) :
    nick(nick)
{

}

QString NickTargetMessageArg::targetToString() const
{
    return nick;
}

bool NickTargetMessageArg::operator ==(const MessageArg &other) const
{
    const auto *myTypeOther = dynamic_cast<const NickTargetMessageArg*>(&other);
    if (myTypeOther == nullptr)
        return false;

    return nick == myTypeOther->nick;
}

KeyMessageArg::KeyMessageArg(const QString &key) :
    key(key)
{

}

bool KeyMessageArg::operator ==(const MessageArg &other) const
{
    const auto *myTypeOther = dynamic_cast<const KeyMessageArg*>(&other);
    if (myTypeOther == nullptr)
        return false;

    return key == myTypeOther->key;
}

ChatterDataMessageArg::ChatterDataMessageArg(const QString &chatterData) :
    chatterData(chatterData)
{

}

bool ChatterDataMessageArg::operator ==(const MessageArg &other) const
{
    const auto *myTypeOther = dynamic_cast<const ChatterDataMessageArg*>(&other);
    if (myTypeOther == nullptr)
        return false;

    return chatterData == myTypeOther->chatterData;
}

MessageType::MessageType(const QString &name, originType_ptr originType, const QList<MessageType::msgArgType_ptr> &argTypes) :
    _name(name), _originType(originType), _argTypes(argTypes)
{

}

const QString &MessageType::name() const
{
    return _name;
}

MessageType::originType_ptr MessageType::originType() const
{
    return _originType;
}

const QList<MessageType::msgArgType_ptr> &MessageType::argTypes() const
{
    return _argTypes;
}

QList<MessageBase::msgArg_ptr> MessageType::argsFromMessageAsTokens(const MessageAsTokens &msgTokens) const
{
    QList<MessageBase::msgArg_ptr> ret;
    TokensReader reader(msgTokens);

    try {
        for (std::shared_ptr<MessageArgTypeBase> argType : _argTypes) {
            ret.append(argType->fromTokensUnsafe_call()(&reader));
        }

        if (!reader.atEnd())
            throw std::runtime_error("Trailing arguments, that is, more arguments than we had syntax for");
    }
    catch (const std::exception &ex) {
        throw std::runtime_error(std::string("Message type \"") + qPrintable(_name) + "\": Message tokens failed to convert to type, error: " +
                                 ex.what());
    }

    return ret;
}

std::shared_ptr<MessageBase> MessageType::fromMessageAsTokens(const MessageAsTokens &msgTokens) const
{
    MessageOrigin origin = _originType->fromPrefixBytes(msgTokens.prefix);
    QList<MessageBase::msgArg_ptr> args = argsFromMessageAsTokens(msgTokens);
    return std::make_shared<MessageBase>(origin, args);
}

std::shared_ptr<MessageType> MessageType::make_shared(const QString &name, MessageType::originType_ptr originType, const QList<MessageType::msgArgType_ptr> &argTypes)
{
    return std::make_shared<MessageType>(name, originType, argTypes);
}

void MessageTypeVocabulary::registerMessageType(const QString &commandName, std::shared_ptr<MessageType> msgType)
{
    _map.insert(commandName.toUpper(), msgType);
}

std::shared_ptr<MessageType> MessageTypeVocabulary::messageType(const QString &commandName)
{
    return _map.value(commandName.toUpper());
}


MessageBase::MessageBase(const MessageOrigin &origin, const QList<msgArg_ptr> &argsList) :
    origin(origin), argsList(argsList)
{

}

}  // namespace cvnirc::core::IRCProto
}  // namespace cvnirc::core
}  // namespace cvnirc
