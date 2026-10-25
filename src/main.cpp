#include <sailfishapp.h>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <memory>
#include "chatcontroller.h"
#include "credentialstore.h"

int main(int argc, char *argv[])
{
    std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setOrganizationName(QStringLiteral("io.github.imaxeon-anik"));
    app->setApplicationName(QStringLiteral("Sailmate"));
    app->setApplicationVersion(QStringLiteral("0.1.0"));
    ChatController chat;
    CredentialStore vault;
    std::unique_ptr<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty(QStringLiteral("chat"), &chat);
    view->rootContext()->setContextProperty(QStringLiteral("vault"), &vault);
    view->setSource(QUrl(QStringLiteral("file:///usr/share/harbour-sailmate/qml/sailmate.qml")));
    if (view->status() == QQuickView::Error)
        return 1;
    view->showFullScreen();
    return app->exec();
}
