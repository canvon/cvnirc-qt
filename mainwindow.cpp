#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectdialog.h"

#include <QDate>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    baseWindowTitle("cvnirc-qt")
{
    ui->setupUi(this);

    connect(&irc, &IRCProtoClient::notifyUser, this, &MainWindow::logbufferAppend);
    connect(&irc, &IRCProtoClient::receivedMessage, this, &MainWindow::handle_irc_receivedMessage);
    connect(&irc, &IRCProtoClient::connectionStateChanged, this, &MainWindow::handle_irc_connectionStateChanged);

    connect(ui->action_Reconnect, &QAction::triggered, &irc, &IRCProtoClient::reconnectToIRCServer);
    connect(ui->action_Disconnect, &QAction::triggered,
            &irc, static_cast<void (IRCProtoClient::*)()>(&IRCProtoClient::disconnectFromIRCServer));
    // ^ The cast is necessary to select one of the overloaded methods.

    // Immediately let the user type commands.
    ui->lineEditUserInput->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateState()
{
    switch (irc.connectionState()) {
    case IRCConnectionState::Disconnected:
        setWindowTitle(baseWindowTitle);
        break;
    case IRCConnectionState::Connecting:
        setWindowTitle(irc.hostRequested() + " - " + baseWindowTitle + " (Connecting...)");
        break;
    case IRCConnectionState::Registering:
        setWindowTitle(irc.hostRequested() + " - " + baseWindowTitle + " (Registering...)");
        break;
    case IRCConnectionState::Connected:
        setWindowTitle(irc.hostRequested() + " - " + baseWindowTitle + " (Connected)");
        break;
    }
}

void MainWindow::logbufferAppend(const QString &s)
{
    // Prepend timestamp and append to logbuffer.
    QDateTime ts = QDateTime::currentDateTime();
    ui->textEditLogbuffer->append("[" + ts.toString() + "] " + s);
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

    irc.connectToIRCServer(dialog.host(), dialog.port(), dialog.user(), dialog.nick());
}

void MainWindow::on_pushButtonUserInput_clicked()
{
    QString line = ui->lineEditUserInput->text();
    if (line.length() < 1)
        return;

    bool isCommand = false;
    if (line[0] == '/') {
        if (line.length() == 1) {
            line.remove(0, 1);
        }
        else if (line[1] == ' ') {
            line.remove(0, 2);
        }
        else {
            isCommand = true;
            line.remove(0, 1);
        }
    }

    if (!isCommand) {
        // FIXME: Build a PRIVMSG command from that.
        logbufferAppend("(Stub: Would send this as normal text message: \"" + line + "\")");
    }
    else {
        // TODO: Pre-parse as a command, to have local meanings as well.
        //logbufferAppend("Sending command: \"" + line + "\"");
        irc.sendRaw(line);
    }

    ui->lineEditUserInput->setText("");
}

void MainWindow::handle_irc_receivedMessage(IRCProtoMessage &msg)
{
    switch (msg.msgType) {
    default:
        break;
    }
}

void MainWindow::handle_irc_connectionStateChanged()
{
    updateState();
}
