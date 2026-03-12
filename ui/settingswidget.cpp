#include "settingswidget.h"
#include "ui_settingswidget.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QHeaderView>
#include <QTableWidgetItem>

namespace
{
const int kSettingsRefreshIntervalMs = 20000;

QString displayTextForOnlineStatus(const QString &onlineStatus, const QString &languageKey)
{
    const bool isEnglish = (languageKey == QStringLiteral("en_US"));
    if (onlineStatus == QStringLiteral("online"))
    {
        return isEnglish ? QStringLiteral("Online") : QStringLiteral("在线");
    }
    if (onlineStatus == QStringLiteral("offline"))
    {
        return isEnglish ? QStringLiteral("Offline") : QStringLiteral("离线");
    }
    return onlineStatus;
}

QString displayColorForOnlineStatus(const QString &onlineStatus)
{
    if (onlineStatus == QStringLiteral("online"))
    {
        return QStringLiteral("#4CAF50");
    }
    if (onlineStatus == QStringLiteral("offline"))
    {
        return QStringLiteral("#f44336");
    }
    return QStringLiteral("#FF9800");
}
}

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWidget)
    , m_refreshTimer(new QTimer(this))
{
    ui->setupUi(this);
    refreshStaticTexts();
    loadSystemSettings();

    ui->tableWidget_devices->setColumnCount(5);
    ui->tableWidget_devices->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_devices->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_devices->setAlternatingRowColors(true);
    ui->tableWidget_devices->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_devices->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(m_refreshTimer, &QTimer::timeout, this, &SettingsWidget::reloadDevicesFromDatabase);
    m_refreshTimer->start(kSettingsRefreshIntervalMs);

    reloadDevicesFromDatabase();
    adjustDeviceTableForWidth();
}

SettingsWidget::~SettingsWidget()
{
    delete ui;
}

void SettingsWidget::loadSystemSettings()
{
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    const int currentThemeIndex = ui->cmbTheme->currentIndex();
    const int currentLanguageIndex = ui->comboBox_language->currentIndex();

    const QSignalBlocker themeBlocker(ui->cmbTheme);
    ui->cmbTheme->clear();
    if (isEnglish)
    {
        ui->cmbTheme->addItems({QStringLiteral("Light"), QStringLiteral("Dark"), QStringLiteral("Auto")});
    }
    else
    {
        ui->cmbTheme->addItems(m_settingsService.themeOptions());
    }
    ui->cmbTheme->setCurrentIndex(qBound(0, currentThemeIndex < 0 ? 0 : currentThemeIndex, ui->cmbTheme->count() - 1));

    const QSignalBlocker languageBlocker(ui->comboBox_language);
    ui->comboBox_language->clear();
    if (isEnglish)
    {
        ui->comboBox_language->addItems({QStringLiteral("Chinese (Simplified)"), QStringLiteral("English")});
    }
    else
    {
        ui->comboBox_language->addItems(m_settingsService.languageOptions());
    }

    int targetLanguageIndex = currentLanguageIndex;
    if (targetLanguageIndex < 0)
    {
        targetLanguageIndex = (m_languageKey == QStringLiteral("en_US")) ? 1 : 0;
    }
    ui->comboBox_language->setCurrentIndex(qBound(0, targetLanguageIndex, ui->comboBox_language->count() - 1));
}

void SettingsWidget::refreshStaticTexts()
{
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    if (isEnglish)
    {
        ui->groupBox_system->setTitle(QStringLiteral("System Settings"));
        ui->label_theme->setText(QStringLiteral("Theme:"));
        ui->label_language->setText(QStringLiteral("Language:"));
        ui->btnBackupDatabase->setText(QStringLiteral("Backup Database"));

        ui->groupBox_deviceManagement->setTitle(QStringLiteral("Device Management"));
        ui->btnAddDevice->setText(QStringLiteral("Add Device"));
        ui->btnDeleteDevice->setText(QStringLiteral("Delete Device"));
        ui->btnEditDevice->setText(QStringLiteral("Edit Device"));
        ui->btnTestConnection->setText(QStringLiteral("Test Connectivity"));

        ui->groupBox_about->setTitle(QStringLiteral("About"));
        ui->label_about->setText(QStringLiteral("Smart Home Platform v1.0\nTeam: SmartHome Team\nStack: Qt 6.10.2 + C++17"));
        ui->tableWidget_devices->setHorizontalHeaderLabels(
            {QStringLiteral("Device ID"),
             QStringLiteral("Device Name"),
             QStringLiteral("Type"),
             QStringLiteral("IP Address"),
             QStringLiteral("Status")});
    }
    else
    {
        ui->groupBox_system->setTitle(QStringLiteral("系统设置"));
        ui->label_theme->setText(QStringLiteral("界面主题:"));
        ui->label_language->setText(QStringLiteral("语言设置:"));
        ui->btnBackupDatabase->setText(QStringLiteral("一键备份数据库"));

        ui->groupBox_deviceManagement->setTitle(QStringLiteral("设备管理"));
        ui->btnAddDevice->setText(QStringLiteral("录入新设备"));
        ui->btnDeleteDevice->setText(QStringLiteral("注销设备"));
        ui->btnEditDevice->setText(QStringLiteral("编辑设备"));
        ui->btnTestConnection->setText(QStringLiteral("测试连通性"));

        ui->groupBox_about->setTitle(QStringLiteral("关于系统"));
        ui->label_about->setText(QStringLiteral("智能家居监控平台 v1.0\n开发团队：SmartHome Team\n技术栈：Qt 6.10.2 + C++17"));
        ui->tableWidget_devices->setHorizontalHeaderLabels(
            {QStringLiteral("设备ID"),
             QStringLiteral("设备名称"),
             QStringLiteral("设备类型"),
             QStringLiteral("IP地址"),
             QStringLiteral("状态")});
    }
}

