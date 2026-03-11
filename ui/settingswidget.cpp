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
#include <QTableWidgetItem>

namespace
{
    const int kSettingsRefreshIntervalMs = 20000;

    QString displayTextForOnlineStatus(const QString &onlineStatus)
    {
        if (onlineStatus == QStringLiteral("online"))
        {
            return QStringLiteral("在线");
        }
        if (onlineStatus == QStringLiteral("offline"))
        {
            return QStringLiteral("离线");
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
    : QWidget(parent), ui(new Ui::SettingsWidget), m_refreshTimer(new QTimer(this))
{
    ui->setupUi(this);
    loadSystemSettings();

    ui->tableWidget_devices->setColumnCount(5);
    ui->tableWidget_devices->setHorizontalHeaderLabels(
        {QStringLiteral("设备ID"),
         QStringLiteral("设备名称"),
         QStringLiteral("设备类型"),
         QStringLiteral("IP地址"),
         QStringLiteral("状态")});
    ui->tableWidget_devices->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_devices->setAlternatingRowColors(true);
    ui->tableWidget_devices->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_devices->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(m_refreshTimer, &QTimer::timeout, this, &SettingsWidget::reloadDevicesFromDatabase);
    m_refreshTimer->start(kSettingsRefreshIntervalMs);

    reloadDevicesFromDatabase();
}

SettingsWidget::~SettingsWidget()
{
    delete ui;
}

void SettingsWidget::loadSystemSettings()
{
    ui->cmbTheme->blockSignals(true);
    ui->cmbTheme->clear();
    ui->cmbTheme->addItems(m_settingsService.themeOptions());
    ui->cmbTheme->setCurrentIndex(0);
    ui->cmbTheme->blockSignals(false);

    ui->comboBox_language->blockSignals(true);
    ui->comboBox_language->clear();
    ui->comboBox_language->addItems(m_settingsService.languageOptions());
    ui->comboBox_language->setCurrentIndex(0);
    ui->comboBox_language->blockSignals(false);
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
    m_devices = m_settingsService.loadDevices();
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
}

void SettingsWidget::addDeviceRow(const SettingsDeviceEntry &device)
{
    const int row = ui->tableWidget_devices->rowCount();
    ui->tableWidget_devices->insertRow(row);

    ui->tableWidget_devices->setItem(row, 0, new QTableWidgetItem(device.id));
    ui->tableWidget_devices->setItem(row, 1, new QTableWidgetItem(device.name));
    ui->tableWidget_devices->setItem(row, 2, new QTableWidgetItem(device.type));
    ui->tableWidget_devices->setItem(row, 3, new QTableWidgetItem(device.ip));

    QTableWidgetItem *statusItem = new QTableWidgetItem(displayTextForOnlineStatus(device.onlineStatus));
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
                              QStringLiteral("数据库备份失败：\n") + (errorText.isEmpty() ? QStringLiteral("未知错误") : errorText));
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
                              QStringLiteral("添加设备失败：\n") + (errorText.isEmpty() ? QStringLiteral("未知错误") : errorText));
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
                              QStringLiteral("删除设备失败：\n") + (errorText.isEmpty() ? QStringLiteral("未知错误") : errorText));
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
                                 QStringLiteral("目标服务：") + endpointText + QStringLiteral("\n") + QStringLiteral("连接成功\n") + QStringLiteral("延迟：") + QString::number(result.latencyMs) + QStringLiteral("ms"));
        return;
    }

    QMessageBox::warning(this,
                         QStringLiteral("连通性测试"),
                         QStringLiteral("目标服务：") + endpointText + QStringLiteral("\n") + QStringLiteral("连接失败\n") + QStringLiteral("原因：") + (result.errorText.isEmpty() ? QStringLiteral("未知错误") : result.errorText));
}

void SettingsWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    reloadDevicesFromDatabase();
}
