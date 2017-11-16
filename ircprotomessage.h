#ifndef IRCPROTOMESSAGE_H
#define IRCPROTOMESSAGE_H

#include "QString"
#include <vector>

enum class IRCMsgType {
    Unknown,
    Ping, Pong,
};

class IRCProtoMessage
{
public:
    typedef std::vector<QString> tokens_type;

    IRCProtoMessage(const QString &rawLine);
    IRCProtoMessage(const QString &rawLine, const QString &prefix, const tokens_type &mainTokens);

    IRCMsgType msgType;
    QString rawLine;
    QString prefix;
    tokens_type mainTokens;

    static tokens_type splitRawLine(const QString &rawLine);
};

class PingPongIRCProtoMessage : public IRCProtoMessage
{
public:
    PingPongIRCProtoMessage(const QString &rawLine, const QString &prefix, const tokens_type &mainTokens,
                            IRCMsgType msgType, const QString &target);

    QString target;
};

#endif // IRCPROTOMESSAGE_H
