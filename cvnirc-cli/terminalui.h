#ifndef TERMINALUI_H
#define TERMINALUI_H

#include <QObject>
#include "ircprotoclient.h"
#include <QFile>
#include <QTextStream>
#include <QSocketNotifier>

class TerminalUI : public QObject
{
    Q_OBJECT
    IRCProtoClient irc;
    QFile inFile, outFile;
    QTextStream in, out;
    QSocketNotifier inNotify;
public:
    explicit TerminalUI(FILE *inFileC, FILE *outFileC, QObject *parent = 0);

    void promptConnect();

signals:

public slots:
    void outLine(const QString &line);
    void outSendingLine(const QString &rawLine);
    void outReceivedLine(const QString &rawLine);

private slots:
    void handle_inNotify_activated(int socket);

    void handle_irc_connectionStateChanged();
    void handle_irc_receivedMessage(IRCProtoMessage &msg);
};

#endif // TERMINALUI_H
