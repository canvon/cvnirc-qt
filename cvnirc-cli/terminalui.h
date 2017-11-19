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
};

#endif // TERMINALUI_H
