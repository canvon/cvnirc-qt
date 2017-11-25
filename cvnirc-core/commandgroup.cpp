#include "commandgroup.h"

CommandGroup::CommandGroup(const QString &groupName, QObject *parent) :
    QObject(parent), _groupName(groupName)
{
    // This would have been too easy:
    //registerAllCommandDefinitions();
    // It turns out that, in the ctor, the final overrider of the ctor's class
    // is used (which does not exist in this case as it is purely virtual
    // so fails at link time; but it wouldn't have worked as expected anyhow)...
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

void CommandGroup::registerAllCommandDefinitionsOnce()
{
    if (_registeredOnce)
        return;

    registerAllCommandDefinitions();
    // Set here, too, in case the overrider forgets it.
    _registeredOnce = true;
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
    group->registerAllCommandDefinitionsOnce();
}


RootCommandGroup::RootCommandGroup(QObject *parent) :
    CommandGroup(QString(), parent)
{

}

void RootCommandGroup::registerAllCommandDefinitions()
{
    // Nothing to be done here.

    _registeredOnce = true;
}
