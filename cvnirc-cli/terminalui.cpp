#include "terminalui.h"

#include <QMetaEnum>

TerminalUI::TerminalUI(FILE *inFileC, FILE *outFileC, QObject *parent) :
    QObject(parent),
    inFile(), outFile(),
    in((inFile.open(inFileC, QIODevice::ReadOnly), &inFile)),
    out((outFile.open(outFileC, QIODevice::WriteOnly), &outFile)),
    inNotify(inFile.handle(), QSocketNotifier::Read)
{
    connect(&inNotify, &QSocketNotifier::activated, this, &TerminalUI::handle_inNotify_activated);

    connect(&irc, &IRCProtoClient::notifyUser, this, &TerminalUI::outLine);
    connect(&irc, &IRCProtoClient::sendingLine, this, &TerminalUI::outSendingLine);
    connect(&irc, &IRCProtoClient::receivedLine, this, &TerminalUI::outReceivedLine);

    connect(&irc, &IRCProtoClient::connectionStateChanged, this, &TerminalUI::handle_irc_connectionStateChanged);
    connect(&irc, &IRCProtoClient::receivedMessage, this, &TerminalUI::handle_irc_receivedMessage);

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

void TerminalUI::handle_inNotify_activated(int socket)
{
    QString userInput;
    while (in.readLineInto(&userInput)) {
        irc.sendRaw(userInput);
    }
}

void TerminalUI::handle_irc_connectionStateChanged()
{
    auto state = irc.connectionState();
    outLine(QString("Connection state changed to ") +
        QString::number((int)state) + QString(": ") +
        // Translate to human-readable.
        QMetaEnum::fromType<IRCProtoClient::ConnectionState>().valueToKey((int)state)
    );
}

void TerminalUI::handle_irc_receivedMessage(IRCProtoMessage &msg)
{
    // FIXME: Implement
}
