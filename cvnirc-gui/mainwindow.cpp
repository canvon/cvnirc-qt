#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectdialog.h"
#include "irccorecommandgroup.h"

#include <QMetaEnum>
#include <QFileInfo>
#include <QDir>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _helpViewer(this),
    irc(this),
    cmdLayer(this),
    ui(new Ui::MainWindow),
    baseWindowTitle("cvnirc-qt")
{
    ui->setupUi(this);

    ui->logBufferProto->setType(LogBuffer::Type::Protocol);

    // Make some commands available to the user.
    cmdLayer.rootCommandGroup().addSubGroup(new IRCCoreCommandGroup(&irc, "IRC"));
    // TODO: Also register UI-specific commands.

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

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::handle_tabWidget_currentChanged);

#ifdef CVN_HAVE_QPROCESS_ERROROCCURRED
    connect(&_helpViewer, &QProcess::errorOccurred, this, &MainWindow::handle_helpViewer_errorOccurred);
#else
#warning "QProcess signal errorOccurred missing. GUI will not be able to tell when invoking the external help browser has failed."
#endif

    // Immediately let the user type commands.
    ui->lineEditUserInput->setFocus();

    updateState();
    updateSwitchToTabMenu();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateState()
{
    auto &clients(irc.ircProtoClients());

    // Update window title.
    int len = clients.length();
    switch (len) {
    case 0:
        setWindowTitle(baseWindowTitle + " (Disconnected)");
        break;
    case 1:
        {
            auto &client(*clients.front());
            QString host = client.hostRequestedLast();
            QString connectionInfo = host.isNull() ? "N.A." : host;

            switch (client.connectionState()) {
            case IRCProtoClient::ConnectionState::Disconnected:
                setWindowTitle(connectionInfo + " - " + baseWindowTitle + " (Disconnected)");
                break;
            case IRCProtoClient::ConnectionState::Connecting:
                setWindowTitle(connectionInfo + " - " + baseWindowTitle + " (Connecting...)");
                break;
            case IRCProtoClient::ConnectionState::Registering:
                setWindowTitle(connectionInfo + " - " + baseWindowTitle + " (Registering...)");
                break;
            case IRCProtoClient::ConnectionState::Connected:
                setWindowTitle(connectionInfo + " - " + baseWindowTitle + " (Connected)");
                break;
            }
        }
        break;
    default:
        setWindowTitle(baseWindowTitle + " (" + QString::number(len) + " clients)");
        break;
    }

    // Update tab names.
    // N.B.: Skip Main tab.
    for (int i = 1; i < ui->tabWidget->count(); i++) {
        QWidget *w = ui->tabWidget->widget(i);
        auto *logBuf = dynamic_cast<LogBuffer *>(w);
        if (logBuf == nullptr) {
            qDebug() << Q_FUNC_INFO << i << "This tab is not a LogBuffer!";
            continue;
        }

        applyTabNameComponents(logBuf, tabNameComponents(*logBuf));
    }
}

void MainWindow::updateSwitchToTabMenu()
{
    auto &menu(*ui->menuSwitchToTab);
    menu.clear();
    for (int i = 0; i < ui->tabWidget->count(); i++) {
        const QString tabName = ui->tabWidget->tabText(i);
        const int num = i + 1;
        const QString numStr = QString::number(num);
        const QString actionText = (num < 10 ? "&" : "") + numStr + " - " + tabName;
        const QKeySequence shortcut("Alt+" + numStr);

        QAction *action = menu.addAction(actionText,
            this, &MainWindow::handle_menuTab_triggered,
            num < 10 ? QKeySequence("Alt+" + numStr)
                     : num == 10 ? QKeySequence("Alt+0") : 0);
        if (action == nullptr) {
            qDebug() << Q_FUNC_INFO << "menu.addAction() failed, ignoring";
            continue;
        }

        action->setData(i);
    }
}

