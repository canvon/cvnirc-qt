#ifndef IRCPROTOCLIENT_H
#define IRCPROTOCLIENT_H

#include <QObject>

class IRCProtoClient : public QObject
{
    Q_OBJECT
public:
    explicit IRCProtoClient(QObject *parent = 0);

signals:

public slots:
};

#endif // IRCPROTOCLIENT_H