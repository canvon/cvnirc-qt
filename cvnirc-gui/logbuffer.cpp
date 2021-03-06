#include "logbuffer.h"
#include "ui_logbuffer.h"

#include <QDate>
#include <QMetaEnum>
#include <stdexcept>

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

const QList<IRCCoreContext *> &LogBuffer::contexts() const
{
    return _contexts;
}

void LogBuffer::addContext(IRCCoreContext *context)
{
    if (context == nullptr)
        throw std::runtime_error("LogBuffer::addContext(): Got null pointer, which is invalid here");

    if (_contexts.contains(context))
        return;

    _contexts.append(context);
    switch (_type) {
    case Type::General:
        connect(context, &IRCCoreContext::notifyUser, this, &LogBuffer::appendLine);
        break;
    case Type::Protocol:
        connect(context, &IRCCoreContext::sendingLine, this, &LogBuffer::appendSendingLine);
        connect(context, &IRCCoreContext::receivedLine, this, &LogBuffer::appendReceivedLine);
        connect(context, &IRCCoreContext::connectionStateChanged, this, &LogBuffer::handle_ircContext_connectionStateChanged);
        break;
    }
}

void LogBuffer::removeContext(IRCCoreContext *context)
{
    if (context == nullptr)
        throw std::runtime_error("LogBuffer::removeContext(): Got null pointer, which is invalid here");

    if (!_contexts.contains(context))
        throw std::runtime_error("LogBuffer::removeContext(): No such context");

    switch (_type) {
    case Type::Protocol:
        disconnect(context, &IRCCoreContext::connectionStateChanged, this, &LogBuffer::handle_ircContext_connectionStateChanged);
        disconnect(context, &IRCCoreContext::receivedLine, this, &LogBuffer::appendReceivedLine);
        disconnect(context, &IRCCoreContext::sendingLine, this, &LogBuffer::appendSendingLine);
        break;
    case Type::General:
        disconnect(context, &IRCCoreContext::notifyUser, this, &LogBuffer::appendLine);
        break;
    }
    _contexts.removeOne(context);
}

LogBuffer::Type LogBuffer::type() const
{
    return _type;
}

void LogBuffer::setType(LogBuffer::Type newType)
{
    QList<IRCCoreContext *> list(_contexts);
    for (IRCCoreContext *context : list)
        removeContext(context);

    _type = newType;
    for (IRCCoreContext *context : list)
        addContext(context);
}

LogBuffer::Activity LogBuffer::activity() const
{
    return _activity;
}

void LogBuffer::setActivity(LogBuffer::Activity newActivity)
{
    if (_activity == newActivity)
        return;

    _activity = newActivity;
    activityChanged();
}

QString LogBuffer::_contextToStr(const IRCCoreContext *context)
{
    if (context == nullptr)
        // Missing.
        return "[No context] ";

    // Is that our single context?
    if (_contexts.length() == 1 && *_contexts.front() == *context)
        // Not missing but empty.
        return "";

    // Describe context.
    return context->disambiguator() + " ";
}

void LogBuffer::appendLine(const QString &line, IRCCoreContext *context)
{
    // Prepend timestamp and context information, and append to logbuffer.
    QDateTime ts = QDateTime::currentDateTime();
    ui->textEdit->append("[" + ts.toString() + "] " + _contextToStr(context) + line);

    // Support colored tabs.
    if (_activity < Activity::General)
        setActivity(Activity::General);
}

void LogBuffer::appendSendingLine(const QString &rawLine, IRCCoreContext *context)
{
    return appendLine("< " + rawLine, context);
}

void LogBuffer::appendReceivedLine(const QString &rawLine, IRCCoreContext *context)
{
    return appendLine("> " + rawLine, context);
}

void LogBuffer::handle_ircContext_connectionStateChanged(IRCCoreContext *context)
{
    if (context == nullptr)
        throw std::runtime_error("LogBuffer, handle IRCCoreContext connection state changed: Got context which is a null pointer, which is invalid here");

    auto state = context->ircProtoClient()->connectionState();
    appendLine(QString("Connection state changed to ") +
        QString::number((int)state)
#ifdef CVN_HAVE_Q_ENUM
        + QString(": ")
        // Translate to human-readable.
        + QMetaEnum::fromType<IRCProtoClient::ConnectionState>().valueToKey((int)state)
#endif
        , context
    );
}
