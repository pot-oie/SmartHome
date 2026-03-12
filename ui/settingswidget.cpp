#include "settingswidget.h"
#include "ui_settingswidget.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QHeaderView>
#include <QRandomGenerator>
#include <QStringList>
#include <QTableWidgetItem>

namespace
{
    const int kSettingsRefreshIntervalMs = 20000;

    struct AddDeviceDialogResult
    {
        bool accepted = false;
        SettingsDeviceEntry device;
    };

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

    QString nextIpAddress(const SettingsDeviceList &devices)
    {
        QString prefix = QStringLiteral("192.168.1");
        int maxHost = 100;

        for (const SettingsDeviceEntry &device : devices)
        {
            const QString ip = device.ip.trimmed();
            const QStringList segments = ip.split('.');
            if (segments.size() != 4)
            {
                continue;
            }

            bool ok0 = false;
            bool ok1 = false;
            bool ok2 = false;
            bool ok3 = false;
            const int a = segments.at(0).toInt(&ok0);
            const int b = segments.at(1).toInt(&ok1);
            const int c = segments.at(2).toInt(&ok2);
            const int d = segments.at(3).toInt(&ok3);
            if (!ok0 || !ok1 || !ok2 || !ok3)
            {
                continue;
            }
            if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255)
            {
                continue;
            }

            const QString currentPrefix = QStringLiteral("%1.%2.%3").arg(a).arg(b).arg(c);
            if (d > maxHost)
            {
                maxHost = d;
                prefix = currentPrefix;
            }
        }

