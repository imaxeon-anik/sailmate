#include "chatcontroller.h"
#include "chatimage.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QSqlRecord>
#include "appsettings.h"
#include "providerprofile.h"
#include <QSettings>

#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSqlQuery>
#include <QSslError>
#include <QStandardPaths>
#include <QTextCodec>
#include <QUuid>
#include <QUrl>
#include <algorithm>

namespace {
const int MaxInputBytes = 16384;
const int MaxHistoryBytes = 98304;
const int MaxMessages = 80;
const int MaxAssistantBytes = 65536;
const int MaxEventBytes = 262144;
const int MaxResponseBytes = 4 * 1024 * 1024;
const int RequestTimeoutMs = 120000;

QVariantMap message(const QString &role, const QString &content, const QString &status)
{
    QVariantMap result;
    result.insert(QStringLiteral("role"), role);
    result.insert(QStringLiteral("content"), content);
    result.insert(QStringLiteral("status"), status);
    return result;
}

QString normalizedBaseUrl(const QString &value)
{
    const QUrl url(value.trimmed(), QUrl::StrictMode);
    if (!url.isValid() || url.scheme() != QLatin1String("https") || url.host().isEmpty()
            || !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment()
            || value.size() > 2048)
        return QString();
    QString result = url.toString(QUrl::FullyEncoded);
    while (result.endsWith(QLatin1Char('/')))
        result.chop(1);
    return result;
}

bool chatModel(const QString &id)
{
    if (id.isEmpty() || id.size() > 200)
        return false;
    for (int i = 0; i < id.size(); ++i) {
        const ushort c = id.at(i).unicode();
        if (c < 33 || c > 126)
            return false;
    }
    return true;
}

bool openAIChatModel(const QString &id)
{
    if (!chatModel(id))
        return false;
    const bool family = id.startsWith(QStringLiteral("gpt-"))
            || id.startsWith(QStringLiteral("chatgpt-"))
            || (id.size() > 1 && id.at(0) == QLatin1Char('o') && id.at(1).isDigit());
    const QStringList excluded = QStringList() << QStringLiteral("instruct")
            << QStringLiteral("audio") << QStringLiteral("realtime")
            << QStringLiteral("transcribe") << QStringLiteral("tts")
            << QStringLiteral("image") << QStringLiteral("embedding")
            << QStringLiteral("search") << QStringLiteral("deep-research")
            << QStringLiteral("codex") << QStringLiteral("-pro");
    foreach (const QString &part, excluded) {
        if (id.contains(part))
            return false;
    }
    return family;
}

bool decodeUtf8(const QByteArray &bytes, QString *text)
{
    QTextCodec::ConverterState state;
    *text = QTextCodec::codecForName("UTF-8")->toUnicode(bytes.constData(), bytes.size(), &state);
    return state.invalidChars == 0 && state.remainingChars == 0;
}
}

ChatController::ChatController(QObject *parent)
    : QObject(parent), m_network(this), m_operation(Idle), m_busy(false),
      m_sawDone(false), m_sawFinish(false), m_messageStarted(false),
      m_messageDelta(false), m_assistantIndex(-1), m_receivedBytes(0)
{
    const QVariantList stored = QSettings(settingsFile(), QSettings::IniFormat).value(QStringLiteral("providers/custom")).toList();
    for (const QVariant &value : stored) {
        const QVariantMap profile = value.toMap();
        const QString id = profile.value(QStringLiteral("id")).toString();
        const QString name = profile.value(QStringLiteral("name")).toString();
        const QString url = profile.value(QStringLiteral("baseUrl")).toString();
        const QString model = profile.value(QStringLiteral("model")).toString();
        if (isCustomProviderId(id) && customProfile(id).isEmpty() && !name.trimmed().isEmpty()
                && name.size() <= 80 && !url.isEmpty() && normalizedBaseUrl(url) == url
                && (model.isEmpty() || chatModel(model)))
            m_customProfiles.append(profile);
    }
    m_timeout.setSingleShot(true);
    connect(&m_imageWatcher, &QFutureWatcher<QString>::finished, this, [this]() {
        const QString image = m_imageWatcher.result();
        m_imageProcessing = false;
        if (image.isEmpty())
            setError(tr("Choose a readable JPEG or PNG up to 10 MiB and 20 megapixels. The resized image must fit within 1 MiB."));
        else {
            m_pendingImage = image;
            clearError();
        }
        emit pendingImageChanged();
    });
    connect(&m_timeout, &QTimer::timeout, this, [this]() {
        stopRequest(tr("The request timed out. You can retry manually."));
    });
}

ChatController::~ChatController()
{
    releaseReply();
    if (m_database.isValid()) {
        m_database.close();
        m_database = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_databaseConnection);
    }
}

QVariantMap ChatController::customProfile(const QString &provider) const
{
    for (const QVariant &value : m_customProfiles) {
        if (value.toMap().value(QStringLiteral("id")).toString() == provider)
            return value.toMap();
    }
    return QVariantMap();
}

bool ChatController::supportedProvider(const QString &provider) const
{
    return provider == QLatin1String("openai") || provider == QLatin1String("anthropic")
            || !customProfile(provider).isEmpty();
}

