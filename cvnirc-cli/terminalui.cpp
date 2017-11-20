#include "terminalui.h"

#include <QMetaEnum>

#include <stdio.h>
#include <readline/readline.h>

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

    // (Don't do this. The readline callback function will be registered
    // only *after* the ctor (to ensure that the global pointer is set
    // to an(/the) instance), which would overwrite the readline prompt
    // set by promptConnect() -> _setUserInputState()...)
    //promptConnect();
}

void TerminalUI::promptConnect()
{
    _setUserInputState(UserInputState::Host);
}

TerminalUI::UserInputState TerminalUI::userinputState()
{
    return _userInputState;
}

void TerminalUI::_setUserInputState(UserInputState newState)
{
    QString helpStr("Enter an empty line for the setting to stay the same.");

    switch (newState) {
    case UserInputState::General:
        rl_set_prompt("cvnirc> ");
        break;
    case UserInputState::Host:
        outLine("Server requested is set to \"" + irc.hostRequested() + "\". " + helpStr);
        rl_set_prompt("Server: ");
        break;
    case UserInputState::Port:
        outLine("Port requested is set to \"" + irc.portRequested() + "\". " + helpStr);
        rl_set_prompt("Port: ");
        break;
    case UserInputState::User:
        outLine("User requested is set to \"" + irc.userRequested() + "\". " + helpStr);
        rl_set_prompt("User: ");
        break;
    case UserInputState::Nick:
        outLine("Nick requested is set to \"" + irc.nickRequested() + "\". " + helpStr);
        rl_set_prompt("Nick: ");
        break;
    }

    _userInputState = newState;
    rl_redisplay();
}

void TerminalUI::queueUserInput(const QString &line)
{
    _userInputQueue.append(line);
}

void TerminalUI::userInput(const QString &line)
{
    switch (userinputState()) {
    case UserInputState::Host:
        if (line.isEmpty()) {
            outLine("Server stays at \"" + irc.hostRequested() + "\".");
        }
        else {
            irc.setHostRequested(line);
        }
        _setUserInputState(UserInputState::Port);
        break;
    case UserInputState::Port:
        if (line.isEmpty()) {
            outLine("Port stays at \"" + irc.portRequested() + "\".");
        }
        else {
            irc.setPortRequested(line);
        }
        _setUserInputState(UserInputState::User);
        break;
    case UserInputState::User:
        if (line.isEmpty()) {
            outLine("User stays at \"" + irc.userRequested() + "\".");
        }
        else {
            irc.setUserRequested(line);
        }
        _setUserInputState(UserInputState::Nick);
        break;
    case UserInputState::Nick:
        if (line.isEmpty()) {
            outLine("Nick stays at \"" + irc.nickRequested() + "\".");
        }
        else {
            irc.setNickRequested(line);
        }
        _setUserInputState(UserInputState::General);
        irc.reconnectToIRCServer();
        break;
    case UserInputState::General:
        irc.sendRaw(line);
        break;
    }
}

void TerminalUI::outLine(const QString &line)
{
    rl_clear_visible_line();
    out << line << endl;
    rl_on_new_line();
    rl_redisplay();
}

void TerminalUI::outSendingLine(const QString &rawLine)
{
    outLine("< " + rawLine);
}

void TerminalUI::outReceivedLine(const QString &rawLine)
{
    outLine("> " + rawLine);
}

void TerminalUI::handle_inNotify_activated(int socket)
{
    /*
    QString line;
    while (in.readLineInto(&line)) {
        userInput(line);
    }
    */

    // Call readline.
    //
    // This will call a callback when a line is complete.
    // There previously was a problem of readline reacting differently
    // when its functions were called from the callback or normally.
    // So let the callback just queue lines to later process
    // from outside the callback.
    rl_callback_read_char();

    // Process the input lines possibly queued by the callback.
    while (_userInputQueue.length() > 0) {
        QString line = _userInputQueue.front();
        _userInputQueue.pop_front();

        userInput(line);
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
