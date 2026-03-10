#pragma once

#include <QShowEvent>
#include <QTimer>
#include <QWidget>

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

private:
    Ui::SettingsWidget *ui;
    SettingsService m_settingsService;
    SettingsDeviceList m_devices;
    QTimer *m_refreshTimer = nullptr;

    void loadSystemSettings();
    void reloadDevicesFromDatabase();
    void addDeviceRow(const SettingsDeviceEntry &device);

protected:
    void showEvent(QShowEvent *event) override;
};
