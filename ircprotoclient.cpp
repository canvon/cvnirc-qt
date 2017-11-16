#include "ircprotoclient.h"

#include <QtNetwork>

IRCProtoClient::IRCProtoClient(QObject *parent) : QObject(parent),
    socket(new QTcpSocket(this)),
    socketReadBuf(10*1024),
    socketReadBufUsed(0)
{
    // Set up signals & slots.
    connect(socket, &QIODevice::readyRead, this, &IRCProtoClient::processIncomingData);
}

void IRCProtoClient::connectToIRCServer(const QString &host, const QString &port, const QString &user, const QString &nick)
{
    // TODO: Do sanity checks on user-supplied data.

    notifyUser("Connecting to " + host + ":" + port);
    socket->connectToHost(host, port.toShort());

    notifyUser("Registering as user " + user + "...");
    // "USER" USERNAME HOSTNAME SERVERNAME REALNAME
    // TODO: Allow setting realname.
    sendRaw("USER " + user + " * * :a cvnirc-qt user");

    notifyUser("Requesting nick " + nick + "...");
    sendRaw("NICK " + nick);
}

void IRCProtoClient::sendRaw(const QString &line)
{
    sendQueue.push_back(line);

    if (socket->state() == QAbstractSocket::ConnectedState)
        processOutgoingData();
}

void IRCProtoClient::processOutgoingData()
{
    while (sendQueue.size() > 0) {
        QString rawLine = sendQueue.front();
        notifyUser("< " + rawLine);
        socket->write((rawLine + "\r\n").toUtf8());
        sendQueue.pop_front();
    }
}

void IRCProtoClient::processIncomingData()
{
    processOutgoingData();

    if (socketReadBufUsed >= socketReadBuf.size()) {
        notifyUser("Socket read buffer size exceeded, aborting connection.");
        socket->abort();
        return;
    }

    qint64 ret = 0;
    while ((ret = socket->read(socketReadBuf.data(),
                               socketReadBuf.size() - socketReadBufUsed)
            ) > 0) {
        socketReadBufUsed += ret;

        while (socketReadBufUsed > 0) {
            int state = 0;
            socketReadBuf_type::const_iterator usedEnd = socketReadBuf.cbegin() + socketReadBufUsed;
            socketReadBuf_type::const_iterator iter;

            for (iter = socketReadBuf.cbegin();
                 iter < usedEnd;
                 iter++)
            {
                switch (state) {
                case 0:
                    if (*iter == '\r')
                        state++;
                    break;
                case 1:
                    if (*iter == '\n') {
                        state++;
                    }
                    else {
                        notifyUser("Protocol error: Server seems to have broken line-termination! Aborting connection.");
                        socket->abort();
                        return;
                    }
                    break;
                }

                if (state == 2) {
                    state = 0;
                    qint64 lineLen = iter - socketReadBuf.cbegin() - 1;
                    socketReadBuf[lineLen] = '\0';
                    QString line(socketReadBuf.data());

                    socketReadBuf_type::iterator writeIter = socketReadBuf.begin();
                    socketReadBuf_type::const_iterator readIter = iter;
                    while (usedEnd - readIter > 0)
                        *writeIter++ = *readIter++;
                    socketReadBufUsed -= lineLen + 2;

                    // Interpret messages.
                    receivedRaw(line);
                    break;
                }
            }

            if (iter >= usedEnd)
                break;
        }
    }

    if (ret < 0) {
        notifyUser("Error reading from network socket, aborting connection.");
        socket->abort();
        return;
    }
}

void IRCProtoClient::receivedRaw(const QString &rawLine)
{
    notifyUser("> " + rawLine);

    std::vector<QString> tokens = IRCProtoMessage::splitRawLine(rawLine);
    if (!(tokens.size() >= 1)) {
        notifyUser("Protocol error, aborting connection: Received raw line with no tokens!");
        socket->abort();
        return;
    }

    IRCProtoMessage *msg = nullptr;
    QString source;

    if (tokens[0].length() >= 1 && tokens[0][0] == ':') {
        source = tokens[0];
        tokens.erase(tokens.begin());
    }

    if (!(tokens.size() >= 1)) {
        notifyUser("Protocol error, aborting connection: Received line with a source token only!");
        socket->abort();
        return;
    }

    if (tokens[0] == "PING") {
        if (!(tokens.size() == 2)) {
            notifyUser("Protocol error, aborting connection: Received PING message with unexpected token count " + tokens.size());
            socket->abort();
            return;
        }
        msg = new PingPongIRCProtoMessage(rawLine, source, IRCMsgType::Ping, tokens[1]);
    }
    else {
        msg = new IRCProtoMessage(rawLine);
    }

    if (msg != nullptr) {
        receivedMessage(*msg);
        delete msg;
    }
}
