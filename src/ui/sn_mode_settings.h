#pragma once

#include <QString>

struct SnModeSettings {
    QString snPrefix = QStringLiteral("SPK");
    QString snStart = QStringLiteral("1");
    QString snWidth = QStringLiteral("8");
    QString snPollMs = QStringLiteral("500");
    QString snTemplate;
    bool usePlant = true;
    bool useManufacturer = true;
    bool useProduct = true;
    bool useMonth = true;
    bool useYear = true;
    bool preventDuplicate = true;
    bool enablePrint = false;
    QString plantCode = QStringLiteral("HZ");
    QString manufacturerCode = QStringLiteral("A");
    QString productCode = QStringLiteral("0011");
    QString monthCode = QStringLiteral("A");
    QString yearCode = QStringLiteral("B");
    QString printerIp = QStringLiteral("192.168.1.100");
    QString printerPort = QStringLiteral("9100");
    QString printTemplate;

    static SnModeSettings defaults();
    static QString storageFilePath();
    bool loadFromFile(QString *errMsg = nullptr);
    bool saveToFile(QString *errMsg = nullptr) const;
};
