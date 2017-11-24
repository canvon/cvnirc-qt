#include "irccore.h"

#include "ircprotoclient.h"
#include "irccorecontext.h"

IRCCore::IRCCore(QObject *parent) : QObject(parent)
{
}

const QList<IRCProtoClient *> &IRCCore::ircProtoClients()
{
    return _ircProtoClients;
}

const QList<IRCCoreContext *> &IRCCore::contexts()
{
    return _contexts;
}

IRCCoreContext *IRCCore::createIRCProtoClient()
{
    auto *client = new IRCProtoClient(this);
    _ircProtoClients.append(client);

    auto *context = new IRCCoreContext(client, IRCCoreContext::Type::Server, QString(), this);
    _contexts.append(context);
    createdContext(context);

    return context;
}

IRCCoreContext *IRCCore::connectToIRCServer(const QString &host, const QString &port, const QString &user, const QString &nick)
{
    IRCCoreContext *context = createIRCProtoClient();

    context->ircProtoClient()->connectToIRCServer(host, port, user, nick);

    return context;
}

IRCCoreContext *IRCCore::getContext(IRCProtoClient *ircProtoClient, IRCCoreContext::Type type, const QString &outgoingTarget)
{
    for (IRCCoreContext *context : _contexts) {
        if (context == nullptr)
            continue;

        if (context->ircProtoClient() != ircProtoClient)
            continue;

        if (context->type() != type)
            continue;

        if (context->outgoingTarget() != outgoingTarget)
            continue;

        return context;
    }

    return nullptr;
}

IRCCoreContext *IRCCore::createOrGetContext(IRCProtoClient *ircProtoClient, IRCCoreContext::Type type, const QString &outgoingTarget, bool *created)
{
    IRCCoreContext *context = getContext(ircProtoClient, type, outgoingTarget);
    if (context != nullptr) {
        if (created != nullptr)
            *created = false;

        return context;
    }

    context = new IRCCoreContext(ircProtoClient, type, outgoingTarget, this);
    _contexts.append(context);
    if (created != nullptr)
        *created = true;

    createdContext(context);

    return context;
}
