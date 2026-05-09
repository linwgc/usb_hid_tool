#include "ui/main_window.h"
#include "ui/app_paths.h"
#include "ui/sn_mode_settings_dialog.h"

#include "core/race_command.h"
#include "service/race_service.h"
#include "service/serial_number_service.h"
#include "transport/hid_transport.h"

#include <QComboBox>
#include <QDialog>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include <QTextStream>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QString loadErr;
    if (!snMode_.loadFromFile(&loadErr)) {
        snMode_ = SnModeSettings::defaults();
    }
    bool okStart = false;
    const quint64 st = snMode_.snStart.toULongLong(&okStart);
    if (okStart) {
        nextSerial_ = st;
    }

    transport_ = new race::HidTransport(this);
    raceService_ = new race::RaceService(transport_, this);
    serialService_ = new race::SerialNumberService(raceService_, this);

    setupUi();
    connect(raceService_, &race::RaceService::logLine, this, &MainWindow::appendLog);
    connect(serialService_, &race::SerialNumberService::logLine, this, &MainWindow::appendLog);
    connect(&autoSnTimer_, &QTimer::timeout, this, &MainWindow::onAutoSnTick);

    if (!loadErr.isEmpty()) {
        appendLog(QStringLiteral("[WARN] SN settings: %1").arg(loadErr));
    }
    appendLog(QStringLiteral("[INFO] Data files root: %1 (subdirs Log, Data)").arg(appToolDataRoot()));
    updateSnModePreview();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Airoha USB HID RACE Tool"));
    resize(900, 720);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);

    auto *cfgGroup = new QGroupBox(QStringLiteral("USB HID Config"), central);
    auto *cfgLayout = new QFormLayout(cfgGroup);
    vidEdit_ = new QLineEdit(QStringLiteral("0x0E8D"), cfgGroup);
    pidEdit_ = new QLineEdit(QStringLiteral("0x0809"), cfgGroup);
    outReportEdit_ = new QLineEdit(QStringLiteral("0x06"), cfgGroup);
    inReportEdit_ = new QLineEdit(QStringLiteral("0x07"), cfgGroup);
    cfgLayout->addRow(QStringLiteral("VID"), vidEdit_);
    cfgLayout->addRow(QStringLiteral("PID"), pidEdit_);
    cfgLayout->addRow(QStringLiteral("OUT Report ID"), outReportEdit_);
    cfgLayout->addRow(QStringLiteral("IN Report ID"), inReportEdit_);

    auto *connLayout = new QHBoxLayout();
    auto *connectBtn = new QPushButton(QStringLiteral("Connect"), cfgGroup);
    auto *disconnectBtn = new QPushButton(QStringLiteral("Disconnect"), cfgGroup);
    connLayout->addWidget(connectBtn);
    connLayout->addWidget(disconnectBtn);
    cfgLayout->addRow(connLayout);
    mainLayout->addWidget(cfgGroup);

    auto *tabs = new QTabWidget(central);

    auto *normalPage = new QWidget(tabs);
    auto *normalLayout = new QVBoxLayout(normalPage);
    targetCombo_ = new QComboBox(normalPage);
    targetCombo_->addItem(QStringLiteral("Local(0x00)"), 0x00);
    targetCombo_->addItem(QStringLiteral("Remote(0x80)"), 0x80);
    normalLayout->addWidget(new QLabel(QStringLiteral("Target Device"), normalPage));
    normalLayout->addWidget(targetCombo_);

    singleCmdEdit_ = new QPlainTextEdit(normalPage);
    singleCmdEdit_->setPlaceholderText(QStringLiteral("05 5A ..."));
    normalLayout->addWidget(new QLabel(QStringLiteral("Single RACE CMD (hex bytes)"), normalPage));
    normalLayout->addWidget(singleCmdEdit_);
    auto *sendSingleBtn = new QPushButton(QStringLiteral("Send Single"), normalPage);
    normalLayout->addWidget(sendSingleBtn);

    sequenceEdit_ = new QPlainTextEdit(normalPage);
    sequenceEdit_->setPlaceholderText(
        QStringLiteral("# One command each line\n05 5A 03 00 ...\n05 5B 03 00 ..."));
    normalLayout->addWidget(new QLabel(QStringLiteral("Sequence Mode (one command per line)"), normalPage));
    normalLayout->addWidget(sequenceEdit_);
    auto *sendSeqBtn = new QPushButton(QStringLiteral("Send Sequence"), normalPage);
    normalLayout->addWidget(sendSeqBtn);

    tabs->addTab(normalPage, QStringLiteral("Normal Mode"));

    auto *snPage = new QWidget(tabs);
    auto *snLayout = new QVBoxLayout(snPage);

    auto *settingsRow = new QHBoxLayout();
    auto *snSettingsBtn = new QPushButton(QStringLiteral("Serial Number Settings…"), snPage);
    settingsRow->addWidget(snSettingsBtn);
    settingsRow->addStretch(1);
    snLayout->addLayout(settingsRow);

    snSummaryLabel_ = new QLabel(snPage);
    snSummaryLabel_->setWordWrap(true);
    snLayout->addWidget(snSummaryLabel_);

    snLayout->addWidget(new QLabel(QStringLiteral("Preview RACE command (hex)"), snPage));
    snPreviewEdit_ = new QPlainTextEdit(snPage);
    snPreviewEdit_->setReadOnly(true);
    snPreviewEdit_->setPlaceholderText(QStringLiteral("Configure settings and connect to preview."));
    snPreviewEdit_->setMinimumHeight(120);
    snLayout->addWidget(snPreviewEdit_, 1);

    snLastResultLabel_ = new QLabel(QStringLiteral("Last: (no operation yet)"), snPage);
    snLastResultLabel_->setWordWrap(true);
    snLayout->addWidget(snLastResultLabel_);

    auto *manualBtnLayout = new QHBoxLayout();
    auto *writeNextBtn = new QPushButton(QStringLiteral("Manual Write Next SN"), snPage);
    manualBtnLayout->addWidget(writeNextBtn);
    snLayout->addLayout(manualBtnLayout);

    auto *autoBtnLayout = new QHBoxLayout();
    auto *startAutoBtn = new QPushButton(QStringLiteral("Start Auto Write"), snPage);
    auto *stopAutoBtn = new QPushButton(QStringLiteral("Stop Auto Write"), snPage);
    autoBtnLayout->addWidget(startAutoBtn);
    autoBtnLayout->addWidget(stopAutoBtn);
    snLayout->addLayout(autoBtnLayout);

    tabs->addTab(snPage, QStringLiteral("Serial Number Mode"));

    mainLayout->addWidget(tabs);

    logEdit_ = new QTextEdit(central);
    logEdit_->setReadOnly(true);
    mainLayout->addWidget(new QLabel(QStringLiteral("Log"), central));
    mainLayout->addWidget(logEdit_, 1);
    setCentralWidget(central);

    connect(connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(sendSingleBtn, &QPushButton::clicked, this, &MainWindow::onSendSingleClicked);
    connect(sendSeqBtn, &QPushButton::clicked, this, &MainWindow::onSendSequenceClicked);
    connect(writeNextBtn, &QPushButton::clicked, this, &MainWindow::onWriteNextSerialClicked);
    connect(startAutoBtn, &QPushButton::clicked, this, &MainWindow::onStartAutoSnClicked);
    connect(stopAutoBtn, &QPushButton::clicked, this, &MainWindow::onStopAutoSnClicked);
    connect(snSettingsBtn, &QPushButton::clicked, this, &MainWindow::onSnSettingsClicked);
}

