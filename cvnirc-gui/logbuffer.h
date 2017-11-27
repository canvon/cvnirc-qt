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
#ifdef CVN_HAVE_Q_ENUM
    Q_ENUM(Type)
#endif

    enum class Activity {
        None,
        General,
        Highlight,
    };
#ifdef CVN_HAVE_Q_ENUM
    Q_ENUM(Activity)
#endif

private:
    Type _type = Type::General;
    Activity _activity = Activity::None;

public:
    explicit LogBuffer(QWidget *parent = 0);
    ~LogBuffer();

    const QList<IRCCoreContext *> &contexts() const;
    void addContext(IRCCoreContext *context);
    void removeContext(IRCCoreContext *context);

    Type type() const;
    void setType(Type newType);

    Activity activity() const;
    void setActivity(Activity newActivity);

signals:
    void activityChanged();

public slots:
    void appendLine(const QString &line, IRCCoreContext *context = nullptr);
    void appendSendingLine(const QString &rawLine, IRCCoreContext *context = nullptr);
    void appendReceivedLine(const QString &rawLine, IRCCoreContext *context = nullptr);

private slots:
    void handle_ircContext_connectionStateChanged(IRCCoreContext *context = nullptr);

private:
    Ui::LogBuffer *ui;

    QString _contextToStr(const IRCCoreContext *context);
};

#endif // LOGBUFFER_H
