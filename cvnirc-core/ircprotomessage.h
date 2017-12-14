#ifndef IRCPROTOMESSAGE_H
#define IRCPROTOMESSAGE_H

#include "cvnirc-core_global.h"

#include <memory>
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

    static MessageOrigin fromPrefix(const QString &prefix, Type onNull = Type::SeePrefix);
};

class MessageArg;
class CVNIRCCORESHARED_EXPORT MessageArgTypeBase
{
    QString _name;
public:
    typedef std::shared_ptr<MessageArg>  messageArgUnsafe_ptr;
    typedef messageArgUnsafe_ptr (fromTokensUnsafe_fun)(TokensReader *reader);
protected:
    MessageArgTypeBase(const QString &name);

public:
    virtual ~MessageArgTypeBase();

    const QString &name() const;
    virtual std::function<fromTokensUnsafe_fun> fromTokensUnsafe_call() const = 0;
};

template <class A = MessageArg>
class CVNIRCCORESHARED_EXPORT MessageArgType : public MessageArgTypeBase
{
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
std::shared_ptr<ConstMessageArgType<T>> make_const(const QString &name,
    std::shared_ptr<T> typeToWrap, std::shared_ptr<const typename T::messageArg_type> constArg)
{
    return std::make_shared<ConstMessageArgType<T>>(name, typeToWrap, constArg);
}

template <class T = MessageArgType<>, typename... Aargs>
std::shared_ptr<ConstMessageArgType<T>> make_const_fwd(const QString &name,
    std::shared_ptr<T> typeToWrap, Aargs... args)
{
    return make_const(name, typeToWrap, std::make_shared<const typename T::messageArg_type>(std::forward<Aargs>(args)...));
}

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

template <class T = MessageArgType<>>
std::shared_ptr<OptionalMessageArgType<T>> make_optional(const QString &name, std::shared_ptr<T> typeToWrap)
{
    return std::make_shared<OptionalMessageArgType<T>>(name, typeToWrap);
}

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

template <class T = MessageArgType<>>
std::shared_ptr<CommaListMessageArgType<T>> make_commalist(const QString &name, std::shared_ptr<T> elementType)
{
    return std::make_shared<CommaListMessageArgType<T>>(name, elementType);
}

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

class CVNIRCCORESHARED_EXPORT UnrecognizedMessageArg : public MessageArg
{
public:
    QByteArray token;

    UnrecognizedMessageArg(const QByteArray &token);

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

class CVNIRCCORESHARED_EXPORT ChatterDataMessageArg : public MessageArg
{
public:
    QString chatterData;

    ChatterDataMessageArg(const QString &chatterData);

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

class CVNIRCCORESHARED_EXPORT MessageArgTypesHolder
{
public:
    std::shared_ptr<MessageArgType<CommandNameMessageArg>>
        commandNameType;
    std::shared_ptr<MessageArgType<NumericCommandNameMessageArg>>
        numericCommandNameType;
    std::shared_ptr<MessageArgType<UnrecognizedMessageArg>>
        unrecognizedType;
    std::shared_ptr<MessageArgType<ListMessageArg<UnrecognizedMessageArg>>>
        unrecognizedArgListType;
    std::shared_ptr<MessageArgType<SourceMessageArg>>
        sourceType;
    std::shared_ptr<MessageArgType<TargetMessageArg>>
        targetType;
    std::shared_ptr<CommaListMessageArgType<MessageArgType<TargetMessageArg>>>
        targetListType;
    std::shared_ptr<MessageArgType<ChannelTargetMessageArg>>
        channelType;
    std::shared_ptr<CommaListMessageArgType<MessageArgType<ChannelTargetMessageArg>>>
        channelListType;
    std::shared_ptr<MessageArgType<KeyMessageArg>>
        keyType;
    std::shared_ptr<CommaListMessageArgType<MessageArgType<KeyMessageArg>>>
        keyListType;
    std::shared_ptr<MessageArgType<ChatterDataMessageArg>>
        chatterDataType;
};

class CVNIRCCORESHARED_EXPORT Message
{
public:
    typedef std::shared_ptr<MessageArg> msgArg_ptr;

    MessageOrigin      origin;
    QList<msgArg_ptr>  args;

    Message(const MessageOrigin &origin, const QList<msgArg_ptr> args);
    virtual ~Message();
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
    std::shared_ptr<Message> fromMessageAsTokens(const MessageAsTokens &msgTokens,
        MessageOrigin::Type origin_onNullPrefix = MessageOrigin::Type::SeePrefix) const;
};

class CVNIRCCORESHARED_EXPORT MessageTypeVocabulary
{
    QMap<QString, std::shared_ptr<MessageType>> _map;

public:
    void registerMessageType(const QString &commandName, std::shared_ptr<MessageType> msgType);
    std::shared_ptr<MessageType> messageType(const QString &commandName);
};

}  // namespace cvnirc::core::IRCProto
}  // namespace cvnirc::core
}  // namespace cvnirc

// For compatibility.
// TODO: Change all using code and remove this.
using IRCProtoMessage = cvnirc::core::IRCProto::Message;

#endif // IRCPROTOMESSAGE_H
