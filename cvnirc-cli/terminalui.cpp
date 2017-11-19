#include "terminalui.h"

TerminalUI::TerminalUI(QObject *parent, FILE *inFile, FILE *outFile) :
    QObject(parent), in(inFile), out(outFile)
{
    connect(&irc, &IRCProtoClient::notifyUser, this, &TerminalUI::outLine);
    connect(&irc, &IRCProtoClient::sendingLine, this, &TerminalUI::outSendingLine);
    connect(&irc, &IRCProtoClient::receivedLine, this, &TerminalUI::outReceivedLine);

    out << "Welcome to cvnirc-qt-cli." << endl;

    promptConnect();
}

void TerminalUI::promptConnect()
{
    QString host, port, user, nick;

    out << "Server: "; out.flush(); in >> host;
    out << "Port: ";   out.flush(); in >> port;
    out << "User: ";   out.flush(); in >> user;
    out << "Nick: ";   out.flush(); in >> nick;

    out << "Connecting..." << endl;
    irc.connectToIRCServer(host, port, user, nick);
}

void TerminalUI::outLine(const QString &line)
{
    out << line << endl;
}

void TerminalUI::outSendingLine(const QString &rawLine)
{
    out << "< " << rawLine << endl;
}

void TerminalUI::outReceivedLine(const QString &rawLine)
{
    out << "> " << rawLine << endl;
}
