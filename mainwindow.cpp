#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectdialog.h"

#include <QDate>
#include <QtNetwork>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    socket(new QTcpSocket(this)),
    socketReadBuf(10*1024),
    socketReadBufUsed(0)
{
    ui->setupUi(this);

    // Set up signals & slots.
    connect(socket, &QIODevice::readyRead, this, &MainWindow::processIncomingData);
}

MainWindow::~MainWindow()
{
    delete ui;
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

    logbufferAppend("Connecting to " + dialog.host() + ":" + dialog.port() +
        ", as user " + dialog.user() + " and nick " + dialog.nick());
    socket->connectToHost(dialog.host(), dialog.port().toShort());
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
        logbufferAppend("Sending command: \"" + line + "\"");
        socket->write((line + "\r\n").toUtf8());
    }
}

void MainWindow::processIncomingData()
{
    if (socketReadBufUsed >= socketReadBuf.size()) {
        logbufferAppend("Socket read buffer size exceeded, aborting connection.");
        socket->abort();
        return;
    }

    qint64 ret = 0;
    while ((ret = socket->read(socketReadBuf.data(),
                               socketReadBuf.size() - socketReadBufUsed)
            ) > 0) {
        socketReadBufUsed += ret;

        while (socketReadBufUsed > 0) {
            int state = 0;
            socketReadBuf_type::const_iterator usedEnd = socketReadBuf.cbegin() + socketReadBufUsed;
            socketReadBuf_type::const_iterator iter;

            for (iter = socketReadBuf.cbegin();
                 iter < usedEnd;
                 iter++)
            {
                switch (state) {
                case 0:
                    if (*iter == '\r')
                        state++;
                    break;
                case 1:
                    if (*iter == '\n') {
                        state++;
                    }
                    else {
                        logbufferAppend("Protocol error: Server seems to have broken line-termination! Aborting connection.");
                        socket->abort();
                        return;
                    }
                    break;
                }

                if (state == 2) {
                    state = 0;
                    qint64 lineLen = iter - socketReadBuf.cbegin() - 1;
                    socketReadBuf[lineLen] = '\0';
                    QString line(socketReadBuf.data());

                    socketReadBuf_type::iterator writeIter = socketReadBuf.begin();
                    socketReadBuf_type::const_iterator readIter = iter;
                    while (usedEnd - readIter > 0)
                        *writeIter++ = *readIter++;
                    socketReadBufUsed -= lineLen + 2;

                    logbufferAppend("> " + line);

                    // TODO: Interpret messages.

                    break;
                }
            }

            if (iter >= usedEnd)
                break;
        }
    }

    if (ret < 0) {
        logbufferAppend("Error reading from network socket, aborting connection.");
        socket->abort();
        return;
    }
}
