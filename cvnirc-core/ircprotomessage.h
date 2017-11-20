#ifndef IRCPROTOMESSAGE_H
#define IRCPROTOMESSAGE_H

#include "cvnirc-core_global.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <vector>

class CVNIRCCORESHARED_EXPORT IRCProtoMessage
{
    Q_GADGET
public:
    enum class MsgType {
        Unknown,
        Ping, Pong,
        Welcome,
        Join,
        PrivMsg, Notice,
    };
    Q_ENUM(MsgType)

    typedef std::vector<QString> tokens_type;

    IRCProtoMessage(const QString &rawLine);
    IRCProtoMessage(const QString &rawLine, const QString &prefix, const tokens_type &mainTokens);

    bool handled = false;
    MsgType msgType;
    QString rawLine;
    QString prefix;
    tokens_type mainTokens;

    static tokens_type splitRawLine(const QString &rawLine);
};

class CVNIRCCORESHARED_EXPORT PingPongIRCProtoMessage : public IRCProtoMessage
{
public:
    PingPongIRCProtoMessage(const QString &rawLine, const QString &prefix, const tokens_type &mainTokens,
                            MsgType msgType, const QString &target);

    QString target;
};

class CVNIRCCORESHARED_EXPORT NumericIRCProtoMessage : public IRCProtoMessage
{
public:
    NumericIRCProtoMessage(const QString &rawLine, const QString &prefix, const tokens_type &mainTokens,
                           MsgType msgType, int numeric);

    int numeric;
};

class CVNIRCCORESHARED_EXPORT JoinIRCProtoMessage : public IRCProtoMessage
{
public:
    typedef QStringList channels_type, keys_type;

    JoinIRCProtoMessage(const QString &rawLine, const QString &prefix, const tokens_type &mainTokens,
                        MsgType msgType, channels_type channels, keys_type keys);

    channels_type channels;
    keys_type keys;
};

class CVNIRCCORESHARED_EXPORT ChatterIRCProtoMessage : public IRCProtoMessage
{
public:
    ChatterIRCProtoMessage(const QString &rawLine, const QString &prefix, const tokens_type &mainTokens,
                           MsgType msgType, QString target, QString chatterData);

    // FIXME: PRIVMSGs can have multiple targets, too!
    QString target;
    QString chatterData;
};

#endif // IRCPROTOMESSAGE_H
