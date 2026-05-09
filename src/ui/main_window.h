#pragma once

#include "core/race_packet.h"
#include "ui/sn_mode_settings.h"

#include <QMainWindow>
#include <QTimer>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QLabel;
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
    void onSnSettingsClicked();

private:
    void setupUi();
    void appendLog(const QString &line);
    quint8 currentTarget() const;
    bool parseUsbConfig(quint16 &vid, quint16 &pid);
    bool prepareSerialConfig(quint64 &start, int &width);
    bool validateSerialRange(quint64 &start, int &width, QString *errMsg = nullptr);
    bool writeCurrentSerialOnce();
    QString buildSerialString(quint64 serialValue, int width, QString &errMsg) const;
    bool readMacAddress(QString &macString, QString &errMsg);
    bool appendCsvRecord(const QString &mac, const QString &serial, const QString &burnResult, const QString &printResult, QString &errMsg);
    bool isDuplicateRecord(const QString &mac, const QString &serial) const;
    bool printSerialLabel(const QString &serial, const QString &mac, QString &errMsg);
    QString buildPrintTemplate(const QString &serial, const QString &mac) const;
    QString currentLogFilePath() const;
    QString currentCsvFilePath() const;
    void updateSnModePreview();
    void setSnLastResult(bool ok, const QString &detail);

    race::HidTransport *transport_ = nullptr;
    race::RaceService *raceService_ = nullptr;
    race::SerialNumberService *serialService_ = nullptr;
    race::HidReportConfig cfg_;
    SnModeSettings snMode_;

    QLineEdit *vidEdit_ = nullptr;
    QLineEdit *pidEdit_ = nullptr;
    QLineEdit *outReportEdit_ = nullptr;
    QLineEdit *inReportEdit_ = nullptr;
    QComboBox *targetCombo_ = nullptr;
    QPlainTextEdit *singleCmdEdit_ = nullptr;
    QPlainTextEdit *sequenceEdit_ = nullptr;
    QPlainTextEdit *snPreviewEdit_ = nullptr;
    QLabel *snSummaryLabel_ = nullptr;
    QLabel *snLastResultLabel_ = nullptr;
    QTextEdit *logEdit_ = nullptr;

    quint64 nextSerial_ = 1;
    bool autoSnArmed_ = false;
    QTimer autoSnTimer_;
};
