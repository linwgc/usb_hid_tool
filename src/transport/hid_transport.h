#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTimer>

struct hid_device_;

namespace race {

class HidTransport : public QObject {
    Q_OBJECT
public:
    explicit HidTransport(QObject *parent = nullptr);
    ~HidTransport() override;

    static bool deviceExists(quint16 vid, quint16 pid);

    bool open(quint16 vid, quint16 pid, QString &errMsg);
    void close();
    bool isOpen() const;
    bool writeReport(const QByteArray &report, QString &errMsg);

    /** Airoha Speaker HID returns IN data on GET_REPORT (EP0), not only interrupt IN. When @p reportId != 0, polling uses hid_get_input_report. */
    void configureInputPolling(quint8 reportId, int readBufferSize);
    bool getInputReport(quint8 reportId, QByteArray &report, QString &errMsg);

signals:
    void reportReceived(const QByteArray &report);
    void transportError(const QString &err);

private slots:
    void pollRead();

private:
    static QString pickDevicePath(quint16 vid, quint16 pid);

    hid_device_ *device_ = nullptr;
    QTimer pollTimer_;
    quint8 pollInputReportId_ = 0;
    int pollReadBufferSize_ = 64;
};

}  // namespace race