void MainWindow::appendLog(const QString &line)
{
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    const QString full = QStringLiteral("[%1] %2").arg(ts, line);
    logEdit_->append(full);

    const QString logPath = currentLogFilePath();
    QFile file(logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << full << "\n";
    } else {
        logEdit_->append(QStringLiteral("[%1] [ERR] Cannot write log file: %2")
                             .arg(ts, logPath));
    }
}

void MainWindow::onSnSettingsClicked()
{
    SnModeSettingsDialog dlg(this);
    dlg.setSettings(snMode_);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    snMode_ = dlg.settings();
    QString saveErr;
    if (!snMode_.saveToFile(&saveErr)) {
        appendLog(QStringLiteral("[ERR] Save SN settings: %1").arg(saveErr));
    } else {
        appendLog(QStringLiteral("[INFO] SN settings saved to %1").arg(SnModeSettings::storageFilePath()));
    }
    bool ok = false;
    const quint64 start = snMode_.snStart.toULongLong(&ok);
    if (ok && nextSerial_ < start) {
        nextSerial_ = start;
    }
    updateSnModePreview();
}

void MainWindow::updateSnModePreview()
{
    quint64 start = 0;
    int width = 0;
    QString err;
    if (!validateSerialRange(start, width, &err)) {
        snPreviewEdit_->setPlainText(err.isEmpty() ? QStringLiteral("Invalid start/width in settings.") : err);
        snSummaryLabel_->clear();
        return;
    }

    QString serErr;
    const QString serial = buildSerialString(nextSerial_, width, serErr);
    if (serial.isEmpty()) {
        snPreviewEdit_->setPlainText(serErr);
        snSummaryLabel_->clear();
        return;
    }

    QString hexErr;
    const QByteArray cmd = serialService_->buildCmdFromTemplate(snMode_.snTemplate, serial, hexErr);
    if (cmd.isEmpty()) {
        snPreviewEdit_->setPlainText(hexErr.isEmpty() ? QStringLiteral("(empty command)") : hexErr);
        snSummaryLabel_->setText(QStringLiteral("Next serial string: %1").arg(serial));
        return;
    }
    snPreviewEdit_->setPlainText(race::RaceCommand::toHexString(cmd));
    snSummaryLabel_->setText(QStringLiteral("Next serial string: %1").arg(serial));
}

