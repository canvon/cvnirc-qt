#ifndef IRCPROTOCLIENT_H
#define IRCPROTOCLIENT_H

#include "cvnirc-core_global.h"

#include <QObject>
#include <QAbstractSocket>
#include <QByteArray>
#include <deque>

#include "ircprotomessage.h"

// FIXME: Replace by wrapping in namespace.
namespace IRCProto = cvnirc::core::IRCProto;

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
#ifdef CVN_HAVE_Q_ENUM
    Q_ENUM(ConnectionState)
#endif


    explicit IRCProtoClient(QObject *parent = 0);

    void connectToIRCServer(const QString &host, const QString &port, const QString &user, const QString &nick);
    void sendRaw(const QString &line);
    void receivedRaw(const IRCProto::MessageOnNetwork &raw);
    void receivedMessageAutonomous(IRCProto::Incoming *in);

    ConnectionState connectionState() const;
    const QString &hostRequestedLast() const;
    const QString &portRequestedLast() const;
    const QString &userRequestedLast() const;
    const QString &nickRequestedLast() const;
    const QString &hostRequestNext() const;
    const QString &portRequestNext() const;
    const QString &userRequestNext() const;
    const QString &nickRequestNext() const;
    void setHostRequestNext(const QString &host);
    void setPortRequestNext(const QString &port);
    void setUserRequestNext(const QString &user);
    void setNickRequestNext(const QString &nick);

    int verboseLevel() const;
    void setVerboseLevel(int newVerboseLevel);

    const QByteArray &rawLineWhitelist() const;
    void setRawLineWhitelist(const QByteArray &newRawLineWhitelist);

    bool isChannel(const QByteArray &token);
    static QString nickUserHost2nick(const QString &nickUserHost);

signals:
    void notifyUser(const QString &msg);
    void sendingLine(const QString &rawLine);
    void receivedLine(const QString &rawLine);
    void receivedMessage(IRCProto::Incoming *in);
    void connectionStateChanged();
    void hostPortRequestedLastChanged();
    void userRequestedLastChanged();
    void nickRequestedLastChanged();

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
    QByteArray  socketReadBuf;
    qint64      socketReadBufUsed = 0;

    std::deque<QString> sendQueue;

    QString _hostRequestedLast, _portRequestedLast;
    QString _userRequestedLast;
    QString _nickRequestedLast;

    QString _hostRequestNext, _portRequestNext;
    QString _userRequestNext;
    QString _nickRequestNext;

    ConnectionState _connectionState;
    void _setConnectionState(ConnectionState newState);

    int _verboseLevel = 1;
    QByteArray _rawLineWhitelist;
    IRCProto::MessageArgTypesHolder _msgArgTypesHolder;
    IRCProto::MessageTypeVocabulary _msgTypeVocabIn;

    void _loadMsgArgTypes();
    void _loadMsgTypeVocabIn();
};

#endif // IRCPROTOCLIENT_H
