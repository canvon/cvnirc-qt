#include "terminalui.h"

TerminalUI::TerminalUI(QObject *parent, FILE *inFile, FILE *outFile) :
    QObject(parent), in(inFile), out(outFile)
{
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
