#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <vector>

namespace Ui {
class MainWindow;
}

class QTcpSocket;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void logbufferAppend(const QString &s);

private slots:
    void on_action_Quit_triggered();

    void on_action_Connect_triggered();

    void on_pushButtonUserInput_clicked();

    void processIncomingData();

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;

    typedef std::vector<char> socketReadBuf_type;
    socketReadBuf_type socketReadBuf;
    int socketReadBufUsed;
};

#endif // MAINWINDOW_H
