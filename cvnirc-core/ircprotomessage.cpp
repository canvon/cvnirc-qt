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

Message::Message() :
    msgType(MsgType::Unknown)
{

}

Message::Message(Message::MsgType msgType) :
    msgType(msgType)
{

}

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

}  // namespace cvnirc::core::IRCProto
}  // namespace cvnirc::core
}  // namespace cvnirc
