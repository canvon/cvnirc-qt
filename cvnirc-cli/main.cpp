#include <QCoreApplication>
#include "terminalui.h"
#include <QCommandLineParser>

#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

TerminalUI *pUI;

static void cb_linehandler(char *lineC)
{
    QString line(lineC);
    if (lineC != nullptr) {
        add_history(lineC);
        free(lineC);
        lineC = nullptr;
    }

    if (line.isNull()) {
        printf("\n");
        rl_callback_handler_remove();
        return qApp->exit();
    }

    pUI->queueUserInput(line);
}

int cycle_context(int count, int /* key */)
{
    if (!pUI->cycleCurrentContext(count))
        return 1;

    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;

    parser.setApplicationDescription("canvon IRC client built-with-Qt-framework CLI (command-line interface)");
    parser.addHelpOption();
    //parser.addVersionOption();  // Conflicts with -v to increase verbose level.

    QCommandLineOption optQuiet({ "q", "quiet" }, "Decrease verbose level.");
    QCommandLineOption optVerbose({ "v", "verbose" }, "Increase verbose level.");
    if (!parser.addOptions({ optQuiet, optVerbose })) {
        fputs("Failed to add options for verbose level\n", stderr);
        return 1;
    }

#if 0  // TODO: Implement additional command-line arguments.
    parser.addPositionalArgument("URLs", "IRC URL(s) to open.", "[irc://SERVER/CHANNEL [...]]");
    parser.addPositionalArgument("commands", "One or more /COMMAND to execute as if it was typed in.", "[/COMMAND [...]]");
#endif

    parser.process(a);


    // Create the user interface.
    TerminalUI ui(stdin, stdout);
    pUI = &ui;


    // Determine verbose level.
    for (QString optionName : parser.optionNames()) {
        if (optionName == "q") {
            ui.setVerboseLevel(ui.verboseLevel() - 1);
        }
        else if (optionName == "v") {
            ui.setVerboseLevel(ui.verboseLevel() + 1);
        }
        //else  // Trust parser.process() that all is fine.
    }

    // Automatically adjust first/single IRC protocol client to match.
    //
    // (On subsequent adjusts, both general and per-client verbose level
    // will need to get adjusted independently of each other, once the command
    // for that will have been implemented at all.)
    //
    ui.getIRC().ircProtoClients().front()->setVerboseLevel(ui.verboseLevel());


    // Set up GNU readline library.
    //
    // Use the alternate interface.
    rl_callback_handler_install("cvnirc> ", cb_linehandler);
    //
    // Disable tab completion on file names.
    rl_bind_key('\t', rl_insert);
    //
    // Allow easy current context switching.
    // Note: ^X did not work, but ^O does, on my system.
    //rl_bind_key(24 /* ^X */, cycle_context);
    rl_bind_key(15 /* ^O */, cycle_context);


    // Start prompting for connection information.
    ui.promptConnect();


    return a.exec();
}
