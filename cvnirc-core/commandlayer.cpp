#include "commandlayer.h"

CommandLayer::CommandLayer(IRCCore *irc, QObject *parent) :
    QObject(parent), _irc(irc)
{

}

IRCCore *CommandLayer::irc()
{
    return _irc;
}

const IRCCore *CommandLayer::irc() const
{
    return _irc;
}

void CommandLayer::processCommand(const Command &cmd, IRCCoreContext *context)
{

}

void CommandLayer::processUserInput(const QString &line, IRCCoreContext *context)
{

}
