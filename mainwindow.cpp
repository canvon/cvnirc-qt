#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::logbufferAppend(const QString &s)
{
    // TODO: Prepend timestamp.
    ui->textEditLogbuffer->append(s);
}

void MainWindow::on_action_Quit_triggered()
{
    // TODO: Close the UI more gently, provide ability to cancel quit.

    // For now, this will have to suffice.
    exit(0);
}

void MainWindow::on_action_Connect_triggered()
{
    ConnectDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    logbufferAppend(
        "Would connect to " + dialog.host() + ":" + dialog.port() +
        ", as user " + dialog.user() + " and nick " + dialog.nick()
    );
}
