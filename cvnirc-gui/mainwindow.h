#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "irccore.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void updateState();
    QWidget *findTabWidgetForContext(IRCCoreContext *context);
    QWidget *openTabForContext(IRCCoreContext *context);
    IRCCoreContext *contextFromUI();

    IRCCore irc;

private slots:
    void on_action_Quit_triggered();
    void on_action_Connect_triggered();
    void on_action_Reconnect_triggered();
    void on_action_Disconnect_triggered();

    void on_pushButtonUserInput_clicked();

    void handle_irc_createdContext(IRCCoreContext *context);
    //void handle_irc_receivedMessage(IRCProtoMessage &msg);
    void handle_ircContext_connectionStateChanged(IRCCoreContext *context);

private:
    Ui::MainWindow *ui;
    QString baseWindowTitle;
};

#endif // MAINWINDOW_H
