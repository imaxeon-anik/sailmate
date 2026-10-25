#include <QtTest>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QUuid>
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
private slots:
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
