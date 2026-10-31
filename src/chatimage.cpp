#include "chatimage.h"
#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QPainter>
#include <QUrl>

namespace {
const QString Prefix = QStringLiteral("data:image/jpeg;base64,");
const int MaxEncodedBytes = 1400000;
}

QString ChatImage::prepare(const QString &fileUrl)
{
    const QUrl url = fileUrl.startsWith('/') ? QUrl::fromLocalFile(fileUrl) : QUrl(fileUrl);
    if (!url.isLocalFile() || !url.host().isEmpty())
        return QString();
    QFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly) || file.size() > 10 * 1024 * 1024)
        return QString();
    const QByteArray input = file.read(10 * 1024 * 1024 + 1);
    if (input.size() > 10 * 1024 * 1024)
        return QString();
    QBuffer source;
    source.setData(input);
    source.open(QIODevice::ReadOnly);
    QImageReader reader(&source);
    const QByteArray format = reader.format().toLower();
    const QSize size = reader.size();
    if ((format != "jpeg" && format != "jpg" && format != "png")
            || !size.isValid() || qint64(size.width()) * size.height() > 20000000)
        return QString();
    reader.setAutoTransform(true);
    if (size.width() > 1600 || size.height() > 1600)
        reader.setScaledSize(size.scaled(QSize(1600, 1600), Qt::KeepAspectRatio));
    QImage image = reader.read();
    if (image.isNull())
        return QString();
    if (image.width() > 1600 || image.height() > 1600)
        image = image.scaled(1600, 1600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QImage clean(image.size(), QImage::Format_RGB32);
    clean.fill(Qt::white);
    QPainter painter(&clean);
    painter.drawImage(0, 0, image);
    painter.end();
    QByteArray output;
    QBuffer destination(&output);
    destination.open(QIODevice::WriteOnly);
    if (!clean.save(&destination, "JPEG", 80))
        return QString();
    const QString result = Prefix + QString::fromLatin1(output.toBase64());
    return valid(result) ? result : QString();
}

bool ChatImage::valid(const QString &dataUrl)
{
    if (!dataUrl.startsWith(Prefix) || dataUrl.size() > MaxEncodedBytes)
        return false;
    const QByteArray encoded = dataUrl.mid(Prefix.size()).toLatin1();
    const QByteArray decoded = QByteArray::fromBase64(encoded);
    if (!decoded.startsWith(QByteArray::fromHex("ffd8"))
            || !decoded.endsWith(QByteArray::fromHex("ffd9")) || decoded.toBase64() != encoded)
        return false;
    QBuffer buffer;
    buffer.setData(decoded);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer, "JPEG");
    const QSize size = reader.size();
    return size.isValid() && size.width() <= 1600 && size.height() <= 1600 && !reader.read().isNull();
}

QJsonObject ChatImage::requestMessage(const QVariantMap &message, bool anthropic)
{
    QJsonObject result;
    result.insert(QStringLiteral("role"), message.value(QStringLiteral("role")).toString());
    const QString text = message.value(QStringLiteral("content")).toString();
    const QString image = message.value(QStringLiteral("image")).toString();
    if (image.isEmpty()) {
        result.insert(QStringLiteral("content"), text);
        return result;
    }
    QJsonArray content;
    QJsonObject textBlock;
    textBlock.insert(QStringLiteral("type"), QStringLiteral("text"));
    textBlock.insert(QStringLiteral("text"), text);
    content.append(textBlock);
    QJsonObject imageBlock;
    if (anthropic) {
        QJsonObject source;
        source.insert(QStringLiteral("type"), QStringLiteral("base64"));
        source.insert(QStringLiteral("media_type"), QStringLiteral("image/jpeg"));
        source.insert(QStringLiteral("data"), image.mid(Prefix.size()));
        imageBlock.insert(QStringLiteral("type"), QStringLiteral("image"));
        imageBlock.insert(QStringLiteral("source"), source);
    } else {
        QJsonObject source;
        source.insert(QStringLiteral("url"), image);
        imageBlock.insert(QStringLiteral("type"), QStringLiteral("image_url"));
        imageBlock.insert(QStringLiteral("image_url"), source);
    }
    content.append(imageBlock);
    result.insert(QStringLiteral("content"), content);
    return result;
}