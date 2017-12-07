#ifndef IRCPROTOMESSAGE_H
#define IRCPROTOMESSAGE_H

#include "cvnirc-core_global.h"

#include <memory>
//#include <QObject>
#include <QByteArray>
#include <QByteArrayList>
#include <QString>
#include <QStringList>
#include <QMap>

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

class CVNIRCCORESHARED_EXPORT TokensReader
{
    QByteArrayList _remainingTokens;

public:
    TokensReader(const QByteArrayList &tokens);
    TokensReader(const MessageAsTokens &msgTokens);  // (for convenience)

    const QByteArrayList &remainingTokens() const;

    bool atEnd() const;
    bool isByteAvailable() const;
    bool isTokenAvailable() const;

    char takeByte();
    QByteArray takeToken();
};


class MessageType;
class Message;
class CVNIRCCORESHARED_EXPORT Incoming
{
public:
    typedef std::shared_ptr<MessageOnNetwork>  raw_ptr;
    typedef std::shared_ptr<MessageAsTokens>   tokens_ptr;
    typedef std::shared_ptr<MessageType>       messageType_ptr;
    typedef std::shared_ptr<Message>           message_ptr;

    raw_ptr      inRaw;
    tokens_ptr   inTokens;
    messageType_ptr  inMessageType;
    message_ptr  inMessage;

    bool handled = false;

    Incoming(raw_ptr inRaw = nullptr, tokens_ptr inTokens = nullptr, messageType_ptr inMessageType = nullptr, message_ptr inMessage = nullptr);
};


class CVNIRCCORESHARED_EXPORT MessageOrigin
{
public:
    enum class Type {
        LinkServer,
        ThisClient,
        SeePrefix,
    };

    Type     type;
    QString  prefix;
};

class MessageArg;
class CVNIRCCORESHARED_EXPORT MessageArgType
{
    QString _name;
public:
    typedef std::shared_ptr<MessageArg>  messageArg_ptr;
    typedef messageArg_ptr (fromTokens_fun)(TokensReader *reader);
private:
    std::function<fromTokens_fun> _fromTokens_call;

public:
    MessageArgType(const QString &name, const std::function<fromTokens_fun> &fromTokens_call);
    virtual ~MessageArgType();

    const QString &name() const;
    const std::function<fromTokens_fun> &fromTokens_call() const;
};

class CVNIRCCORESHARED_EXPORT ConstMessageArgType : public MessageArgType
{
    std::shared_ptr<const MessageArg>  _constArg;
    std::function<fromTokens_fun>      _origFromTokens_call;

public:
    ConstMessageArgType(const QString &name, std::shared_ptr<const MessageArg> constArg, const std::function<fromTokens_fun> &fromTokens_call);
    ConstMessageArgType(const QString &name, std::shared_ptr<const MessageArg> constArg, const MessageArgType &msgArgType);

    std::shared_ptr<const MessageArg> constArg();
    const std::function<fromTokens_fun> &origFromTokens_call() const;

    messageArg_ptr fromTokens(TokensReader *reader) const;
};

class CVNIRCCORESHARED_EXPORT OptionalMessageArgType : public MessageArgType
{
    std::function<fromTokens_fun>  _origFromTokens_call;

public:
    OptionalMessageArgType(const QString &name, const std::function<fromTokens_fun> &fromTokens_call);

    const std::function<fromTokens_fun> &origFromTokens_call() const;

    messageArg_ptr fromTokens(TokensReader *reader) const;
};

class CVNIRCCORESHARED_EXPORT MessageArg
{
public:
    virtual ~MessageArg();

    virtual bool operator ==(const MessageArg &other) const = 0;
};

class CVNIRCCORESHARED_EXPORT CommandNameMessageArg : public MessageArg
{
public:
    QString commandOrig;
    QString commandUpper;

    CommandNameMessageArg(const QString &commandOrig);

    bool operator ==(const MessageArg &other) const override;
};

class CVNIRCCORESHARED_EXPORT NumericCommandNameMessageArg : public CommandNameMessageArg
{
public:
    int numeric;

    NumericCommandNameMessageArg(const QString &commandOrig);

    bool operator ==(const MessageArg &other) const override;
};

class CVNIRCCORESHARED_EXPORT SourceMessageArg : public MessageArg
{
public:
    QString source;

    SourceMessageArg(const QString &source);

    bool operator ==(const MessageArg &other) const override;
};

class CVNIRCCORESHARED_EXPORT ChannelMessageArg : public MessageArg
{
public:
    QString channel;

    ChannelMessageArg(const QString &channel);

    bool operator ==(const MessageArg &other) const override;
};

class CVNIRCCORESHARED_EXPORT ChannelListMessageArg : public MessageArg
{
public:
    QList<std::shared_ptr<ChannelMessageArg>> channelList;

    ChannelListMessageArg();
    ChannelListMessageArg(const QList<std::shared_ptr<ChannelMessageArg>> &channelList);

    bool operator ==(const MessageArg &other) const override;
};

class CVNIRCCORESHARED_EXPORT MessageArgTypesHolder
{
public:
    std::shared_ptr<MessageArgType>
        commandNameType, numericCommandNameType,
        sourceType, channelType, channelListType, keyType, keyListType,
        targetType, chatterDataType;
};

class CVNIRCCORESHARED_EXPORT Message
{
#if 0
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
#endif
public:
    typedef std::shared_ptr<MessageArg> msgArg_ptr;

    //MsgType msgType;
    MessageOrigin      origin;
    QList<msgArg_ptr>  args;

    Message(const MessageOrigin &origin, const QList<msgArg_ptr> args);
    virtual ~Message();
#if 0
protected:
    Message(MsgType msgType);
#endif
};

class CVNIRCCORESHARED_EXPORT MessageType
{
    QString _name;
public:
    typedef std::shared_ptr<MessageArgType> msgArgType_ptr;
private:
    QList<msgArgType_ptr> _argTypes;

public:
    MessageType(const QString &name, const QList<msgArgType_ptr> &argTypes);

    const QString &name() const;
    const QList<msgArgType_ptr> &argTypes() const;

    QList<Message::msgArg_ptr> argsFromMessageAsTokens(const MessageAsTokens &msgTokens) const;
    std::shared_ptr<Message> fromMessageAsTokens(const MessageAsTokens &msgTokens) const;
};

class CVNIRCCORESHARED_EXPORT MessageTypeVocabulary
{
    QMap<QString, std::shared_ptr<MessageType>> _map;

public:
    void registerMessageType(const QString &commandName, std::shared_ptr<MessageType> msgType);
    std::shared_ptr<MessageType> messageType(const QString &commandName);
};


#if 0
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
#endif

}  // namespace cvnirc::core::IRCProto
}  // namespace cvnirc::core
}  // namespace cvnirc

// For compatibility.
// TODO: Change all using code and remove this.
using IRCProtoMessage = cvnirc::core::IRCProto::Message;

#endif // IRCPROTOMESSAGE_H
