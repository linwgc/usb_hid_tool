#include "service/race_service.h"

#include "core/race_command.h"
#include "transport/hid_transport.h"

#include <QEventLoop>
#include <QTimer>

namespace race {

RaceService::RaceService(HidTransport *transport, QObject *parent)
    : QObject(parent), transport_(transport)
{
    connect(transport_, &HidTransport::reportReceived, this, &RaceService::onHidReportReceived);
    connect(transport_, &HidTransport::transportError, this, [this](const QString &err) {
        emit logLine("[HID][ERR] " + err);
    });
}

void RaceService::setPacketConfig(const HidReportConfig &cfg)
{
    cfg_ = cfg;
}

bool RaceService::sendSingle(const QByteArray &raceCmd, quint8 target, QString &errMsg)
{
    const QByteArray report = RacePacket::buildOutReport(raceCmd, target, cfg_);
    if (!transport_->writeReport(report, errMsg)) {
        return false;
    }
    emit logLine(QString("[TX] target=0x%1 cmd=%2")
                     .arg(target, 2, 16, QChar('0'))
                     .arg(RaceCommand::toHexString(raceCmd)));
    return true;
}

bool RaceService::sendSequence(const QStringList &hexLines, quint8 target, QString &errMsg)
{
    for (int i = 0; i < hexLines.size(); ++i) {
        const QString line = hexLines[i].trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        QByteArray cmd;
        if (!RaceCommand::parseHexString(line, cmd, errMsg)) {
            errMsg = QString("Line %1 parse failed: %2").arg(i + 1).arg(errMsg);
            return false;
        }
        if (!sendSingle(cmd, target, errMsg)) {
            errMsg = QString("Line %1 send failed: %2").arg(i + 1).arg(errMsg);
            return false;
        }
    }
    return true;
}

bool RaceService::requestResponse(const QByteArray &raceCmd, quint8 target, QByteArray &responsePayload, int timeoutMs, QString &errMsg)
{
    if (!sendSingle(raceCmd, target, errMsg)) {
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    responsePayload.clear();

    const auto rxConn = connect(this, &RaceService::racePacketReceived, &loop, [&](const QByteArray &payload, quint8) {
        responsePayload = payload;
        loop.quit();
    });
    const auto tmConn = connect(&timer, &QTimer::timeout, &loop, [&]() { loop.quit(); });

    timer.start(timeoutMs);
    loop.exec();
    disconnect(rxConn);
    disconnect(tmConn);

    if (responsePayload.isEmpty()) {
        errMsg = QString("RACE response timeout (%1 ms).").arg(timeoutMs);
        return false;
    }
    errMsg.clear();
    return true;
}

void RaceService::onHidReportReceived(const QByteArray &report)
{
    QByteArray payload;
    quint8 target = 0;
    QString err;
    if (!RacePacket::parseInReport(report, payload, target, cfg_, err)) {
        if (!err.isEmpty()) {
            emit logLine("[RX][ERR] " + err);
        }
        return;
    }
    emit logLine(QString("[RX] target=0x%1 payload=%2")
                     .arg(target, 2, 16, QChar('0'))
                     .arg(RaceCommand::toHexString(payload)));
    emit racePacketReceived(payload, target);
}

}  // namespace race
