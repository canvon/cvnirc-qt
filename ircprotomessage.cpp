#include "ircprotomessage.h"

#include <stdexcept>

IRCProtoMessage::tokens_type IRCProtoMessage::splitRawLine(const QString &rawLine)
{
    tokens_type ret;
    QString token;
    bool quoteStarted = false;

    for (QString::const_iterator iter = rawLine.cbegin();
         iter < rawLine.cend();
         iter++)
    {
        QChar c = *iter;

        if (quoteStarted) {
            token.append(c);
            continue;
        }

        switch (c.toLatin1()) {
        case ' ':
        case '\t':
            // White-space.
            if (token.length() == 0) {
                // Skip.
                continue;
            }

            // Otherwise, break token.
            ret.push_back(token);
            token.clear();
            break;
        case ':':
            if (ret.size() == 0 || token.length() > 0) {
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
        ret.push_back(token);

    return ret;
}

IRCProtoMessage::IRCProtoMessage(const QString &rawLine) :
    msgType(IRCMsgType::Unknown),
    rawLine(rawLine)
{
    mainTokens = splitRawLine(rawLine);
    if (mainTokens.size() >= 1 && mainTokens[0].length() >= 1 && mainTokens[0][0] == ':') {
        prefix = mainTokens[0];
        mainTokens.erase(mainTokens.begin());
    }
}

IRCProtoMessage::IRCProtoMessage(const QString &rawLine, const QString &prefix, const IRCProtoMessage::tokens_type &mainTokens) :
    msgType(IRCMsgType::Unknown),
    rawLine(rawLine),
    prefix(prefix),
    mainTokens(mainTokens)
{

}

PingPongIRCProtoMessage::PingPongIRCProtoMessage(
    const QString &rawLine, const QString &prefix, const tokens_type &mainTokens,
    IRCMsgType msgType, const QString &target) :
        IRCProtoMessage(rawLine, prefix, mainTokens),
        target(target)
{
    switch (msgType) {
    case IRCMsgType::Ping:
    case IRCMsgType::Pong:
        break;
    default:
        throw std::runtime_error("PingPongIRCProtoMessage ctor: Invalid msgType " + std::to_string((int)msgType));
    }
    this->msgType = msgType;
}

NumericIRCProtoMessage::NumericIRCProtoMessage(
    const QString &rawLine, const QString &prefix, const tokens_type &mainTokens,
    IRCMsgType msgType, int numeric) :
        IRCProtoMessage(rawLine, prefix, mainTokens),
        numeric(numeric)
{
    this->msgType = msgType;

    if (numeric < 0 || numeric > 999)
        throw std::runtime_error("NumericIRCProtoMessage ctor: Numeric " + std::to_string(numeric) + " out of range");
}