QString ChatController::providerTitle() const
{
    if (m_provider == QLatin1String("openai"))
        return QStringLiteral("OpenAI");
    if (m_provider == QLatin1String("anthropic"))
        return QStringLiteral("Anthropic");
    return customProfile(m_provider).value(QStringLiteral("name")).toString();
}

QString ChatController::providerEndpoint() const
{
    return customProfile(m_provider).value(QStringLiteral("baseUrl")).toString();
}

QString ChatController::createCustomProfile(QString name, QString baseUrl, QString model)
{
    if (!idle())
        return QString();
    name = name.trimmed();
    model = model.trimmed();
    const QString url = normalizedBaseUrl(baseUrl);
    if (name.isEmpty() || name.size() > 80 || url.isEmpty() || (!model.isEmpty() && !chatModel(model))) {
        setError(tr("Enter a name (up to 80 characters), an HTTPS base URL without credentials, query or fragment, and an optional model ID."));
        return QString();
    }
    for (const QVariant &value : m_customProfiles) {
        if (value.toMap().value(QStringLiteral("name")).toString().compare(name, Qt::CaseInsensitive) == 0) {
            setError(tr("Choose a different profile name."));
            return QString();
        }
    }
    QString id = QUuid::createUuid().toString();
    id.remove(QLatin1Char('{')).remove(QLatin1Char('}')).remove(QLatin1Char('-'));
    id.prepend(QStringLiteral("custom-"));
    QVariantMap profile;
    profile.insert(QStringLiteral("id"), id);
    profile.insert(QStringLiteral("name"), name);
    profile.insert(QStringLiteral("baseUrl"), url);
    profile.insert(QStringLiteral("model"), model);
    QVariantList profiles = m_customProfiles;
    profiles.append(profile);
    QSettings settings(settingsFile(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("providers/custom"), profiles);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        setError(tr("Could not save the custom profile."));
        return QString();
    }
    m_customProfiles = profiles;
    clearError();
    emit customProfilesChanged();
    return id;
}

void ChatController::showProviderHistory(QString provider)
{
    if (!idle() || !canReplaceConversation())
        return;
    if (!supportedProvider(provider)) {
        setError(tr("Unsupported provider."));
        return;
    }
    if (m_provider != provider) {
        m_key.clear();
        m_model.clear();
        m_models.clear();
        m_provider = provider;
        emit providerChanged();
        emit modelChanged();
        emit modelsChanged();
    }
    resetChat();
    clearError();
    m_historyLimit = 200;
    refreshConversations();
}