void MainWindow::switchToContextTab(IRCCoreContext *context)
{
    if (context == nullptr) {
        // Switch to Main tab.
        ui->tabWidget->setCurrentIndex(0);
        return;
    }

    auto *tabWidget = findTabWidgetForContext(context);
    if (tabWidget == nullptr) {
        ui->logBufferMain->appendLine("Switch to context tab: Can't find associated tab");
        return;
    }

    ui->tabWidget->setCurrentWidget(tabWidget);
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

        ui->tabWidget->addTab(w, "New tab");
        applyTabNameComponents(logBuf, tabNameComponents(*logBuf));

        connect(logBuf, &LogBuffer::activityChanged, this, &MainWindow::handle_logBuffer_activityChanged);

        updateSwitchToTabMenu();
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

QStringList MainWindow::tabNameComponents(const LogBuffer &logBuf)
{
    QStringList ret;

    for (IRCCoreContext *context : logBuf.contexts()) {
        if (context == nullptr)
            throw std::runtime_error("MainWindow::tabNameComponents(): Got context that is a null pointer, which is invalid here");

        QString tabName = context->disambiguator();

        ret.append(tabName);
    }

    return ret;
}

void MainWindow::applyTabNameComponents(LogBuffer *logBuf, const QStringList &components)
{
    int index = ui->tabWidget->indexOf(logBuf);
    if (index < 0) {
        qDebug() << Q_FUNC_INFO << "Tab not found";
        return;
    }

    QString tabText, tabToolTip;
    switch (components.length()) {
    case 0:
        tabText = "(empty)";
        tabToolTip.clear();
        break;
    case 1:
        tabText = components.front();
        tabToolTip.clear();
        break;
    default:
        tabText = components.front() + ",...";
        tabToolTip = components.join(',');
        break;
    }

    // The tab text gets scanned for shortcuts.
    tabText.replace("&", "&&");
    if (!components.isEmpty()) {
        int iAlpha = -1;
        for (int i = tabText.startsWith("Q:") ? 2 : 0; i < tabText.length(); i++) {
            if (tabText[i].isLetter()) {
                iAlpha = i;
                break;
            }
        }

        if (iAlpha > 0)
            tabText.insert(iAlpha, "&");
    }

    ui->tabWidget->setTabText(index, tabText);
    ui->tabWidget->setTabToolTip(index, tabToolTip);
}

void MainWindow::on_action_Quit_triggered()
{
    // TODO: Close the UI more gently, provide ability to cancel quit.

    qApp->exit();
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

void MainWindow::on_actionFocusUserInput_triggered()
{
    ui->lineEditUserInput->setFocus();
}

void MainWindow::on_pushButtonUserInput_clicked()
{
    QString line = ui->lineEditUserInput->text();
    IRCCoreContext *context = contextFromUI();
    if (context == nullptr) {
        ui->logBufferMain->appendLine("Error getting context. Don't know to where I should send!");
        return;
    }

    try {
        cmdLayer.processUserInput(line, context);
    }
    catch (const std::exception &ex) {
        context->notifyUser(QString("Error processing user input: ") + ex.what());
        return;
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

    // Allow the context to request focus.
    connect(context, &IRCCoreContext::focusWanted, this, &MainWindow::switchToContextTab);
}

void MainWindow::handle_menuTab_triggered()
{
    auto *action = dynamic_cast<QAction *>(sender());
    if (action == nullptr) {
        qDebug() << Q_FUNC_INFO << "Sender is not a QAction";
        return;
    }

    bool ok = false;
    int i = action->data().toInt(&ok);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "Action data to int was not ok";
        return;
    }

    if (!(i >= 0 && i < ui->tabWidget->count())) {
        qDebug() << Q_FUNC_INFO << "Action data int is not a valid index";
        return;
    }

    // Switch to tab that got selected via dynamically generated menu items.
    ui->tabWidget->setCurrentIndex(i);
}

void MainWindow::handle_tabWidget_currentChanged(int index)
{
    if (index < 0) {
        qDebug() << Q_FUNC_INFO << "New tab index is invalid";
        return;
    }

    auto *logBuf = dynamic_cast<LogBuffer *>(ui->tabWidget->widget(index));
    if (logBuf == nullptr) {
        qDebug() << Q_FUNC_INFO << "New tab is not a LogBuffer";
        return;
    }

    // Clear tab color when switching to a tab.
    logBuf->setActivity(LogBuffer::Activity::None);
}

void MainWindow::handle_logBuffer_activityChanged()
{
    auto *logBuf = dynamic_cast<LogBuffer *>(sender());
    if (logBuf == nullptr) {
        qDebug() << Q_FUNC_INFO << "Sender is not a LogBuffer";
        return;
    }

    int iTab = ui->tabWidget->indexOf(logBuf);
    if (iTab < 0) {
        qDebug() << Q_FUNC_INFO << "Sender LogBuffer is not a tab";
        return;
    }

    QColor color;
    color.setNamedColor("brown");
    switch (logBuf->activity()) {
    case LogBuffer::Activity::None:
        color.setNamedColor("black");
        break;
    case LogBuffer::Activity::General:
        color.setNamedColor("green");
        break;
    case LogBuffer::Activity::Highlight:
        color.setNamedColor("red");
        break;
    }

    // Keep color off for the active tab, or else we'd have to switch
    // back and forth all the time to clear the color.
    if (iTab == ui->tabWidget->currentIndex())
        color.setNamedColor("black");

    // Have colored tabs.
    ui->tabWidget->tabBar()->setTabTextColor(iTab, color);
}

void MainWindow::on_actionLocalOnlineHelp_triggered()
{
    if (_helpViewer.state() == QProcess::Running) {
        ui->statusBar->showMessage("Help viewer already running");
        return;
    }

    QString binPath = qApp->applicationDirPath();
    if (binPath.isEmpty()) {
        ui->statusBar->showMessage("Can't determine application directory");
        return;
    }

    QString helpFileBasename = "cvnirc-qt-collection.qch";
    QString helpFileName;
    QStringList locations = {
        ".",
        "../doc",
        "/usr/local/share/cvnirc-qt",
    };

    for (QString location : locations) {
        QString fileName = (QDir::isAbsolutePath(location) ? "" : binPath + "/") +
            location + "/" + helpFileBasename;
        if (QFileInfo::exists(fileName)) {
            helpFileName = fileName;
            break;
        }
    }

    if (helpFileName.isEmpty()) {
        ui->statusBar->showMessage("Help collection file not found");
        return;
    }

    ui->statusBar->showMessage("Starting help viewer...");
    _helpViewer.start("assistant", { "-collectionFile", helpFileName }, QIODevice::NotOpen);
}

void MainWindow::handle_helpViewer_errorOccurred(QProcess::ProcessError err)
{
    ui->statusBar->showMessage(QString("Help viewer: Process error ")
#ifdef CVN_HAVE_Q_ENUM
        + QMetaEnum::fromType<QProcess::ProcessError>().valueToKey(err)
#else
        + QString::number(err)
#endif
        + ": " + _helpViewer.errorString()
    );
}
