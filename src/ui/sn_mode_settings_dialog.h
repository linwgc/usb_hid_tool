#pragma once

#include "ui/sn_mode_settings.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;

class SnModeSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SnModeSettingsDialog(QWidget *parent = nullptr);

    void setSettings(const SnModeSettings &s);
    SnModeSettings settings() const;

private:
    void populateCombos();

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
    QCheckBox *enablePrintCheck_ = nullptr;
    QComboBox *plantCombo_ = nullptr;
    QComboBox *manufacturerCombo_ = nullptr;
    QComboBox *productCombo_ = nullptr;
    QComboBox *monthCombo_ = nullptr;
    QComboBox *yearCombo_ = nullptr;
    QLineEdit *printerIpEdit_ = nullptr;
    QLineEdit *printerPortEdit_ = nullptr;
    QPlainTextEdit *printTemplateEdit_ = nullptr;
};
