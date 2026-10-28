#ifndef CHATCONTROLLER_H
#define CHATCONTROLLER_H

#include <QObject>
#include <QByteArray>
#include <QMap>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSqlDatabase>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QFutureWatcher>

class QNetworkReply;
class ChatControllerTest;

class ChatController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString provider READ provider NOTIFY providerChanged)
    Q_PROPERTY(QString providerTitle READ providerTitle NOTIFY providerChanged)
    Q_PROPERTY(QString providerEndpoint READ providerEndpoint NOTIFY providerChanged)
    Q_PROPERTY(QVariantList customProfiles READ customProfiles NOTIFY customProfilesChanged)
    Q_PROPERTY(QString model READ model NOTIFY modelChanged)
    Q_PROPERTY(QStringList models READ models NOTIFY modelsChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QVariantList conversations READ conversations NOTIFY conversationsChanged)
    Q_PROPERTY(bool hasMoreConversations READ hasMoreConversations NOTIFY conversationsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool unsaved READ unsaved NOTIFY unsavedChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY modelsChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString pendingImage READ pendingImage NOTIFY pendingImageChanged)
    Q_PROPERTY(bool imageBusy READ imageBusy NOTIFY pendingImageChanged)
public:
    explicit ChatController(QObject *parent = 0);
    ~ChatController();

    QString provider() const { return m_provider; }
    QString model() const { return m_model; }
    QStringList models() const { return m_models; }
    QVariantList messages() const { return m_messages; }
    QVariantList conversations() const { return m_conversations; }
    bool hasMoreConversations() const { return m_hasMoreConversations; }
    Q_INVOKABLE void loadMoreConversations();
    bool busy() const { return m_busy; }
    bool unsaved() const { return m_unsaved; }
    bool connected() const { return !m_key.isEmpty() && !m_models.isEmpty(); }
    Q_INVOKABLE void retrySave();
    Q_INVOKABLE void discardUnsavedConversation();
    Q_INVOKABLE void connectProvider(QString key);
    QString error() const { return m_error; }
    QString pendingImage() const { return m_pendingImage; }
    bool imageBusy() const { return m_imageProcessing; }
    Q_INVOKABLE void attachImage(QString fileUrl);
    Q_INVOKABLE void removeImage();

    QString providerTitle() const;
    QString providerEndpoint() const;
    QVariantList customProfiles() const { return m_customProfiles; }
    Q_INVOKABLE QString createCustomProfile(QString name, QString baseUrl, QString model);
    Q_INVOKABLE void showProviderHistory(QString provider);
    Q_INVOKABLE bool deleteCustomProfile(QString provider);
    Q_INVOKABLE void validateKey(QString provider, QString key);
    Q_INVOKABLE void activateProvider(QString provider, QString key);
    Q_INVOKABLE void setModel(QString model);
    Q_INVOKABLE void sendMessage(QString text);
    Q_INVOKABLE void retry();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void newChat();
    Q_INVOKABLE void openConversation(QString id);
    Q_INVOKABLE void forgetProvider(QString provider);
    Q_INVOKABLE void clearError();

signals:
    void providerChanged();
    void customProfilesChanged();
    void modelChanged();
    void modelsChanged();
    void messagesChanged();
    void conversationsChanged();
    void busyChanged();
    void unsavedChanged();
    void errorChanged();
    void pendingImageChanged();
    void keyValidated(QString provider, QString key);
    void requestFinished(bool success);

private:
    friend class ChatControllerTest;
    enum Operation { Idle, ValidateModels, LoadModels, Chat };
    bool idle();
    bool canReplaceConversation();
    bool m_unsaved = false;
    bool supportedProvider(const QString &provider) const;
    QVariantMap customProfile(const QString &provider) const;
    QNetworkRequest apiRequest(const QString &provider, const QString &key, bool chat) const;
    QVariantList m_customProfiles;
    bool validCredentials(const QString &provider, const QString &key);
    void setError(const QString &error);
    void setBusy(bool busy);
    void resetChat();
    void fetchModels(Operation operation, const QString &provider, const QString &key);
    void attachReply(QNetworkReply *reply, Operation operation);
    void readReply();
    void finishReply();
    void stopRequest(const QString &reason);
    void releaseReply();
    QStringList parseModels(const QByteArray &body) const;
    bool startChat(const QString &text, bool retrying);
    void finishChat(bool success, const QString &reason);
    void feedStream(const QByteArray &bytes);
    void consumeLine(QByteArray line);
    void dispatchEvent();
    void streamError(const QString &reason);
    void appendAssistant(const QString &text);
    bool ensureDatabase();
    bool saveConversation();
    bool ensureImageSchema();
    void refreshConversations();

    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QTimer m_timeout;
    Operation m_operation;
    bool m_busy;
    QString m_provider;
    QString m_key;
    QString m_model;
    QStringList m_models;
    QMap<QString, QString> m_previousModels;
    QVariantList m_messages;
    QVariantList m_conversations;
    int m_historyLimit = 200;
    bool m_hasMoreConversations = false;
    QString m_error;
    QString m_conversationId;
    QString m_title;
    QString m_requestProvider;
    QString m_requestKey;
    QByteArray m_body;
    QByteArray m_lineBuffer;
    QByteArray m_eventData;
    QByteArray m_eventName;
    QString m_streamFailure;
    QString m_assistantText;
    bool m_sawDone;
    bool m_sawFinish;
    bool m_messageStarted;
    bool m_messageDelta;
    QMap<int, QString> m_contentBlocks;
    int m_assistantIndex;
    qint64 m_receivedBytes;
    QString m_databaseConnection;
    QSqlDatabase m_database;
    QString m_pendingImage;
    bool m_imageProcessing = false;
    QFutureWatcher<QString> m_imageWatcher;
    bool m_imageSchemaReady = false;
};

#endif
