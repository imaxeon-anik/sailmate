#include "credentialstore.h"
#include "providerprofile.h"
#include <QSettings>
#include <Sailfish/Secrets/storesecretrequest.h>
#include <Sailfish/Secrets/storedsecretrequest.h>
#include <Sailfish/Secrets/deletesecretrequest.h>

using namespace Sailfish::Secrets;

namespace {
Secret::Identifier identifier(const QString &provider)
{
    return Secret::Identifier(QStringLiteral("sailmate-api-") + provider, QString(),
                              SecretManager::DefaultStoragePluginName);
}
}

CredentialStore::CredentialStore(QObject *parent)
    : QObject(parent), m_manager(this), m_busy(false)
{
    const QStringList known = QSettings().value(QStringLiteral("credentials/providers")).toStringList();
    for (const QString &provider : known) {
        if (isProviderId(provider) && !m_saved.contains(provider))
            m_saved.append(provider);
    }
}

bool CredentialStore::begin(const QString &provider)
{
    if (m_busy)
        return false;
    if (!isProviderId(provider)) {
        finish(tr("Unknown provider."));
        return false;
    }
    m_error.clear();
    emit errorChanged();
    m_busy = true;
    emit busyChanged();
    return true;
}

void CredentialStore::finish(const QString &error)
{
    m_error = error;
    m_busy = false;
    emit errorChanged();
    emit busyChanged();
}

bool CredentialStore::remember(const QString &provider, bool saved)
{
    if (saved && !m_saved.contains(provider))
        m_saved.append(provider);
    else if (!saved)
        m_saved.removeAll(provider);
    // This index contains provider names only, never credentials.
    QSettings settings;
    settings.setValue(QStringLiteral("credentials/providers"), m_saved);
    settings.sync();
    emit savedProvidersChanged();
    return settings.status() == QSettings::NoError;
}

void CredentialStore::saveKey(const QString &provider, const QString &key)
{
    if (!begin(provider))
        return;
    if (key.trimmed().isEmpty() || key.size() > 4096) {
        finish(tr("Enter a valid API key."));
        return;
    }
    StoreSecretRequest *request = new StoreSecretRequest(this);
    request->setManager(&m_manager);
    request->setSecretStorageType(StoreSecretRequest::StandaloneDeviceLockSecret);
    request->setEncryptionPluginName(SecretManager::DefaultEncryptionPluginName);
    request->setDeviceLockUnlockSemantic(SecretManager::DeviceLockRelock);
    request->setAccessControlMode(SecretManager::OwnerOnlyMode);
    request->setUserInteractionMode(SecretManager::SystemInteraction);
    Secret secret(identifier(provider));
    secret.setData(key.trimmed().toUtf8());
    secret.setType(Secret::TypeBlob);
    request->setSecret(secret);
    connect(request, &Request::statusChanged, this, [this, request, provider]() {
        if (request->status() != Request::Finished)
            return;
        const bool success = request->result().code() == Result::Succeeded;
        request->setSecret(Secret());
        request->deleteLater();
        if (!success) {
            finish(tr("Could not save the key using Sailfish Secrets. Unlock the device and retry. No plaintext fallback is used."));
            return;
        }
        if (!remember(provider, true)) {
            finish(tr("The key is stored in Sailfish Secrets, but its settings index could not be saved. Free storage or fix settings access, then recover the saved key."));
            return;
        }
        finish();
        emit keySaved(provider);
    });
    request->startRequest();
}

void CredentialStore::loadKey(const QString &provider)
{
    if (!begin(provider))
        return;
    StoredSecretRequest *request = new StoredSecretRequest(this);
    request->setManager(&m_manager);
    request->setIdentifier(identifier(provider));
    request->setUserInteractionMode(SecretManager::SystemInteraction);
    connect(request, &Request::statusChanged, this, [this, request, provider]() {
        if (request->status() != Request::Finished)
            return;
        const Result result = request->result();
        const QString key = QString::fromUtf8(request->secret().data());
        request->deleteLater();
        if (result.code() != Result::Succeeded || key.isEmpty()) {
            if (result.errorCode() == Result::InvalidSecretError) {
                if (!remember(provider, false)) {
                    finish(tr("No saved key was found, and the settings index could not be updated."));
                    return;
                }
                finish();
                emit keyMissing(provider);
                return;
            }
            finish(tr("Could not read the saved key. Unlock the device and retry, or remove and reconnect the provider."));
            return;
        }
        if (!remember(provider, true)) {
            finish(tr("The saved key was found, but its settings index could not be saved. Check free storage and settings access, then retry."));
            return;
        }
        finish();
        emit keyLoaded(provider, key);
    });
    request->startRequest();
}

void CredentialStore::removeKey(const QString &provider)
{
    if (!begin(provider))
        return;
    DeleteSecretRequest *request = new DeleteSecretRequest(this);
    request->setManager(&m_manager);
    request->setIdentifier(identifier(provider));
    request->setUserInteractionMode(SecretManager::SystemInteraction);
    connect(request, &Request::statusChanged, this, [this, request, provider]() {
        if (request->status() != Request::Finished)
            return;
        const Result result = request->result();
        request->deleteLater();
        if (result.code() != Result::Succeeded && result.errorCode() != Result::InvalidSecretError) {
            finish(tr("Could not remove the saved key. Unlock the device and retry."));
            return;
        }
        if (!remember(provider, false)) {
            finish(tr("The key was removed, but its settings index could not be updated. Retry removal."));
            emit keyRemoved(provider);
            return;
        }
        finish();
        emit keyRemoved(provider);
    });
    request->startRequest();
}
