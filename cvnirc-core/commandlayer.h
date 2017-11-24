#ifndef COMMANDLAYER_H
#define COMMANDLAYER_H

#include "cvnirc-core_global.h"

#include <QObject>
#include "command.h"

class IRCCore;
class IRCCoreContext;

class CVNIRCCORESHARED_EXPORT CommandLayer : public QObject
{
    Q_OBJECT
    IRCCore *_irc;
public:
    explicit CommandLayer(IRCCore *irc, QObject *parent = 0);

    IRCCore *irc();
    const IRCCore *irc() const;

signals:

public slots:
    void processCommand(const Command &cmd, IRCCoreContext *context = nullptr);
    void processUserInput(const QString &line, IRCCoreContext *context = nullptr);
};

#endif // COMMANDLAYER_H
