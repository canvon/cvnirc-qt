#include "terminalui.h"

#include <QMetaEnum>

#include <stdio.h>
#include <readline/readline.h>

TerminalUI::TerminalUI(FILE *inFileC, FILE *outFileC, QObject *parent) :
    QObject(parent),
    irc(this),
    cmdLayer(&irc, this),
    inFile(this), outFile(this),
    in((inFile.open(inFileC, QIODevice::ReadOnly), &inFile)),
    out((outFile.open(outFileC, QIODevice::WriteOnly), &outFile)),
    inNotify(inFile.handle(), QSocketNotifier::Read, this)
{
    connect(&inNotify, &QSocketNotifier::activated, this, &TerminalUI::handle_inNotify_activated);

    connect(&irc, &IRCCore::createdContext, this, &TerminalUI::handle_irc_createdContext);

    out << "Welcome to cvnirc-qt-cli." << endl;

    currentContext = irc.createIRCProtoClient();

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
    IRCProtoClient *client = nullptr;

    if (newState != UserInputState::General) {
        if (currentContext == nullptr)
            throw std::runtime_error("TerminalUI UserInputState setter: Current context was not set");

        client = currentContext->ircProtoClient();
        if (client == nullptr)
            throw std::runtime_error("TerminalUI UserInputState setter: Context's IRC protocol client was not set");
    }

    QString prevValue;
    QString helpStr("Enter an empty line for the setting to stay the same.");

    switch (newState) {
    case UserInputState::General:
        rl_set_prompt("cvnirc> ");
        break;
    case UserInputState::Host:
        prevValue = client->hostRequestNext();
        if (!prevValue.isEmpty())
            outLine("Previous server to request next was \"" + prevValue + "\". " + helpStr);
        rl_set_prompt("Server: ");
        break;
    case UserInputState::Port:
        prevValue = client->portRequestNext();
        if (!prevValue.isEmpty())
            outLine("Previous port to request next was \"" + prevValue + "\". " + helpStr);
        rl_set_prompt("Port: ");
        break;
    case UserInputState::User:
        prevValue = client->userRequestNext();
        if (!prevValue.isEmpty())
            outLine("Previous user to request next was \"" + prevValue + "\". " + helpStr);
        rl_set_prompt("User: ");
        break;
    case UserInputState::Nick:
        prevValue = client->nickRequestNext();
        if (!prevValue.isEmpty())
            outLine("Previous nick to request next was \"" + prevValue + "\". " + helpStr);
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
    IRCProtoClient *client = nullptr;

    if (_userInputState != UserInputState::General) {
        if (currentContext == nullptr)
            throw std::runtime_error("TerminalUI user input handler: Current context was not set");

        client = currentContext->ircProtoClient();
        if (client == nullptr)
            throw std::runtime_error("TerminalUI user input handler: Context's IRC protocol client was not set");
    }

    QString prevValue;

    switch (_userInputState) {
    case UserInputState::Host:
        prevValue = client->hostRequestNext();
        if (prevValue.isEmpty() && line.isEmpty()) {
            outLine("Error: There is no previous value set.");
            return;
        }
        else if (line.isEmpty()) {
            outLine("Server to request next stays at \"" + prevValue + "\".");
        }
        else {
            client->setHostRequestNext(line);
        }
        _setUserInputState(UserInputState::Port);
        break;
    case UserInputState::Port:
        prevValue = client->portRequestNext();
        if (prevValue.isEmpty() && line.isEmpty()) {
            outLine("Error: There is no previous value set.");
            return;
        }
        else if (line.isEmpty()) {
            outLine("Port to request next stays at \"" + prevValue + "\".");
        }
        else {
            client->setPortRequestNext(line);
        }
        _setUserInputState(UserInputState::User);
        break;
    case UserInputState::User:
        prevValue = client->userRequestNext();
        if (prevValue.isEmpty() && line.isEmpty()) {
            outLine("Error: There is no previous value set.");
            return;
        }
        else if (line.isEmpty()) {
            outLine("User to request next stays at \"" + prevValue + "\".");
        }
        else {
            client->setUserRequestNext(line);
        }
        _setUserInputState(UserInputState::Nick);
        break;
    case UserInputState::Nick:
        prevValue = client->nickRequestNext();
        if (prevValue.isEmpty() && line.isEmpty()) {
            outLine("Error: There is no previous value set.");
            return;
        }
        else if (line.isEmpty()) {
            outLine("Nick to request next stays at \"" + prevValue + "\".");
        }
        else {
            client->setNickRequestNext(line);
        }
        _setUserInputState(UserInputState::General);
        client->reconnectToIRCServer();
        break;
    case UserInputState::General:
        cmdLayer.processUserInput(line, currentContext);
        break;
    }
}

void TerminalUI::outLine(const QString &line, IRCCoreContext *context)
{
#if RL_VERSION_MAJOR < 7
#warning "Your GNU readline library is too old, will have to do without rl_clear_visible_line()..."
    // Just leave the prompt + partial input line where it is,
    // but enter a new line so that output will look sane.
    out << endl;
#else
    rl_clear_visible_line();
#endif
    out << line << endl;
    rl_on_new_line();
    rl_redisplay();
}

void TerminalUI::outSendingLine(const QString &rawLine, IRCCoreContext *context)
{
    outLine("< " + rawLine, context);
}

void TerminalUI::outReceivedLine(const QString &rawLine, IRCCoreContext *context)
{
    outLine("> " + rawLine, context);
}

void TerminalUI::handle_inNotify_activated(int /* socket */)
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

void TerminalUI::handle_context_connectionStateChanged(IRCCoreContext *context)
{
    auto state = context->ircProtoClient()->connectionState();
    outLine(QString("Connection state changed to ") +
        QString::number((int)state)
#ifdef CVN_HAVE_Q_ENUM
        + QString(": ")
        // Translate to human-readable.
        + QMetaEnum::fromType<IRCProtoClient::ConnectionState>().valueToKey((int)state)
#endif
        , context
    );
}

void TerminalUI::handle_irc_createdContext(IRCCoreContext *context)
{
    connect(context, &IRCCoreContext::notifyUser, this, &TerminalUI::outLine);
    connect(context, &IRCCoreContext::sendingLine, this, &TerminalUI::outSendingLine);
    connect(context, &IRCCoreContext::receivedLine, this, &TerminalUI::outReceivedLine);

    connect(context, &IRCCoreContext::connectionStateChanged, this, &TerminalUI::handle_context_connectionStateChanged);
}
