#include "ui/app_paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace {

bool tryCreateWritableTest(const QString &dirPath)
{
    if (dirPath.isEmpty()) {
        return false;
    }
    QDir d(dirPath);
    if (!d.exists() && !d.mkpath(QStringLiteral("."))) {
        return false;
    }
    const QString probe = d.filePath(QStringLiteral(".air_race_hid_write_probe"));
    QFile f(probe);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }
    f.close();
    f.remove();
    return true;
}

}  // namespace

QString appToolDataRoot()
{
    static QString root;
    if (!root.isEmpty()) {
        return root;
    }

    const QString exeDir = QCoreApplication::applicationDirPath();
    if (tryCreateWritableTest(exeDir)) {
        root = QDir(exeDir).absolutePath();
        return root;
    }

    const QString fallback = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!fallback.isEmpty() && tryCreateWritableTest(fallback)) {
        root = QDir(fallback).absolutePath();
        return root;
    }

    root = QDir(exeDir).absolutePath();
    return root;
}

QString appLogDir()
{
    QDir d(appToolDataRoot());
    d.mkpath(QStringLiteral("Log"));
    return d.filePath(QStringLiteral("Log"));
}

QString appDataDir()
{
    QDir d(appToolDataRoot());
    d.mkpath(QStringLiteral("Data"));
    return d.filePath(QStringLiteral("Data"));
}
