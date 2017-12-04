#ifndef IRCPROTOMESSAGE_H
#define IRCPROTOMESSAGE_H

#include "cvnirc-core_global.h"

#include <memory>
#include <QObject>
#include <QByteArray>
#include <QByteArrayList>
#include <QString>
#include <QStringList>

namespace cvnirc   {
namespace core     {  // cvnirc::core
namespace IRCProto {  // cvnirc::core::IRCProto

class CVNIRCCORESHARED_EXPORT MessageOnNetwork
{
public:
    QByteArray bytes;

    class MessageAsTokens parse() const;
};

class CVNIRCCORESHARED_EXPORT MessageAsTokens
{
public:
    QByteArray prefix;
    QByteArrayList mainTokens;

    MessageOnNetwork pack() const;
};

class CVNIRCCORESHARED_EXPORT Incoming
{
public:
    typedef std::shared_ptr<MessageOnNetwork>  raw_ptr;
    typedef std::shared_ptr<MessageAsTokens>   tokens_ptr;
    typedef std::shared_ptr<class Message>     message_ptr;

    raw_ptr      inRaw;
    tokens_ptr   inTokens;
    message_ptr  inMessage;

    bool handled = false;

    Incoming(raw_ptr inRaw = nullptr, tokens_ptr inTokens = nullptr, message_ptr inMessage = nullptr);
};

class CVNIRCCORESHARED_EXPORT Message
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
#ifdef CVN_HAVE_Q_ENUM
    Q_ENUM(MsgType)
#endif

    MsgType msgType;

    Message();
    virtual ~Message();
protected:
    Message(MsgType msgType);
};

class CVNIRCCORESHARED_EXPORT PingPongMessage : public Message
{
public:
    PingPongMessage(MsgType msgType, const QString &target);

    QString target;
};

class CVNIRCCORESHARED_EXPORT NumericMessage : public Message
{
public:
    NumericMessage(MsgType msgType, int numeric);

    int numeric;
};

class CVNIRCCORESHARED_EXPORT JoinMessage : public Message
{
public:
    JoinMessage(MsgType msgType, const QStringList &channels, const QStringList &keys);

    QStringList channels;
    QStringList keys;
};

class CVNIRCCORESHARED_EXPORT ChatterMessage : public Message
{
public:
    ChatterMessage(MsgType msgType, const QString &target, const QString &chatterData);

    // FIXME: PRIVMSGs can have multiple targets, too!
    QString target;
    QString chatterData;
};

}  // namespace cvnirc::core::IRCProto
}  // namespace cvnirc::core
}  // namespace cvnirc

// For compatibility.
// TODO: Change all using code and remove this.
using IRCProtoMessage = cvnirc::core::IRCProto::Message;

#endif // IRCPROTOMESSAGE_H
