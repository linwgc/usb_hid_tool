#include "ui/sn_mode_settings_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

void selectComboData(QComboBox *c, const QString &data)
{
    const int idx = c->findData(data);
    if (idx >= 0) {
        c->setCurrentIndex(idx);
    }
}

}  // namespace

SnModeSettingsDialog::SnModeSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Serial Number Mode — Settings"));
    resize(520, 640);

    auto *root = new QVBoxLayout(this);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto *content = new QWidget;
    scroll->setWidget(content);
    auto *form = new QFormLayout(content);

    snPrefixEdit_ = new QLineEdit(content);
    snStartEdit_ = new QLineEdit(content);
    snWidthEdit_ = new QLineEdit(content);
    snPollMsEdit_ = new QLineEdit(content);
    snTemplateEdit_ = new QPlainTextEdit(content);
    snTemplateEdit_->setPlaceholderText(QStringLiteral("05 5A ... {SERIAL_ASCII}"));
    snTemplateEdit_->setMinimumHeight(72);

    usePlantCheck_ = new QCheckBox(QStringLiteral("Enable Plant"), content);
    useManufacturerCheck_ = new QCheckBox(QStringLiteral("Enable Manufacturer"), content);
    useProductCheck_ = new QCheckBox(QStringLiteral("Enable Product+Variant"), content);
    useMonthCheck_ = new QCheckBox(QStringLiteral("Enable Month"), content);
    useYearCheck_ = new QCheckBox(QStringLiteral("Enable Year"), content);
    preventDuplicateCheck_ = new QCheckBox(QStringLiteral("Prevent duplicate burning (CSV history)"), content);
    enablePrintCheck_ = new QCheckBox(QStringLiteral("Print label after successful write"), content);

    plantCombo_ = new QComboBox(content);
    manufacturerCombo_ = new QComboBox(content);
    productCombo_ = new QComboBox(content);
    monthCombo_ = new QComboBox(content);
    yearCombo_ = new QComboBox(content);
    populateCombos();

    printerIpEdit_ = new QLineEdit(content);
    printerPortEdit_ = new QLineEdit(content);
    printTemplateEdit_ = new QPlainTextEdit(content);
    printTemplateEdit_->setPlaceholderText(
        QStringLiteral("^XA\n^FO30,30^A0N,40,40^FD{SERIAL}^FS\n..."));
    printTemplateEdit_->setMinimumHeight(100);

    form->addRow(QStringLiteral("SN Prefix"), snPrefixEdit_);
    form->addRow(usePlantCheck_, plantCombo_);
    form->addRow(useManufacturerCheck_, manufacturerCombo_);
    form->addRow(useProductCheck_, productCombo_);
    form->addRow(QStringLiteral("Start Number"), snStartEdit_);
    form->addRow(QStringLiteral("Number Width"), snWidthEdit_);
    form->addRow(useMonthCheck_, monthCombo_);
    form->addRow(useYearCheck_, yearCombo_);
    form->addRow(QStringLiteral("Auto poll (ms)"), snPollMsEdit_);
    form->addRow(preventDuplicateCheck_);
    form->addRow(enablePrintCheck_);
    form->addRow(QStringLiteral("Printer IP"), printerIpEdit_);
    form->addRow(QStringLiteral("Printer Port"), printerPortEdit_);
    form->addRow(QStringLiteral("Print template (ZPL)"), printTemplateEdit_);
    form->addRow(QStringLiteral("RACE template ({SERIAL_ASCII})"), snTemplateEdit_);

    root->addWidget(scroll);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