bool ChatController::deleteCustomProfile(QString provider)
{
    if (!idle() || !canReplaceConversation())
        return false;
    if (!isCustomProviderId(provider) || customProfile(provider).isEmpty()) {
        setError(tr("Unknown custom profile."));
        return false;
    }
    if (!ensureDatabase())
        return false;
    QVariantList profiles;
    for (const QVariant &value : m_customProfiles) {
        if (value.toMap().value(QStringLiteral("id")).toString() != provider)
            profiles.append(value);
    }
    if (!m_database.transaction()) {
        setError(tr("Could not delete the profile's chats. The profile was kept; retry deletion."));
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM messages WHERE conversation_id IN (SELECT id FROM conversations WHERE provider = ?)"));
    query.addBindValue(provider);
    bool ok = query.exec();
    if (ok) {
        query.prepare(QStringLiteral("DELETE FROM conversations WHERE provider = ?"));
        query.addBindValue(provider);
        ok = query.exec();
    }
    if (!ok || !m_database.commit()) {
        m_database.rollback();
        setError(tr("Could not delete the profile's chats. The profile was kept; retry deletion."));
        return false;
    }
    QSettings settings(settingsFile(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("providers/custom"), profiles);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        setError(tr("Chats were deleted, but the profile could not be removed. Retry deletion."));
        return false;
    }
    forgetProvider(provider);
    m_customProfiles = profiles;
    emit customProfilesChanged();
    return true;
}

QNetworkRequest ChatController::apiRequest(const QString &provider, const QString &key, bool chat) const
{
    const bool anthropic = provider == QLatin1String("anthropic");
    QString base = customProfile(provider).value(QStringLiteral("baseUrl")).toString();
    if (base.isEmpty())
        base = anthropic ? QStringLiteral("https://api.anthropic.com/v1") : QStringLiteral("https://api.openai.com/v1");
    const QString path = !chat ? QStringLiteral("/models") : anthropic
            ? QStringLiteral("/messages") : QStringLiteral("/chat/completions");
    QNetworkRequest request(QUrl(base + path));
    request.setRawHeader("Accept", chat ? "text/event-stream" : "application/json");
    if (anthropic) {
        request.setRawHeader("x-api-key", key.toLatin1());
        request.setRawHeader("anthropic-version", "2023-06-01");
    } else {
        request.setRawHeader("Authorization", "Bearer " + key.toLatin1());
    }
    return request;
}

void ChatController::setError(const QString &error)
{
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void ChatController::clearError() { setError(QString()); }

void ChatController::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

bool ChatController::idle()
{
    if (imageBusy()) {
        setError(tr("Wait for the image to finish processing."));
        return false;
    }
    if (!m_busy)
        return true;
    setError(tr("Wait for the current request to finish or cancel it first."));
    return false;
}

bool ChatController::validCredentials(const QString &provider, const QString &key)
{
    if (!supportedProvider(provider)) {
        setError(tr("Unsupported provider."));
        return false;
    }
    if (key.isEmpty() || key.size() > 4096) {
        setError(tr("Enter a valid API key."));
        return false;
    }
    for (int i = 0; i < key.size(); ++i) {
        const ushort c = key.at(i).unicode();
        if (c < 33 || c > 126) {
            setError(tr("The API key contains invalid characters."));
            return false;
        }
    }
    return true;
}

void ChatController::validateKey(QString provider, QString key)
{
    if (!idle() || !validCredentials(provider, key))
        return;
    fetchModels(ValidateModels, provider, key);
}

void ChatController::activateProvider(QString provider, QString key)
{
    if (!idle() || !canReplaceConversation() || !validCredentials(provider, key))
        return;
    if (!m_provider.isEmpty() && !m_model.isEmpty())
        m_previousModels.insert(m_provider, m_model);
    m_provider = provider;
    m_key = key;
    m_model.clear();
    m_models.clear();
    resetChat();
    emit providerChanged();
    emit modelChanged();
    emit modelsChanged();
    m_historyLimit = 200;
    refreshConversations();
    fetchModels(LoadModels, provider, key);
}

void ChatController::connectProvider(QString key)
{
    if (!idle() || !canReplaceConversation() || !validCredentials(m_provider, key))
        return;
    if (!m_model.isEmpty())
        m_previousModels.insert(m_provider, m_model);
    m_key = key;
    m_models.clear();
    emit modelsChanged();
    fetchModels(LoadModels, m_provider, key);
}

void ChatController::fetchModels(Operation operation, const QString &provider, const QString &key)
{
    clearError();
    m_requestProvider = provider;
    m_requestKey = key;
    const QString manualModel = customProfile(provider).value(QStringLiteral("model")).toString();
    if (!manualModel.isEmpty()) {
        m_requestProvider.clear();
        m_requestKey.clear();
        if (operation == LoadModels) {
            m_models = QStringList() << manualModel;
            if (m_conversationId.isEmpty())
                m_model = manualModel;
            emit modelsChanged();
            emit modelChanged();
        }
        // A manually configured model skips discovery, not a successful authentication check.
        if (operation == ValidateModels)
            emit keyValidated(provider, key);
        emit requestFinished(true);
        return;
    }
    attachReply(m_network.get(apiRequest(provider, key, false)), operation);
}

void ChatController::attachReply(QNetworkReply *reply, Operation operation)
{
    m_operation = operation;
    m_reply = reply;
    m_body.clear();
    m_receivedBytes = 0;
    connect(reply, &QIODevice::readyRead, this, &ChatController::readReply);
    connect(reply, &QNetworkReply::finished, this, &ChatController::finishReply);
    // Qt 5.6 does not automatically follow redirects. Never forward credentials elsewhere.
    connect(reply, &QNetworkReply::sslErrors, this, [this](const QList<QSslError> &) {
        stopRequest(tr("A secure connection could not be established."));
    });
    m_timeout.start(RequestTimeoutMs);
    setBusy(true);
}

void ChatController::readReply()
{
    if (!m_reply)
        return;
    const QByteArray bytes = m_reply->readAll();
    m_receivedBytes += bytes.size();
    if (m_receivedBytes > MaxResponseBytes) {
        stopRequest(tr("The server response exceeded the size limit."));
        return;
    }
    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status < 200 || status >= 300)
        return;
    if (m_operation == Chat) {
        const QByteArray contentType = m_reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
        if (contentType.split(';').value(0).trimmed().toLower() != "text/event-stream") {
            stopRequest(tr("The server did not return an event stream."));
            return;
        }
        feedStream(bytes);
        if (!m_streamFailure.isEmpty())
            stopRequest(m_streamFailure);
    } else {
        m_body.append(bytes);
    }
}

QStringList ChatController::parseModels(const QByteArray &body) const
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    QStringList result;
    if (parseError.error != QJsonParseError::NoError || !document.isObject()
            || !document.object().value(QStringLiteral("data")).isArray())
        return result;
    const QJsonArray data = document.object().value(QStringLiteral("data")).toArray();
    foreach (const QJsonValue &entry, data) {
        if (!entry.isObject())
            continue;
        const QString id = entry.toObject().value(QStringLiteral("id")).toString();
        if (isCustomProviderId(m_requestProvider) ? chatModel(id)
                : m_requestProvider == QStringLiteral("openai") ? openAIChatModel(id)
                : (chatModel(id) && id.startsWith(QStringLiteral("claude-"))))
            result.append(id);
    }
    result.removeDuplicates();
    std::sort(result.begin(), result.end());
    return result;
}

