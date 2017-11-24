#include <QCoreApplication>
#include "terminalui.h"

#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>

TerminalUI *pUI;

static void cb_linehandler(char *lineC)
{
    QString line(lineC);
    if (lineC != nullptr) {
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
    TerminalUI ui(stdin, stdout);
    pUI = &ui;

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
