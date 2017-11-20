#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include <QWidget>
#include <QList>

#include "irccore.h"

namespace Ui {
class LogBuffer;
}

class LogBuffer : public QWidget
{
    Q_OBJECT
    QList<IRCCoreContext *> _contexts;

public:
    enum class Type {
        General,
        Protocol,
    };
    Q_ENUM(Type)

private:
    Type _type = Type::General;

public:
    explicit LogBuffer(QWidget *parent = 0);
    ~LogBuffer();

    const QList<IRCCoreContext *> &contexts();
    void addContext(IRCCoreContext *context);
    void removeContext(IRCCoreContext *context);
    void setType(Type newType);

public slots:
    void appendLine(const QString &line);
    void appendSendingLine(const QString &rawLine);
    void appendReceivedLine(const QString &rawLine);

    void appendContextLine(IRCCoreContext *context, const QString &line);
    void appendContextSendingLine(IRCCoreContext *context, const QString &rawLine);
    void appendContextReceivedLine(IRCCoreContext *context, const QString &rawLine);

private slots:
    void handle_ircContext_connectionStateChanged(IRCCoreContext *context);

private:
    Ui::LogBuffer *ui;

    QString _contextToStr(const IRCCoreContext *context);
};

#endif // LOGBUFFER_H
