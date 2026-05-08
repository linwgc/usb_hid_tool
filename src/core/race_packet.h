#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

namespace race {

struct HidReportConfig {
    quint8 outReportId = 0x06;
    quint8 inReportId = 0x07;
    int reportSize = 62;
};

class RacePacket {
public:
    static QByteArray buildOutReport(const QByteArray &raceCmd, quint8 target, const HidReportConfig &cfg);
    static bool parseInReport(const QByteArray &rawReport, QByteArray &racePayload, quint8 &target, const HidReportConfig &cfg, QString &errMsg);
};

}  // namespace race
