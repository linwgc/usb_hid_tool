#include "ui/main_window.h"

#include "core/race_command.h"
#include "service/race_service.h"
#include "service/serial_number_service.h"
#include "transport/hid_transport.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
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
#include <QRegularExpression>
#include <QTabWidget>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    transport_ = new race::HidTransport(this);
    raceService_ = new race::RaceService(transport_, this);
    serialService_ = new race::SerialNumberService(raceService_, this);

    setupUi();
    connect(raceService_, &race::RaceService::logLine, this, &MainWindow::appendLog);
    connect(serialService_, &race::SerialNumberService::logLine, this, &MainWindow::appendLog);
    connect(&autoSnTimer_, &QTimer::timeout, this, &MainWindow::onAutoSnTick);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    setWindowTitle("Airoha USB HID RACE Tool");
    resize(1100, 760);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);

    auto *cfgGroup = new QGroupBox("USB HID Config", central);
    auto *cfgLayout = new QFormLayout(cfgGroup);
    vidEdit_ = new QLineEdit("0x0E8D", cfgGroup);
    pidEdit_ = new QLineEdit("0x0003", cfgGroup);
    outReportEdit_ = new QLineEdit("0x06", cfgGroup);
    inReportEdit_ = new QLineEdit("0x07", cfgGroup);
    cfgLayout->addRow("VID", vidEdit_);
    cfgLayout->addRow("PID", pidEdit_);
    cfgLayout->addRow("OUT Report ID", outReportEdit_);
    cfgLayout->addRow("IN Report ID", inReportEdit_);

    auto *connLayout = new QHBoxLayout();
    auto *connectBtn = new QPushButton("Connect", cfgGroup);
    auto *disconnectBtn = new QPushButton("Disconnect", cfgGroup);
    connLayout->addWidget(connectBtn);
    connLayout->addWidget(disconnectBtn);
    cfgLayout->addRow(connLayout);
    mainLayout->addWidget(cfgGroup);

    auto *tabs = new QTabWidget(central);

    auto *normalPage = new QWidget(tabs);
    auto *normalLayout = new QVBoxLayout(normalPage);
    targetCombo_ = new QComboBox(normalPage);
    targetCombo_->addItem("Local(0x00)", 0x00);
    targetCombo_->addItem("Remote(0x80)", 0x80);
    normalLayout->addWidget(new QLabel("Target Device", normalPage));
    normalLayout->addWidget(targetCombo_);

    singleCmdEdit_ = new QPlainTextEdit(normalPage);
    singleCmdEdit_->setPlaceholderText("05 5A ...");
    normalLayout->addWidget(new QLabel("Single RACE CMD (hex bytes)", normalPage));
    normalLayout->addWidget(singleCmdEdit_);
    auto *sendSingleBtn = new QPushButton("Send Single", normalPage);
    normalLayout->addWidget(sendSingleBtn);

    sequenceEdit_ = new QPlainTextEdit(normalPage);
    sequenceEdit_->setPlaceholderText("# One command each line\n05 5A 03 00 ...\n05 5B 03 00 ...");
    normalLayout->addWidget(new QLabel("Sequence Mode (one command per line)", normalPage));
    normalLayout->addWidget(sequenceEdit_);
    auto *sendSeqBtn = new QPushButton("Send Sequence", normalPage);
    normalLayout->addWidget(sendSeqBtn);

    tabs->addTab(normalPage, "Normal Mode");

    auto *snPage = new QWidget(tabs);
    auto *snLayout = new QFormLayout(snPage);
    snPrefixEdit_ = new QLineEdit("SPK", snPage);
    snStartEdit_ = new QLineEdit("1", snPage);
    snWidthEdit_ = new QLineEdit("8", snPage);
    snPollMsEdit_ = new QLineEdit("500", snPage);
    snTemplateEdit_ = new QPlainTextEdit(snPage);
    snTemplateEdit_->setPlaceholderText("05 5A ... {SERIAL_ASCII}");

    usePlantCheck_ = new QCheckBox("Enable Plant", snPage);
    useManufacturerCheck_ = new QCheckBox("Enable Manufacturer", snPage);
    useProductCheck_ = new QCheckBox("Enable Product+Variant", snPage);
    useMonthCheck_ = new QCheckBox("Enable Month", snPage);
    useYearCheck_ = new QCheckBox("Enable Year", snPage);
    preventDuplicateCheck_ = new QCheckBox("Prevent duplicate burning (check CSV history)", snPage);
    usePlantCheck_->setChecked(true);
    useManufacturerCheck_->setChecked(true);
    useProductCheck_->setChecked(true);
    useMonthCheck_->setChecked(true);
    useYearCheck_->setChecked(true);
    preventDuplicateCheck_->setChecked(true);

    plantCombo_ = new QComboBox(snPage);
    plantCombo_->addItem("HZ - HuiZhou", "HZ");
    plantCombo_->addItem("DG - DongGuan", "DG");

    manufacturerCombo_ = new QComboBox(snPage);
    manufacturerCombo_->addItem("A - AUT", "A");
    manufacturerCombo_->addItem("H - Honsenn", "H");

    productCombo_ = new QComboBox(snPage);
    productCombo_->addItem("0011 - MIX SKYSCRAPER BLACK", "0011");
    productCombo_->addItem("0012 - MIX OLYMPIC WHITE", "0012");
    productCombo_->addItem("0013 - MIX SURF BLUE", "0013");
    productCombo_->addItem("0014 - MIX LILAC", "0014");
    productCombo_->addItem("0015 - MIX SAKURA PINK", "0015");
    productCombo_->addItem("0021 - ELIE6 SKYSCRAPER BLACK", "0021");
    productCombo_->addItem("0022 - ELIE6 OLYMPIC WHITE", "0022");
    productCombo_->addItem("0031 - ELIE12 SKYSCRAPER BLACK", "0031");
    productCombo_->addItem("0032 - ELIE12 OLYMPIC WHITE", "0032");
    productCombo_->addItem("0041 - TRACK 02 SKYSCRAPER BLACK", "0041");
    productCombo_->addItem("0042 - TRACK 02 OLYMPIC WHITE", "0042");
    productCombo_->addItem("0051 - TOUR 02 SKYSCRAPER BLACK", "0051");
    productCombo_->addItem("0052 - TOUR 02 OLYMPIC WHITE", "0052");

    monthCombo_ = new QComboBox(snPage);
    monthCombo_->addItem("A - January", "A");
    monthCombo_->addItem("B - February", "B");
    monthCombo_->addItem("C - March", "C");
    monthCombo_->addItem("D - April", "D");
    monthCombo_->addItem("E - May", "E");
    monthCombo_->addItem("F - June", "F");
    monthCombo_->addItem("G - July", "G");
    monthCombo_->addItem("H - August", "H");
    monthCombo_->addItem("I - September", "I");
    monthCombo_->addItem("J - October", "J");

    yearCombo_ = new QComboBox(snPage);
    yearCombo_->addItem("A - 2025", "A");
    yearCombo_->addItem("B - 2026", "B");
    yearCombo_->addItem("C - 2027", "C");
    yearCombo_->addItem("D - 2028", "D");
    yearCombo_->addItem("E - 2029", "E");
    yearCombo_->addItem("F - 2030", "F");

    snLayout->addRow("SN Prefix", snPrefixEdit_);
    snLayout->addRow(usePlantCheck_, plantCombo_);
    snLayout->addRow(useManufacturerCheck_, manufacturerCombo_);
    snLayout->addRow(useProductCheck_, productCombo_);
    snLayout->addRow("Start Number", snStartEdit_);
    snLayout->addRow("Number Width", snWidthEdit_);
    snLayout->addRow(useMonthCheck_, monthCombo_);
    snLayout->addRow(useYearCheck_, yearCombo_);
    snLayout->addRow("Auto Poll ms", snPollMsEdit_);
    snLayout->addRow(preventDuplicateCheck_);
    snLayout->addRow("RACE Template ({SERIAL_ASCII})", snTemplateEdit_);

    auto *manualBtnLayout = new QHBoxLayout();
    auto *writeNextBtn = new QPushButton("Manual Write Next SN", snPage);
    manualBtnLayout->addWidget(writeNextBtn);
    snLayout->addRow(manualBtnLayout);

    auto *autoBtnLayout = new QHBoxLayout();
    auto *startAutoBtn = new QPushButton("Start Auto Write", snPage);
    auto *stopAutoBtn = new QPushButton("Stop Auto Write", snPage);
    autoBtnLayout->addWidget(startAutoBtn);
    autoBtnLayout->addWidget(stopAutoBtn);
    snLayout->addRow(autoBtnLayout);
    tabs->addTab(snPage, "Serial Number Mode");

    mainLayout->addWidget(tabs);

    logEdit_ = new QTextEdit(central);
    logEdit_->setReadOnly(true);
    mainLayout->addWidget(new QLabel("Log", central));
    mainLayout->addWidget(logEdit_, 1);
    setCentralWidget(central);

    connect(connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(sendSingleBtn, &QPushButton::clicked, this, &MainWindow::onSendSingleClicked);
    connect(sendSeqBtn, &QPushButton::clicked, this, &MainWindow::onSendSequenceClicked);
    connect(writeNextBtn, &QPushButton::clicked, this, &MainWindow::onWriteNextSerialClicked);
    connect(startAutoBtn, &QPushButton::clicked, this, &MainWindow::onStartAutoSnClicked);
    connect(stopAutoBtn, &QPushButton::clicked, this, &MainWindow::onStopAutoSnClicked);
}

