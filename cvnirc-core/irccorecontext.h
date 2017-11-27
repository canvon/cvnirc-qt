#ifndef IRCCORECONTEXT_H
#define IRCCORECONTEXT_H

#include "cvnirc-core_global.h"

#include <QObject>
#include "ircprotoclient.h"

class CVNIRCCORESHARED_EXPORT IRCCoreContext : public QObject
{
    Q_OBJECT
    IRCProtoClient *_ircProtoClient;

public:
    enum class Type {
        Server,
        Channel,
        Query,
    };
#ifdef CVN_HAVE_Q_ENUM
    Q_ENUM(Type)
#endif

private:
    Type _type;
    QString _outgoingTarget;

public:
    explicit IRCCoreContext(IRCProtoClient *ircProtoClient, Type type, const QString &outgoingTarget, QObject *parent = 0);

    bool operator ==(const IRCCoreContext &other);

    IRCProtoClient *ircProtoClient();
    const IRCProtoClient *ircProtoClient() const;
    Type type() const;
    const QString &outgoingTarget() const;

    QString disambiguator() const;

    void requestFocus();

signals:
    void connectionStateChanged(IRCCoreContext *context = nullptr);
    void notifyUser(const QString &line, IRCCoreContext *context = nullptr);
    void sendingLine(const QString &rawLine, IRCCoreContext *context = nullptr);
    void receivedLine(const QString &rawLine, IRCCoreContext *context = nullptr);

    void focusWanted(IRCCoreContext *context = nullptr);

public slots:
    void receiveIRCProtoMessage(IRCProtoMessage &msg);
    void sendChatMessage(const QString &line);

private slots:
    void handle_connectionStateChanged();
    void handle_notifyUser(const QString &line);
    void handle_sendingLine(const QString &rawLine);
    void handle_receivedLine(const QString &rawLine);
};

#endif // IRCCORECONTEXT_H