void ChatController::finishReply()
{
    if (!m_reply)
        return;
    readReply();
    if (!m_reply)
        return;
    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString failure;
    if (status == 401 || status == 403)
        failure = tr("The provider rejected access. Check the API key and permissions.");
    else if (status == 429)
        failure = tr("The provider rate limit or quota was reached. Check billing and retry later.");
    else if (status == 413)
        failure = tr("The provider rejected the request size. Start a new chat or use a smaller photo.");
    else if ((status == 400 || status == 415 || status == 422) && m_operation == Chat)
        failure = tr("The provider rejected the message format. If this chat contains photos, select an image-capable model and an endpoint that accepts images.");
    else if (status == 0)
        failure = tr("The network request failed. Check your connection.");
    else if (status < 200 || status >= 300)
        failure = tr("The provider request failed (HTTP %1).").arg(status);
    else if (m_reply->error() != QNetworkReply::NoError)
        failure = tr("The network request failed. Check your connection.");

    const Operation operation = m_operation;
    const QString requestProvider = m_requestProvider;
    const QString requestKey = m_requestKey;
    QStringList available;
    if (operation != Chat && failure.isEmpty()) {
        available = parseModels(m_body);
        if (available.isEmpty())
            failure = tr("The provider returned no supported chat models or an invalid model list.");
    }
    if (operation == Chat && failure.isEmpty()) {
        // A frame is only complete after a blank line; unfinished frames are not accepted.
        if (!m_streamFailure.isEmpty())
            failure = m_streamFailure;
        else if (!m_sawDone || !m_sawFinish || m_assistantText.trimmed().isEmpty()
                 || !m_lineBuffer.trimmed().isEmpty() || !m_eventData.isEmpty())
            failure = tr("The response was empty or incomplete. You can retry manually.");
    }
    releaseReply();
    if (operation == Chat) {
        finishChat(failure.isEmpty(), failure);
        return;
    }
    if (failure.isEmpty() && operation == LoadModels) {
        m_models = available;
        const QString previous = m_previousModels.value(m_provider);
        if (!m_conversationId.isEmpty()) {
            m_previousModels.insert(m_provider, m_model);
        } else if (available.contains(previous)) {
            m_model = previous;
        } else if (m_provider == QStringLiteral("openai")
                   && available.contains(QStringLiteral("gpt-4.1-mini"))) {
            m_model = QStringLiteral("gpt-4.1-mini");
        } else {
            m_model = available.first();
            if (m_provider == QStringLiteral("anthropic")) {
                for (int i = available.size() - 1; i >= 0; --i) {
                    if (available.at(i).contains(QStringLiteral("sonnet"))) {
                        m_model = available.at(i);
                        break;
                    }
                }
            }
        }
        m_previousModels.insert(m_provider, m_model);
        emit modelsChanged();
        emit modelChanged();
    }
    setError(failure);
    setBusy(false);
    if (failure.isEmpty() && operation == ValidateModels)
        emit keyValidated(requestProvider, requestKey);
    emit requestFinished(failure.isEmpty());
}

void ChatController::releaseReply()
{
    m_timeout.stop();
    if (m_reply) {
        QNetworkReply *reply = m_reply.data();
        m_reply.clear();
        reply->disconnect(this);
        if (reply->isRunning())
            reply->abort();
        reply->deleteLater();
    }
    m_operation = Idle;
    m_requestKey.clear();
    m_requestProvider.clear();
    m_body.clear();
}

void ChatController::stopRequest(const QString &reason)
{
    if (!m_reply)
        return;
    const Operation operation = m_operation;
    releaseReply();
    if (operation == Chat) {
        finishChat(false, reason);
    } else {
        setError(reason);
        setBusy(false);
        emit requestFinished(false);
    }
}

void ChatController::cancel()
{
    stopRequest(tr("Request cancelled. Any partial response was not saved. You can retry manually."));
}

void ChatController::setModel(QString model)
{
    if (!idle() || !canReplaceConversation())
        return;
    if (!m_models.contains(model)) {
        setError(tr("Select an available model."));
        return;
    }
    if (m_model != model) {
        m_model = model;
        m_previousModels.insert(m_provider, model);
        emit modelChanged();
    }
    clearError();
}

void ChatController::resetChat()
{
    m_pendingImage.clear();
    emit pendingImageChanged();
    m_messages.clear();
    m_conversationId.clear();
    m_title.clear();
    m_assistantIndex = -1;
    emit messagesChanged();
}

void ChatController::newChat()
{
    if (!idle() || !canReplaceConversation())
        return;
    resetChat();
    clearError();
}

void ChatController::forgetProvider(QString provider)
{
    if (!idle() || !canReplaceConversation())
        return;
    if (!supportedProvider(provider)) {
        setError(tr("Unsupported provider."));
        return;
    }
    m_previousModels.remove(provider);
    if (m_provider == provider) {
        m_key.clear();
        m_provider.clear();
        m_model.clear();
        m_models.clear();
        m_conversations.clear();
        m_hasMoreConversations = false;
        resetChat();
        emit providerChanged();
        emit modelChanged();
        emit modelsChanged();
        emit conversationsChanged();
    }
    clearError();
}

void ChatController::attachImage(QString fileUrl)
{
    if (!idle() || !canReplaceConversation() || m_provider.isEmpty())
        return;
    m_imageProcessing = true;
    m_imageWatcher.setFuture(QtConcurrent::run(ChatImage::prepare, fileUrl));
    emit pendingImageChanged();
}

void ChatController::removeImage()
{
    if (!idle() || !canReplaceConversation())
        return;
    m_pendingImage.clear();
    emit pendingImageChanged();
}

void ChatController::sendMessage(QString text)
{
    if (idle())
        startChat(text, false);
}

