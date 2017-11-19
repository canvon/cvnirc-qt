#include <QCoreApplication>
#include "terminalui.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    TerminalUI ui(stdin, stdout);

    return a.exec();
}
