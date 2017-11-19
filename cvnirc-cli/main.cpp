#include "ircprotoclient.h"

#include <QTextStream>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    IRCProtoClient irc;
    QTextStream in(stdin), out(stdout);
    QString host, port, user, nick;

    out << "Welcome to cvnirc-qt-cli." << endl;

    out << "Server: "; out.flush(); in >> host;
    out << "Port: ";   out.flush(); in >> port;
    out << "User: ";   out.flush(); in >> user;
    out << "Nick: ";   out.flush(); in >> nick;

    out << "Connecting..." << endl;
    irc.connectToIRCServer(host, port, user, nick);

    return a.exec();
}