void ChatController::retry()
{
    if (!idle())
        return;
    if (m_unsaved) {
        retrySave();
        return;
    }
    if (m_models.isEmpty() && supportedProvider(m_provider) && !m_key.isEmpty()) {
        fetchModels(LoadModels, m_provider, m_key);
        return;
    }
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        const QVariantMap entry = m_messages.at(i).toMap();
        if (entry.value(QStringLiteral("status")).toString() == QStringLiteral("complete"))
            break;
        if (entry.value(QStringLiteral("role")).toString() == QStringLiteral("user")) {
            startChat(entry.value(QStringLiteral("content")).toString(), true);
            return;
        }
    }
    setError(tr("There is no failed message to retry."));
}

bool ChatController::startChat(const QString &text, bool retrying)
{
    QString image = m_pendingImage;
    if (retrying) {
        for (int index = m_messages.size() - 1; index >= 0; --index) {
            const QVariantMap entry = m_messages.at(index).toMap();
            if (entry.value("role").toString() == "user") {
                image = entry.value("image").toString();
                break;
            }
        }
    }
    if (!canReplaceConversation())
        return false;
    if (m_key.isEmpty() || !supportedProvider(m_provider) || !m_models.contains(m_model)) {
        setError(tr("Activate a provider and select an available model first."));
        return false;
    }
    if (text.trimmed().isEmpty() || text.toUtf8().size() > MaxInputBytes) {
        setError(tr("Enter a nonempty message of at most 16 KiB."));
        return false;
    }
    if (!image.isEmpty() && !ChatImage::valid(image)) {
        setError(tr("The attached image is invalid. Remove it and attach the photo again."));
        return false;
    }
    QVariantList complete;
    QJsonArray history;
    int bytes = text.toUtf8().size();
    int imageBytes = image.size();
    // Only committed, answered turns belong in subsequent requests.
    for (int i = 0; i + 1 < m_messages.size(); i += 2) {
        const QVariantMap user = m_messages.at(i).toMap();
        const QVariantMap assistant = m_messages.at(i + 1).toMap();
        if (user.value(QStringLiteral("status")).toString() != QStringLiteral("complete")
                || assistant.value(QStringLiteral("status")).toString() != QStringLiteral("complete"))
            break;
        complete.append(user);
        complete.append(assistant);
        for (int j = 0; j < 2; ++j) {
            const QVariantMap entry = j == 0 ? user : assistant;
            history.append(ChatImage::requestMessage(entry, m_provider == "anthropic"));
            imageBytes += entry.value("image").toString().size();
            bytes += entry.value(QStringLiteral("content")).toString().toUtf8().size();
        }
    }
    if (bytes > MaxHistoryBytes || history.size() + 2 > MaxMessages || imageBytes > 8 * 1024 * 1024) {
        setError(tr("This conversation reached the history limit. Start a new chat."));
        return false;
    }
    if (!ensureDatabase() || !ensureImageSchema())
        return false;
    QVariantMap userMessage = message(QStringLiteral("user"), text, QStringLiteral("pending"));
    userMessage.insert(QStringLiteral("image"), image);
    history.append(ChatImage::requestMessage(userMessage, m_provider == "anthropic"));
    QJsonObject body;
    body.insert(QStringLiteral("model"), m_model);
    body.insert(QStringLiteral("messages"), history);
    body.insert(QStringLiteral("stream"), true);
    body.insert(m_provider == QStringLiteral("openai") ? QStringLiteral("max_completion_tokens")
                                                        : QStringLiteral("max_tokens"), 2048);
    QNetworkRequest request = apiRequest(m_provider, m_key, true);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    m_messages = complete;
    m_messages.append(userMessage);
    if (!retrying) {
        m_pendingImage.clear();
        emit pendingImageChanged();
    }
    m_messages.append(message(QStringLiteral("assistant"), QString(), QStringLiteral("streaming")));
    m_assistantIndex = m_messages.size() - 1;
    m_lineBuffer.clear();
    m_eventData.clear();
    m_eventName.clear();
    m_streamFailure.clear();
    m_assistantText.clear();
    m_sawDone = false;
    m_sawFinish = false;
    m_messageStarted = false;
    m_messageDelta = false;
    m_contentBlocks.clear();
    clearError();
    emit messagesChanged();
    attachReply(m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact)), Chat);
    return true;
}

void ChatController::finishChat(bool success, const QString &reason)
{
    if (m_assistantIndex > 0 && m_assistantIndex < m_messages.size()) {
        const QString status = success ? QStringLiteral("complete") : QStringLiteral("failed");
        QVariantMap assistant = m_messages.at(m_assistantIndex).toMap();
        QVariantMap user = m_messages.at(m_assistantIndex - 1).toMap();
        assistant.insert(QStringLiteral("status"), status);
        user.insert(QStringLiteral("status"), status);
        m_messages[m_assistantIndex] = assistant;
        m_messages[m_assistantIndex - 1] = user;
        emit messagesChanged();
    }
    setError(reason);
    if (success) {
        m_unsaved = !saveConversation();
        emit unsavedChanged();
        success = !m_unsaved;
    }
    setBusy(false);
    emit requestFinished(success);
}

bool ChatController::canReplaceConversation()
{
    if (!m_unsaved)
        return true;
    setError(tr("This response has not been saved. Retry saving before leaving or sending another message."));
    return false;
}

