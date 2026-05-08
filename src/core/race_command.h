#pragma once

#include <QByteArray>
#include <QString>

namespace race {

class RaceCommand {
public:
    static bool parseHexString(const QString &text, QByteArray &out, QString &errMsg);
    static QString toHexString(const QByteArray &data);
};

}  // namespace race