void MainWindow::setSnLastResult(bool ok, const QString &detail)
{
    const QString prefix = ok ? QStringLiteral("Last: OK — ") : QStringLiteral("Last: FAIL — ");
    snLastResultLabel_->setText(prefix + detail);
}

quint8 MainWindow::currentTarget() const
{
    return static_cast<quint8>(targetCombo_->currentData().toUInt());
}

bool MainWindow::parseUsbConfig(quint16 &vid, quint16 &pid)
{
    bool okVid = false;
    bool okPid = false;
    bool okOut = false;
    bool okIn = false;

    vid = vidEdit_->text().toUShort(&okVid, 16);
    pid = pidEdit_->text().toUShort(&okPid, 16);
    cfg_.outReportId = static_cast<quint8>(outReportEdit_->text().toUInt(&okOut, 16));
    cfg_.inReportId = static_cast<quint8>(inReportEdit_->text().toUInt(&okIn, 16));

    if (!okVid || !okPid || !okOut || !okIn) {
        appendLog(QStringLiteral("[ERR] Invalid VID/PID/Report ID input."));
        return false;
    }
    raceService_->setPacketConfig(cfg_);
    return true;
}

bool MainWindow::validateSerialRange(quint64 &start, int &width, QString *errMsg)
{
    bool okStart = false;
    bool okWidth = false;
    start = snMode_.snStart.toULongLong(&okStart);
    width = snMode_.snWidth.toInt(&okWidth);
    if (!okStart || !okWidth || width <= 0) {
        if (errMsg) {
            *errMsg = QStringLiteral("Invalid start number or width.");
        }
        return false;
    }
    if (nextSerial_ < start) {
        nextSerial_ = start;
    }
    if (errMsg) {
        errMsg->clear();
    }
    return true;
}

bool MainWindow::prepareSerialConfig(quint64 &start, int &width)
{
    if (!validateSerialRange(start, width, nullptr)) {
        appendLog(QStringLiteral("[ERR] Invalid serial setup."));
        return false;
    }
    return true;
}