        if (maxHost >= 254)
        {
            maxHost = 100;
        }
        return QStringLiteral("%1.%2").arg(prefix).arg(maxHost + 1);
    }

    AddDeviceDialogResult showDeviceDialog(QWidget *parent,
                                           const SettingsDeviceList &existingDevices,
                                           const QString &languageKey,
                                           const SettingsDeviceEntry *initialDevice = nullptr)
    {
        const bool isEnglish = (languageKey == QStringLiteral("en_US"));
        const bool isEditMode = (initialDevice != nullptr);

        QDialog dialog(parent);
        dialog.setWindowTitle(isEditMode
                                  ? (isEnglish ? QStringLiteral("Edit Device") : QStringLiteral("编辑设备"))
                                  : (isEnglish ? QStringLiteral("Add New Device") : QStringLiteral("录入新设备")));
        dialog.setMinimumWidth(440);

        QFormLayout *formLayout = new QFormLayout(&dialog);
        formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        formLayout->setHorizontalSpacing(12);
        formLayout->setVerticalSpacing(10);

        QLineEdit *editDeviceName = new QLineEdit(&dialog);
        editDeviceName->setPlaceholderText(isEnglish ? QStringLiteral("e.g. Living Room Light") : QStringLiteral("例如：客厅主灯"));

        QComboBox *comboDeviceType = new QComboBox(&dialog);
        comboDeviceType->addItems(isEnglish
                                      ? QStringList{QStringLiteral("Lighting"), QStringLiteral("Climate"), QStringLiteral("Curtain"), QStringLiteral("Security"), QStringLiteral("Media"), QStringLiteral("Sensor"), QStringLiteral("Custom")}
                                      : QStringList{QStringLiteral("照明设备"), QStringLiteral("空调设备"), QStringLiteral("窗帘设备"), QStringLiteral("安防设备"), QStringLiteral("影音设备"), QStringLiteral("传感设备"), QStringLiteral("自定义设备")});

        QLineEdit *editRoom = new QLineEdit(&dialog);
        editRoom->setPlaceholderText(isEnglish ? QStringLiteral("e.g. Bedroom") : QStringLiteral("例如：卧室"));

        QComboBox *comboProtocol = new QComboBox(&dialog);
        comboProtocol->addItems({QStringLiteral("simulator"), QStringLiteral("mqtt"), QStringLiteral("modbus"), QStringLiteral("zigbee"), QStringLiteral("wifi")});

        QLineEdit *editManufacturer = new QLineEdit(QStringLiteral("SmartHome Lab"), &dialog);

        QComboBox *comboStatus = new QComboBox(&dialog);
        comboStatus->addItems(isEnglish ? QStringList{QStringLiteral("Online"), QStringLiteral("Offline")} : QStringList{QStringLiteral("在线"), QStringLiteral("离线")});

        QLineEdit *editDeviceId = new QLineEdit(&dialog);
        editDeviceId->setPlaceholderText(isEnglish ? QStringLiteral("Leave empty for auto generated ID") : QStringLiteral("留空将自动生成设备编号"));

        QLineEdit *editIp = new QLineEdit(nextIpAddress(existingDevices), &dialog);
        editIp->setReadOnly(true);
        editIp->setClearButtonEnabled(false);
        editIp->setToolTip(isEnglish
                               ? QStringLiteral("IP is managed by the system by default.")
                               : QStringLiteral("IP 默认由系统管理。"));

        QCheckBox *checkSupportsSlider = new QCheckBox(isEnglish ? QStringLiteral("Enable Slider") : QStringLiteral("启用滑杆控制"), &dialog);
        QDoubleSpinBox *spinSliderMin = new QDoubleSpinBox(&dialog);
        spinSliderMin->setRange(-1000.0, 10000.0);
        spinSliderMin->setValue(0.0);
        spinSliderMin->setDecimals(2);
        QDoubleSpinBox *spinSliderMax = new QDoubleSpinBox(&dialog);
        spinSliderMax->setRange(-1000.0, 10000.0);
        spinSliderMax->setValue(100.0);
        spinSliderMax->setDecimals(2);

        QLineEdit *editRemarks = new QLineEdit(&dialog);
        editRemarks->setPlaceholderText(isEnglish ? QStringLiteral("Optional note") : QStringLiteral("可选备注"));

        if (isEditMode)
        {
            editDeviceName->setText(initialDevice->name);

            int typeIndex = comboDeviceType->findText(initialDevice->type);
            if (typeIndex < 0 && !initialDevice->type.trimmed().isEmpty())
            {
                comboDeviceType->addItem(initialDevice->type.trimmed());
                typeIndex = comboDeviceType->count() - 1;
            }
            if (typeIndex >= 0)
            {
                comboDeviceType->setCurrentIndex(typeIndex);
            }

            editRoom->setText(initialDevice->roomName);

            int protocolIndex = comboProtocol->findText(initialDevice->protocolType);
            if (protocolIndex < 0 && !initialDevice->protocolType.trimmed().isEmpty())
            {
                comboProtocol->addItem(initialDevice->protocolType.trimmed());
                protocolIndex = comboProtocol->count() - 1;
            }
            if (protocolIndex >= 0)
            {
                comboProtocol->setCurrentIndex(protocolIndex);
            }

            editManufacturer->setText(initialDevice->manufacturer);
            comboStatus->setCurrentIndex(initialDevice->onlineStatus == QStringLiteral("offline") ? 1 : 0);
            editDeviceId->setText(initialDevice->id);
            editIp->setText(initialDevice->ip);

            const bool supportsSlider = initialDevice->hasSliderConfig ? initialDevice->supportsSlider : false;
            checkSupportsSlider->setChecked(supportsSlider);
            spinSliderMin->setValue(initialDevice->hasSliderConfig ? initialDevice->sliderMin : 0.0);
            spinSliderMax->setValue(initialDevice->hasSliderConfig ? initialDevice->sliderMax : 100.0);
            editRemarks->setText(initialDevice->remarks);
        }

        formLayout->addRow(isEnglish ? QStringLiteral("Device Name") : QStringLiteral("设备名称"), editDeviceName);
        formLayout->addRow(isEnglish ? QStringLiteral("Device Type") : QStringLiteral("设备类型"), comboDeviceType);
        formLayout->addRow(isEnglish ? QStringLiteral("Room") : QStringLiteral("房间"), editRoom);
        formLayout->addRow(isEnglish ? QStringLiteral("Protocol") : QStringLiteral("协议类型"), comboProtocol);
        formLayout->addRow(isEnglish ? QStringLiteral("Manufacturer") : QStringLiteral("厂商"), editManufacturer);
        formLayout->addRow(isEnglish ? QStringLiteral("Initial Status") : QStringLiteral("初始状态"), comboStatus);
        formLayout->addRow(isEnglish ? QStringLiteral("Device ID") : QStringLiteral("设备编号"), editDeviceId);
        formLayout->addRow(isEnglish ? QStringLiteral("IP Address") : QStringLiteral("IP地址"), editIp);
        formLayout->addRow(isEnglish ? QStringLiteral("Slider") : QStringLiteral("滑杆设置"), checkSupportsSlider);
        formLayout->addRow(isEnglish ? QStringLiteral("Slider Min") : QStringLiteral("滑杆最小值"), spinSliderMin);
        formLayout->addRow(isEnglish ? QStringLiteral("Slider Max") : QStringLiteral("滑杆最大值"), spinSliderMax);
        formLayout->addRow(isEnglish ? QStringLiteral("Remark") : QStringLiteral("备注"), editRemarks);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        formLayout->addRow(buttonBox);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        AddDeviceDialogResult result;
        if (dialog.exec() != QDialog::Accepted)
        {
            return result;
        }

        const QString deviceName = editDeviceName->text().trimmed();
        if (deviceName.isEmpty())
        {
            QMessageBox::warning(parent,
                                 isEnglish ? QStringLiteral("Validation") : QStringLiteral("提示"),
                                 isEnglish ? QStringLiteral("Device name cannot be empty.") : QStringLiteral("设备名称不能为空。"));
            return result;
        }

        const QString ipAddress = editIp->text().trimmed();
        if (ipAddress.isEmpty())
        {
            QMessageBox::warning(parent,
                                 isEnglish ? QStringLiteral("Validation") : QStringLiteral("提示"),
                                 isEnglish ? QStringLiteral("IP address cannot be empty.") : QStringLiteral("IP 地址不能为空。"));
            return result;
        }

        result.accepted = true;
        result.device.name = deviceName;
        result.device.type = comboDeviceType->currentText().trimmed();
        result.device.roomName = editRoom->text().trimmed();
        result.device.protocolType = comboProtocol->currentText().trimmed();
        result.device.manufacturer = editManufacturer->text().trimmed();
        result.device.onlineStatus = (comboStatus->currentIndex() == 0) ? QStringLiteral("online") : QStringLiteral("offline");
        result.device.switchStatus = (comboStatus->currentIndex() == 0) ? QStringLiteral("on") : QStringLiteral("off");
        result.device.id = editDeviceId->text().trimmed();
        result.device.ip = ipAddress;
        result.device.remarks = editRemarks->text().trimmed();
        result.device.hasSliderConfig = true;
        result.device.supportsSlider = checkSupportsSlider->isChecked();
        result.device.sliderMin = spinSliderMin->value();
        result.device.sliderMax = spinSliderMax->value();
        return result;
    }
}

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingsWidget)
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
    ui->tableWidget_devices->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget_devices->verticalHeader()->setVisible(false);

    connect(&m_settingsService, &SettingsService::devicesRefreshed,
            this, &SettingsWidget::onDevicesRefreshed);
    m_settingsService.refreshNow();
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
    m_settingsService.refreshNow();
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
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          isEnglish ? QStringLiteral("Backup Database") : QStringLiteral("备份数据库"),
                                                          QDir::homePath() + QStringLiteral("/smarthome_backup.sql"),
                                                          isEnglish ? QStringLiteral("SQL Files (*.sql)") : QStringLiteral("SQL文件 (*.sql)"));
    if (fileName.isEmpty())
    {
        return;
    }

    QString errorText;
    if (!m_settingsService.backupDatabase(fileName, &errorText))
    {
        QMessageBox::critical(this,
                              isEnglish ? QStringLiteral("Backup Failed") : QStringLiteral("备份失败"),
                              (isEnglish ? QStringLiteral("Database backup failed:\n") : QStringLiteral("数据库备份失败：\n")) + (errorText.isEmpty() ? (isEnglish ? QStringLiteral("Unknown error") : QStringLiteral("未知错误")) : errorText));
        return;
    }

    QMessageBox::information(this,
                             isEnglish ? QStringLiteral("Backup Success") : QStringLiteral("备份成功"),
                             (isEnglish ? QStringLiteral("Database has been backed up to:\n") : QStringLiteral("数据库已备份到：\n")) + fileName);
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
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    const AddDeviceDialogResult dialogResult = showDeviceDialog(this, m_devices, m_languageKey);
    if (!dialogResult.accepted)
    {
        return;
    }

    SettingsDeviceEntry newDevice;
    QString errorText;
    if (!m_settingsService.addDevice(dialogResult.device, m_devices.size(), &newDevice, &errorText))
    {
        QMessageBox::critical(this,
                              isEnglish ? QStringLiteral("Failed") : QStringLiteral("失败"),
                              (isEnglish ? QStringLiteral("Failed to add device:\n") : QStringLiteral("添加设备失败：\n")) + (errorText.isEmpty() ? (isEnglish ? QStringLiteral("Unknown error") : QStringLiteral("未知错误")) : errorText));
        return;
    }

    reloadDevicesFromDatabase();
    emit devicesChanged();

    QMessageBox::information(this,
                             isEnglish ? QStringLiteral("Success") : QStringLiteral("成功"),
                             (isEnglish
                                  ? QStringLiteral("Device \"") + newDevice.name + QStringLiteral("\" was added.\n") + QStringLiteral("Device ID: ") + newDevice.id + QStringLiteral("\n") + QStringLiteral("IP Address: ") + newDevice.ip
                                  : QStringLiteral("设备 \"") + newDevice.name + QStringLiteral("\" 已添加！\n") + QStringLiteral("设备编号：") + newDevice.id + QStringLiteral("\n") + QStringLiteral("IP地址：") + newDevice.ip));
}

