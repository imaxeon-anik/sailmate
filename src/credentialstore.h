#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include <QObject>
#include <QStringList>
#include <Sailfish/Secrets/secretmanager.h>

class CredentialStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList savedProviders READ savedProviders NOTIFY savedProvidersChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
public:
    explicit CredentialStore(QObject *parent = 0);
    QStringList savedProviders() const { return m_saved; }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    Q_INVOKABLE void saveKey(const QString &provider, const QString &key);
    Q_INVOKABLE void loadKey(const QString &provider);
    Q_INVOKABLE void removeKey(const QString &provider);
signals:
    void savedProvidersChanged();
    void busyChanged();
    void errorChanged();
    void keySaved(QString provider);
    void keyLoaded(QString provider, QString key);
    void keyRemoved(QString provider);
    void keyMissing(QString provider);
private:
    bool begin(const QString &provider);
    void finish(const QString &error = QString());
    bool remember(const QString &provider, bool saved);
    Sailfish::Secrets::SecretManager m_manager;
    QStringList m_saved;
    bool m_busy;
    QString m_error;
};
#endif