bool MainWindow::writeCurrentSerialOnce()
{
    quint64 start = 0;
    int width = 0;
    if (!prepareSerialConfig(start, width)) {
        setSnLastResult(false, QStringLiteral("invalid serial setup"));
        return false;
    }

    QString serialErr;
    const QString serial = buildSerialString(nextSerial_, width, serialErr);
    if (serial.isEmpty()) {
        appendLog(QStringLiteral("[ERR] %1").arg(serialErr));
        setSnLastResult(false, serialErr);
        return false;
    }

    QString mac;
    QString err;
    if (!readMacAddress(mac, err)) {
        appendLog(QStringLiteral("[ERR] MAC read failed: %1").arg(err));
        QString csvErr;
        appendCsvRecord(QStringLiteral("-"), serial, QStringLiteral("MAC_READ_FAIL"), QStringLiteral("NOT_RUN"), csvErr);
        setSnLastResult(false, QStringLiteral("MAC read: %1").arg(err));
        return false;
    }
    appendLog(QStringLiteral("[INFO] MAC read: %1").arg(mac));

    if (snMode_.preventDuplicate && isDuplicateRecord(mac, serial)) {
        appendLog(QStringLiteral("[WARN] Duplicate detected, skip write. mac=%1 serial=%2").arg(mac, serial));
        QString csvErr;
        appendCsvRecord(mac, serial, QStringLiteral("DUPLICATE_SKIP"), QStringLiteral("NOT_RUN"), csvErr);
        setSnLastResult(false, QStringLiteral("duplicate skipped"));
        return false;
    }

    QString writeErr;
    if (!serialService_->writeSerialByTemplate(snMode_.snTemplate, serial, currentTarget(), writeErr)) {
        appendLog(QStringLiteral("[ERR] %1").arg(writeErr));
        QString csvErr;
        appendCsvRecord(mac, serial, QStringLiteral("WRITE_FAIL"), QStringLiteral("NOT_RUN"), csvErr);
        setSnLastResult(false, writeErr);
        return false;
    }
    QString burnResult = QStringLiteral("OK");
    QString printResult = QStringLiteral("NOT_RUN");
    if (snMode_.enablePrint) {
        QString printErr;
        if (!printSerialLabel(serial, mac, printErr)) {
            appendLog(QStringLiteral("[ERR] Print failed: %1").arg(printErr));
            printResult = QStringLiteral("FAIL");
        } else {
            appendLog(QStringLiteral("[INFO] Label printed."));
            printResult = QStringLiteral("OK");
        }
    }

    QString csvErr;
    if (!appendCsvRecord(mac, serial, burnResult, printResult, csvErr)) {
        appendLog(QStringLiteral("[ERR] CSV write failed: %1").arg(csvErr));
    }
    appendLog(QStringLiteral("[INFO] Write done. serial=%1 mac=%2").arg(serial, mac));
    setSnLastResult(true, QStringLiteral("serial=%1 mac=%2").arg(serial, mac));
    ++nextSerial_;
    updateSnModePreview();
    return true;
}

QString MainWindow::buildSerialString(quint64 serialValue, int width, QString &errMsg) const
{
    const QString serialNum = QStringLiteral("%1").arg(serialValue, width, 10, QChar('0'));
    if (serialNum.size() != width) {
        errMsg = QStringLiteral("Serial width overflow.");
        return {};
    }

    QString result = snMode_.snPrefix.trimmed();
    if (snMode_.usePlant) {
        result += snMode_.plantCode;
    }
    if (snMode_.useManufacturer) {
        result += snMode_.manufacturerCode;
    }
    if (snMode_.useProduct) {
        result += snMode_.productCode;
    }
    result += serialNum;
    if (snMode_.useMonth) {
        result += snMode_.monthCode;
    }
    if (snMode_.useYear) {
        result += snMode_.yearCode;
    }
    errMsg.clear();
    return result;
}

bool MainWindow::readMacAddress(QString &macString, QString &errMsg)
{
    QByteArray cmd;
    if (!race::RaceCommand::parseHexString(QStringLiteral("05 5A 03 00 D5 0C 00"), cmd, errMsg)) {
        return false;
    }

    QByteArray resp;
    if (!raceService_->requestResponse(cmd, currentTarget(), resp, 1500, errMsg)) {
        return false;
    }
    if (resp.size() < 14) {
        errMsg = QStringLiteral("MAC response too short.");
        return false;
    }
    if (static_cast<quint8>(resp[0]) != 0x05 || static_cast<quint8>(resp[1]) != 0x5B) {
        errMsg = QStringLiteral("Unexpected MAC response header.");
        return false;
    }
    if (static_cast<quint8>(resp[4]) != 0xD5 || static_cast<quint8>(resp[5]) != 0x0C) {
        errMsg = QStringLiteral("Unexpected MAC response ID.");
        return false;
    }
    if (static_cast<quint8>(resp[6]) != 0x00) {
        errMsg = QStringLiteral("MAC read status fail: 0x%1")
                     .arg(static_cast<quint8>(resp[6]), 2, 16, QChar('0'));
        return false;
    }

    const QByteArray macLe = resp.mid(8, 6);
    QStringList parts;
    for (int i = macLe.size() - 1; i >= 0; --i) {
        parts.append(QStringLiteral("%1")
                         .arg(static_cast<quint8>(macLe[i]), 2, 16, QChar('0'))
                         .toUpper());
    }
    macString = parts.join(':');
    errMsg.clear();
    return true;
}

