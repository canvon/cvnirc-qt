#include "commandgroup.h"

CommandGroup::CommandGroup(const QString &groupName, QObject *parent) :
    QObject(parent), _groupName(groupName)
{

}

const QString &CommandGroup::groupName()
{
    return _groupName;
}

const CommandGroup::commandMap_type &CommandGroup::commandDefinitions() const
{
    return _commandDefinitions;
}

CommandGroup::commandDefinition_ptr CommandGroup::commandDefinition(const QString &lookupName)
{
    if (!_commandDefinitions.contains(lookupName))
        return nullptr;

    return _commandDefinitions.value(lookupName);
}

void CommandGroup::registerCommandDefinition(const CommandDefinition &cmdDef)
{
    // TODO: Have checks whether something already exists?
    // But what to do, then? Surely don't silently crash!
    // So, silently replace, for now. ...
    _commandDefinitions[cmdDef.name()] = std::make_shared<CommandDefinition>(cmdDef);
}

const CommandGroup::groupMap_type &CommandGroup::subGroups() const
{
    return _subGroups;
}

CommandGroup *CommandGroup::subGroup(const QString &lookupName)
{
    if (!_subGroups.contains(lookupName))
        return nullptr;

    return _subGroups.value(lookupName);
}

void CommandGroup::addSubGroup(CommandGroup *group)
{
    if (group == nullptr)
        throw std::invalid_argument("Command group, add subgroup: Group can't be null");

    if (group == this)
        throw std::invalid_argument("Command group, add subgroup: Can't add self");

    if (group->groupName().isEmpty())
        throw std::invalid_argument("Command group, add subgroup: Can't add a root command group");

    // TODO: Have checks ... (see comment in command registration)
    _subGroups[group->groupName()] = group;
    group->setParent(this);
}
