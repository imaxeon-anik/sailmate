#include <QtTest>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QUuid>
#include <QSettings>
#include <QTemporaryDir>
#include "appsettings.h"
#include "providerprofile.h"
#include "chatcontroller.h"

class FakeReply : public QNetworkReply
{
public:
    FakeReply(const QByteArray &body, int status, const QByteArray &type, QObject *parent)
        : QNetworkReply(parent), m_body(body), m_position(0), m_aborted(false)
    {
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
        setHeader(QNetworkRequest::ContentTypeHeader, type);
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }
    void abort() { m_aborted = true; setFinished(true); }
    void complete() { setFinished(true); emit finished(); }
    qint64 bytesAvailable() const { return m_body.size() - m_position + QNetworkReply::bytesAvailable(); }
    bool m_aborted;
protected:
    qint64 readData(char *data, qint64 maxSize)
    {
        const qint64 size = qMin(maxSize, qint64(m_body.size()) - m_position);
        if (!size)
            return -1;
        memcpy(data, m_body.constData() + m_position, size_t(size));
        m_position += size;
        return size;
    }
private:
    QByteArray m_body;
    qint64 m_position;
};

class ChatControllerTest : public QObject
{
    Q_OBJECT
private:
    static QByteArray openAIStream()
    {
        return QByteArray("data: {\"choices\":[{\"index\":0,\"delta\":{\"content\":\"")
                + QString::fromUtf8("Hello 🌍").toUtf8()
                + "\"},\"finish_reason\":null}]}\r\n\r\n"
                  "data: {\"choices\":[{\"index\":0,\"delta\":{},\n"
                  "data: \"finish_reason\":\"stop\"}]}\n\n"
                  "data: [DONE]\n\n";
    }
    static void database(ChatController &controller)
    {
        controller.m_databaseConnection = QUuid::createUuid().toString();
        controller.m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), controller.m_databaseConnection);
        controller.m_database.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(controller.m_database.open());
        QSqlQuery query(controller.m_database);
        QVERIFY(query.exec(QStringLiteral("CREATE TABLE conversations (id TEXT PRIMARY KEY, title TEXT, provider TEXT, model TEXT, updated INTEGER)")));
        QVERIFY(query.exec(QStringLiteral("CREATE TABLE messages (conversation_id TEXT, position INTEGER, role TEXT, content TEXT)")));
    }
    static QVariantMap entry(const QString &role, const QString &content, const QString &status)
    {
        QVariantMap value;
        value.insert(QStringLiteral("role"), role);
        value.insert(QStringLiteral("content"), content);
        value.insert(QStringLiteral("status"), status);
        return value;
    }
    QTemporaryDir settingsDirectory;
