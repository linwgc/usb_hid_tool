#pragma once

#include "core/race_packet.h"

#include <QByteArray>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QStringList>

namespace race {

class HidTransport;

class RaceService : public QObject {
    Q_OBJECT
public:
    explicit RaceService(HidTransport *transport, QObject *parent = nullptr);

    void setPacketConfig(const HidReportConfig &cfg);
    bool sendSingle(const QByteArray &raceCmd, quint8 target, QString &errMsg);
    bool sendSequence(const QStringList &hexLines, quint8 target, QString &errMsg);
    bool requestResponse(const QByteArray &raceCmd, quint8 target, QByteArray &responsePayload, int timeoutMs, QString &errMsg);

signals:
    void logLine(const QString &line);
    void racePacketReceived(const QByteArray &payload, quint8 target);

private slots:
    void onHidReportReceived(const QByteArray &report);

private:
    HidTransport *transport_ = nullptr;
    HidReportConfig cfg_;
};

}  // namespace race