void MainWindow::appendLog(const QString &line)
{
    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    const QString full = QString("[%1] %2").arg(ts, line);
    logEdit_->append(full);

    QFile file(currentLogFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << full << "\n";
    }
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
        appendLog("[ERR] Invalid VID/PID/Report ID input.");
        return false;
    }
    raceService_->setPacketConfig(cfg_);
    return true;
}

bool MainWindow::prepareSerialConfig(quint64 &start, int &width)
{
    bool okStart = false;
    bool okWidth = false;
    start = snStartEdit_->text().toULongLong(&okStart);
    width = snWidthEdit_->text().toInt(&okWidth);
    if (!okStart || !okWidth || width <= 0) {
        appendLog("[ERR] Invalid serial setup.");
        return false;
    }
    if (nextSerial_ < start) {
        nextSerial_ = start;
    }
    return true;
}

bool MainWindow::writeCurrentSerialOnce()
{
    quint64 start = 0;
    int width = 0;
    if (!prepareSerialConfig(start, width)) {
        return false;
    }

    QString serialErr;
    const QString serial = buildSerialString(nextSerial_, width, serialErr);
    if (serial.isEmpty()) {
        appendLog("[ERR] " + serialErr);
        return false;
    }

    QString mac;
    QString err;
    if (!readMacAddress(mac, err)) {
        appendLog("[ERR] MAC read failed: " + err);
        appendCsvRecord("-", serial, "MAC_READ_FAIL", err);
        return false;
    }
    appendLog("[INFO] MAC read: " + mac);

    if (preventDuplicateCheck_->isChecked() && isDuplicateRecord(mac, serial)) {
        appendLog(QString("[WARN] Duplicate detected, skip write. mac=%1 serial=%2").arg(mac, serial));
        QString csvErr;
        appendCsvRecord(mac, serial, "DUPLICATE_SKIP", csvErr);
        return false;
    }

    QString writeErr;
    if (!serialService_->writeSerialByTemplate(snTemplateEdit_->toPlainText(), serial, currentTarget(), writeErr)) {
        appendLog("[ERR] " + writeErr);
        QString csvErr;
        appendCsvRecord(mac, serial, "WRITE_FAIL", csvErr);
        return false;
    }
    QString csvErr;
    if (!appendCsvRecord(mac, serial, "OK", csvErr)) {
        appendLog("[ERR] CSV write failed: " + csvErr);
    }
    appendLog(QString("[INFO] Write done. serial=%1 mac=%2").arg(serial, mac));
    ++nextSerial_;
    return true;
}

