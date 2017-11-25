#ifndef IRCCORECOMMANDGROUP_H
#define IRCCORECOMMANDGROUP_H

#include "cvnirc-core_global.h"

#include <QObject>
#include "commandgroup.h"

class IRCCore;
class IRCCoreContext;
class Command;

class CVNIRCCORESHARED_EXPORT IRCCoreCommandGroup : public CommandGroup
{
    Q_OBJECT
    IRCCore *_irc;
public:
    explicit IRCCoreCommandGroup(IRCCore *irc, const QString &groupName, QObject *parent = nullptr);

    IRCCore *irc();
    const IRCCore *irc() const;

    QStringList cmdhelp_raw();
    void cmd_raw(Command *cmd, IRCCoreContext *context);

    void registerAllCommandDefinitions();
};

#endif // IRCCORECOMMANDGROUP_H