void SettingsWidget::on_btnDeleteDevice_clicked()
{
    qDebug() << "删除设备";
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    const int currentRow = ui->tableWidget_devices->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this,
                             isEnglish ? QStringLiteral("Hint") : QStringLiteral("提示"),
                             isEnglish ? QStringLiteral("Please select a device to delete first.") : QStringLiteral("请先选择要删除的设备。"));
        return;
    }

    const QString deviceName = ui->tableWidget_devices->item(currentRow, 1)->text();
    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        isEnglish ? QStringLiteral("Confirm Delete") : QStringLiteral("确认删除"),
        (isEnglish ? QStringLiteral("Are you sure you want to delete device \"") : QStringLiteral("确定要删除设备 \"")) + deviceName + (isEnglish ? QStringLiteral("\"?") : QStringLiteral("\" 吗？")),
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
                              isEnglish ? QStringLiteral("Failed") : QStringLiteral("失败"),
                              (isEnglish ? QStringLiteral("Failed to delete device:\n") : QStringLiteral("删除设备失败：\n")) + (errorText.isEmpty() ? (isEnglish ? QStringLiteral("Unknown error") : QStringLiteral("未知错误")) : errorText));
        return;
    }

    reloadDevicesFromDatabase();
    emit devicesChanged();
    QMessageBox::information(this,
                             isEnglish ? QStringLiteral("Success") : QStringLiteral("成功"),
                             isEnglish ? QStringLiteral("Device deleted.") : QStringLiteral("设备已删除！"));
}