QString MainWindow::buildSerialString(quint64 serialValue, int width, QString &errMsg) const
{
    const QString serialNum = QString("%1").arg(serialValue, width, 10, QChar('0'));
    if (serialNum.size() != width) {
        errMsg = "Serial width overflow.";
        return {};
    }

    QString result = snPrefixEdit_->text().trimmed();
    if (usePlantCheck_->isChecked()) {
        result += plantCombo_->currentData().toString();
    }
    if (useManufacturerCheck_->isChecked()) {
        result += manufacturerCombo_->currentData().toString();
    }
    if (useProductCheck_->isChecked()) {
        result += productCombo_->currentData().toString();
    }
    result += serialNum;
    if (useMonthCheck_->isChecked()) {
        result += monthCombo_->currentData().toString();
    }
    if (useYearCheck_->isChecked()) {
        result += yearCombo_->currentData().toString();
    }
    errMsg.clear();
    return result;
}

bool MainWindow::readMacAddress(QString &macString, QString &errMsg)
{
    QByteArray cmd;
    if (!race::RaceCommand::parseHexString("05 5A 03 00 D5 0C 00", cmd, errMsg)) {
        return false;
    }

    QByteArray resp;
    if (!raceService_->requestResponse(cmd, currentTarget(), resp, 1500, errMsg)) {
        return false;
    }
    if (resp.size() < 14) {
        errMsg = "MAC response too short.";
        return false;
    }
    if (static_cast<quint8>(resp[0]) != 0x05 || static_cast<quint8>(resp[1]) != 0x5B) {
        errMsg = "Unexpected MAC response header.";
        return false;
    }
    if (static_cast<quint8>(resp[4]) != 0xD5 || static_cast<quint8>(resp[5]) != 0x0C) {
        errMsg = "Unexpected MAC response ID.";
        return false;
    }
    if (static_cast<quint8>(resp[6]) != 0x00) {
        errMsg = QString("MAC read status fail: 0x%1").arg(static_cast<quint8>(resp[6]), 2, 16, QChar('0'));
        return false;
    }

    const QByteArray macLe = resp.mid(8, 6);
    QStringList parts;
    for (int i = macLe.size() - 1; i >= 0; --i) {
        parts.append(QString("%1").arg(static_cast<quint8>(macLe[i]), 2, 16, QChar('0')).toUpper());
    }
    macString = parts.join(':');
    errMsg.clear();
    return true;
}

