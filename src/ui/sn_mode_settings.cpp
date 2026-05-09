#include "ui/sn_mode_settings.h"
#include "ui/app_paths.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace {

QString jsonString(const QJsonObject &o, const char *key, const QString &fallback)
{
    const QJsonValue v = o.value(QString::fromUtf8(key));
    return v.isString() ? v.toString() : fallback;
}

bool jsonBool(const QJsonObject &o, const char *key, bool fallback)
{
    const QJsonValue v = o.value(QString::fromUtf8(key));
    return v.isBool() ? v.toBool() : fallback;
}

}  // namespace

SnModeSettings SnModeSettings::defaults()
{
    SnModeSettings s;
    s.snTemplate = QStringLiteral("05 5A 0B 00 07 1C 00 {SERIAL_ASCII}");
    s.printTemplate = QStringLiteral(
        "^XA\n^FO30,30^A0N,40,40^FD{SERIAL}^FS\n^FO30,90^BY2\n^BCN,80,Y,N,N^FD{SERIAL}^FS\n^FO30,190^A0N,28,28^FDMAC:{MAC}^FS\n^XZ");
    return s;
}

QString SnModeSettings::storageFilePath()
{
    return QDir(appDataDir()).filePath(QStringLiteral("sn_mode_settings.json"));
}

bool SnModeSettings::loadFromFile(QString *errMsg)
{
    const QString path = storageFilePath();
    QFile f(path);
    if (!f.exists()) {
        *this = defaults();
        if (errMsg) {
            errMsg->clear();
        }
        return true;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        if (errMsg) {
            *errMsg = QStringLiteral("Cannot read %1").arg(path);
        }
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) {
        if (errMsg) {
            *errMsg = QStringLiteral("Invalid JSON in sn_mode_settings.");
        }
        return false;
    }
    const QJsonObject o = doc.object();
    SnModeSettings d = defaults();
    d.snPrefix = jsonString(o, "snPrefix", d.snPrefix);
    d.snStart = jsonString(o, "snStart", d.snStart);
    d.snWidth = jsonString(o, "snWidth", d.snWidth);
    d.snPollMs = jsonString(o, "snPollMs", d.snPollMs);
    d.snTemplate = jsonString(o, "snTemplate", d.snTemplate);
    d.usePlant = jsonBool(o, "usePlant", d.usePlant);
    d.useManufacturer = jsonBool(o, "useManufacturer", d.useManufacturer);
    d.useProduct = jsonBool(o, "useProduct", d.useProduct);
    d.useMonth = jsonBool(o, "useMonth", d.useMonth);
    d.useYear = jsonBool(o, "useYear", d.useYear);
    d.preventDuplicate = jsonBool(o, "preventDuplicate", d.preventDuplicate);
    d.enablePrint = jsonBool(o, "enablePrint", d.enablePrint);
    d.plantCode = jsonString(o, "plantCode", d.plantCode);
    d.manufacturerCode = jsonString(o, "manufacturerCode", d.manufacturerCode);
    d.productCode = jsonString(o, "productCode", d.productCode);
    d.monthCode = jsonString(o, "monthCode", d.monthCode);
    d.yearCode = jsonString(o, "yearCode", d.yearCode);
    d.printerIp = jsonString(o, "printerIp", d.printerIp);
    d.printerPort = jsonString(o, "printerPort", d.printerPort);
    d.printTemplate = jsonString(o, "printTemplate", d.printTemplate);
    *this = d;
    if (errMsg) {
        errMsg->clear();
    }
    return true;
}

bool SnModeSettings::saveToFile(QString *errMsg) const
{
    const QString path = storageFilePath();
    QJsonObject o;
    o.insert(QStringLiteral("snPrefix"), snPrefix);
    o.insert(QStringLiteral("snStart"), snStart);
    o.insert(QStringLiteral("snWidth"), snWidth);
    o.insert(QStringLiteral("snPollMs"), snPollMs);
    o.insert(QStringLiteral("snTemplate"), snTemplate);
    o.insert(QStringLiteral("usePlant"), usePlant);
    o.insert(QStringLiteral("useManufacturer"), useManufacturer);
    o.insert(QStringLiteral("useProduct"), useProduct);
    o.insert(QStringLiteral("useMonth"), useMonth);
    o.insert(QStringLiteral("useYear"), useYear);
    o.insert(QStringLiteral("preventDuplicate"), preventDuplicate);
    o.insert(QStringLiteral("enablePrint"), enablePrint);
    o.insert(QStringLiteral("plantCode"), plantCode);
    o.insert(QStringLiteral("manufacturerCode"), manufacturerCode);
    o.insert(QStringLiteral("productCode"), productCode);
    o.insert(QStringLiteral("monthCode"), monthCode);
    o.insert(QStringLiteral("yearCode"), yearCode);
    o.insert(QStringLiteral("printerIp"), printerIp);
    o.insert(QStringLiteral("printerPort"), printerPort);
    o.insert(QStringLiteral("printTemplate"), printTemplate);

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errMsg) {
            *errMsg = QStringLiteral("Cannot write %1").arg(path);
        }
        return false;
    }
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    if (!f.commit()) {
        if (errMsg) {
            *errMsg = QStringLiteral("Commit failed: %1").arg(path);
        }
        return false;
    }
    if (errMsg) {
        errMsg->clear();
    }
    return true;
}