void ChatController::retrySave()
{
    if (!idle() || !m_unsaved)
        return;
    if (saveConversation()) {
        m_unsaved = false;
        emit unsavedChanged();
        clearError();
    }
}

void ChatController::discardUnsavedConversation()
{
    if (!idle() || !m_unsaved)
        return;
    m_unsaved = false;
    resetChat();
    emit unsavedChanged();
    clearError();
}

void ChatController::streamError(const QString &reason)
{
    if (m_streamFailure.isEmpty())
        m_streamFailure = reason;
}

void ChatController::feedStream(const QByteArray &bytes)
{
    m_lineBuffer.append(bytes);
    int newline;
    while ((newline = m_lineBuffer.indexOf('\n')) >= 0 && m_streamFailure.isEmpty()) {
        QByteArray line = m_lineBuffer.left(newline);
        m_lineBuffer.remove(0, newline + 1);
        if (line.endsWith('\r'))
            line.chop(1);
        consumeLine(line);
    }
    if (m_lineBuffer.size() > MaxEventBytes)
        streamError(tr("The event stream exceeded the size limit."));
}

void ChatController::consumeLine(QByteArray line)
{
    if (line.isEmpty()) {
        if (!m_eventData.isEmpty())
            dispatchEvent();
        m_eventData.clear();
        m_eventName.clear();
        return;
    }
    if (line.startsWith(':'))
        return;
    const int colon = line.indexOf(':');
    const QByteArray field = colon < 0 ? line : line.left(colon);
    QByteArray value = colon < 0 ? QByteArray() : line.mid(colon + 1);
    if (value.startsWith(' '))
        value.remove(0, 1);
    if (field == "data") {
        m_eventData.append(value);
        m_eventData.append('\n');
    } else if (field == "event") {
        m_eventName = value;
    }
    if (m_eventData.size() > MaxEventBytes || m_eventName.size() > 256)
        streamError(tr("The event stream exceeded the size limit."));
}

void ChatController::appendAssistant(const QString &text)
{
    if ((m_assistantText + text).toUtf8().size() > MaxAssistantBytes) {
        streamError(tr("The response exceeded the text size limit."));
        return;
    }
    m_assistantText.append(text);
    if (m_assistantIndex >= 0 && m_assistantIndex < m_messages.size()) {
        QVariantMap entry = m_messages.at(m_assistantIndex).toMap();
        entry.insert(QStringLiteral("content"), m_assistantText);
        m_messages[m_assistantIndex] = entry;
        emit messagesChanged();
    }
}

