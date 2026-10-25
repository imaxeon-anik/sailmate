#ifndef PROVIDERPROFILE_H
#define PROVIDERPROFILE_H

#include <QRegularExpression>
#include <QString>

inline bool isCustomProviderId(const QString &provider)
{
    static const QRegularExpression pattern(QStringLiteral("^custom-[a-f0-9]{32}$"));
    return pattern.match(provider).hasMatch();
}

inline bool isProviderId(const QString &provider)
{
    return provider == QLatin1String("openai") || provider == QLatin1String("anthropic")
            || isCustomProviderId(provider);
}

#endif
