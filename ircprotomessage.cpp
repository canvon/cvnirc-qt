#include "ircprotomessage.h"

IRCProtoMessage::IRCProtoMessage(const QString &rawLine) :
    msgType(IRCMsgType::Raw),
    rawLine(rawLine)
{

}
