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

    pUI->userInput(line);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    TerminalUI ui(stdin, stdout);
    pUI = &ui;

    rl_callback_handler_install("cvnirc> ", cb_linehandler);

    return a.exec();
}
