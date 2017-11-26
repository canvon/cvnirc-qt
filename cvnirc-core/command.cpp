#include "command.h"

#include <QChar>
#include <stdexcept>

Command::Command(const QStringList &tokens) :
    _tokens(tokens)
{
    if (_tokens.length() < 1)
        throw std::invalid_argument("Command ctor: A command need to have at least one token, the command name");
}

Command::Command(const QString &line) :
    Command(splitLine(line))
{

}

const QStringList &Command::tokens() const
{
    return _tokens;
}

QStringList Command::splitLine(const QString &line)
{
    bool isOnSpace = true, isQuote = false;
    bool tokenStarted = false;
    QString partialToken;
    QStringList ret;

    for (const QChar c : line) {
        if (isQuote) {
            if (c == '"') {
                isQuote = false;
                continue;
            }

            partialToken.append(c);
        }
        else if (c.isSpace()) {
            if (isOnSpace)
                continue;
            isOnSpace = true;

            ret.append(partialToken);
            partialToken.clear();
            tokenStarted = false;
        }
        else {
            isOnSpace = false;

            if (c == '"') {
                isQuote = true;
                tokenStarted = true;
                continue;
            }

            partialToken.append(c);
        }
    }

    if (isQuote)
        throw std::runtime_error("Command::splitLine(): Unbalanced quote");

    if (tokenStarted || !partialToken.isNull())
        ret.append(partialToken);

    return ret;
}