QString MainWindow::currentLogFilePath() const
{
    return QDir(appLogDir())
        .filePath(QStringLiteral("tool_%1.log")
                       .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd"))));
}

QString MainWindow::currentCsvFilePath() const
{
    QDir csvRoot(appDataDir());
    csvRoot.mkpath(QStringLiteral("csv"));
    const QString csvDirPath = csvRoot.filePath(QStringLiteral("csv"));

    const QString day = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd"));
    const qint64 maxSize = 5 * 1024 * 1024;
    for (int idx = 1; idx < 1000; ++idx) {
        const QString name =
            QStringLiteral("mac_sn_%1_%2.csv").arg(day).arg(idx, 3, 10, QChar('0'));
        const QString path = QDir(csvDirPath).filePath(name);
        QFileInfo fi(path);
        if (!fi.exists() || fi.size() < maxSize) {
            return path;
        }
    }
    return QDir(csvDirPath).filePath(QStringLiteral("mac_sn_%1_999.csv").arg(day));
}

bool MainWindow::appendCsvRecord(const QString &mac, const QString &serial, const QString &burnResult,
                                 const QString &printResult, QString &errMsg)
{
    const QString path = currentCsvFilePath();
    QFile file(path);
    const bool newFile = !QFileInfo::exists(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        errMsg = QStringLiteral("Cannot open csv file: %1").arg(path);
        return false;
    }

    QTextStream out(&file);
    if (newFile) {
        out << "timestamp,mac,serial_number,burn_result,print_result\n";
    }

    QString safeMac = mac;
    QString safeSn = serial;
    safeMac.replace(',', '_');
    safeSn.replace(',', '_');
    out << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) << "," << safeMac << ","
        << safeSn << "," << burnResult << "," << printResult << "\n";
    errMsg.clear();
    return true;
}

bool MainWindow::isDuplicateRecord(const QString &mac, const QString &serial) const
{
    const QString csvPath = QDir(appDataDir()).filePath(QStringLiteral("csv"));
    QDir().mkpath(csvPath);
    QDir csvDir(csvPath);
    const QStringList files = csvDir.entryList(QStringList() << QStringLiteral("mac_sn_*.csv"), QDir::Files,
                                               QDir::Name);

    for (const QString &name : files) {
        QFile file(csvDir.filePath(name));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        QTextStream in(&file);
        bool firstLine = true;
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }
            if (firstLine) {
                firstLine = false;
                if (line.startsWith(QStringLiteral("timestamp,mac,serial_number"))) {
                    continue;
                }
            }
            const QStringList cols = line.split(',');
            if (cols.size() < 5) {
                continue;
            }
            const QString oldMac = cols[1].trimmed();
            const QString oldSn = cols[2].trimmed();
            const QString oldBurnResult = cols[3].trimmed();
            if (oldBurnResult == QStringLiteral("OK") &&
                (oldMac.compare(mac, Qt::CaseInsensitive) == 0 || oldSn == serial)) {
                return true;
            }
        }
    }
    return false;
}

QString MainWindow::buildPrintTemplate(const QString &serial, const QString &mac) const
{
    QString tpl = snMode_.printTemplate;
    tpl.replace(QStringLiteral("{SERIAL}"), serial);
    tpl.replace(QStringLiteral("{MAC}"), mac);
    return tpl;
}

bool MainWindow::printSerialLabel(const QString &serial, const QString &mac, QString &errMsg)
{
    bool okPort = false;
    const int port = snMode_.printerPort.toInt(&okPort);
    if (!okPort || port <= 0 || port > 65535) {
        errMsg = QStringLiteral("Invalid printer port.");
        return false;
    }
    const QString ip = snMode_.printerIp.trimmed();
    if (ip.isEmpty()) {
        errMsg = QStringLiteral("Printer IP is empty.");
        return false;
    }

    QTcpSocket sock;
    sock.connectToHost(ip, static_cast<quint16>(port));
    if (!sock.waitForConnected(1500)) {
        errMsg = QStringLiteral("Printer connect timeout/fail.");
        return false;
    }

    QByteArray payload = buildPrintTemplate(serial, mac).toUtf8();
    if (!payload.endsWith('\n')) {
        payload.append('\n');
    }
    if (sock.write(payload) < 0 || !sock.waitForBytesWritten(1500)) {
        errMsg = QStringLiteral("Printer write failed.");
        sock.disconnectFromHost();
        return false;
    }

    sock.disconnectFromHost();
    errMsg.clear();
    return true;
}

