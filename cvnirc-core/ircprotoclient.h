#ifndef IRCPROTOCLIENT_H
#define IRCPROTOCLIENT_H

#include "cvnirc-core_global.h"

#include <QObject>
#include <QAbstractSocket>
#include <vector>
#include <deque>

#include "ircprotomessage.h"

class QTcpSocket;

class CVNIRCCORESHARED_EXPORT IRCProtoClient : public QObject
{
    Q_OBJECT

public:

    enum class ConnectionState {
        Disconnected,
        Connecting,
        Registering,
        Connected
    };
    Q_ENUM(ConnectionState)


    explicit IRCProtoClient(QObject *parent = 0);

    void connectToIRCServer(const QString &host, const QString &port, const QString &user, const QString &nick);
    void sendRaw(const QString &line);
    void receivedRaw(const QString &rawLine);
    void receivedMessageAutonomous(IRCProtoMessage &msg);

    ConnectionState connectionState();
    const QString &hostRequested();
    const QString &portRequested();
    const QString &userRequested();
    const QString &nickRequested();
    void setHostRequested(const QString &host);
    void setPortRequested(const QString &port);
    void setUserRequested(const QString &user);
    void setNickRequested(const QString &nick);

signals:
    void notifyUser(const QString &msg);
    void sendingLine(const QString &rawLine);
    void receivedLine(const QString &rawLine);
    void receivedMessage(IRCProtoMessage &msg);
    void connectionStateChanged();

public slots:
    void reconnectToIRCServer();
    void disconnectFromIRCServer();
    void disconnectFromIRCServer(const QString &quitMsg);

private slots:
    void handle_socket_connected();
    void handle_socket_error(QAbstractSocket::SocketError err);
    void processOutgoingData();
    void processIncomingData();

private:
    QTcpSocket *socket;

    typedef std::vector<char> socketReadBuf_type;
    socketReadBuf_type socketReadBuf;
    int socketReadBufUsed;

    std::deque<QString> sendQueue;

    QString _hostRequested, _portRequested;
    QString _userRequested;
    QString _nickRequested;

    ConnectionState _connectionState;
    void _setConnectionState(ConnectionState newState);
};

#endif // IRCPROTOCLIENT_H
