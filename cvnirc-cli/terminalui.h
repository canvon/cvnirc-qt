#ifndef TERMINALUI_H
#define TERMINALUI_H

#include <QObject>
#include "irccore.h"
#include "commandlayer.h"
#include <QFile>
#include <QTextStream>
#include <QSocketNotifier>

class TerminalUI : public QObject
{
    Q_OBJECT
    IRCCore irc;
    IRCCoreContext *currentContext = nullptr;
    CommandLayer cmdLayer;
    QFile inFile, outFile;
    QTextStream in, out;
    QSocketNotifier inNotify;
public:
    enum class UserInputState {
        General,
        Host, Port, User, Nick,
    };
#ifdef CVN_HAVE_Q_ENUM
    Q_ENUM(UserInputState)
#endif

    explicit TerminalUI(FILE *inFileC, FILE *outFileC, QObject *parent = 0);

    void promptConnect();
    UserInputState userinputState();

signals:

public slots:
    void queueUserInput(const QString &line);
    void userInput(const QString &line);
    void outLine(const QString &line, IRCCoreContext *context = nullptr);
    void outSendingLine(const QString &rawLine, IRCCoreContext *context = nullptr);
    void outReceivedLine(const QString &rawLine, IRCCoreContext *context = nullptr);

private slots:
    void handle_inNotify_activated(int socket);
    void handle_context_connectionStateChanged(IRCCoreContext *context = nullptr);
    void handle_irc_createdContext(IRCCoreContext *context);

private:
    QStringList _userInputQueue;
    UserInputState _userInputState = UserInputState::General;

    void _setUserInputState(UserInputState newState);
};

#endif // TERMINALUI_H
