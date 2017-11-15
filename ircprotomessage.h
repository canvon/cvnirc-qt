#ifndef IRCPROTOMESSAGE_H
#define IRCPROTOMESSAGE_H

#include "QString"

enum class IRCMsgType {
    Raw,
};

class IRCProtoMessage
{
public:
    IRCProtoMessage(const QString &rawLine);

    IRCMsgType msgType;
    QString rawLine;
};

#endif // IRCPROTOMESSAGE_H
