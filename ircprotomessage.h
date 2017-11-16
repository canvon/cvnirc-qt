#ifndef IRCPROTOMESSAGE_H
#define IRCPROTOMESSAGE_H

#include "QString"
#include <vector>

enum class IRCMsgType {
    Unknown,
    Ping, Pong,
    Welcome,
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

class NumericIRCProtoMessage : public IRCProtoMessage
{
public:
    NumericIRCProtoMessage(const QString &rawLine, const QString &prefix, const tokens_type &mainTokens,
                           IRCMsgType msgType, int numeric);

    int numeric;
};

#endif // IRCPROTOMESSAGE_H