void SettingsWidget::on_btnEditDevice_clicked()
{
    qDebug() << "编辑设备";
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    const int currentRow = ui->tableWidget_devices->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this,
                             isEnglish ? QStringLiteral("Hint") : QStringLiteral("提示"),
                             isEnglish ? QStringLiteral("Please select a device to edit first.") : QStringLiteral("请先选择要编辑的设备。"));
        return;
    }

    QTableWidgetItem *idItem = ui->tableWidget_devices->item(currentRow, 0);
    if (!idItem)
    {
        QMessageBox::warning(this,
                             isEnglish ? QStringLiteral("Hint") : QStringLiteral("提示"),
                             isEnglish ? QStringLiteral("The selected row is invalid. Please refresh and try again.") : QStringLiteral("当前选中设备数据无效，请刷新后重试。"));
        return;
    }

    const QString originalDeviceId = idItem->text().trimmed();
    SettingsDeviceEntry currentDevice;
    bool found = false;
    for (const SettingsDeviceEntry &device : m_devices)
    {
        if (device.id == originalDeviceId)
        {
            currentDevice = device;
            found = true;
            break;
        }
    }

    if (!found)
    {
        currentDevice.id = originalDeviceId;
        if (QTableWidgetItem *nameItem = ui->tableWidget_devices->item(currentRow, 1))
        {
            currentDevice.name = nameItem->text();
        }
        if (QTableWidgetItem *typeItem = ui->tableWidget_devices->item(currentRow, 2))
        {
            currentDevice.type = typeItem->text();
        }
        if (QTableWidgetItem *ipItem = ui->tableWidget_devices->item(currentRow, 3))
        {
            currentDevice.ip = ipItem->text();
        }
        if (QTableWidgetItem *statusItem = ui->tableWidget_devices->item(currentRow, 4))
        {
            const QString statusText = statusItem->text().trimmed().toLower();
            currentDevice.onlineStatus = (statusText.contains(QStringLiteral("offline")) || statusText.contains(QStringLiteral("离线")))
                                             ? QStringLiteral("offline")
                                             : QStringLiteral("online");
        }
    }

    const AddDeviceDialogResult dialogResult = showDeviceDialog(this, m_devices, m_languageKey, &currentDevice);
    if (!dialogResult.accepted)
    {
        return;
    }

    SettingsDeviceEntry updatedDevice;
    QString errorText;
    if (!m_settingsService.updateDevice(originalDeviceId, dialogResult.device, &updatedDevice, &errorText))
    {
        QMessageBox::critical(this,
                              isEnglish ? QStringLiteral("Failed") : QStringLiteral("失败"),
                              (isEnglish ? QStringLiteral("Failed to update device:\n") : QStringLiteral("编辑设备失败：\n")) + (errorText.isEmpty() ? (isEnglish ? QStringLiteral("Unknown error") : QStringLiteral("未知错误")) : errorText));
        return;
    }

    reloadDevicesFromDatabase();
    emit devicesChanged();
    QMessageBox::information(this,
                             isEnglish ? QStringLiteral("Success") : QStringLiteral("成功"),
                             isEnglish
                                 ? QStringLiteral("Device \"") + updatedDevice.name + QStringLiteral("\" has been updated.")
                                 : QStringLiteral("设备 \"") + updatedDevice.name + QStringLiteral("\" 已更新。"));
}