QString MainWindow::currentLogFilePath() const
{
    QDir dataDir(QCoreApplication::applicationDirPath());
    dataDir.mkpath("Data/logs");
    return dataDir.filePath("Data/logs/tool_" + QDateTime::currentDateTime().toString("yyyyMMdd") + ".log");
}

QString MainWindow::currentCsvFilePath() const
{
    QDir dataDir(QCoreApplication::applicationDirPath());
    dataDir.mkpath("Data/csv");

    const QString day = QDateTime::currentDateTime().toString("yyyyMMdd");
    const qint64 maxSize = 5 * 1024 * 1024;
    for (int idx = 1; idx < 1000; ++idx) {
        const QString name = QString("Data/csv/mac_sn_%1_%2.csv").arg(day).arg(idx, 3, 10, QChar('0'));
        const QString path = dataDir.filePath(name);
        QFileInfo fi(path);
        if (!fi.exists() || fi.size() < maxSize) {
            return path;
        }
    }
    return dataDir.filePath(QString("Data/csv/mac_sn_%1_999.csv").arg(day));
}

bool MainWindow::appendCsvRecord(const QString &mac, const QString &serial, const QString &result, QString &errMsg)
{
    const QString path = currentCsvFilePath();
    QFile file(path);
    const bool newFile = !QFileInfo::exists(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        errMsg = "Cannot open csv file: " + path;
        return false;
    }

    QTextStream out(&file);
    if (newFile) {
        out << "timestamp,mac,serial_number,result\n";
    }

    QString safeMac = mac;
    QString safeSn = serial;
    safeMac.replace(',', '_');
    safeSn.replace(',', '_');
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
        << "," << safeMac
        << "," << safeSn
        << "," << result
        << "\n";
    errMsg.clear();
    return true;
}

