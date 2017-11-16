#ifndef IRCPROTOCLIENT_H
#define IRCPROTOCLIENT_H

#include <QObject>
#include <vector>
#include <deque>

#include "ircprotomessage.h"

class QTcpSocket;

enum class IRCConnectionState {
    Disconnected,
    Connecting,
    Registering,
    Connected
};

class IRCProtoClient : public QObject
{
    Q_OBJECT
public:
    explicit IRCProtoClient(QObject *parent = 0);

    void connectToIRCServer(const QString &host, const QString &port, const QString &user, const QString &nick);
    void reconnectToIRCServer();
    void disconnectFromIRCServer();
    void disconnectFromIRCServer(const QString &quitMsg);
    void sendRaw(const QString &line);
    void receivedRaw(const QString &rawLine);
    void receivedMessageAutonomous(const IRCProtoMessage &msg);

    IRCConnectionState connectionState();

signals:
    void notifyUser(const QString &msg);
    void receivedMessage(const IRCProtoMessage &msg);
    void connectionStateChanged();

public slots:

private slots:
    void on_socket_connected();
    void processOutgoingData();
    void processIncomingData();

private:
    QTcpSocket *socket;

    typedef std::vector<char> socketReadBuf_type;
    socketReadBuf_type socketReadBuf;
    int socketReadBufUsed;

    std::deque<QString> sendQueue;

    QString hostRequested, portRequested;
    QString userRequested;
    QString nickRequested;

    IRCConnectionState _connectionState;
    void _setConnectionState(IRCConnectionState newState);

    void disconnectFromIRCServer(const QString *quitMsg);
};

#endif // IRCPROTOCLIENT_H
