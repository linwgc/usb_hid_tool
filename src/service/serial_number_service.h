#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

namespace race {

class RaceService;

class SerialNumberService : public QObject {
    Q_OBJECT
public:
    explicit SerialNumberService(RaceService *raceService, QObject *parent = nullptr);

    QString formatSerial(const QString &prefix, quint64 value, int width) const;
    QByteArray buildCmdFromTemplate(const QString &hexTemplate, const QString &serial, QString &errMsg) const;
    bool writeSerialByTemplate(const QString &hexTemplate, const QString &serial, quint8 target, QString &errMsg);
    bool checkRaceGeneralSuccess(const QByteArray &requestCmd, const QByteArray &response, QString &errMsg) const;

signals:
    void logLine(const QString &line);

private:
    RaceService *raceService_ = nullptr;
};

}  // namespace race
