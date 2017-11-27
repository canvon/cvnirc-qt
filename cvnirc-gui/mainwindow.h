#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QProcess>

#include "irccore.h"
#include "commandlayer.h"

namespace Ui {
class MainWindow;
}

class LogBuffer;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QProcess _helpViewer;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QWidget *findTabWidgetForContext(IRCCoreContext *context);
    QWidget *openTabForContext(IRCCoreContext *context);
    IRCCoreContext *contextFromUI();

    QStringList tabNameComponents(const LogBuffer &logBuf);
    void applyTabNameComponents(LogBuffer *logBuf, const QStringList &components);

    IRCCore irc;
    CommandLayer cmdLayer;

public slots:
    void updateState();
    void updateSwitchToTabMenu();
    void switchToContextTab(IRCCoreContext *context);

private slots:
    void on_action_Quit_triggered();
    void on_action_Connect_triggered();
    void on_action_Reconnect_triggered();
    void on_action_Disconnect_triggered();
    void on_actionFocusUserInput_triggered();
    void on_actionLocalOnlineHelp_triggered();

    void on_pushButtonUserInput_clicked();

    void handle_irc_createdContext(IRCCoreContext *context);
    //void handle_irc_receivedMessage(IRCProtoMessage &msg);
    void handle_menuTab_triggered();
    void handle_tabWidget_currentChanged(int index);
    void handle_logBuffer_activityChanged();
    void handle_helpViewer_errorOccurred(QProcess::ProcessError err);

private:
    Ui::MainWindow *ui;
    QString baseWindowTitle;
};

#endif // MAINWINDOW_H
