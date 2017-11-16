#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include <QWidget>

namespace Ui {
class LogBuffer;
}

class LogBuffer : public QWidget
{
    Q_OBJECT

public:
    explicit LogBuffer(QWidget *parent = 0);
    ~LogBuffer();

public slots:
    void appendLine(const QString &line);
    void appendSendingLine(const QString &rawLine);
    void appendReceivedLine(const QString &rawLine);

private:
    Ui::LogBuffer *ui;
};

#endif // LOGBUFFER_H
