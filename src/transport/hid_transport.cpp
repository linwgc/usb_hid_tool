#include "transport/hid_transport.h"

#if __has_include(<hidapi/hidapi.h>)
#include <hidapi/hidapi.h>
#elif __has_include(<hidapi.h>)
#include <hidapi.h>
#else
#error "Cannot find hidapi header. Please configure include path in .pro (HIDAPI_ROOT)."
#endif

#include <QtGlobal>

namespace race {

namespace {
/** Top-level usage page for RACE mux HID in Airoha usbhid_drv.c (report_mux_dscr). */
constexpr unsigned short kAirohaRaceHidUsagePage = 0xFF13;
}  // namespace

QString HidTransport::pickDevicePath(quint16 vid, quint16 pid)
{
    struct hid_device_info *devs = hid_enumerate(vid, pid);
    if (!devs) {
        return {};
    }
    struct hid_device_info *fallback = nullptr;
    for (struct hid_device_info *cur = devs; cur; cur = cur->next) {
        if (cur->vendor_id != vid || cur->product_id != pid) {
            continue;
        }
        if (cur->usage_page == kAirohaRaceHidUsagePage) {
            const QString path = QString::fromUtf8(cur->path);
            hid_free_enumeration(devs);
            return path;
        }
        if (!fallback) {
            fallback = cur;
        }
    }
    const QString path = fallback ? QString::fromUtf8(fallback->path) : QString();
    hid_free_enumeration(devs);
    return path;
}

HidTransport::HidTransport(QObject *parent)
    : QObject(parent)
{
    hid_init();
    pollTimer_.setInterval(20);
    connect(&pollTimer_, &QTimer::timeout, this, &HidTransport::pollRead);
}

HidTransport::~HidTransport()
{
    close();
    hid_exit();
}

bool HidTransport::deviceExists(quint16 vid, quint16 pid)
{
    return !pickDevicePath(vid, pid).isEmpty();
}

bool HidTransport::open(quint16 vid, quint16 pid, QString &errMsg)
{
    /* close() clears pollInputReportId_; preserve GET_REPORT polling configured before open(). */
    const quint8 savedPollInputReportId = pollInputReportId_;
    const int savedPollReadBufferSize = pollReadBufferSize_;
    close();
    pollInputReportId_ = savedPollInputReportId;
    pollReadBufferSize_ = savedPollReadBufferSize;

    const QString path = pickDevicePath(vid, pid);
    if (path.isEmpty()) {
        errMsg = QString("No HID interface found for VID:0x%1 PID:0x%2")
                     .arg(vid, 4, 16, QChar('0'))
                     .arg(pid, 4, 16, QChar('0'))
                     .toUpper();
        return false;
    }
    device_ = hid_open_path(path.toUtf8().constData());
    if (!device_) {
        errMsg = QString("Failed to open HID path for VID:0x%1 PID:0x%2")
                     .arg(vid, 4, 16, QChar('0'))
                     .arg(pid, 4, 16, QChar('0'))
                     .toUpper();
        return false;
    }
    hid_set_nonblocking(device_, 1);
    pollTimer_.start();
    errMsg.clear();
    return true;
}

void HidTransport::close()
{
    pollTimer_.stop();
    pollInputReportId_ = 0;
    if (device_) {
        hid_close(device_);
        device_ = nullptr;
    }
}

void HidTransport::configureInputPolling(quint8 reportId, int readBufferSize)
{
    pollInputReportId_ = reportId;
    pollReadBufferSize_ = qMax(64, readBufferSize);
}

bool HidTransport::getInputReport(quint8 reportId, QByteArray &report, QString &errMsg)
{
    if (!device_) {
        errMsg = QStringLiteral("HID device is not open.");
        return false;
    }
    QByteArray buf(pollReadBufferSize_, '\0');
    buf[0] = static_cast<char>(reportId);
    const int n = hid_get_input_report(device_, reinterpret_cast<unsigned char *>(buf.data()),
                                       static_cast<size_t>(buf.size()));
    if (n < 0) {
        const wchar_t *werr = hid_error(device_);
        errMsg = werr ? QStringLiteral("hid_get_input_report failed: %1").arg(QString::fromWCharArray(werr))
                      : QStringLiteral("hid_get_input_report failed.");
        return false;
    }
#if defined(Q_OS_LINUX)
    if (n == 0) {
        n = buf.size();
    }
#else
    if (n == 0) {
        errMsg.clear();
        report.clear();
        return false;
    }
#endif
    report = buf.left(n);
    errMsg.clear();
    return true;
}

bool HidTransport::isOpen() const
{
    return device_ != nullptr;
}

bool HidTransport::writeReport(const QByteArray &report, QString &errMsg)
{
    if (!device_) {
        errMsg = "HID device is not open.";
        return false;
    }
    const int wrote = hid_write(device_, reinterpret_cast<const unsigned char *>(report.constData()), report.size());
    if (wrote < 0) {
        const wchar_t *werr = hid_error(device_);
        errMsg = werr ? QStringLiteral("hid_write failed: %1").arg(QString::fromWCharArray(werr))
                      : QStringLiteral("hid_write failed.");
        return false;
    }
    errMsg.clear();
    return true;
}

void HidTransport::pollRead()
{
    if (!device_) {
        return;
    }
    if (pollInputReportId_ != 0) {
        QByteArray report;
        QString err;
        if (getInputReport(pollInputReportId_, report, err)) {
            emit reportReceived(report);
        } else if (!err.isEmpty()) {
            emit transportError(err);
        }
        return;
    }
    unsigned char buffer[64] = {0};
    const int n = hid_read(device_, buffer, sizeof(buffer));
    if (n > 0) {
        emit reportReceived(QByteArray(reinterpret_cast<const char *>(buffer), n));
    } else if (n < 0) {
        emit transportError("hid_read failed.");
    }
}

}  // namespace race
