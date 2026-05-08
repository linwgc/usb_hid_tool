#pragma once

#include "core/race_packet.h"

#include <QMainWindow>
#include <QTimer>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTextEdit;

namespace race {
class HidTransport;
class RaceService;
class SerialNumberService;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onSendSingleClicked();
    void onSendSequenceClicked();
    void onWriteNextSerialClicked();
    void onStartAutoSnClicked();
    void onStopAutoSnClicked();
    void onAutoSnTick();

private:
    void setupUi();
    void appendLog(const QString &line);
    quint8 currentTarget() const;
    bool parseUsbConfig(quint16 &vid, quint16 &pid);
    bool prepareSerialConfig(quint64 &start, int &width);
    bool writeCurrentSerialOnce();
    QString buildSerialString(quint64 serialValue, int width, QString &errMsg) const;
    bool readMacAddress(QString &macString, QString &errMsg);
    bool appendCsvRecord(const QString &mac, const QString &serial, const QString &result, QString &errMsg);
    bool isDuplicateRecord(const QString &mac, const QString &serial) const;
    QString currentLogFilePath() const;
    QString currentCsvFilePath() const;

    race::HidTransport *transport_ = nullptr;
    race::RaceService *raceService_ = nullptr;
    race::SerialNumberService *serialService_ = nullptr;
    race::HidReportConfig cfg_;

    QLineEdit *vidEdit_ = nullptr;
    QLineEdit *pidEdit_ = nullptr;
    QLineEdit *outReportEdit_ = nullptr;
    QLineEdit *inReportEdit_ = nullptr;
    QComboBox *targetCombo_ = nullptr;
    QPlainTextEdit *singleCmdEdit_ = nullptr;
    QPlainTextEdit *sequenceEdit_ = nullptr;
    QLineEdit *snPrefixEdit_ = nullptr;
    QLineEdit *snStartEdit_ = nullptr;
    QLineEdit *snWidthEdit_ = nullptr;
    QLineEdit *snPollMsEdit_ = nullptr;
    QPlainTextEdit *snTemplateEdit_ = nullptr;
    QCheckBox *usePlantCheck_ = nullptr;
    QCheckBox *useManufacturerCheck_ = nullptr;
    QCheckBox *useProductCheck_ = nullptr;
    QCheckBox *useMonthCheck_ = nullptr;
    QCheckBox *useYearCheck_ = nullptr;
    QCheckBox *preventDuplicateCheck_ = nullptr;
    QComboBox *plantCombo_ = nullptr;
    QComboBox *manufacturerCombo_ = nullptr;
    QComboBox *productCombo_ = nullptr;
    QComboBox *monthCombo_ = nullptr;
    QComboBox *yearCombo_ = nullptr;
    QTextEdit *logEdit_ = nullptr;

    quint64 nextSerial_ = 1;
    bool autoSnArmed_ = false;
    QTimer autoSnTimer_;
};