void SettingsWidget::reloadDevicesFromDatabase()
{
    const int currentRow = ui->tableWidget_devices->currentRow();
    QString selectedDeviceId;
    if (currentRow >= 0 && ui->tableWidget_devices->item(currentRow, 0))
    {
        selectedDeviceId = ui->tableWidget_devices->item(currentRow, 0)->text();
    }

    ui->tableWidget_devices->setRowCount(0);
    m_devices = m_settingsService.loadDefaultDevices();
    for (const SettingsDeviceEntry &device : m_devices)
    {
        addDeviceRow(device);
    }

    if (!selectedDeviceId.isEmpty())
    {
        for (int row = 0; row < ui->tableWidget_devices->rowCount(); ++row)
        {
            QTableWidgetItem *idItem = ui->tableWidget_devices->item(row, 0);
            if (idItem && idItem->text() == selectedDeviceId)
            {
                ui->tableWidget_devices->setCurrentCell(row, 0);
                break;
            }
        }
    }

    adjustDeviceTableForWidth();
}

void SettingsWidget::adjustDeviceTableForWidth()
{
    if (!ui || !ui->tableWidget_devices)
    {
        return;
    }

    const int tableWidth = ui->tableWidget_devices->viewport()->width();
    if (tableWidth <= 0)
    {
        return;
    }

    // 窄宽度下优先保证设备名称和状态可读，其它列按优先级折叠。
    if (tableWidth < 430)
    {
        ui->tableWidget_devices->setColumnHidden(0, true);
        ui->tableWidget_devices->setColumnHidden(2, true);
        ui->tableWidget_devices->setColumnHidden(3, true);
    }
    else if (tableWidth < 620)
    {
        ui->tableWidget_devices->setColumnHidden(0, false);
        ui->tableWidget_devices->setColumnHidden(2, true);
        ui->tableWidget_devices->setColumnHidden(3, true);
    }
    else if (tableWidth < 760)
    {
        ui->tableWidget_devices->setColumnHidden(0, false);
        ui->tableWidget_devices->setColumnHidden(2, false);
        ui->tableWidget_devices->setColumnHidden(3, true);
    }
    else
    {
        ui->tableWidget_devices->setColumnHidden(0, false);
        ui->tableWidget_devices->setColumnHidden(2, false);
        ui->tableWidget_devices->setColumnHidden(3, false);
    }
}

void SettingsWidget::addDeviceRow(const SettingsDeviceEntry &device)
{
    const int row = ui->tableWidget_devices->rowCount();
    ui->tableWidget_devices->insertRow(row);

    ui->tableWidget_devices->setItem(row, 0, new QTableWidgetItem(device.id));
    ui->tableWidget_devices->setItem(row, 1, new QTableWidgetItem(device.name));
    ui->tableWidget_devices->setItem(row, 2, new QTableWidgetItem(device.type));
    ui->tableWidget_devices->setItem(row, 3, new QTableWidgetItem(device.ip));

    QTableWidgetItem *statusItem = new QTableWidgetItem(displayTextForOnlineStatus(device.onlineStatus, m_languageKey));
    statusItem->setForeground(QBrush(QColor(displayColorForOnlineStatus(device.onlineStatus))));
    ui->tableWidget_devices->setItem(row, 4, statusItem);
}

void SettingsWidget::on_cmbTheme_currentIndexChanged(int index)
{
    qDebug() << "切换主题:" << index;
    const QString themeName = m_settingsService.themeKeyByIndex(index);
    emit themeChanged(themeName);
}

