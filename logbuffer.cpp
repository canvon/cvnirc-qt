#include "logbuffer.h"
#include "ui_logbuffer.h"

#include <QDate>

LogBuffer::LogBuffer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogBuffer)
{
    ui->setupUi(this);
}

LogBuffer::~LogBuffer()
{
    delete ui;
}

void LogBuffer::appendLine(const QString &line)
{
    // Prepend timestamp and append to logbuffer.
    QDateTime ts = QDateTime::currentDateTime();
    ui->textEdit->append("[" + ts.toString() + "] " + line);
}

void LogBuffer::appendSentLine(const QString &rawLine)
{
    return appendLine("< " + rawLine);
}

void LogBuffer::appendReceivedLine(const QString &rawLine)
{
    return appendLine("> " + rawLine);
}