private slots:
    void initTestCase()
    {
        QVERIFY(settingsDirectory.isValid());
        qputenv("XDG_CONFIG_HOME", settingsDirectory.path().toUtf8());
        QCoreApplication::setOrganizationName(QStringLiteral("SailmateTests"));
        QCoreApplication::setApplicationName(QStringLiteral("Sailmate"));
    }
    void init() { QSettings(settingsFile(), QSettings::IniFormat).clear(); }
    void customProfilesAndRequests()
    {
        ChatController controller;
        const QString id = controller.createCustomProfile("Work", "https://example.invalid/gateway/v1/", "");
        QVERIFY(isCustomProviderId(id));
        QVERIFY(isProviderId(id));
        QVERIFY(!isProviderId("custom-../../key"));
        const QString second = controller.createCustomProfile("Personal", "https://other.invalid/v1", "vendor/model");
        QVERIFY(id != second);
        QVERIFY(!second.isEmpty());
        QVERIFY(controller.createCustomProfile("work", "https://example.invalid/v1", "").isEmpty());
        ChatController restored;
        QCOMPARE(restored.customProfiles().size(), 2);
        QVERIFY(restored.supportedProvider(id));
        QVERIFY(!restored.supportedProvider("custom-00000000000000000000000000000000"));
        const QNetworkRequest models = restored.apiRequest(id, "fake-key", false);
        QCOMPARE(models.url().toString(), QStringLiteral("https://example.invalid/gateway/v1/models"));
        QCOMPARE(models.rawHeader("Authorization"), QByteArray("Bearer fake-key"));
        QVERIFY(!models.hasRawHeader("x-api-key"));
        const QNetworkRequest chat = restored.apiRequest(second, "other-key", true);
        QCOMPARE(chat.url().toString(), QStringLiteral("https://other.invalid/v1/chat/completions"));
        QCOMPARE(chat.rawHeader("Authorization"), QByteArray("Bearer other-key"));
        QCOMPARE(restored.apiRequest("openai", "key", false).url().toString(), QStringLiteral("https://api.openai.com/v1/models"));
        QCOMPARE(restored.apiRequest("anthropic", "key", true).rawHeader("x-api-key"), QByteArray("key"));
        QVERIFY(!QSettings(settingsFile(), QSettings::IniFormat).value("providers/custom").toString().contains("fake-key"));
        QCOMPARE(QFileInfo(settingsFile()).absolutePath(),
                 QDir(settingsDirectory.path()).absoluteFilePath(QStringLiteral("SailmateTests/Sailmate")));
        QVERIFY(QFile::exists(settingsFile()));
        restored.m_requestProvider = id;
        QCOMPARE(restored.parseModels("{\"data\":[{\"id\":\"vendor/model\"},{\"id\":\"custom-model\"}]}"),
                 QStringList() << QStringLiteral("custom-model") << QStringLiteral("vendor/model"));
        restored.m_provider = id;
        restored.feedStream(openAIStream());
        QVERIFY(restored.m_streamFailure.isEmpty());
        QVERIFY(restored.m_sawDone);
        QCOMPARE(restored.providerTitle(), QStringLiteral("Work"));
        QCOMPARE(restored.providerEndpoint(), QStringLiteral("https://example.invalid/gateway/v1"));
    }
    void invalidCustomUrls_data()
    {
        QTest::addColumn<QString>("url");
        QTest::newRow("http") << "http://example.invalid/v1";
        QTest::newRow("credentials") << "https://user:password@example.invalid/v1";
        QTest::newRow("query") << "https://example.invalid/v1?api-key=secret";
        QTest::newRow("fragment") << "https://example.invalid/v1#secret";
        QTest::newRow("relative") << "/v1";
        QTest::newRow("hostless") << "https:///v1";
        QTest::newRow("newline") << "https://example.invalid/\nheader";
    }
    void invalidCustomUrls()
    {
        QFETCH(QString, url);
        ChatController controller;
        QVERIFY(controller.createCustomProfile("Invalid", url, "").isEmpty());
        QVERIFY(controller.customProfiles().isEmpty());
        QVERIFY(!controller.busy());
    }
    void customManualModelAndIsolation()
    {
        ChatController controller;
        database(controller);
        const QString first = controller.createCustomProfile("First", "https://first.invalid/v1", "vendor/model");
        const QString second = controller.createCustomProfile("Second", "https://second.invalid/v1", "another-model");
        QSignalSpy validated(&controller, SIGNAL(keyValidated(QString,QString)));
        controller.validateKey(first, "fake-key");
        QCOMPARE(validated.count(), 1);
        QVERIFY(!controller.m_reply);
        controller.activateProvider(first, "fake-key");
        QCOMPARE(controller.model(), QStringLiteral("vendor/model"));
        QVERIFY(!controller.busy());
        controller.m_messages << entry("user", "private", "complete") << entry("assistant", "answer", "complete");
        QVERIFY(controller.saveConversation());
        const QString conversation = controller.m_conversationId;
        controller.activateProvider(second, "other-key");
        QVERIFY(controller.messages().isEmpty());
        QVERIFY(controller.conversations().isEmpty());
        controller.openConversation(conversation);
        QVERIFY(!controller.error().isEmpty());
        QVERIFY(controller.messages().isEmpty());
        controller.activateProvider(first, "fake-key");
        controller.openConversation(conversation);
        QCOMPARE(controller.messages().size(), 2);
        controller.forgetProvider(first);
        QVERIFY(controller.m_key.isEmpty());
        QCOMPARE(controller.customProfiles().size(), 2);
    }
    void customHistoryAndDeletion()
    {
        ChatController controller;
        database(controller);
        const QString first = controller.createCustomProfile("First", "https://first.invalid/v1", "model");
        const QString second = controller.createCustomProfile("Second", "https://second.invalid/v1", "model");
        controller.activateProvider(first, "fake-key");
        controller.m_messages << entry("user", "first", "complete") << entry("assistant", "answer", "complete");
        QVERIFY(controller.saveConversation());
        const QString firstChat = controller.m_conversationId;
        controller.activateProvider(second, "other-key");
        controller.m_messages << entry("user", "second", "complete") << entry("assistant", "answer", "complete");
        QVERIFY(controller.saveConversation());
        const QString secondChat = controller.m_conversationId;

        controller.showProviderHistory(first);
        QCOMPARE(controller.provider(), first);
        QCOMPARE(controller.conversations().size(), 1);
        QVERIFY(controller.m_key.isEmpty());
        QVERIFY(controller.messages().isEmpty());
        QVERIFY(!controller.m_reply);
        QVERIFY(!controller.busy());
        controller.openConversation(firstChat);
        QCOMPARE(controller.messages().size(), 2);
        controller.openConversation(secondChat);
        QVERIFY(!controller.error().isEmpty());

        controller.setBusy(true);
        QVERIFY(!controller.deleteCustomProfile(first));
        controller.showProviderHistory(second);
        QCOMPARE(controller.provider(), first);
        controller.setBusy(false);
        QVERIFY(!controller.deleteCustomProfile("openai"));
        QCOMPARE(controller.customProfiles().size(), 2);
        QVERIFY(controller.deleteCustomProfile(first));
        QVERIFY(controller.provider().isEmpty());
        QVERIFY(controller.messages().isEmpty());
        QCOMPARE(controller.customProfiles().size(), 1);
        QVERIFY(!controller.deleteCustomProfile(first));
        QSqlQuery query(controller.m_database);
        QVERIFY(query.exec("SELECT COUNT(*) FROM messages"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 2);
        controller.showProviderHistory(second);
        QCOMPARE(controller.conversations().size(), 1);
        controller.openConversation(secondChat);
        QCOMPARE(controller.messages().size(), 2);
        ChatController restored;
        QCOMPARE(restored.customProfiles().size(), 1);
        QVERIFY(restored.supportedProvider(second));
        QVERIFY(!restored.supportedProvider(first));
        QVERIFY(!controller.m_reply);
    }
    void customDeletionRollback()
    {
        ChatController controller;
        database(controller);
        const QString provider = controller.createCustomProfile("Keep", "https://keep.invalid/v1", "model");
        controller.activateProvider(provider, "fake-key");
        controller.m_messages << entry("user", "keep", "complete") << entry("assistant", "answer", "complete");
        QVERIFY(controller.saveConversation());
        QSqlQuery query(controller.m_database);
        QVERIFY(query.exec("CREATE TRIGGER prevent_delete BEFORE DELETE ON conversations BEGIN SELECT RAISE(ABORT, 'test failure'); END"));
        QVERIFY(!controller.deleteCustomProfile(provider));
        QVERIFY(!controller.error().isEmpty());
        QCOMPARE(controller.customProfiles().size(), 1);
        QVERIFY(query.exec("SELECT COUNT(*) FROM messages"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 2);
        ChatController restored;
        QVERIFY(restored.supportedProvider(provider));
    }
    void unsavedResponseCanBeRetriedLocally()
    {
        ChatController controller;
        database(controller);
        const QString provider = controller.createCustomProfile("Save retry", "https://save.invalid/v1", "model");
        controller.activateProvider(provider, "fake-key");
        QSqlQuery query(controller.m_database);
        QVERIFY(query.exec("CREATE TRIGGER fail_save BEFORE INSERT ON messages BEGIN SELECT RAISE(ABORT, 'test failure'); END"));
        controller.m_messages << entry("user", "question", "pending") << entry("assistant", "answer", "streaming");
        controller.m_assistantIndex = 1;
        controller.finishChat(true, QString());
        QVERIFY(controller.unsaved());
        QVERIFY(!controller.error().isEmpty());
        controller.newChat();
        controller.showProviderHistory(provider);
        controller.forgetProvider(provider);
        controller.openConversation("other");
        controller.setModel("model");
        controller.connectProvider("another-key");
        QVERIFY(!controller.deleteCustomProfile(provider));
        QVERIFY(!controller.startChat("another question", false));
        QCOMPARE(controller.messages().size(), 2);
        QCOMPARE(controller.provider(), provider);
        QVERIFY(!controller.m_reply);
        controller.retry();
        QVERIFY(controller.unsaved());
        QVERIFY(query.exec("DROP TRIGGER fail_save"));
        controller.retry();
        QVERIFY(!controller.unsaved());
        QVERIFY(controller.error().isEmpty());
        QVERIFY(!controller.m_reply);
        QCOMPARE(controller.conversations().size(), 1);
        const QString conversation = controller.m_conversationId;
        controller.newChat();
        controller.openConversation(conversation);
        QCOMPARE(controller.messages().at(1).toMap().value("content").toString(), QStringLiteral("answer"));
    }
    void offlineHistoryAndReconnectKeepConversation()
    {
        ChatController controller;
        database(controller);
        const QString provider = controller.createCustomProfile("Offline", "https://offline.invalid/v1", "model");
        controller.activateProvider(provider, "fake-key");
        controller.m_messages << entry("user", "question", "complete") << entry("assistant", "answer", "complete");
        QVERIFY(controller.saveConversation());
        const QString conversation = controller.m_conversationId;
        controller.forgetProvider(provider);
        controller.showProviderHistory(provider);
        controller.openConversation(conversation);
        QVERIFY(controller.error().isEmpty());
        QVERIFY(!controller.connected());
        QVERIFY(!controller.m_reply);
        QCOMPARE(controller.model(), QStringLiteral("model"));
        QCOMPARE(controller.messages().size(), 2);
        controller.connectProvider("recovered-key");
        QVERIFY(controller.connected());
        QCOMPARE(controller.m_conversationId, conversation);
        QCOMPARE(controller.messages().size(), 2);
        QVERIFY(!controller.m_reply);
    }
    void unavailableSavedModelRequiresSelection()
    {
        ChatController controller;
        database(controller);
        const QString provider = controller.createCustomProfile("Models", "https://models.invalid/v1", "replacement");
        controller.activateProvider(provider, "fake-key");
        controller.m_model = QStringLiteral("retired-model");
        controller.m_messages << entry("user", "question", "complete") << entry("assistant", "answer", "complete");
        QVERIFY(controller.saveConversation());
        const QString conversation = controller.m_conversationId;
        controller.forgetProvider(provider);
        controller.showProviderHistory(provider);
        controller.openConversation(conversation);
        controller.connectProvider("fake-key");
        QCOMPARE(controller.model(), QStringLiteral("retired-model"));
        QVERIFY(!controller.models().contains(controller.model()));
        QVERIFY(!controller.startChat("must not send", false));
        QVERIFY(!controller.m_reply);

        controller.m_requestProvider = provider;
        FakeReply *reply = new FakeReply("{\"data\":[{\"id\":\"replacement\"}]}", 200, "application/json", &controller);
        controller.attachReply(reply, ChatController::LoadModels);
        reply->complete();
        QVERIFY(controller.error().isEmpty());
        QCOMPARE(controller.model(), QStringLiteral("retired-model"));
        QCOMPARE(controller.m_conversationId, conversation);
        QCOMPARE(controller.messages().size(), 2);
        controller.setModel("replacement");
        QCOMPARE(controller.model(), QStringLiteral("replacement"));
        QVERIFY(controller.models().contains(controller.model()));
        QVERIFY(!controller.m_reply);
    }
    void discardUnsavedKeepsStoredHistory()
    {
        ChatController controller;
        database(controller);
        const QString provider = controller.createCustomProfile("Discard", "https://discard.invalid/v1", "model");
        controller.activateProvider(provider, "fake-key");
        controller.m_messages << entry("user", "saved question", "complete") << entry("assistant", "saved answer", "complete");
        QVERIFY(controller.saveConversation());
        const QString conversation = controller.m_conversationId;
        controller.discardUnsavedConversation();
        QCOMPARE(controller.messages().size(), 2);
        QSqlQuery query(controller.m_database);
        QVERIFY(query.exec("CREATE TRIGGER fail_save BEFORE INSERT ON messages BEGIN SELECT RAISE(ABORT, 'test failure'); END"));
        controller.m_messages << entry("user", "unsaved question", "pending") << entry("assistant", "unsaved answer", "streaming");
        controller.m_assistantIndex = 3;
        controller.finishChat(true, QString());
        QVERIFY(controller.unsaved());
        controller.setBusy(true);
        controller.discardUnsavedConversation();
        QVERIFY(controller.unsaved());
        QCOMPARE(controller.messages().size(), 4);
        controller.setBusy(false);
        controller.discardUnsavedConversation();
        QVERIFY(!controller.unsaved());
        QVERIFY(controller.messages().isEmpty());
        QVERIFY(controller.error().isEmpty());
        QVERIFY(controller.m_conversationId.isEmpty());
        QVERIFY(!controller.m_reply);
        controller.openConversation(conversation);
        QCOMPARE(controller.messages().size(), 2);
        QCOMPARE(controller.messages().last().toMap().value("content").toString(), QStringLiteral("saved answer"));
    }
    void historyPagination()
    {
        ChatController controller;
        database(controller);
        const QString provider = controller.createCustomProfile("Many chats", "https://many.invalid/v1", "model");
        QSqlQuery query(controller.m_database);
        for (int index = 0; index < 401; ++index) {
            query.prepare("INSERT INTO conversations VALUES (?, 'chat', ?, 'model', ?)");
            query.addBindValue(QString::number(index));
            query.addBindValue(index == 400 ? "openai" : provider);
            query.addBindValue(index);
            QVERIFY(query.exec());
        }
        controller.showProviderHistory(provider);
        QCOMPARE(controller.conversations().size(), 200);
        QVERIFY(controller.hasMoreConversations());
        controller.loadMoreConversations();
        QCOMPARE(controller.conversations().size(), 400);
        QVERIFY(!controller.hasMoreConversations());
        controller.loadMoreConversations();
        QCOMPARE(controller.conversations().size(), 400);
        controller.showProviderHistory("openai");
        QCOMPARE(controller.conversations().size(), 1);
        QVERIFY(!controller.hasMoreConversations());
    }
    void customDiscoveryFailures()
    {
        ChatController controller;
        const QString id = controller.createCustomProfile("Work", "https://example.invalid/v1", "");
        QSignalSpy validated(&controller, SIGNAL(keyValidated(QString,QString)));
        const QList<int> statuses = QList<int>() << 401 << 403 << 404 << 302;
        for (int status : statuses) {
            controller.m_requestProvider = id;
            controller.m_requestKey = "test-only";
            FakeReply *reply = new FakeReply("do-not-display", status, "application/json", &controller);
            controller.attachReply(reply, ChatController::ValidateModels);
            reply->complete();
            QCOMPARE(validated.count(), 0);
            QVERIFY(!controller.busy());
            QVERIFY(!controller.error().contains("do-not-display"));
            QVERIFY(controller.m_requestKey.isEmpty());
        }
        controller.setBusy(true);
        QVERIFY(controller.createCustomProfile("Busy", "https://busy.invalid", "").isEmpty());
        QCOMPARE(controller.customProfiles().size(), 1);
    }
    void openAISplitFrames()
    {
        const QByteArray stream = openAIStream();
        for (int split = 0; split <= stream.size(); ++split) {
            ChatController controller;
            controller.m_provider = QStringLiteral("openai");
            controller.feedStream(stream.left(split));
            controller.feedStream(stream.mid(split));
            QVERIFY2(controller.m_streamFailure.isEmpty(), qPrintable(controller.m_streamFailure));
            QVERIFY(controller.m_sawDone);
            QVERIFY(controller.m_sawFinish);
            QCOMPARE(controller.m_assistantText, QString::fromUtf8("Hello 🌍"));
        }
    }
    void anthropicBytewise()
    {
        ChatController controller;
        controller.m_provider = QStringLiteral("anthropic");
        const QByteArray stream =
                ": heartbeat\r\n\r\n"
                "event: message_start\r\ndata: {\"type\":\"message_start\",\"message\":{\"role\":\"assistant\",\"content\":[]}}\r\n\r\n"
                "event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"text\",\"text\":\"\"}}\n\n"
                "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":\"Hi\"}}\n\n"
                "event: content_block_stop\ndata: {\"type\":\"content_block_stop\",\"index\":0}\n\n"
                "event: message_delta\ndata: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"end_turn\"}}\n\n"
                "event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n";
        for (int i = 0; i < stream.size(); ++i)
            controller.feedStream(stream.mid(i, 1));
        QVERIFY2(controller.m_streamFailure.isEmpty(), qPrintable(controller.m_streamFailure));
        QVERIFY(controller.m_sawDone);
        QVERIFY(controller.m_sawFinish);
        QCOMPARE(controller.m_assistantText, QStringLiteral("Hi"));
    }
    void malformedStreams_data()
    {
        QTest::addColumn<QByteArray>("stream");
        QTest::newRow("invalid-json") << QByteArray("data: not json\n\n");
        QTest::newRow("early-done") << QByteArray("data: [DONE]\n\n");
        QTest::newRow("provider-error") << QByteArray("event: error\ndata: {\"secret\":\"not-for-display\"}\n\n");
        QTest::newRow("error-object") << QByteArray("data: {\"error\":{\"message\":\"not-for-display\"}}\n\n");
        QTest::newRow("invalid-choice") << QByteArray("data: {\"choices\":[{\"index\":1,\"delta\":{}}]}\n\n");
        QTest::newRow("token-limit") << QByteArray("data: {\"choices\":[{\"index\":0,\"delta\":{},\"finish_reason\":\"length\"}]}\n\n");
        QTest::newRow("invalid-utf8") << QByteArray("data: {\"choices\":[{\"index\":0,\"delta\":{\"content\":\"\xff\"}}]}\n\n");
        QTest::newRow("after-done") << (openAIStream() + "data: {}\n\n");
    }
    void malformedStreams()
    {
        QFETCH(QByteArray, stream);
        ChatController controller;
        controller.m_provider = QStringLiteral("openai");
        controller.feedStream(stream);
        QVERIFY(!controller.m_streamFailure.isEmpty());
        QVERIFY(!controller.m_streamFailure.contains(QStringLiteral("not-for-display")));
    }
    void incompleteFrame()
    {
        ChatController controller;
        controller.m_provider = QStringLiteral("openai");
        controller.feedStream("data: {\"choices\":[{\"index\":0,\"delta\":{\"content\":\"hello\"}}]}");
        QVERIFY(controller.m_assistantText.isEmpty());
        QVERIFY(!controller.m_sawDone);
    }
    void modelsAndValidation()
    {
        ChatController controller;
        controller.m_requestProvider = QStringLiteral("openai");
        const QByteArray body = "{\"data\":[{\"id\":\"gpt-4.1-mini\"},{\"id\":\"gpt-4o-realtime-preview\"},{\"id\":\"text-embedding-3-small\"},{\"id\":\"o3-mini\"},{\"id\":\"gpt-4.1-mini\"}]}";
        QCOMPARE(controller.parseModels(body), QStringList() << QStringLiteral("gpt-4.1-mini") << QStringLiteral("o3-mini"));
        QSignalSpy validated(&controller, SIGNAL(keyValidated(QString,QString)));
        controller.m_requestKey = QStringLiteral("test-only");
        FakeReply *reply = new FakeReply(body, 200, "application/json", &controller);
        controller.attachReply(reply, ChatController::ValidateModels);
        reply->complete();
        QCOMPARE(validated.count(), 1);
        QVERIFY(!controller.busy());
        QVERIFY(controller.error().isEmpty());
        QCOMPARE(controller.m_requestKey, QString());
        controller.m_requestProvider = QStringLiteral("openai");
        controller.m_requestKey = QStringLiteral("test-only");
        reply = new FakeReply("secret body", 401, "application/json", &controller);
        controller.attachReply(reply, ChatController::ValidateModels);
        reply->complete();
        QCOMPARE(validated.count(), 1);
        QVERIFY(!controller.error().contains(QStringLiteral("secret")));
        QVERIFY(!controller.busy());
    }
    void connectionFailureIsNotAuthenticationFailure()
    {
        ChatController controller;
        QSignalSpy validated(&controller, SIGNAL(keyValidated(QString,QString)));
        FakeReply *reply = new FakeReply(QByteArray(), 0, "application/json", &controller);
        controller.attachReply(reply, ChatController::ValidateModels);
        reply->complete();
        QCOMPARE(validated.count(), 0);
        QVERIFY(controller.error().contains(QStringLiteral("connection")));
        QVERIFY(!controller.error().contains(QStringLiteral("HTTP 0")));
        QVERIFY(!controller.busy());
    }
    void cancellationKeepsPartialUncommitted()
    {
        ChatController controller;
        database(controller);
        controller.m_provider = QStringLiteral("openai");
        controller.m_messages << entry("user", "question", "pending") << entry("assistant", "partial", "streaming");
        controller.m_assistantIndex = 1;
        FakeReply *reply = new FakeReply(QByteArray(), 200, "text/event-stream", &controller);
        controller.attachReply(reply, ChatController::Chat);
        controller.cancel();
        QVERIFY(reply->m_aborted);
        QVERIFY(!controller.busy());
        QCOMPARE(controller.messages().at(0).toMap().value("status").toString(), QStringLiteral("failed"));
        QCOMPARE(controller.messages().at(1).toMap().value("content").toString(), QStringLiteral("partial"));
        QSqlQuery query(controller.m_database);
        QVERIFY(query.exec("SELECT COUNT(*) FROM messages"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
    }
    void providerBoundHistory()
    {
        ChatController controller;
        database(controller);
        controller.m_provider = QStringLiteral("openai");
        controller.m_model = QStringLiteral("gpt-4.1-mini");
        controller.m_models << controller.m_model;
        controller.m_messages << entry("user", "question", "complete") << entry("assistant", "answer", "complete");
        QVERIFY(controller.saveConversation());
        const QString id = controller.m_conversationId;
        controller.newChat();
        controller.openConversation(id);
        QCOMPARE(controller.messages().size(), 2);
        controller.m_provider = QStringLiteral("anthropic");
        controller.newChat();
        controller.openConversation(id);
        QVERIFY(controller.messages().isEmpty());
        QVERIFY(!controller.error().isEmpty());
    }
    void busyAndCredentials()
    {
        ChatController controller;
        QSignalSpy validated(&controller, SIGNAL(keyValidated(QString,QString)));
        controller.validateKey(QStringLiteral("https://example.com"), QStringLiteral("key"));
        controller.validateKey(QStringLiteral("openai"), QStringLiteral("bad\r\nheader"));
        QVERIFY(!controller.busy());
        QCOMPARE(validated.count(), 0);
        controller.m_provider = QStringLiteral("openai");
        controller.m_model = QStringLiteral("gpt-4.1-mini");
        controller.m_models << controller.m_model;
        controller.m_messages << entry("user", "pending", "pending");
        controller.setBusy(true);
        controller.newChat();
        controller.setModel(QStringLiteral("different"));
        controller.activateProvider(QStringLiteral("anthropic"), QStringLiteral("test-only"));
        controller.forgetProvider(QStringLiteral("openai"));
        QCOMPARE(controller.messages().size(), 1);
        QCOMPARE(controller.provider(), QStringLiteral("openai"));
        QCOMPARE(controller.model(), QStringLiteral("gpt-4.1-mini"));
    }
};

QTEST_GUILESS_MAIN(ChatControllerTest)
#include "tst_chatcontroller.moc"
