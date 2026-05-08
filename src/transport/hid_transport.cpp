#include "transport/hid_transport.h"

#if __has_include(<hidapi/hidapi.h>)
#include <hidapi/hidapi.h>
#elif __has_include(<hidapi.h>)
#include <hidapi.h>
#else
#error "Cannot find hidapi header. Please configure include path in .pro (HIDAPI_ROOT)."
#endif

namespace race {

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
    hid_device_info *devs = hid_enumerate(vid, pid);
    const bool found = (devs != nullptr);
    hid_free_enumeration(devs);
    return found;
}

bool HidTransport::open(quint16 vid, quint16 pid, QString &errMsg)
{
    close();
    device_ = hid_open(vid, pid, nullptr);
    if (!device_) {
        errMsg = QString("Failed to open HID device VID:0x%1 PID:0x%2")
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
    if (device_) {
        hid_close(device_);
        device_ = nullptr;
    }
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
        errMsg = "hid_write failed.";
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
    unsigned char buffer[64] = {0};
    const int n = hid_read(device_, buffer, sizeof(buffer));
    if (n > 0) {
        emit reportReceived(QByteArray(reinterpret_cast<const char *>(buffer), n));
    } else if (n < 0) {
        emit transportError("hid_read failed.");
    }
}

}  // namespace race
