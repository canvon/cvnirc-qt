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
    receivedMessage("Connecting to " + host + ":" + port +
        ", as user " + user + " and nick " + nick);
    socket->connectToHost(host, port.toShort());
}

void IRCProtoClient::sendRaw(const QString &line)
{
    receivedMessage("< " + line);
    socket->write((line + "\r\n").toUtf8());
}

void IRCProtoClient::processIncomingData()
{
    if (socketReadBufUsed >= socketReadBuf.size()) {
        receivedMessage("Socket read buffer size exceeded, aborting connection.");
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
                        receivedMessage("Protocol error: Server seems to have broken line-termination! Aborting connection.");
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

                    receivedMessage("> " + line);

                    // TODO: Interpret messages.

                    break;
                }
            }

            if (iter >= usedEnd)
                break;
        }
    }

    if (ret < 0) {
        receivedMessage("Error reading from network socket, aborting connection.");
        socket->abort();
        return;
    }
}
