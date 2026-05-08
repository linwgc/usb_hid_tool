#include "core/race_command.h"

#include <QRegularExpression>
#include <QStringList>

namespace race {

bool RaceCommand::parseHexString(const QString &text, QByteArray &out, QString &errMsg)
{
    const QString cleaned = text.trimmed().replace(QRegularExpression("[,;\\s]+"), " ");
    if (cleaned.isEmpty()) {
        errMsg = "Empty race command.";
        return false;
    }

    const QStringList parts = cleaned.split(' ', Qt::SkipEmptyParts);
    QByteArray bytes;
    bytes.reserve(parts.size());
    for (const QString &token : parts) {
        bool ok = false;
        int value = token.toInt(&ok, 16);
        if (!ok || value < 0 || value > 0xFF) {
            errMsg = QString("Invalid byte: %1").arg(token);
            return false;
        }
        bytes.append(static_cast<char>(value));
    }
    out = bytes;
    errMsg.clear();
    return true;
}

QString RaceCommand::toHexString(const QByteArray &data)
{
    QStringList out;
    out.reserve(data.size());
    for (unsigned char b : data) {
        out.append(QString("%1").arg(b, 2, 16, QChar('0')).toUpper());
    }
    return out.join(' ');
}

}  // namespace race
