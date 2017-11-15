#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "ircprotoclient.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void logbufferAppend(const QString &s);

    IRCProtoClient irc;

private slots:
    void on_action_Quit_triggered();

    void on_action_Connect_triggered();

    void on_pushButtonUserInput_clicked();

    void on_irc_receivedMessage(const IRCProtoMessage &msg);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