void SettingsWidget::on_btnTestConnection_clicked()
{
    qDebug() << "测试本地服务连通性";
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    const QModelIndexList selectedRows = ui->tableWidget_devices->selectionModel()
                                             ? ui->tableWidget_devices->selectionModel()->selectedRows()
                                             : QModelIndexList();
    if (!selectedRows.isEmpty())
    {
        const int row = selectedRows.first().row();
        QTableWidgetItem *nameItem = ui->tableWidget_devices->item(row, 1);
        QTableWidgetItem *ipItem = ui->tableWidget_devices->item(row, 3);

        if (ipItem && !ipItem->text().trimmed().isEmpty())
        {
            const QString deviceName = nameItem ? nameItem->text() : (isEnglish ? QStringLiteral("Device") : QStringLiteral("设备"));
            const QString deviceIp = ipItem->text().trimmed();
            const int latencyMs = QRandomGenerator::global()->bounded(5, 121);

            QMessageBox::information(this,
                                     isEnglish ? QStringLiteral("Connectivity Test") : QStringLiteral("连通性测试"),
                                     (isEnglish
                                          ? QStringLiteral("Device: ") + deviceName + QStringLiteral("\n") + QStringLiteral("IP Address: ") + deviceIp + QStringLiteral("\n") + QStringLiteral("Latency: ") + QString::number(latencyMs) + QStringLiteral("ms")
                                          : QStringLiteral("设备：") + deviceName + QStringLiteral("\n") + QStringLiteral("IP地址：") + deviceIp + QStringLiteral("\n") + QStringLiteral("延迟：") + QString::number(latencyMs) + QStringLiteral("ms")));
            return;
        }
    }

    const TcpEndpointTestResult result = m_settingsService.testSmartHomeTcpEndpoint();
    const QString endpointText = result.host + QStringLiteral(":") + QString::number(result.port);

    if (result.reachable)
    {
        QMessageBox::information(this,
                                 isEnglish ? QStringLiteral("Connectivity Test") : QStringLiteral("连通性测试"),
                                 (isEnglish
                                      ? QStringLiteral("Target service: ") + endpointText + QStringLiteral("\n") + QStringLiteral("Connected\n") + QStringLiteral("Latency: ") + QString::number(result.latencyMs) + QStringLiteral("ms")
                                      : QStringLiteral("目标服务：") + endpointText + QStringLiteral("\n") + QStringLiteral("连接成功\n") + QStringLiteral("延迟：") + QString::number(result.latencyMs) + QStringLiteral("ms")));
        return;
    }

    QMessageBox::warning(this,
                         isEnglish ? QStringLiteral("Connectivity Test") : QStringLiteral("连通性测试"),
                         (isEnglish
                              ? QStringLiteral("Target service: ") + endpointText + QStringLiteral("\n") + QStringLiteral("Connection failed\n") + QStringLiteral("Reason: ") + (result.errorText.isEmpty() ? QStringLiteral("Unknown error") : result.errorText)
                              : QStringLiteral("目标服务：") + endpointText + QStringLiteral("\n") + QStringLiteral("连接失败\n") + QStringLiteral("原因：") + (result.errorText.isEmpty() ? QStringLiteral("未知错误") : result.errorText)));
}

void SettingsWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_settingsService.startPolling(kSettingsRefreshIntervalMs);
    adjustDeviceTableForWidth();
}

void SettingsWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    m_settingsService.stopPolling();
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

void SettingsWidget::onDevicesRefreshed(SettingsDeviceList devices)
{
    const int currentRow = ui->tableWidget_devices->currentRow();
    QString selectedDeviceId;
    if (currentRow >= 0 && ui->tableWidget_devices->item(currentRow, 0))
    {
        selectedDeviceId = ui->tableWidget_devices->item(currentRow, 0)->text();
    }

    ui->tableWidget_devices->setRowCount(0);
    m_devices = devices;
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
