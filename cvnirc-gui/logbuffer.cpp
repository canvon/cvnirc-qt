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
        connect(context, &IRCCoreContext::notifyUser, this, &LogBuffer::appendContextLine);
        break;
    case Type::Protocol:
        connect(context, &IRCCoreContext::sendingLine, this, &LogBuffer::appendContextSendingLine);
        connect(context, &IRCCoreContext::receivedLine, this, &LogBuffer::appendContextReceivedLine);
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
        disconnect(context, &IRCCoreContext::receivedLine, this, &LogBuffer::appendContextReceivedLine);
        disconnect(context, &IRCCoreContext::sendingLine, this, &LogBuffer::appendContextSendingLine);
        break;
    case Type::General:
        disconnect(context, &IRCCoreContext::notifyUser, this, &LogBuffer::appendContextLine);
        break;
    }
    _contexts.removeOne(context);
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

void LogBuffer::appendLine(const QString &line)
{
    return appendContextLine(line);
}

void LogBuffer::appendSendingLine(const QString &rawLine)
{
    return appendContextSendingLine(rawLine);
}

void LogBuffer::appendReceivedLine(const QString &rawLine)
{
    return appendContextReceivedLine(rawLine);
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

void LogBuffer::appendContextLine(const QString &line, IRCCoreContext *context)
{
    // Prepend timestamp and context information, and append to logbuffer.
    QDateTime ts = QDateTime::currentDateTime();
    ui->textEdit->append("[" + ts.toString() + "] " + _contextToStr(context) + line);
}

void LogBuffer::appendContextSendingLine(const QString &rawLine, IRCCoreContext *context)
{
    return appendContextLine("< " + rawLine, context);
}

void LogBuffer::appendContextReceivedLine(const QString &rawLine, IRCCoreContext *context)
{
    return appendContextLine("> " + rawLine, context);
}

void LogBuffer::handle_ircContext_connectionStateChanged(IRCCoreContext *context)
{
    if (context == nullptr)
        throw std::runtime_error("LogBuffer, handle IRCCoreContext connection state changed: Got context which is a null pointer, which is invalid here");

    auto state = context->ircProtoClient()->connectionState();
    appendContextLine(QString("Connection state changed to ") +
        QString::number((int)state)
#ifdef CVN_HAVE_Q_ENUM
        + QString(": ")
        // Translate to human-readable.
        + QMetaEnum::fromType<IRCProtoClient::ConnectionState>().valueToKey((int)state)
#endif
        , context
    );
}
