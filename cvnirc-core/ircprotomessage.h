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
class CVNIRCCORESHARED_EXPORT MessageArgTypeBase
{
    QString _name;
public:
    typedef std::shared_ptr<MessageArg>  messageArgUnsafe_ptr;
    typedef messageArgUnsafe_ptr (fromTokensUnsafe_fun)(TokensReader *reader);
private:
    //std::function<fromTokensUnsafe_fun> _fromTokensUnsafe_call;
protected:
    MessageArgTypeBase(const QString &name);

public:
    //MessageArgTypeBase(const QString &name, const std::function<fromTokensUnsafe_fun> &fromTokensUnsafe_call);
    virtual ~MessageArgTypeBase();

    const QString &name() const;
    virtual std::function<fromTokensUnsafe_fun> fromTokensUnsafe_call() const = 0;
};

template <class A = MessageArg>
class CVNIRCCORESHARED_EXPORT MessageArgType : public MessageArgTypeBase
{
    //QString _name;
public:
    typedef A  messageArg_type;
    typedef std::shared_ptr<A>  messageArg_ptr;
    typedef messageArg_ptr (fromTokens_fun)(TokensReader *reader);
private:
    std::function<fromTokens_fun> _fromTokens_call;

public:
    MessageArgType(const QString &name, const std::function<fromTokens_fun> &fromTokens_call) :
        MessageArgTypeBase(name), _fromTokens_call(fromTokens_call)
    {

    }

    //virtual ~MessageArgType();

    //const QString &name() const;
    std::function<fromTokensUnsafe_fun> fromTokensUnsafe_call() const override
    {
        return _fromTokens_call;
    }

    std::function<fromTokens_fun> fromTokens_call() const
    {
        return _fromTokens_call;
    }
};

template <class T = MessageArgType<>>
class CVNIRCCORESHARED_EXPORT ConstMessageArgType : public MessageArgType<typename T::messageArg_type>
{
public:
    typedef MessageArgType<typename T::messageArg_type>  base_type;
    using typename base_type::messageArg_type;
    using typename base_type::messageArg_ptr;
    using typename base_type::fromTokens_fun;

    typedef std::shared_ptr<T>                      wrappedType_ptr;
    typedef std::shared_ptr<const messageArg_type>  constMessageArg_ptr;

private:
    wrappedType_ptr                _wrappedType;
    constMessageArg_ptr            _constArg;
    //std::function<fromTokens_fun>  _origFromTokens_call;

public:
    ConstMessageArgType(const QString &name, wrappedType_ptr typeToWrap, constMessageArg_ptr constArg) :
        MessageArgType<messageArg_type>(name, [this](TokensReader *reader) { return _fromTokens(reader); }),
        _wrappedType(typeToWrap), _constArg(constArg)
    {

    }

    wrappedType_ptr wrappedType() const
    {
        return _wrappedType;
    }

    constMessageArg_ptr constArg()
    {
        return _constArg;
    }

private:
    messageArg_ptr _fromTokens(TokensReader *reader) const
    {
        messageArg_ptr arg = _wrappedType->fromTokens_call()(reader);
        if (!(*arg == *_constArg))
            throw std::runtime_error("Const message arg type: The retrieved message arg isn't equal to the const/reference arg");

        return arg;
    }
};

template <class T = MessageArgType<>>
class CVNIRCCORESHARED_EXPORT OptionalMessageArgType : public MessageArgType<typename T::messageArg_type>
{
public:
    typedef MessageArgType<typename T::messageArg_type>  base_type;
    using typename base_type::messageArg_type;
    using typename base_type::messageArg_ptr;
    using typename base_type::fromTokens_fun;

    typedef std::shared_ptr<T>  wrappedType_ptr;

private:
    wrappedType_ptr                _wrappedType;
    //std::function<fromTokens_fun>  _origFromTokens_call;

public:
    OptionalMessageArgType(const QString &name, wrappedType_ptr typeToWrap) :
        MessageArgType<messageArg_type>(name, [this](TokensReader *reader) { return _fromTokens(reader); }),
        _wrappedType(typeToWrap)
    {

    }

    wrappedType_ptr wrappedType() const
    {
        return _wrappedType;
    }

private:
    messageArg_ptr _fromTokens(TokensReader *reader) const
    {
        if (reader->atEnd())
            return nullptr;

        return _wrappedType->fromTokens_call()(reader);
    }
};

template <class A> class ListMessageArg;

