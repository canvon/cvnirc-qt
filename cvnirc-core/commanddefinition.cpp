#include "commanddefinition.h"

#include <stdexcept>

CommandDefinition::CommandDefinition(
        const QString &name,
        const std::function<command_fun> &definition,
        const std::function<commandHelp_fun> &helpGetter):
    _name(name), _definition(definition), _helpGetter(helpGetter)
{
    if (name.isEmpty())
        throw std::invalid_argument("CommandDefinition ctor: Command name can't be empty");
}

const QString &CommandDefinition::name() const
{
    return _name;
}

const std::function<CommandDefinition::command_fun> &CommandDefinition::definition() const
{
    return _definition;
}

const std::function<CommandDefinition::commandHelp_fun> &CommandDefinition::helpGetter() const
{
    return _helpGetter;
}
