#ifndef TERMINALUI_H
#define TERMINALUI_H

#include <QObject>
#include "irccore.h"
#include "commandlayer.h"
#include <QFile>
#include <QTextStream>
#include <QSocketNotifier>
#include <QByteArray>

class TerminalUI : public QObject
{
    Q_OBJECT
    IRCCore _irc;
    IRCCoreContext *_currentContext = nullptr;
    CommandLayer _cmdLayer;
    QFile _inFile, _outFile;
    QTextStream _in, _out;
    QSocketNotifier _inNotify;
    int _verboseLevel = 1;
    QByteArray _rlPromptHolder;
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
    void updateGeneralPrompt();
    UserInputState userinputState();

    // FIXME: Rename the private field to _irc, so that the getter can be irc().
    IRCCore &irc();
    const IRCCore &irc() const;

    int verboseLevel() const;
    void setVerboseLevel(int newVerboseLevel);

signals:

public slots:
    void queueUserInput(const QString &line);
    void userInput(const QString &line);
    bool cycleCurrentContext(int count);
    bool switchToContext(IRCCoreContext *context);
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
