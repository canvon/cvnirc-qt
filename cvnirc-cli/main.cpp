#include <QCoreApplication>
#include "terminalui.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    TerminalUI ui(nullptr, stdin, stdout);

    return a.exec();
}