bool MainWindow::isDuplicateRecord(const QString &mac, const QString &serial) const
{
    QDir dataDir(QCoreApplication::applicationDirPath());
    dataDir.mkpath("Data/csv");
    QDir csvDir(dataDir.filePath("Data/csv"));
    const QStringList files = csvDir.entryList(QStringList() << "mac_sn_*.csv", QDir::Files, QDir::Name);

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
                if (line.startsWith("timestamp,mac,serial_number")) {
                    continue;
                }
            }
            const QStringList cols = line.split(',');
            if (cols.size() < 4) {
                continue;
            }
            const QString oldMac = cols[1].trimmed();
            const QString oldSn = cols[2].trimmed();
            const QString oldResult = cols[3].trimmed();
            if (oldResult == "OK" && (oldMac.compare(mac, Qt::CaseInsensitive) == 0 || oldSn == serial)) {
                return true;
            }
        }
    }
    return false;
}

void MainWindow::onConnectClicked()
{
    quint16 vid = 0;
    quint16 pid = 0;
    if (!parseUsbConfig(vid, pid)) {
        return;
    }

    QString err;
    if (!transport_->open(vid, pid, err)) {
        appendLog("[ERR] " + err);
        return;
    }
    appendLog(QString("[INFO] HID connected VID=0x%1 PID=0x%2")
                  .arg(vid, 4, 16, QChar('0'))
                  .arg(pid, 4, 16, QChar('0'))
                  .toUpper());
}

void MainWindow::onDisconnectClicked()
{
    transport_->close();
    appendLog("[INFO] HID disconnected.");
}

void MainWindow::onSendSingleClicked()
{
    QByteArray cmd;
    QString err;
    if (!race::RaceCommand::parseHexString(singleCmdEdit_->toPlainText(), cmd, err)) {
        appendLog("[ERR] " + err);
        return;
    }
    if (!raceService_->sendSingle(cmd, currentTarget(), err)) {
        appendLog("[ERR] " + err);
    }
}

void MainWindow::onSendSequenceClicked()
{
    QString err;
    const QStringList lines = sequenceEdit_->toPlainText().split('\n');
    if (!raceService_->sendSequence(lines, currentTarget(), err)) {
        appendLog("[ERR] " + err);
    } else {
        appendLog("[INFO] Sequence send completed.");
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
        QString err;
        if (!transport_->open(vid, pid, err)) {
            appendLog("[ERR] " + err);
            return;
        }
    }
    writeCurrentSerialOnce();
}

void MainWindow::onStartAutoSnClicked()
{
    if (transport_->isOpen()) {
        appendLog("[ERR] Disconnect manual HID session before auto SN mode.");
        return;
    }

    bool okPoll = false;
    const int pollMs = snPollMsEdit_->text().toInt(&okPoll);
    if (!okPoll || pollMs < 100) {
        appendLog("[ERR] Auto poll must be >= 100 ms.");
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
    appendLog(QString("[INFO] Auto SN started, polling every %1 ms.").arg(pollMs));
}

void MainWindow::onStopAutoSnClicked()
{
    autoSnTimer_.stop();
    autoSnArmed_ = false;
    if (transport_->isOpen()) {
        transport_->close();
    }
    appendLog("[INFO] Auto SN stopped.");
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
            appendLog("[INFO] Device removed, ready for next unit.");
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

    QString err;
    if (!transport_->open(vid, pid, err)) {
        appendLog("[ERR] Auto open failed: " + err);
        return;
    }

    if (writeCurrentSerialOnce()) {
        autoSnArmed_ = true;  // Write once for current plugged device.
        appendLog("[INFO] Auto write done, waiting for device removal.");
    }
    transport_->close();
}
