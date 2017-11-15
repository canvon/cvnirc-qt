#ifndef IRCPROTOCLIENT_H
#define IRCPROTOCLIENT_H

#include <QObject>
#include <vector>

#include "ircprotomessage.h"

class QTcpSocket;

class IRCProtoClient : public QObject
{
    Q_OBJECT
public:
    explicit IRCProtoClient(QObject *parent = 0);

    void connectToIRCServer(const QString &host, const QString &port, const QString &user, const QString &nick);
    void sendRaw(const QString &line);

signals:
    void notifyUser(const QString &msg);
    void receivedMessage(const IRCProtoMessage &msg);

public slots:

private slots:
    void processIncomingData();

private:
    QTcpSocket *socket;

    typedef std::vector<char> socketReadBuf_type;
    socketReadBuf_type socketReadBuf;
    int socketReadBufUsed;
};

#endif // IRCPROTOCLIENT_H
