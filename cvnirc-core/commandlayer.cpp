#include "commandlayer.h"

#include "irccorecontext.h"

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
    if (line.length() < 1)
        return;

    QString lineCopy = line;
    bool isCommand = false;
    if (lineCopy[0] == '/') {
        if (lineCopy.length() == 1) {
            lineCopy.remove(0, 1);
        }
        else if (lineCopy[1] == ' ') {
            lineCopy.remove(0, 2);
        }
        else {
            isCommand = true;
            lineCopy.remove(0, 1);
        }
    }

    if (context == nullptr)
        throw std::invalid_argument("CommandLayer::processUserInput(): context can't be null");

    if (!isCommand) {
        context->sendChatMessage(lineCopy);
    }
    else {
        // TODO: Pre-parse as a command, to have local meanings as well.
        context->ircProtoClient()->sendRaw(lineCopy);
    }
}
