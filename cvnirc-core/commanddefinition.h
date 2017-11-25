#ifndef COMMANDDEFINITION_H
#define COMMANDDEFINITION_H

#include "cvnirc-core_global.h"

#include <functional>
#include <QStringList>

class Command;
class IRCCoreContext;

class CVNIRCCORESHARED_EXPORT CommandDefinition
{
    QString _name;
public:
    typedef void (command_fun)(Command *cmd, IRCCoreContext *context);
    typedef QStringList (commandHelp_fun)();
private:
    std::function<command_fun> _definition;
    std::function<commandHelp_fun> _helpGetter;

public:
    CommandDefinition(
            const QString &name,
            const std::function<command_fun> &definition,
            const std::function<commandHelp_fun> &helpGetter);

    const QString &name() const;
    const std::function<command_fun> &definition() const;
    const std::function<commandHelp_fun> &helpGetter() const;
};

#endif // COMMANDDEFINITION_H
