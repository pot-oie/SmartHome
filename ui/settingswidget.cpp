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
    // 加载系统配置
    // 阻止信号以避免在初始化时触发 currentIndexChanged
    ui->cmbTheme->blockSignals(true);
    ui->cmbTheme->clear();
    ui->cmbTheme->addItems({"浅色主题", "深色主题", "自动"});
    ui->cmbTheme->setCurrentIndex(0);
    ui->cmbTheme->blockSignals(false);
}

void SettingsWidget::loadFakeDevices()
{
    ui->tableWidget_devices->setRowCount(0);

    // 假设备数据
    QVector<QStringList> devices = {
        {"light_living", "客厅主灯", "照明设备", "192.168.1.101", "在线"},
        {"light_bedroom", "卧室灯", "照明设备", "192.168.1.102", "在线"},
        {"ac_living", "客厅空调", "空调设备", "192.168.1.103", "在线"},
        {"curtain_living", "客厅窗帘", "窗帘设备", "192.168.1.104", "在线"},
        {"lock_door", "前门智能锁", "安防设备", "192.168.1.105", "在线"},
        {"camera_01", "客厅摄像头", "安防设备", "192.168.1.106", "离线"},
        {"tv_living", "客厅电视", "影音设备", "192.168.1.107", "在线"}};

    for (const auto &device : devices)
    {
        int row = ui->tableWidget_devices->rowCount();
        ui->tableWidget_devices->insertRow(row);

        for (int col = 0; col < device.size(); col++)
        {
            QTableWidgetItem *item = new QTableWidgetItem(device[col]);
            ui->tableWidget_devices->setItem(row, col, item);

            // 状态列设置颜色
            if (col == 4)
            {
                if (device[col] == "在线")
                {
                    item->setForeground(QBrush(QColor("#4CAF50")));
                }
                else
                {
                    item->setForeground(QBrush(QColor("#f44336")));
                }
            }
        }
    }
}

void SettingsWidget::on_cmbTheme_currentIndexChanged(int index)
{
    qDebug() << "切换主题：" << index;

    QString themeName;
    switch (index)
    {
    case 0:
        themeName = "light";
        break;
    case 1:
        themeName = "dark";
        break;
    case 2:
        themeName = "auto";
        break;
    }

    emit themeChanged(themeName);
    // 移除弹窗提示，主题切换在设置中静默进行
}

void SettingsWidget::on_btnBackupDatabase_clicked()
{
    qDebug() << "备份数据库";

    QString fileName = QFileDialog::getSaveFileName(this, "备份数据库",
                                                    QDir::homePath() + "/smarthome_backup.db",
                                                    "数据库文件 (*.db)");
    if (fileName.isEmpty())
    {
        return;
    }

    QMessageBox::information(this, "备份成功",
                             "数据库已备份到：\n" + fileName + "\n\n" +
                                 "（演示版本，实际未生成文件）");
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
        int row = ui->tableWidget_devices->rowCount();
        ui->tableWidget_devices->insertRow(row);

        QString deviceId = "device_" + QString::number(row + 100);
        ui->tableWidget_devices->setItem(row, 0, new QTableWidgetItem(deviceId));
        ui->tableWidget_devices->setItem(row, 1, new QTableWidgetItem(deviceName));
        ui->tableWidget_devices->setItem(row, 2, new QTableWidgetItem("新设备"));
        ui->tableWidget_devices->setItem(row, 3, new QTableWidgetItem("192.168.1." + QString::number(108 + row)));

        QTableWidgetItem *statusItem = new QTableWidgetItem("在线");
        statusItem->setForeground(QBrush(QColor("#4CAF50")));
        ui->tableWidget_devices->setItem(row, 4, statusItem);

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

        // 模拟ping测试
        int latency = 10 + rand() % 40; // 10-50ms

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
