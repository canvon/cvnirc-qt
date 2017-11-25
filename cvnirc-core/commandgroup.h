#ifndef COMMANDGROUP_H
#define COMMANDGROUP_H

#include "cvnirc-core_global.h"

#include <memory>
#include <QObject>
#include "commanddefinition.h"
#include <QMap>

class CVNIRCCORESHARED_EXPORT CommandGroup : public QObject
{
    Q_OBJECT
public:
    typedef std::shared_ptr<CommandDefinition>    commandDefinition_ptr;
    typedef QMap<QString, commandDefinition_ptr>  commandMap_type;
    typedef QMap<QString, CommandGroup *>         groupMap_type;
private:
    QString _groupName;

    commandMap_type  _commandDefinitions;
    groupMap_type    _subGroups;

public:
    explicit CommandGroup(const QString &groupName, QObject *parent = nullptr);

    const QString &groupName();

    const commandMap_type &commandDefinitions() const;
    commandDefinition_ptr commandDefinition(const QString &lookupName);
    void registerCommandDefinition(const CommandDefinition &cmdDef);

    const groupMap_type &subGroups() const;
    CommandGroup *subGroup(const QString &lookupName);
    void addSubGroup(CommandGroup *group);

signals:

public slots:
};

#endif // COMMANDGROUP_H