void SnModeSettingsDialog::populateCombos()
{
    plantCombo_->clear();
    plantCombo_->addItem(QStringLiteral("HZ - HuiZhou"), QStringLiteral("HZ"));
    plantCombo_->addItem(QStringLiteral("DG - DongGuan"), QStringLiteral("DG"));

    manufacturerCombo_->clear();
    manufacturerCombo_->addItem(QStringLiteral("A - AUT"), QStringLiteral("A"));
    manufacturerCombo_->addItem(QStringLiteral("H - Honsenn"), QStringLiteral("H"));

    productCombo_->clear();
    productCombo_->addItem(QStringLiteral("0011 - MIX SKYSCRAPER BLACK"), QStringLiteral("0011"));
    productCombo_->addItem(QStringLiteral("0012 - MIX OLYMPIC WHITE"), QStringLiteral("0012"));
    productCombo_->addItem(QStringLiteral("0013 - MIX SURF BLUE"), QStringLiteral("0013"));
    productCombo_->addItem(QStringLiteral("0014 - MIX LILAC"), QStringLiteral("0014"));
    productCombo_->addItem(QStringLiteral("0015 - MIX SAKURA PINK"), QStringLiteral("0015"));
    productCombo_->addItem(QStringLiteral("0021 - ELIE6 SKYSCRAPER BLACK"), QStringLiteral("0021"));
    productCombo_->addItem(QStringLiteral("0022 - ELIE6 OLYMPIC WHITE"), QStringLiteral("0022"));
    productCombo_->addItem(QStringLiteral("0031 - ELIE12 SKYSCRAPER BLACK"), QStringLiteral("0031"));
    productCombo_->addItem(QStringLiteral("0032 - ELIE12 OLYMPIC WHITE"), QStringLiteral("0032"));
    productCombo_->addItem(QStringLiteral("0041 - TRACK 02 SKYSCRAPER BLACK"), QStringLiteral("0041"));
    productCombo_->addItem(QStringLiteral("0042 - TRACK 02 OLYMPIC WHITE"), QStringLiteral("0042"));
    productCombo_->addItem(QStringLiteral("0051 - TOUR 02 SKYSCRAPER BLACK"), QStringLiteral("0051"));
    productCombo_->addItem(QStringLiteral("0052 - TOUR 02 OLYMPIC WHITE"), QStringLiteral("0052"));

    monthCombo_->clear();
    monthCombo_->addItem(QStringLiteral("A - January"), QStringLiteral("A"));
    monthCombo_->addItem(QStringLiteral("B - February"), QStringLiteral("B"));
    monthCombo_->addItem(QStringLiteral("C - March"), QStringLiteral("C"));
    monthCombo_->addItem(QStringLiteral("D - April"), QStringLiteral("D"));
    monthCombo_->addItem(QStringLiteral("E - May"), QStringLiteral("E"));
    monthCombo_->addItem(QStringLiteral("F - June"), QStringLiteral("F"));
    monthCombo_->addItem(QStringLiteral("G - July"), QStringLiteral("G"));
    monthCombo_->addItem(QStringLiteral("H - August"), QStringLiteral("H"));
    monthCombo_->addItem(QStringLiteral("I - September"), QStringLiteral("I"));
    monthCombo_->addItem(QStringLiteral("J - October"), QStringLiteral("J"));

    yearCombo_->clear();
    yearCombo_->addItem(QStringLiteral("A - 2025"), QStringLiteral("A"));
    yearCombo_->addItem(QStringLiteral("B - 2026"), QStringLiteral("B"));
    yearCombo_->addItem(QStringLiteral("C - 2027"), QStringLiteral("C"));
    yearCombo_->addItem(QStringLiteral("D - 2028"), QStringLiteral("D"));
    yearCombo_->addItem(QStringLiteral("E - 2029"), QStringLiteral("E"));
    yearCombo_->addItem(QStringLiteral("F - 2030"), QStringLiteral("F"));
}

void SnModeSettingsDialog::setSettings(const SnModeSettings &s)
{
    snPrefixEdit_->setText(s.snPrefix);
    snStartEdit_->setText(s.snStart);
    snWidthEdit_->setText(s.snWidth);
    snPollMsEdit_->setText(s.snPollMs);
    snTemplateEdit_->setPlainText(s.snTemplate);

    usePlantCheck_->setChecked(s.usePlant);
    useManufacturerCheck_->setChecked(s.useManufacturer);
    useProductCheck_->setChecked(s.useProduct);
    useMonthCheck_->setChecked(s.useMonth);
    useYearCheck_->setChecked(s.useYear);
    preventDuplicateCheck_->setChecked(s.preventDuplicate);
    enablePrintCheck_->setChecked(s.enablePrint);

    selectComboData(plantCombo_, s.plantCode);
    selectComboData(manufacturerCombo_, s.manufacturerCode);
    selectComboData(productCombo_, s.productCode);
    selectComboData(monthCombo_, s.monthCode);
    selectComboData(yearCombo_, s.yearCode);

    printerIpEdit_->setText(s.printerIp);
    printerPortEdit_->setText(s.printerPort);
    printTemplateEdit_->setPlainText(s.printTemplate);
}

SnModeSettings SnModeSettingsDialog::settings() const
{
    SnModeSettings s;
    s.snPrefix = snPrefixEdit_->text();
    s.snStart = snStartEdit_->text();
    s.snWidth = snWidthEdit_->text();
    s.snPollMs = snPollMsEdit_->text();
    s.snTemplate = snTemplateEdit_->toPlainText();
    s.usePlant = usePlantCheck_->isChecked();
    s.useManufacturer = useManufacturerCheck_->isChecked();
    s.useProduct = useProductCheck_->isChecked();
    s.useMonth = useMonthCheck_->isChecked();
    s.useYear = useYearCheck_->isChecked();
    s.preventDuplicate = preventDuplicateCheck_->isChecked();
    s.enablePrint = enablePrintCheck_->isChecked();
    s.plantCode = plantCombo_->currentData().toString();
    s.manufacturerCode = manufacturerCombo_->currentData().toString();
    s.productCode = productCombo_->currentData().toString();
    s.monthCode = monthCombo_->currentData().toString();
    s.yearCode = yearCombo_->currentData().toString();
    s.printerIp = printerIpEdit_->text();
    s.printerPort = printerPortEdit_->text();
    s.printTemplate = printTemplateEdit_->toPlainText();
    return s;
}
