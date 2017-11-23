#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectdialog.h"

#include <QMetaEnum>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    irc(this),
    ui(new Ui::MainWindow),
    baseWindowTitle("cvnirc-qt")
{
    ui->setupUi(this);

    ui->logBufferProto->setType(LogBuffer::Type::Protocol);

    connect(&irc, &IRCCore::createdContext, this, &MainWindow::handle_irc_createdContext);
    //connect(&irc, &IRCProtoClient::notifyUser, ui->logBufferMain, &LogBuffer::appendLine);
    //connect(&irc, &IRCProtoClient::sendingLine, ui->logBufferProto, &LogBuffer::appendSendingLine);
    //connect(&irc, &IRCProtoClient::receivedLine, ui->logBufferProto, &LogBuffer::appendReceivedLine);
    //connect(&irc, &IRCProtoClient::receivedMessage, this, &MainWindow::handle_irc_receivedMessage);
    //connect(&irc, &IRCProtoClient::connectionStateChanged, this, &MainWindow::handle_irc_connectionStateChanged);

    //connect(ui->action_Reconnect, &QAction::triggered, &irc, &IRCProtoClient::reconnectToIRCServer);
    //connect(ui->action_Disconnect, &QAction::triggered,
    //        &irc, static_cast<void (IRCProtoClient::*)()>(&IRCProtoClient::disconnectFromIRCServer));
    // ^ The cast is necessary to select one of the overloaded methods.

    // Immediately let the user type commands.
    ui->lineEditUserInput->setFocus();

    updateState();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateState()
{
    auto &clients(irc.ircProtoClients());

    int len = clients.length();
    switch (len) {
    case 0:
        setWindowTitle(baseWindowTitle + " (Disconnected)");
        break;
    case 1:
        {
            auto &client(*clients.front());

            switch (client.connectionState()) {
            case IRCProtoClient::ConnectionState::Disconnected:
                setWindowTitle(baseWindowTitle + " (Disconnected)");
                break;
            case IRCProtoClient::ConnectionState::Connecting:
                setWindowTitle(client.hostRequestedLast() + " - " + baseWindowTitle + " (Connecting...)");
                break;
            case IRCProtoClient::ConnectionState::Registering:
                setWindowTitle(client.hostRequestedLast() + " - " + baseWindowTitle + " (Registering...)");
                break;
            case IRCProtoClient::ConnectionState::Connected:
                setWindowTitle(client.hostRequestedLast() + " - " + baseWindowTitle + " (Connected)");
                break;
            }
        }
        break;
    default:
        setWindowTitle(baseWindowTitle + " (" + QString::number(len) + " clients)");
        break;
    }
}

QWidget *MainWindow::findTabWidgetForContext(IRCCoreContext *context)
{
    // (Start at index 1 to skip main logbuffer.)
    for (int i = 1; i < ui->tabWidget->count(); i++) {
        QWidget *w = ui->tabWidget->widget(i);
        auto *logBuf = dynamic_cast<LogBuffer *>(w);
        if (logBuf == nullptr)
            continue;

        if (logBuf->contexts().contains(context))
            return w;
    }

    return nullptr;
}

QWidget *MainWindow::openTabForContext(IRCCoreContext *context)
{
    if (context == nullptr)
        throw new std::runtime_error("MainWindow::openTabForContext(): Got null pointer, which is invalid here");

    QWidget *w = findTabWidgetForContext(context);
    if (w == nullptr) {
        auto *logBuf = new LogBuffer();
        logBuf->addContext(context);
        w = logBuf;

        QString tabName = context->outgoingTarget();
        if (tabName.isEmpty()) {
            switch (context->type()) {
            case IRCCoreContext::Type::Server:
                {
                    QString host = context->ircProtoClient()->hostRequestedLast();
                    if (host.isEmpty())
                        tabName = "(Disconnected server)";
                    else
                        tabName = "Server " + host;
                }
                break;
            default:
                tabName = "???";
                break;
            }
        }

        ui->tabWidget->addTab(w, tabName);
    }
    return w;
}

// Find out what's the current context.
IRCCoreContext *MainWindow::contextFromUI()
{
    QWidget *w = ui->tabWidget->currentWidget();

    auto *logBuf = dynamic_cast<LogBuffer *>(w);
    if (logBuf == nullptr)
        return nullptr;

    auto &logBufContexts(logBuf->contexts());
    if (!(logBufContexts.count() > 0))
        return nullptr;

    auto *context = logBufContexts.front();
    if (context == nullptr)
        throw std::runtime_error("MainWindow::contextFromUI(): Got context that is a null pointer, which is invalid here");

    return context;
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

void MainWindow::on_action_Reconnect_triggered()
{
    IRCCoreContext *context = contextFromUI();
    if (context == nullptr) {
        ui->logBufferMain->appendLine("No active context. Can't reconnect; please use File->Connect dialog instead.");
        return;
    }

    context->ircProtoClient()->reconnectToIRCServer();
}

void MainWindow::on_action_Disconnect_triggered()
{
    IRCCoreContext *context = contextFromUI();
    if (context == nullptr) {
        ui->logBufferMain->appendLine("No active context. Can't disconnect; please go to a tab of the connection you want to disconnect and try again.");
        return;
    }

    context->ircProtoClient()->disconnectFromIRCServer();
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

    IRCCoreContext *context = contextFromUI();
    if (context == nullptr) {
        ui->logBufferMain->appendLine("Error getting context. Don't know to where I should send!");
        return;
    }

    if (!isCommand) {
        context->sendChatMessage(line);
    }
    else {
        // TODO: Pre-parse as a command, to have local meanings as well.
        context->ircProtoClient()->sendRaw(line);
    }

    ui->lineEditUserInput->setText("");
}

void MainWindow::handle_irc_createdContext(IRCCoreContext *context)
{
    if (context == nullptr)
        throw std::runtime_error("MainWindow, handle IRCCore created context: Got context that is a null pointer, which is invalid here");

    // Create our GUI representation of IRCCore's contexts.
    openTabForContext(context);

    // Is this a server/~connection context?
    if (context->type() == IRCCoreContext::Type::Server) {
        // Wire up the protocol logbuffer.
        ui->logBufferProto->addContext(context);

        connect(context->ircProtoClient(), &IRCProtoClient::connectionStateChanged,
                this, &MainWindow::updateState);
        connect(context->ircProtoClient(), &IRCProtoClient::hostPortRequestedLastChanged,
                this, &MainWindow::updateState);
    }
}
