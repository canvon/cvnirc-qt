#include "commandlayer.h"

#include "irccorecontext.h"
#include <stdexcept>

CommandLayer::CommandLayer(QObject *parent) :
    QObject(parent), _rootCommandGroup(QString(), this)
{

}

CommandGroup &CommandLayer::rootCommandGroup()
{
    return _rootCommandGroup;
}

const CommandGroup &CommandLayer::rootCommandGroup() const
{
    return _rootCommandGroup;
}

void CommandLayer::processCommand(Command *cmd, IRCCoreContext *context)
{
    if (cmd == nullptr)
        throw std::invalid_argument("Command layer, process command: Command object can't be null");

    if (cmd->tokens().isEmpty())
        throw std::invalid_argument("Command layer, process command: Command object is invalid: Can't be empty");

    CommandGroup *group = _rootCommandGroup.subGroup("IRC");
    if (group == nullptr)
        throw std::runtime_error("Command layer, process command: Command group \"IRC\" is missing!");

    const QString &cmdName(cmd->tokens()[0]);
    CommandGroup::commandDefinition_ptr cmdDef = group->commandDefinition(cmdName);
    if (!cmdDef)
        throw std::runtime_error(std::string("Command layer, process command: Command \"") + cmdName.toStdString() + "\" not found");

    cmdDef->definition()(cmd, context);
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
        Command cmd(lineCopy);
        processCommand(&cmd, context);
    }
}
