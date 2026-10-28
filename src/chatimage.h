#ifndef CHATIMAGE_H
#define CHATIMAGE_H

#include <QString>
#include <QJsonObject>
#include <QVariantMap>

namespace ChatImage {
QString prepare(const QString &fileUrl);
bool valid(const QString &dataUrl);
QJsonObject requestMessage(const QVariantMap &message, bool anthropic);
}

#endif