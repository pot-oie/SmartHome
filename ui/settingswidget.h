#pragma once

#include <QShowEvent>
#include <QResizeEvent>
#include <QWidget>
#include <QEvent>

#include "services/settingsservice.h"

namespace Ui
{
    class SettingsWidget;
}

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    ~SettingsWidget();
    void applyLanguage(const QString &languageKey);

signals:
    void themeChanged(const QString &themeName);
    void languageChanged(const QString &languageKey);
    void devicesChanged();

private slots:
    void on_cmbTheme_currentIndexChanged(int index);
    void on_comboBox_language_currentIndexChanged(int index);
    void on_btnBackupDatabase_clicked();
    void on_btnAddDevice_clicked();
    void on_btnDeleteDevice_clicked();
    void on_btnTestConnection_clicked();
    void onDevicesRefreshed(SettingsDeviceList devices);

private:
    Ui::SettingsWidget *ui;
    SettingsService m_settingsService;
    SettingsDeviceList m_devices;
    QString m_languageKey = QStringLiteral("zh_CN");

    void loadSystemSettings();
    void refreshStaticTexts();
    void reloadDevicesFromDatabase();
    void addDeviceRow(const SettingsDeviceEntry &device);
    void adjustDeviceTableForWidth();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};
