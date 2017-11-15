#include "ircprotomessage.h"

#include <stdexcept>

std::vector<QString> IRCProtoMessage::splitRawLine(const QString &rawLine)
{
    std::vector<QString> ret;
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
    msgType(IRCMsgType::Raw),
    rawLine(rawLine)
{

}

PingPongIRCProtoMessage::PingPongIRCProtoMessage(const QString &rawLine, IRCMsgType msgType, const QString &target) :
    IRCProtoMessage(rawLine),
    target(target)
{
    switch (msgType) {
    case IRCMsgType::Ping:
    case IRCMsgType::Pong:
        break;
    default:
        throw std::runtime_error("PingPongIRCProtoMessage ctor: Invalid msgType " + (int)msgType);
    }
    this->msgType = msgType;
}