template <class T = MessageArgType<>>
class CVNIRCCORESHARED_EXPORT CommaListMessageArgType : public MessageArgType<ListMessageArg<typename T::messageArg_type>>
{
public:
    typedef MessageArgType<ListMessageArg<typename T::messageArg_type>>  base_type;
    typedef typename base_type::messageArg_type  listMsgArg_type;
    typedef typename base_type::messageArg_ptr   listMsgArg_ptr;
    typedef typename base_type::fromTokens_fun   listFromTokens_fun;
    typedef typename T::messageArg_ptr           elementMsgArg_ptr;
    typedef typename T::fromTokens_fun           elementFromTokens_fun;

    typedef std::shared_ptr<T>  elementType_ptr;

private:
    elementType_ptr                       _elementType;
    //std::function<elementFromTokens_fun>  _elementFromTokens_call;

public:
    CommaListMessageArgType(const QString &name, elementType_ptr elementType) :
        MessageArgType<listMsgArg_type>(name, [this](TokensReader *reader) { return listFromTokens(reader); }),
        _elementType(elementType)
    {

    }

    elementType_ptr elementType() const
    {
        return _elementType;
    }

    listMsgArg_ptr listFromTokens(TokensReader *reader) const
    {
        auto ret = std::make_shared<listMsgArg_type>();
        QByteArrayList elementsBytes = reader->takeToken().split(',');
        TokensReader innerReader(elementsBytes);
        while (!innerReader.atEnd()) {
            ret->list.append(
                _elementType->fromTokens_call()(&innerReader)
            );
        }
        return ret;
    }
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

class CVNIRCCORESHARED_EXPORT TargetMessageArg : public MessageArg
{
public:
    virtual QString targetToString() const = 0;
};

class CVNIRCCORESHARED_EXPORT ChannelTargetMessageArg : public TargetMessageArg
{
public:
    QString channel;

    ChannelTargetMessageArg(const QString &channel);

    QString targetToString() const override;
    bool operator ==(const MessageArg &other) const override;
};

class CVNIRCCORESHARED_EXPORT NickTargetMessageArg : public TargetMessageArg
{
public:
    QString nick;

    NickTargetMessageArg(const QString &nick);

    QString targetToString() const override;
    bool operator ==(const MessageArg &other) const override;
};

class CVNIRCCORESHARED_EXPORT KeyMessageArg : public MessageArg
{
public:
    QString key;

    KeyMessageArg(const QString &key);

    bool operator ==(const MessageArg &other) const override;
};

template <class A>
class CVNIRCCORESHARED_EXPORT ListMessageArg : public MessageArg
{
public:
    typedef A                   elementMsgArg_type;
    typedef std::shared_ptr<A>  elementMsgArg_ptr;

    QList<elementMsgArg_ptr>  list;

    ListMessageArg()
    {

    }

    ListMessageArg(const QList<elementMsgArg_ptr> &list) :
        list(list)
    {

    }

    bool operator ==(const MessageArg &other) const override
    {
        const auto *myTypeOther = dynamic_cast<const ListMessageArg*>(&other);
        if (myTypeOther == nullptr)
            return false;

        int len = list.length();
        int otherLen = myTypeOther->list.length();
        if (len != otherLen)
            return false;

        for (int i = 0; i < len; i++) {
            if (!(*list[i] == *myTypeOther->list[i]))
                return false;
        }
        return true;
    }
};

class CVNIRCCORESHARED_EXPORT ChannelListMessageArg : public MessageArg
{
public:
    QList<std::shared_ptr<ChannelTargetMessageArg>> channelList;

    ChannelListMessageArg();
    ChannelListMessageArg(const QList<std::shared_ptr<ChannelTargetMessageArg>> &channelList);

    bool operator ==(const MessageArg &other) const override;
};

class CVNIRCCORESHARED_EXPORT MessageArgTypesHolder
{
public:
    std::shared_ptr<MessageArgType<CommandNameMessageArg>>
        commandNameType;
    std::shared_ptr<MessageArgType<NumericCommandNameMessageArg>>
        numericCommandNameType;
    std::shared_ptr<MessageArgType<SourceMessageArg>>
        sourceType;
    std::shared_ptr<MessageArgType<ChannelTargetMessageArg>>
        channelType;
    std::shared_ptr<CommaListMessageArgType<MessageArgType<ChannelTargetMessageArg>>>
        channelListType;
    std::shared_ptr<MessageArgType<KeyMessageArg>>
        keyType;
    std::shared_ptr<CommaListMessageArgType<MessageArgType<KeyMessageArg>>>
        keyListType;
    std::shared_ptr<MessageArgType<TargetMessageArg>>
        targetType;
    std::shared_ptr<MessageArgType<MessageArg>>  // FIXME
        chatterDataType;
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
    typedef std::shared_ptr<MessageArgTypeBase> msgArgType_ptr;
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
