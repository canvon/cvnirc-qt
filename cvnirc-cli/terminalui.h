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
    enum class UserInputState {
        General,
        Host, Port, User, Nick,
    };
    Q_ENUM(UserInputState)

    explicit TerminalUI(FILE *inFileC, FILE *outFileC, QObject *parent = 0);

    void promptConnect();
    UserInputState userinputState();

signals:

public slots:
    void queueUserInput(const QString &line);
    void userInput(const QString &line);
    void outLine(const QString &line);
    void outSendingLine(const QString &rawLine);
    void outReceivedLine(const QString &rawLine);

private slots:
    void handle_inNotify_activated(int socket);

    void handle_irc_connectionStateChanged();
    void handle_irc_receivedMessage(IRCProtoMessage &msg);

private:
    QStringList _userInputQueue;
    UserInputState _userInputState = UserInputState::General;

    void _setUserInputState(UserInputState newState);
};

#endif // TERMINALUI_H
