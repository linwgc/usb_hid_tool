#include "service/serial_number_service.h"

#include "core/race_command.h"
#include "service/race_service.h"

namespace race {

SerialNumberService::SerialNumberService(RaceService *raceService, QObject *parent)
    : QObject(parent), raceService_(raceService)
{
}

QString SerialNumberService::formatSerial(const QString &prefix, quint64 value, int width) const
{
    return QString("%1%2").arg(prefix).arg(value, width, 10, QChar('0'));
}

QByteArray SerialNumberService::buildCmdFromTemplate(const QString &hexTemplate, const QString &serial, QString &errMsg) const
{
    QString text = hexTemplate;
    text.replace("{SERIAL_ASCII}", QString(serial.toUtf8().toHex(' ')).toUpper());
    text.replace("{SERIAL_HEX}", serial);

    QByteArray cmd;
    if (!RaceCommand::parseHexString(text, cmd, errMsg)) {
        return {};
    }
    return cmd;
}

bool SerialNumberService::writeSerialByTemplate(const QString &hexTemplate, const QString &serial, quint8 target, QString &errMsg)
{
    const QByteArray cmd = buildCmdFromTemplate(hexTemplate, serial, errMsg);
    if (cmd.isEmpty() && !errMsg.isEmpty()) {
        return false;
    }
    QByteArray resp;
    if (!raceService_->requestResponse(cmd, target, resp, 2000, errMsg)) {
        return false;
    }
    if (!checkRaceGeneralSuccess(cmd, resp, errMsg)) {
        return false;
    }
    emit logLine("[SN] write serial done: " + serial);
    return true;
}

bool SerialNumberService::checkRaceGeneralSuccess(const QByteArray &requestCmd, const QByteArray &response, QString &errMsg) const
{
    if (response.size() < 7 || requestCmd.size() < 6) {
        errMsg = "RACE response/request length is too short.";
        return false;
    }
    if (static_cast<quint8>(response[0]) != 0x05 || static_cast<quint8>(response[1]) != 0x5B) {
        errMsg = "RACE response type is not 0x5B.";
        return false;
    }
    if (response[4] != requestCmd[4] || response[5] != requestCmd[5]) {
        errMsg = "RACE response ID mismatch.";
        return false;
    }
    const quint8 status = static_cast<quint8>(response[6]);
    if (status != 0x00) {
        errMsg = QString("RACE status fail: 0x%1").arg(status, 2, 16, QChar('0'));
        return false;
    }
    errMsg.clear();
    return true;
}

}  // namespace race
