#ifndef IRCPROTOMESSAGE_H
#define IRCPROTOMESSAGE_H

#include "QString"
#include <vector>

enum class IRCMsgType {
    Raw,
    Ping, Pong,
};

class IRCProtoMessage
{
public:
    IRCProtoMessage(const QString &rawLine);

    IRCMsgType msgType;
    QString rawLine;

    static std::vector<QString> splitRawLine(const QString &rawLine);
};

class PingPongIRCProtoMessage : public IRCProtoMessage
{
public:
    PingPongIRCProtoMessage(const QString &rawLine, IRCMsgType msgType, const QString &target);

    QString target;
};

#endif // IRCPROTOMESSAGE_H
