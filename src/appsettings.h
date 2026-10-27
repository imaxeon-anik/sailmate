#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QStandardPaths>
#include <QString>

// Sailjail only persists writes inside ~/.config/<organization>/<application>/.
// The default QSettings file (~/.config/<organization>/<application>.conf) sits
// outside it, so writes there succeed but vanish when the app exits.
inline QString settingsFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
            + QStringLiteral("/sailmate.conf");
}

#endif
