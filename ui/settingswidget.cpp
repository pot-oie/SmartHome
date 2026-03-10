#include "settingswidget.h"
#include "ui_settingswidget.h"

#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>

SettingsWidget::SettingsWidget(QWidget *parent) : QWidget(parent),
                                                  ui(new Ui::SettingsWidget)
{
    ui->setupUi(this);
    loadSystemSettings();

    // 初始化设备管理表格
    ui->tableWidget_devices->setColumnCount(5);
    ui->tableWidget_devices->setHorizontalHeaderLabels(
        {"设备ID", "设备名称", "设备类型", "IP地址", "状态"});
    ui->tableWidget_devices->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_devices->setAlternatingRowColors(true);
    ui->tableWidget_devices->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_devices->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 加载假设备数据
    loadFakeDevices();
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

void SettingsWidget::loadFakeDevices()
{
    ui->tableWidget_devices->setRowCount(0);
    m_devices = m_settingsService.loadDefaultDevices();
    for (const SettingsDeviceEntry &device : m_devices)
    {
        addDeviceRow(device);
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

    QTableWidgetItem *statusItem = new QTableWidgetItem(device.online ? "在线" : "离线");
    statusItem->setForeground(QBrush(QColor(device.online ? "#4CAF50" : "#f44336")));
    ui->tableWidget_devices->setItem(row, 4, statusItem);
}

void SettingsWidget::on_cmbTheme_currentIndexChanged(int index)
{
    qDebug() << "切换主题：" << index;
    const QString themeName = m_settingsService.themeKeyByIndex(index);
    emit themeChanged(themeName);
}

void SettingsWidget::on_btnBackupDatabase_clicked()
{
    qDebug() << "备份数据库";

    QString fileName = QFileDialog::getSaveFileName(this, "备份数据库",
                                                    QDir::homePath() + "/smarthome_backup.sql",
                                                    "SQL文件 (*.sql)");
    if (fileName.isEmpty())
    {
        return;
    }

    QString errorText;
    if (!m_settingsService.backupDatabase(fileName, &errorText))
    {
        QMessageBox::critical(this, "备份失败", "数据库备份失败：\n" + (errorText.isEmpty() ? "未知错误" : errorText));
        return;
    }

    QMessageBox::information(this, "备份成功",
                             "数据库已备份到：\n" + fileName);
}

void SettingsWidget::on_comboBox_language_currentIndexChanged(int index)
{
    const QString languageKey = m_settingsService.languageKeyByIndex(index);
    emit languageChanged(languageKey);
}

void SettingsWidget::on_btnAddDevice_clicked()
{
    qDebug() << "添加设备";

    bool ok;
    QString deviceName = QInputDialog::getText(this, "添加设备",
                                               "请输入设备名称：",
                                               QLineEdit::Normal, "", &ok);
    if (ok && !deviceName.isEmpty())
    {
        SettingsDeviceEntry newDevice;
        QString errorText;
        if (!m_settingsService.addDevice(deviceName, m_devices.size(), &newDevice, &errorText))
        {
            QMessageBox::critical(this, "失败", "添加设备失败：\n" + (errorText.isEmpty() ? "未知错误" : errorText));
            return;
        }

        m_devices.push_back(newDevice);
        addDeviceRow(newDevice);

        QMessageBox::information(this, "成功", "设备 \"" + deviceName + "\" 已添加！");
    }
}

void SettingsWidget::on_btnDeleteDevice_clicked()
{
    qDebug() << "删除设备";

    int currentRow = ui->tableWidget_devices->currentRow();
    if (currentRow >= 0)
    {
        QString deviceName = ui->tableWidget_devices->item(currentRow, 1)->text();
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "确认删除",
                                      "确定要删除设备 \"" + deviceName + "\" 吗？",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            const QString deviceId = ui->tableWidget_devices->item(currentRow, 0)->text();
            QString errorText;
            if (!m_settingsService.deleteDeviceById(deviceId, &errorText))
            {
                QMessageBox::critical(this, "失败", "删除设备失败：\n" + (errorText.isEmpty() ? "未知错误" : errorText));
                return;
            }

            if (currentRow < m_devices.size())
            {
                m_devices.removeAt(currentRow);
            }
            ui->tableWidget_devices->removeRow(currentRow);
            QMessageBox::information(this, "成功", "设备已删除！");
        }
    }
    else
    {
        QMessageBox::warning(this, "提示", "请先选择要删除的设备！");
    }
}

void SettingsWidget::on_btnTestConnection_clicked()
{
    qDebug() << "测试连接";

    int currentRow = ui->tableWidget_devices->currentRow();
    if (currentRow >= 0)
    {
        QString deviceName = ui->tableWidget_devices->item(currentRow, 1)->text();
        QString deviceIP = ui->tableWidget_devices->item(currentRow, 3)->text();

        const int latency = m_settingsService.mockLatencyMs();

        QMessageBox::information(this, "连接测试",
                                 "设备：" + deviceName + "\n" +
                                     "IP地址：" + deviceIP + "\n\n" +
                                     "连接成功\n" +
                                     "延迟：" + QString::number(latency) + "ms");
    }
    else
    {
        QMessageBox::warning(this, "提示", "请先选择要测试的设备！");
    }
}