void SettingsWidget::on_btnBackupDatabase_clicked()
{
    qDebug() << "备份数据库";

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          QStringLiteral("备份数据库"),
                                                          QDir::homePath() + QStringLiteral("/smarthome_backup.sql"),
                                                          QStringLiteral("SQL文件 (*.sql)"));
    if (fileName.isEmpty())
    {
        return;
    }

    QString errorText;
    if (!m_settingsService.backupDatabase(fileName, &errorText))
    {
        QMessageBox::critical(this,
                              QStringLiteral("备份失败"),
                              QStringLiteral("数据库备份失败：\n")
                                  + (errorText.isEmpty() ? QStringLiteral("未知错误") : errorText));
        return;
    }

    QMessageBox::information(this,
                             QStringLiteral("备份成功"),
                             QStringLiteral("数据库已备份到：\n") + fileName);
}

void SettingsWidget::on_comboBox_language_currentIndexChanged(int index)
{
    const QString languageKey = m_settingsService.languageKeyByIndex(index);
    emit languageChanged(languageKey);
}

void SettingsWidget::applyLanguage(const QString &languageKey)
{
    if (languageKey.trimmed().isEmpty())
    {
        return;
    }

    m_languageKey = languageKey;
    refreshStaticTexts();
    loadSystemSettings();
    reloadDevicesFromDatabase();
}

void SettingsWidget::on_btnAddDevice_clicked()
{
    qDebug() << "添加设备";

    bool ok = false;
    const QString deviceName = QInputDialog::getText(this,
                                                     QStringLiteral("添加设备"),
                                                     QStringLiteral("请输入设备名称："),
                                                     QLineEdit::Normal,
                                                     QString(),
                                                     &ok);
    if (!ok || deviceName.isEmpty())
    {
        return;
    }

    SettingsDeviceEntry newDevice;
    QString errorText;
    if (!m_settingsService.addDevice(deviceName, m_devices.size(), &newDevice, &errorText))
    {
        QMessageBox::critical(this,
                              QStringLiteral("失败"),
                              QStringLiteral("添加设备失败：\n")
                                  + (errorText.isEmpty() ? QStringLiteral("未知错误") : errorText));
        return;
    }

    reloadDevicesFromDatabase();
    emit devicesChanged();

    QMessageBox::information(this,
                             QStringLiteral("成功"),
                             QStringLiteral("设备 \"") + deviceName + QStringLiteral("\" 已添加！"));
}

void SettingsWidget::on_btnDeleteDevice_clicked()
{
    qDebug() << "删除设备";

    const int currentRow = ui->tableWidget_devices->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this,
                             QStringLiteral("提示"),
                             QStringLiteral("请先选择要删除的设备。"));
        return;
    }

    const QString deviceName = ui->tableWidget_devices->item(currentRow, 1)->text();
    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("确认删除"),
        QStringLiteral("确定要删除设备 \"") + deviceName + QStringLiteral("\" 吗？"),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
    {
        return;
    }

    const QString deviceId = ui->tableWidget_devices->item(currentRow, 0)->text();
    QString errorText;
    if (!m_settingsService.deleteDeviceById(deviceId, &errorText))
    {
        QMessageBox::critical(this,
                              QStringLiteral("失败"),
                              QStringLiteral("删除设备失败：\n")
                                  + (errorText.isEmpty() ? QStringLiteral("未知错误") : errorText));
        return;
    }

    reloadDevicesFromDatabase();
    emit devicesChanged();
    QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("设备已删除！"));
}

void SettingsWidget::on_btnTestConnection_clicked()
{
    qDebug() << "测试本地服务连通性";

    const TcpEndpointTestResult result = m_settingsService.testSmartHomeTcpEndpoint();
    const QString endpointText = result.host + QStringLiteral(":") + QString::number(result.port);

    if (result.reachable)
    {
        QMessageBox::information(this,
                                 QStringLiteral("连通性测试"),
                                 QStringLiteral("目标服务：") + endpointText + QStringLiteral("\n")
                                     + QStringLiteral("连接成功\n")
                                     + QStringLiteral("延迟：") + QString::number(result.latencyMs) + QStringLiteral("ms"));
        return;
    }

    QMessageBox::warning(this,
                         QStringLiteral("连通性测试"),
                         QStringLiteral("目标服务：") + endpointText + QStringLiteral("\n")
                             + QStringLiteral("连接失败\n")
                             + QStringLiteral("原因：")
                             + (result.errorText.isEmpty() ? QStringLiteral("未知错误") : result.errorText));
}

void SettingsWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    reloadDevicesFromDatabase();
    adjustDeviceTableForWidth();
}

void SettingsWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (event->type() == QEvent::LanguageChange)
    {
        refreshStaticTexts();
        loadSystemSettings();
        reloadDevicesFromDatabase();
    }
}

void SettingsWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    adjustDeviceTableForWidth();
}