void ChatController::dispatchEvent()
{
    const QString invalid = tr("The provider returned an invalid or unsupported event stream.");
    if (m_sawDone) {
        streamError(invalid);
        return;
    }
    const QByteArray data = m_eventData.trimmed();
    if (m_eventName == "error") {
        streamError(tr("The provider reported a streaming error. Check access, quota and billing."));
        return;
    }
    if (m_provider != QStringLiteral("anthropic") && data == "[DONE]") {
        if (!m_sawFinish)
            streamError(invalid);
        m_sawDone = true;
        return;
    }
    QString decoded;
    QJsonParseError error;
    if (!decodeUtf8(data, &decoded)) {
        streamError(invalid);
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(decoded.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        streamError(invalid);
        return;
    }
    const QJsonObject object = document.object();
    if (object.contains(QStringLiteral("error")) || object.value(QStringLiteral("type")).toString() == QStringLiteral("error")) {
        streamError(tr("The provider reported a streaming error. Check access, quota and billing."));
        return;
    }
    if (m_provider != QStringLiteral("anthropic")) {
        const QJsonValue choicesValue = object.value(QStringLiteral("choices"));
        if (!choicesValue.isArray()) {
            streamError(invalid);
            return;
        }
        const QJsonArray choices = choicesValue.toArray();
        if (choices.isEmpty() && object.value(QStringLiteral("usage")).isObject())
            return;
        if (choices.size() != 1 || !choices.at(0).isObject()) {
            streamError(invalid);
            return;
        }
        const QJsonObject choice = choices.at(0).toObject();
        if (choice.value(QStringLiteral("index")).toInt(-1) != 0 || m_sawFinish
                || !choice.value(QStringLiteral("delta")).isObject()) {
            streamError(invalid);
            return;
        }
        const QJsonObject delta = choice.value(QStringLiteral("delta")).toObject();
        if (delta.contains(QStringLiteral("tool_calls")) || delta.contains(QStringLiteral("function_call"))
                || (delta.contains(QStringLiteral("role")) && delta.value(QStringLiteral("role")).toString() != QStringLiteral("assistant"))) {
            streamError(invalid);
            return;
        }
        const QJsonValue content = delta.value(QStringLiteral("content"));
        if (!content.isUndefined() && !content.isNull() && !content.isString()) {
            streamError(invalid);
            return;
        }
        appendAssistant(content.toString());
        const QJsonValue finish = choice.value(QStringLiteral("finish_reason"));
        if (!finish.isNull() && !finish.isUndefined()) {
            if (finish.toString() != QStringLiteral("stop")) {
                streamError(tr("The response stopped before completing (limit or unsupported response)."));
                return;
            }
            m_sawFinish = true;
        }
        return;
    }
    const QString type = object.value(QStringLiteral("type")).toString();
    if (!m_eventName.isEmpty() && m_eventName != type.toUtf8()) {
        streamError(invalid);
        return;
    }
    if (type == QStringLiteral("ping"))
        return;
    if (type == QStringLiteral("message_start")) {
        const QJsonObject msg = object.value(QStringLiteral("message")).toObject();
        if (m_messageStarted || msg.value(QStringLiteral("role")).toString() != QStringLiteral("assistant")
                || !msg.value(QStringLiteral("content")).isArray()
                || !msg.value(QStringLiteral("content")).toArray().isEmpty()) {
            streamError(invalid);
            return;
        }
        m_messageStarted = true;
    } else if (type == QStringLiteral("content_block_start")) {
        const int index = object.value(QStringLiteral("index")).toInt(-1);
        const QJsonObject block = object.value(QStringLiteral("content_block")).toObject();
        if (!m_messageStarted || m_messageDelta || index < 0 || m_contentBlocks.contains(index)
                || block.value(QStringLiteral("type")).toString() != QStringLiteral("text")
                || !block.value(QStringLiteral("text")).isString()) {
            streamError(invalid);
            return;
        }
        m_contentBlocks.insert(index, QStringLiteral("text"));
        appendAssistant(block.value(QStringLiteral("text")).toString());
    } else if (type == QStringLiteral("content_block_delta")) {
        const int index = object.value(QStringLiteral("index")).toInt(-1);
        const QJsonObject delta = object.value(QStringLiteral("delta")).toObject();
        if (m_messageDelta || !m_contentBlocks.contains(index)
                || delta.value(QStringLiteral("type")).toString() != QStringLiteral("text_delta")
                || !delta.value(QStringLiteral("text")).isString()) {
            streamError(invalid);
            return;
        }
        appendAssistant(delta.value(QStringLiteral("text")).toString());
    } else if (type == QStringLiteral("content_block_stop")) {
        const int index = object.value(QStringLiteral("index")).toInt(-1);
        if (m_messageDelta || !m_contentBlocks.remove(index))
            streamError(invalid);
    } else if (type == QStringLiteral("message_delta")) {
        const QString stop = object.value(QStringLiteral("delta")).toObject().value(QStringLiteral("stop_reason")).toString();
        if (!m_messageStarted || m_messageDelta || !m_contentBlocks.isEmpty()) {
            streamError(invalid);
            return;
        }
        if (stop != QStringLiteral("end_turn") && stop != QStringLiteral("stop_sequence")) {
            streamError(tr("The response stopped before completing (limit or unsupported response)."));
            return;
        }
        m_messageDelta = true;
        m_sawFinish = true;
    } else if (type == QStringLiteral("message_stop")) {
        if (!m_sawFinish || !m_contentBlocks.isEmpty())
            streamError(invalid);
        m_sawDone = true;
    } else {
        streamError(invalid);
    }
}

bool ChatController::ensureDatabase()
{
    if (m_database.isOpen())
        return true;
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directory.isEmpty() || !QDir().mkpath(directory)) {
        setError(tr("Conversation storage is unavailable."));
        return false;
    }
    if (!m_database.isValid()) {
        m_databaseConnection = QStringLiteral("sailmate-history-") + QUuid::createUuid().toString();
        m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_databaseConnection);
        m_database.setDatabaseName(directory + QStringLiteral("/conversations.sqlite"));
    }
    if (!m_database.open()) {
        setError(tr("Conversation storage could not be opened."));
        return false;
    }
    QSqlQuery query(m_database);
    const bool ok = query.exec(QStringLiteral("PRAGMA foreign_keys = ON"))
            && query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS conversations (id TEXT PRIMARY KEY, title TEXT NOT NULL, provider TEXT NOT NULL, model TEXT NOT NULL, updated INTEGER NOT NULL)"))
            && query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS messages (conversation_id TEXT NOT NULL REFERENCES conversations(id) ON DELETE CASCADE, position INTEGER NOT NULL, role TEXT NOT NULL, content TEXT NOT NULL, PRIMARY KEY(conversation_id, position))"));
    if (!ok) {
        m_database.close();
        setError(tr("Conversation storage could not be initialized."));
    }
    return ok;
}

bool ChatController::ensureImageSchema()
{
    if (m_imageSchemaReady)
        return true;
    if (!ensureDatabase())
        return false;
    if (!m_database.record(QStringLiteral("messages")).contains(QStringLiteral("image"))) {
        QSqlQuery query(m_database);
        if (!query.exec(QStringLiteral("ALTER TABLE messages ADD COLUMN image TEXT NOT NULL DEFAULT ''"))) {
            setError(tr("Image history storage could not be initialized."));
            return false;
        }
    }
    m_imageSchemaReady = true;
    return true;
}

