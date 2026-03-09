#pragma once
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
    // 【向外发送】当用户切换暗黑/明亮主题时，通知 MainWindow 刷新全局 QSS 样式表
    void themeChanged(const QString &themeName);

private slots:
    // 【UI 交互】系统设置区
    void on_cmbTheme_currentIndexChanged(int index);
    void on_btnBackupDatabase_clicked(); // 执行 SQL 备份命令

    // 【UI 交互】设备管理区
    void on_btnAddDevice_clicked();
    void on_btnDeleteDevice_clicked();
    void on_btnTestConnection_clicked(); // 测试设备连通性

private:
    Ui::SettingsWidget *ui;
    SettingsService m_settingsService;
    SettingsDeviceList m_devices;

    void loadSystemSettings(); // 使用 QSettings 读取上次保存的配置
    void loadFakeDevices();    // 加载假设备数据
    void addDeviceRow(const SettingsDeviceEntry &device);
};
