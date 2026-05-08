#include "core/race_packet.h"

namespace race {

QByteArray RacePacket::buildOutReport(const QByteArray &raceCmd, quint8 target, const HidReportConfig &cfg)
{
    QByteArray out(cfg.reportSize, '\0');
    out[0] = static_cast<char>(cfg.outReportId);
    const int payloadLen = qMin(raceCmd.size() + 1, cfg.reportSize - 3);  // target + data
    out[1] = static_cast<char>(payloadLen);
    out[2] = static_cast<char>(target);
    for (int i = 0; i < payloadLen - 1; ++i) {
        out[3 + i] = raceCmd[i];
    }
    return out;
}

bool RacePacket::parseInReport(const QByteArray &rawReport, QByteArray &racePayload, quint8 &target, const HidReportConfig &cfg, QString &errMsg)
{
    if (rawReport.size() < cfg.reportSize) {
        errMsg = "HID report length is too short.";
        return false;
    }
    if (static_cast<quint8>(rawReport[0]) != cfg.inReportId) {
        errMsg = "Unexpected report id.";
        return false;
    }
    const int validLen = static_cast<quint8>(rawReport[1]);
    if (validLen < 1 || validLen > (cfg.reportSize - 2)) {
        errMsg = "Invalid valid-length byte in HID report.";
        return false;
    }

    target = static_cast<quint8>(rawReport[2]);
    racePayload = rawReport.mid(3, validLen - 1);
    errMsg.clear();
    return true;
}

}  // namespace race
