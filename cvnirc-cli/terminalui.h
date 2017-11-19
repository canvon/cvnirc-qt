#ifndef TERMINALUI_H
#define TERMINALUI_H

#include <QObject>
#include "ircprotoclient.h"
#include <QTextStream>

class TerminalUI : public QObject
{
    Q_OBJECT
    IRCProtoClient irc;
    QTextStream in, out;
public:
    explicit TerminalUI(QObject *parent, FILE *inFile, FILE *outFile);

    void promptConnect();

signals:

public slots:
    void outLine(const QString &line);
    void outSendingLine(const QString &rawLine);
    void outReceivedLine(const QString &rawLine);

private slots:
    void handle_irc_connectionStateChanged();
    void handle_irc_receivedMessage(IRCProtoMessage &msg);
};

#endif // TERMINALUI_H
