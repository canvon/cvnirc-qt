#ifndef COMMANDLAYER_H
#define COMMANDLAYER_H

#include "cvnirc-core_global.h"

#include <QObject>
#include "command.h"
#include "commandgroup.h"

class IRCCore;
class IRCCoreContext;

class CVNIRCCORESHARED_EXPORT CommandLayer : public QObject
{
    Q_OBJECT
    RootCommandGroup _rootCommandGroup;

public:
    explicit CommandLayer(QObject *parent = 0);

    RootCommandGroup &rootCommandGroup();
    const RootCommandGroup &rootCommandGroup() const;

signals:

public slots:
    void processCommand(Command *cmd, IRCCoreContext *context = nullptr);
    void processUserInput(const QString &line, IRCCoreContext *context = nullptr);
};

#endif // COMMANDLAYER_H