bool ChatController::saveConversation()
{
    if (!ensureDatabase() || !ensureImageSchema())
        return false;
    const QString id = m_conversationId.isEmpty() ? QUuid::createUuid().toString() : m_conversationId;
    QString title = m_title;
    if (title.isEmpty() && !m_messages.isEmpty())
        title = m_messages.first().toMap().value(QStringLiteral("content")).toString().simplified().left(80);
    bool ok = m_database.transaction();
    QSqlQuery query(m_database);
    if (ok) {
        query.prepare(QStringLiteral("INSERT OR REPLACE INTO conversations (id, title, provider, model, updated) VALUES (?, ?, ?, ?, ?)"));
        query.addBindValue(id);
        query.addBindValue(title);
        query.addBindValue(m_provider);
        query.addBindValue(m_model);
        query.addBindValue(QDateTime::currentMSecsSinceEpoch());
        ok = query.exec();
    }
    if (ok) {
        query.prepare(QStringLiteral("DELETE FROM messages WHERE conversation_id = ?"));
        query.addBindValue(id);
        ok = query.exec();
    }
    for (int i = 0; ok && i < m_messages.size(); ++i) {
        const QVariantMap entry = m_messages.at(i).toMap();
        if (entry.value(QStringLiteral("status")).toString() != QStringLiteral("complete"))
            break;
        query.prepare(QStringLiteral("INSERT INTO messages (conversation_id, position, role, content, image) VALUES (?, ?, ?, ?, ?)"));
        query.addBindValue(id);
        query.addBindValue(i);
        query.addBindValue(entry.value(QStringLiteral("role")));
        query.addBindValue(entry.value(QStringLiteral("content")));
        query.addBindValue(entry.value(QStringLiteral("image"), QStringLiteral("")));
        ok = query.exec();
    }
    if (ok)
        ok = m_database.commit();
    if (!ok) {
        m_database.rollback();
        setError(tr("The response was received, but conversation history could not be saved."));
        return false;
    }
    m_conversationId = id;
    m_title = title;
    refreshConversations();
    return true;
}

void ChatController::loadMoreConversations()
{
    if (!idle() || !m_hasMoreConversations)
        return;
    m_historyLimit += 200;
    clearError();
    refreshConversations();
}

void ChatController::refreshConversations()
{
    m_conversations.clear();
    m_hasMoreConversations = false;
    if (!m_provider.isEmpty() && ensureDatabase()) {
        QSqlQuery query(m_database);
        query.prepare(QStringLiteral("SELECT id, title, provider, model FROM conversations WHERE provider = ? ORDER BY updated DESC, id DESC LIMIT ?"));
        query.addBindValue(m_provider);
        query.addBindValue(m_historyLimit + 1);
        if (!query.exec()) {
            setError(tr("Conversation history could not be loaded."));
        } else {
            while (query.next()) {
                if (m_conversations.size() == m_historyLimit) {
                    m_hasMoreConversations = true;
                    break;
                }
                QVariantMap entry;
                entry.insert(QStringLiteral("id"), query.value(0));
                entry.insert(QStringLiteral("title"), query.value(1));
                entry.insert(QStringLiteral("provider"), query.value(2));
                entry.insert(QStringLiteral("model"), query.value(3));
                m_conversations.append(entry);
            }
        }
    }
    emit conversationsChanged();
}

void ChatController::openConversation(QString id)
{
    if (!idle() || !canReplaceConversation())
        return;
    if (m_provider.isEmpty() || id.isEmpty() || id.size() > 100) {
        setError(tr("Select a conversation for the active provider."));
        return;
    }
    if (!ensureDatabase() || !ensureImageSchema())
        return;
    QSqlQuery conversation(m_database);
    conversation.prepare(QStringLiteral("SELECT title, model FROM conversations WHERE id = ? AND provider = ?"));
    conversation.addBindValue(id);
    conversation.addBindValue(m_provider);
    if (!conversation.exec() || !conversation.next()) {
        setError(tr("This conversation is unavailable for the active provider."));
        return;
    }
    const QString title = conversation.value(0).toString();
    const QString model = conversation.value(1).toString();
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT role, content, image FROM messages WHERE conversation_id = ? ORDER BY position LIMIT 81"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(tr("Conversation history could not be loaded."));
        return;
    }
    QVariantList loaded;
    int bytes = 0;
    int imageBytes = 0;
    bool valid = true;
    while (query.next()) {
        const QString role = query.value(0).toString();
        const QString content = query.value(1).toString();
        const QString image = query.value(2).toString();
        imageBytes += image.size();
        if ((!image.isEmpty() && (role != "user" || !ChatImage::valid(image))) || imageBytes > 8 * 1024 * 1024) {
            valid = false;
            break;
        }
        bytes += content.toUtf8().size();
        if (role != (loaded.size() % 2 == 0 ? QStringLiteral("user") : QStringLiteral("assistant"))
                || content.trimmed().isEmpty() || bytes > MaxHistoryBytes + MaxAssistantBytes
                || loaded.size() >= MaxMessages) {
            valid = false;
            break;
        }
        QVariantMap entry = message(role, content, QStringLiteral("complete"));
        entry.insert(QStringLiteral("image"), image);
        loaded.append(entry);
    }
    if (!valid || loaded.isEmpty() || loaded.size() % 2 != 0) {
        setError(tr("Conversation history is invalid or exceeds the size limit."));
        return;
    }
    m_conversationId = id;
    m_title = title;
    m_messages = loaded;
    m_pendingImage.clear();
    emit pendingImageChanged();
    m_assistantIndex = -1;
    if (m_model != model) {
        m_model = model;
        m_previousModels.insert(m_provider, model);
        emit modelChanged();
    }
    emit messagesChanged();
    clearError();
}
