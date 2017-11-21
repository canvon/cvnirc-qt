#ifndef IRCCORE_H
#define IRCCORE_H

#include "cvnirc-core_global.h"

#include <QObject>
#include <QList>
#include <QString>
#include "irccorecontext.h"

class IRCProtoClient;

class CVNIRCCORESHARED_EXPORT IRCCore : public QObject
{
    Q_OBJECT
    QList<IRCProtoClient *> _ircProtoClients;
    QList<IRCCoreContext *> _contexts;
public:
    explicit IRCCore(QObject *parent = 0);

    const QList<IRCProtoClient *> &ircProtoClients();
    const QList<IRCCoreContext *> &contexts();

    IRCCoreContext *connectToIRCServer(const QString &host, const QString &port, const QString &user, const QString &nick);
    IRCCoreContext *getContext(IRCProtoClient *ircProtoClient, IRCCoreContext::Type type, const QString &outgoingTarget);
    IRCCoreContext *createOrGetContext(IRCProtoClient *ircProtoClient, IRCCoreContext::Type type, const QString &outgoingTarget, bool *created = nullptr);

signals:
    void createdContext(IRCCoreContext *context);
};

#endif // IRCCORE_H
