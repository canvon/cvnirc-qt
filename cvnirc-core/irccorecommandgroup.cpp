#include "irccorecommandgroup.h"

#include <stdexcept>
#include "command.h"
#include "irccorecontext.h"
#include "irccore.h"

IRCCoreCommandGroup::IRCCoreCommandGroup(IRCCore *irc, const QString &groupName, QObject *parent) :
    CommandGroup(groupName, parent), _irc(irc)
{
    if (_irc == nullptr)
        throw std::invalid_argument("IRCCore commands ctor: Argument \"irc\" can't be null");
}

IRCCore *IRCCoreCommandGroup::irc()
{
    return _irc;
}

const IRCCore *IRCCoreCommandGroup::irc() const
{
    return _irc;
}


QStringList IRCCoreCommandGroup::cmdhelp_raw()
{
    return {
        "Send raw IRC protocol message",
    };
}

void IRCCoreCommandGroup::cmd_raw(Command *cmd, IRCCoreContext *context)
{
    if (cmd == nullptr)
        throw std::invalid_argument("IRCCore command raw: Command object can't be null");

    if (context == nullptr)
        throw std::invalid_argument("IRCCore command raw: Context can't be null");

    QStringList msgTokens = cmd->tokens();
    if (msgTokens.isEmpty())
        throw std::invalid_argument("IRCCore command raw: Command tokens missing");
    msgTokens.removeFirst();

    context->ircProtoClient()->sendRaw(msgTokens.join(' '));
}


QStringList IRCCoreCommandGroup::cmdhelp_join()
{
    return {
        "Join channel",
    };
}

void IRCCoreCommandGroup::cmd_join(Command *cmd, IRCCoreContext *context)
{
    if (cmd == nullptr)
        throw std::invalid_argument("IRCCore command join: Command object can't be null");

    if (context == nullptr)
        throw std::invalid_argument("IRCCore command join: Context can't be null");

    const QStringList &msgTokens(cmd->tokens());
    if (msgTokens.length() != 2)
        throw std::invalid_argument("IRCCore command join: Usage: /join #CHANNEL");
    const QString channelName = msgTokens[1];

    IRCProtoClient *client = context->ircProtoClient();
    if (client == nullptr)
        throw std::invalid_argument("IRCCore command join: Context's IRC protocol client can't be null");

    IRCCoreContext *newContext = _irc->createOrGetContext(client, IRCCoreContext::Type::Channel, channelName);
    newContext->notifyUser("Joining channel " + channelName);
    client->sendRaw("JOIN " + channelName);
}


void IRCCoreCommandGroup::registerAllCommandDefinitions()
{
    using namespace std::placeholders;
    registerCommandDefinition({ "raw",
        std::bind(&IRCCoreCommandGroup::cmd_raw, this, _1, _2),
        std::bind(&IRCCoreCommandGroup::cmdhelp_raw, this)
    });
    registerCommandDefinition({ "join",
        std::bind(&IRCCoreCommandGroup::cmd_join, this, _1, _2),
        std::bind(&IRCCoreCommandGroup::cmdhelp_join, this)
    });

    _registeredOnce = true;
}
