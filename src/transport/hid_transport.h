#pragma once

#include <QByteArray>
#include <QObject>
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

signals:
    void reportReceived(const QByteArray &report);
    void transportError(const QString &err);

private slots:
    void pollRead();

private:
    hid_device_ *device_ = nullptr;
    QTimer pollTimer_;
};

}  // namespace race