void MainWindow::onConnectClicked()
{
    quint16 vid = 0;
    quint16 pid = 0;
    if (!parseUsbConfig(vid, pid)) {
        return;
    }

    transport_->configureInputPolling(cfg_.inReportId, cfg_.reportSize);
    QString err;
    if (!transport_->open(vid, pid, err)) {
        appendLog(QStringLiteral("[ERR] %1").arg(err));
        return;
    }
    appendLog(QStringLiteral("[INFO] HID connected VID=0x%1 PID=0x%2")
                  .arg(vid, 4, 16, QChar('0'))
                  .arg(pid, 4, 16, QChar('0'))
                  .toUpper());
}

void MainWindow::onDisconnectClicked()
{
    transport_->close();
    appendLog(QStringLiteral("[INFO] HID disconnected."));
}

void MainWindow::onSendSingleClicked()
{
    QByteArray cmd;
    QString err;
    if (!race::RaceCommand::parseHexString(singleCmdEdit_->toPlainText(), cmd, err)) {
        appendLog(QStringLiteral("[ERR] %1").arg(err));
        return;
    }
    if (!raceService_->sendSingle(cmd, currentTarget(), err)) {
        appendLog(QStringLiteral("[ERR] %1").arg(err));
    }
}

void MainWindow::onSendSequenceClicked()
{
    QString err;
    const QStringList lines = sequenceEdit_->toPlainText().split('\n');
    if (!raceService_->sendSequence(lines, currentTarget(), err)) {
        appendLog(QStringLiteral("[ERR] %1").arg(err));
    } else {
        appendLog(QStringLiteral("[INFO] Sequence send completed."));
    }
}

void MainWindow::onWriteNextSerialClicked()
{
    quint16 vid = 0;
    quint16 pid = 0;
    if (!parseUsbConfig(vid, pid)) {
        return;
    }
    if (!transport_->isOpen()) {
        transport_->configureInputPolling(cfg_.inReportId, cfg_.reportSize);
        QString err;
        if (!transport_->open(vid, pid, err)) {
            appendLog(QStringLiteral("[ERR] %1").arg(err));
            return;
        }
    }
    writeCurrentSerialOnce();
}

void MainWindow::onStartAutoSnClicked()
{
    if (transport_->isOpen()) {
        appendLog(QStringLiteral("[ERR] Disconnect manual HID session before auto SN mode."));
        return;
    }

    bool okPoll = false;
    const int pollMs = snMode_.snPollMs.toInt(&okPoll);
    if (!okPoll || pollMs < 100) {
        appendLog(QStringLiteral("[ERR] Auto poll must be >= 100 ms (check Settings)."));
        return;
    }

    quint16 vid = 0;
    quint16 pid = 0;
    if (!parseUsbConfig(vid, pid)) {
        return;
    }

    quint64 start = 0;
    int width = 0;
    if (!prepareSerialConfig(start, width)) {
        return;
    }

    autoSnArmed_ = false;
    autoSnTimer_.start(pollMs);
    appendLog(QStringLiteral("[INFO] Auto SN started, polling every %1 ms.").arg(pollMs));
}

void MainWindow::onStopAutoSnClicked()
{
    autoSnTimer_.stop();
    autoSnArmed_ = false;
    if (transport_->isOpen()) {
        transport_->close();
    }
    appendLog(QStringLiteral("[INFO] Auto SN stopped."));
}

void MainWindow::onAutoSnTick()
{
    quint16 vid = 0;
    quint16 pid = 0;
    if (!parseUsbConfig(vid, pid)) {
        onStopAutoSnClicked();
        return;
    }

    const bool present = race::HidTransport::deviceExists(vid, pid);
    if (!present) {
        if (autoSnArmed_) {
            appendLog(QStringLiteral("[INFO] Device removed, ready for next unit."));
            autoSnArmed_ = false;
        }
        if (transport_->isOpen()) {
            transport_->close();
        }
        return;
    }

    if (autoSnArmed_) {
        return;
    }

    transport_->configureInputPolling(cfg_.inReportId, cfg_.reportSize);
    QString err;
    if (!transport_->open(vid, pid, err)) {
        appendLog(QStringLiteral("[ERR] Auto open failed: %1").arg(err));
        return;
    }

    if (writeCurrentSerialOnce()) {
        autoSnArmed_ = true;
        appendLog(QStringLiteral("[INFO] Auto write done, waiting for device removal."));
    }
    transport_->close();
}
