#ifndef COMMAND_H
#define COMMAND_H

#include "cvnirc-core_global.h"

#include <QStringList>

class CVNIRCCORESHARED_EXPORT Command
{
    QStringList _tokens;

public:
    Command(const QStringList &tokens);
    Command(const QString &line);

    const QStringList &tokens() const;

    static QStringList splitLine(const QString &line);
};

#endif // COMMAND_H
