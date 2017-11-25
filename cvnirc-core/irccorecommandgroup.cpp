#include "irccorecommandgroup.h"

#include <stdexcept>
#include "command.h"
#include "irccorecontext.h"

IRCCoreCommandGroup::IRCCoreCommandGroup(IRCCore *irc, const QString &groupName, QObject *parent) :
    CommandGroup(groupName, parent), _irc(irc)
{

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

    context->ircProtoClient()->sendRaw(cmd->tokens().join(' '));
}


void IRCCoreCommandGroup::registerAllCommandDefinitions()
{
    using namespace std::placeholders;
    registerCommandDefinition({ "raw",
        std::bind(&IRCCoreCommandGroup::cmd_raw, this, _1, _2),
        std::bind(&IRCCoreCommandGroup::cmdhelp_raw, this)
    });
}
