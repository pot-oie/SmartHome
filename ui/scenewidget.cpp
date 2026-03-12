#include "scenewidget.h"
#include "ui_scenewidget.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHash>
#include <QIcon>
#include <QLineEdit>
#include <QAbstractSpinBox>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidgetItem>

namespace
{
    const QString kTextHint = QStringLiteral("\u63d0\u793a");
    const QString kTextFailed = QStringLiteral("\u5931\u8d25");
    const QString kTextSuccess = QStringLiteral("\u6210\u529f");
    const QString kTextSceneNamePrefix = QStringLiteral("\u573a\u666f\u540d\u79f0\uff1a");

    QString localizedSceneName(const QString &name, bool isEnglish)
    {
        if (!isEnglish)
        {
            return name;
        }

        static const QHash<QString, QString> map = {
            {QStringLiteral("回家模式"), QStringLiteral("Home Mode")},
            {QStringLiteral("睡眠模式"), QStringLiteral("Sleep Mode")},
            {QStringLiteral("观影模式"), QStringLiteral("Movie Mode")},
            {QStringLiteral("影院模式"), QStringLiteral("Movie Mode")},
            {QStringLiteral("离家模式"), QStringLiteral("Away Mode")},
            {QStringLiteral("派对模式"), QStringLiteral("Party Mode")},
            {QStringLiteral("起床模式"), QStringLiteral("Wake-up Mode")},
            {QStringLiteral("晚餐模式"), QStringLiteral("Dinner Mode")}};
        return map.value(name, name);
    }

    bool isDarkPalette(const QPalette &palette)
    {
        return palette.color(QPalette::Window).lightness() < 128;
    }

    QIcon sceneListIcon(const QString &iconPath, const QPalette &palette)
    {
        QPixmap source(iconPath);
        if (source.isNull())
        {
            return QIcon(iconPath);
        }

        QPixmap tinted(source.size());
        tinted.fill(Qt::transparent);

        QPainter painter(&tinted);
        painter.drawPixmap(0, 0, source);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        const QColor color = isDarkPalette(palette) ? QColor(223, 232, 245) : QColor(44, 56, 72);
        painter.fillRect(tinted.rect(), color);
        painter.end();

        return QIcon(tinted);
    }

    QString localizedDeviceName(const QString &name, bool isEnglish)
    {
        if (!isEnglish)
        {
            return name;
        }

        static const QHash<QString, QString> map = {
            {QStringLiteral("客厅主灯"), QStringLiteral("Living Room Light")},
            {QStringLiteral("卧室灯"), QStringLiteral("Bedroom Light")},
            {QStringLiteral("客厅空调"), QStringLiteral("Living Room AC")},
            {QStringLiteral("卧室空调"), QStringLiteral("Bedroom AC")},
            {QStringLiteral("客厅窗帘"), QStringLiteral("Living Room Curtain")},
            {QStringLiteral("卧室窗帘"), QStringLiteral("Bedroom Curtain")},
            {QStringLiteral("前门智能锁"), QStringLiteral("Front Door Lock")},
            {QStringLiteral("客厅摄像头"), QStringLiteral("Living Room Camera")},
            {QStringLiteral("客厅电视"), QStringLiteral("Living Room TV")}};
        return map.value(name, name);
    }

    QString localizedActionText(const QString &action, bool isEnglish)
    {
        if (!isEnglish)
        {
            return action;
        }

        static const QHash<QString, QString> map = {
            {QStringLiteral("开启"), QStringLiteral("On")},
            {QStringLiteral("关闭"), QStringLiteral("Off")},
            {QStringLiteral("打开"), QStringLiteral("Open")},
            {QStringLiteral("调节亮度"), QStringLiteral("Set Brightness")},
            {QStringLiteral("设置色温"), QStringLiteral("Set Color Temp")},
            {QStringLiteral("设置温度"), QStringLiteral("Set Temperature")},
            {QStringLiteral("设置开合度"), QStringLiteral("Set Openness")},
            {QStringLiteral("设置音量"), QStringLiteral("Set Volume")},
            {QStringLiteral("解锁"), QStringLiteral("Unlock")},
            {QStringLiteral("上锁"), QStringLiteral("Lock")}};
        return map.value(action, action);
    }

    void populateSceneIconCombo(QComboBox *comboBox)
    {
        comboBox->addItem(QIcon(":/icons/home.svg"), QStringLiteral("\u56de\u5bb6"), ":/icons/home.svg");
        comboBox->addItem(QIcon(":/icons/bedtime.svg"), QStringLiteral("\u7761\u7720"), ":/icons/bedtime.svg");
        comboBox->addItem(QIcon(":/icons/movie.svg"), QStringLiteral("\u89c2\u5f71"), ":/icons/movie.svg");
        comboBox->addItem(QIcon(":/icons/flight_takeoff.svg"), QStringLiteral("\u79bb\u5bb6"), ":/icons/flight_takeoff.svg");
        comboBox->addItem(QIcon(":/icons/celebration.svg"), QStringLiteral("\u6d3e\u5bf9"), ":/icons/celebration.svg");
        comboBox->addItem(QIcon(":/icons/wb_sunny.svg"), QStringLiteral("\u8d77\u5e8a"), ":/icons/wb_sunny.svg");
        comboBox->addItem(QIcon(":/icons/restaurant.svg"), QStringLiteral("\u7528\u9910"), ":/icons/restaurant.svg");
        comboBox->addItem(QIcon(":/icons/self_improvement.svg"), QStringLiteral("\u51a5\u60f3"), ":/icons/self_improvement.svg");
        comboBox->addItem(QIcon(":/icons/sports_esports.svg"), QStringLiteral("\u6e38\u620f"), ":/icons/sports_esports.svg");
        comboBox->addItem(QIcon(":/icons/cleaning_services.svg"), QStringLiteral("\u6e05\u6d01"), ":/icons/cleaning_services.svg");
        comboBox->addItem(QIcon(":/icons/pets.svg"), QStringLiteral("\u5ba0\u7269"), ":/icons/pets.svg");
        comboBox->addItem(QIcon(":/icons/music.svg"), QStringLiteral("\u97f3\u4e50"), ":/icons/music.svg");
        comboBox->addItem(QIcon(":/icons/scene.svg"), QStringLiteral("\u901a\u7528"), ":/icons/scene.svg");
    }

    void populateActionCombo(QComboBox *comboBox)
    {
        comboBox->addItem(QStringLiteral("\u5f00\u542f"));
        comboBox->addItem(QStringLiteral("\u5173\u95ed"));
        comboBox->addItem(QStringLiteral("\u6253\u5f00"));
        comboBox->addItem(QStringLiteral("\u89e3\u9501"));
        comboBox->addItem(QStringLiteral("\u4e0a\u9501"));
    }
}

SceneWidget::SceneWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::SceneWidget)
{
    ui->setupUi(this);
    ui->tableWidget_devices->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_devices->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_devices->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget_devices->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_devices->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_devices->setAlternatingRowColors(true);
    ui->tableWidget_devices->verticalHeader()->setVisible(false);

    // 强化信息层级：场景名作为一级信息，描述作为二级信息。
    ui->label_sceneName->setStyleSheet(QStringLiteral("font-size: 13pt; font-weight: 700;"));
    ui->label_sceneDesc->setStyleSheet(QStringLiteral("font-size: 10pt; color: #7D8A99;"));

    // 场景列表区域提升可用性与视觉一致性。
    ui->listWidget_scenes->setIconSize(QSize(18, 18));
    ui->listWidget_scenes->setSpacing(2);

    connect(ui->listWidget_scenes, &QListWidget::currentRowChanged, this, &SceneWidget::updateSceneDetails);
    connect(ui->listWidget_scenes, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *)
            { editSelectedScene(); });
    connect(ui->tableWidget_devices, &QTableWidget::cellDoubleClicked, this, &SceneWidget::editSelectedAction);

    loadScenesFromDatabase();
    applyLanguage(QStringLiteral("zh_CN"));
}

SceneWidget::~SceneWidget()
{
    delete ui;
}

void SceneWidget::applyLanguage(const QString &languageKey)
{
    if (languageKey.trimmed().isEmpty())
    {
        return;
    }

    m_languageKey = languageKey;
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    ui->groupBox_sceneList->setTitle(isEnglish ? QStringLiteral("Scenes") : QStringLiteral("场景列表"));
    ui->groupBox_sceneDetail->setTitle(isEnglish ? QStringLiteral("Scene Details") : QStringLiteral("场景详情"));
    ui->groupBox_devices->setTitle(isEnglish ? QStringLiteral("Linked Devices & Actions") : QStringLiteral("绑定的设备和动作"));
    ui->btnAddScene->setText(isEnglish ? QStringLiteral("New Scene") : QStringLiteral("新建场景"));
    ui->btnDeleteScene->setText(isEnglish ? QStringLiteral("Delete") : QStringLiteral("删除"));
    ui->btnAddDeviceToScene->setText(isEnglish ? QStringLiteral("Add Device") : QStringLiteral("添加设备"));
    ui->btnEditDeviceInScene->setText(isEnglish ? QStringLiteral("Edit Device") : QStringLiteral("修改设备"));
    ui->btnRemoveDevice->setText(isEnglish ? QStringLiteral("Remove Device") : QStringLiteral("移除设备"));
    ui->btnActivateScene->setText(isEnglish ? QStringLiteral("Activate Scene") : QStringLiteral("一键激活场景"));
    ui->tableWidget_devices->setHorizontalHeaderLabels(isEnglish
                                                           ? QStringList{QStringLiteral("Device"), QStringLiteral("Action"), QStringLiteral("Parameter")}
                                                           : QStringList{QStringLiteral("设备名称"), QStringLiteral("动作"), QStringLiteral("参数")});

    // 操作按钮分级：激活场景主按钮，移除设备危险按钮，其余为次级按钮。
    ui->btnAddDeviceToScene->setProperty("class", "secondary");
    ui->btnEditDeviceInScene->setProperty("class", "secondary");
    ui->btnRemoveDevice->setProperty("class", "danger");
    ui->btnActivateScene->setProperty("class", "");
    ui->btnAddScene->setProperty("class", "secondary");
    ui->btnDeleteScene->setProperty("class", "danger");

    const QList<QPushButton *> styledButtons = {
        ui->btnAddDeviceToScene,
        ui->btnEditDeviceInScene,
        ui->btnRemoveDevice,
        ui->btnActivateScene,
        ui->btnAddScene,
        ui->btnDeleteScene};
    for (QPushButton *button : styledButtons)
    {
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }

    const int row = ui->listWidget_scenes->currentRow();
    renderSceneList();
    if (row >= 0 && row < ui->listWidget_scenes->count())
    {
        QSignalBlocker blocker(ui->listWidget_scenes);
        ui->listWidget_scenes->setCurrentRow(row);
    }
    if (row >= 0 && row < m_scenes.size())
    {
        renderSceneDetails(m_scenes.at(row));
    }

    updateActionButtonsLayout();
}

void SceneWidget::loadScenesFromDatabase(const QString &sceneCodeToSelect, qint64 actionIdToSelect)
{
    m_scenes = m_sceneService.loadDefaultScenes();
    renderSceneList();

    if (m_scenes.isEmpty())
    {
        ui->label_sceneName->setText(kTextSceneNamePrefix);
        ui->label_sceneDesc->clear();
        ui->tableWidget_devices->clearContents();
        ui->tableWidget_devices->setRowCount(0);
        return;
    }

    int targetRow = 0;
    if (!sceneCodeToSelect.trimmed().isEmpty())
    {
        const int matchedRow = findSceneRowByCode(sceneCodeToSelect);
        if (matchedRow >= 0)
        {
            targetRow = matchedRow;
        }
    }

    {
        QSignalBlocker blocker(ui->listWidget_scenes);
        ui->listWidget_scenes->setCurrentRow(targetRow);
    }

    renderSceneDetails(m_scenes.at(targetRow));

    if (actionIdToSelect > 0)
    {
        const int actionRow = findActionRowById(m_scenes.at(targetRow), actionIdToSelect);
        if (actionRow >= 0)
        {
            ui->tableWidget_devices->setCurrentCell(actionRow, 0);
        }
    }
}

void SceneWidget::renderSceneList()
{
    ui->listWidget_scenes->clear();
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    const QPalette pal = this->palette();
    for (const SceneDefinition &scene : m_scenes)
    {
        QListWidgetItem *item = new QListWidgetItem(localizedSceneName(scene.name, isEnglish));
        item->setIcon(sceneListIcon(scene.icon, pal));
        ui->listWidget_scenes->addItem(item);
    }
}

void SceneWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (!event)
    {
        return;
    }

    if (event->type() != QEvent::PaletteChange && event->type() != QEvent::StyleChange)
    {
        return;
    }

    const int row = ui->listWidget_scenes->currentRow();
    renderSceneList();
    if (row >= 0 && row < ui->listWidget_scenes->count())
    {
        QSignalBlocker blocker(ui->listWidget_scenes);
        ui->listWidget_scenes->setCurrentRow(row);
    }
    if (row >= 0 && row < m_scenes.size())
    {
        renderSceneDetails(m_scenes.at(row));
    }
}

void SceneWidget::renderSceneDetails(const SceneDefinition &scene)
{
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    ui->label_sceneName->setText((isEnglish ? QStringLiteral("Scene: ") : kTextSceneNamePrefix) + localizedSceneName(scene.name, isEnglish));
    ui->label_sceneDesc->setText(scene.description);

    ui->tableWidget_devices->clearContents();
    ui->tableWidget_devices->setRowCount(0);

    for (const SceneDeviceAction &action : scene.actions)
    {
        const int row = ui->tableWidget_devices->rowCount();
        ui->tableWidget_devices->insertRow(row);
        ui->tableWidget_devices->setItem(row, 0, new QTableWidgetItem(localizedDeviceName(action.deviceName, isEnglish)));
        ui->tableWidget_devices->setItem(row, 1, new QTableWidgetItem(localizedActionText(action.actionText, isEnglish)));
        ui->tableWidget_devices->setItem(row, 2, new QTableWidgetItem(action.paramText));
    }
}

void SceneWidget::updateActionButtonsLayout()
{
    const bool compact = this->width() < 920;
    const QList<QPushButton *> buttons = {ui->btnAddDeviceToScene, ui->btnEditDeviceInScene, ui->btnRemoveDevice};
    for (QPushButton *button : buttons)
    {
        if (!button)
        {
            continue;
        }
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setMinimumWidth(compact ? 0 : 140);
    }
}

void SceneWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateActionButtonsLayout();
}

bool SceneWidget::openSceneDialog(SceneDefinition *scene, const QString &title)
{
    if (!scene)
    {
        return false;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.resize(360, 240);

    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *editName = new QLineEdit(scene->name, &dialog);
    editName->setPlaceholderText(QStringLiteral("\u4f8b\u5982\uff1a\u665a\u9910\u6a21\u5f0f"));
    form->addRow(QStringLiteral("\u573a\u666f\u540d\u79f0:"), editName);

    QLineEdit *editDesc = new QLineEdit(scene->description, &dialog);
    editDesc->setPlaceholderText(QStringLiteral("\u4f8b\u5982\uff1a\u5f00\u542f\u9910\u5385\u706f\u5e76\u5173\u95ed\u5ba2\u5385\u7535\u89c6"));
    form->addRow(QStringLiteral("\u573a\u666f\u63cf\u8ff0:"), editDesc);

    QComboBox *cmbIcon = new QComboBox(&dialog);
    populateSceneIconCombo(cmbIcon);
    const int iconIndex = cmbIcon->findData(scene->icon);
    cmbIcon->setCurrentIndex(iconIndex >= 0 ? iconIndex : cmbIcon->count() - 1);
    form->addRow(QStringLiteral("\u573a\u666f\u56fe\u6807:"), cmbIcon);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    const QString sceneName = editName->text().trimmed();
    const QString sceneDesc = editDesc->text().trimmed();
    if (sceneName.isEmpty() || sceneDesc.isEmpty())
    {
        QMessageBox::warning(this, kTextHint, QStringLiteral("\u573a\u666f\u540d\u79f0\u548c\u573a\u666f\u63cf\u8ff0\u5747\u4e3a\u5fc5\u586b\u9879\u3002"));
        return false;
    }

    scene->name = sceneName;
    scene->description = sceneDesc;
    scene->icon = cmbIcon->currentData().toString();
    return true;
}

bool SceneWidget::openActionDialog(SceneDeviceAction *action, const QString &title)
{
    if (!action)
    {
        return false;
    }

    const SettingsDeviceList devices = m_sceneService.loadAvailableDevices();
    if (devices.isEmpty() && action->deviceName.trimmed().isEmpty())
    {
        QMessageBox::warning(this, kTextHint, QStringLiteral("\u5f53\u524d\u6ca1\u6709\u53ef\u7528\u8bbe\u5907\uff0c\u8bf7\u5148\u5230\u7cfb\u7edf\u8bbe\u7f6e\u4e2d\u7ef4\u62a4\u8bbe\u5907\u3002"));
        return false;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.resize(380, 240);
    dialog.setMinimumSize(360, 220);

    QFormLayout *form = new QFormLayout(&dialog);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setVerticalSpacing(10);

    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    const QString textDevice = isEnglish ? QStringLiteral("Device:") : QStringLiteral("选择设备:");
    const QString textAction = isEnglish ? QStringLiteral("Action:") : QStringLiteral("执行动作:");
    const QString textParam = isEnglish ? QStringLiteral("Parameter:") : QStringLiteral("动作参数:");
    const QString textNoParam = isEnglish ? QStringLiteral("This action does not need a parameter") : QStringLiteral("该动作无需参数");
    const QString textParamHint = isEnglish ? QStringLiteral("e.g. 24C or 80%") : QStringLiteral("例如：24C 或 80%");

    const QStringList defaultActions = {
        QStringLiteral("开启"),
        QStringLiteral("关闭"),
        QStringLiteral("打开"),
        QStringLiteral("解锁"),
        QStringLiteral("上锁")};

    QComboBox *cmbDevice = new QComboBox(&dialog);
    cmbDevice->setMaxVisibleItems(8);
    QHash<QString, QString> deviceTypeById;
    QHash<QString, QString> deviceTypeByName;
    for (const SettingsDeviceEntry &device : devices)
    {
        cmbDevice->addItem(device.name, device.id);
        deviceTypeById.insert(device.id, device.type);
        deviceTypeByName.insert(device.name, device.type);
    }

    int deviceIndex = cmbDevice->findData(action->deviceId);
    if (deviceIndex < 0)
    {
        deviceIndex = cmbDevice->findText(action->deviceName);
    }
    if (deviceIndex < 0 && !action->deviceName.trimmed().isEmpty())
    {
        cmbDevice->addItem(action->deviceName, action->deviceId);
        deviceIndex = cmbDevice->count() - 1;
    }
    if (deviceIndex >= 0)
    {
        cmbDevice->setCurrentIndex(deviceIndex);
    }
    form->addRow(textDevice, cmbDevice);

    QComboBox *cmbAction = new QComboBox(&dialog);
    cmbAction->setMaxVisibleItems(6);
    form->addRow(textAction, cmbAction);

    QWidget *paramEditor = new QWidget(&dialog);
    QHBoxLayout *paramLayout = new QHBoxLayout(paramEditor);
    paramLayout->setContentsMargins(0, 0, 0, 0);
    paramLayout->setSpacing(6);

    QLineEdit *editParam = new QLineEdit(action->paramText, paramEditor);
    editParam->setClearButtonEnabled(true);
    editParam->setPlaceholderText(textParamHint);

    QSpinBox *spinParam = new QSpinBox(paramEditor);
    spinParam->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    spinParam->setAccelerated(true);
    spinParam->setVisible(false);

    QComboBox *cmbParam = new QComboBox(paramEditor);
    cmbParam->setVisible(false);
    cmbParam->addItem(isEnglish ? QStringLiteral("Cool") : QStringLiteral("亮色"), QStringLiteral("6500K"));
    cmbParam->addItem(isEnglish ? QStringLiteral("Warm") : QStringLiteral("暖色"), QStringLiteral("3000K"));
    cmbParam->addItem(isEnglish ? QStringLiteral("Mixed") : QStringLiteral("混合"), QStringLiteral("4500K"));

    paramLayout->addWidget(editParam, 1);
    paramLayout->addWidget(spinParam, 1);
    paramLayout->addWidget(cmbParam, 1);
    form->addRow(textParam, paramEditor);

    auto selectedDeviceType = [=]() {
        const QString deviceId = cmbDevice->currentData().toString();
        const QString deviceName = cmbDevice->currentText();
        const QString typeById = deviceTypeById.value(deviceId);
        if (!typeById.trimmed().isEmpty())
        {
            return typeById;
        }
        return deviceTypeByName.value(deviceName);
    };

    auto actionsForCurrentDevice = [=]() {
        const QString typeText = selectedDeviceType();
        const QString deviceName = cmbDevice->currentText();
        if (typeText.contains(QStringLiteral("照明")) || deviceName.contains(QStringLiteral("灯")))
        {
            return QStringList{QStringLiteral("开启"), QStringLiteral("关闭"), QStringLiteral("调节亮度"), QStringLiteral("设置色温")};
        }
        if (typeText.contains(QStringLiteral("空调")))
        {
            return QStringList{QStringLiteral("开启"), QStringLiteral("关闭"), QStringLiteral("设置温度")};
        }
        if (typeText.contains(QStringLiteral("窗帘")))
        {
            return QStringList{QStringLiteral("打开"), QStringLiteral("关闭"), QStringLiteral("设置开合度")};
        }
        if (typeText.contains(QStringLiteral("影音")) || deviceName.contains(QStringLiteral("电视")))
        {
            return QStringList{QStringLiteral("开启"), QStringLiteral("关闭"), QStringLiteral("设置音量")};
        }
        if (typeText.contains(QStringLiteral("安防")) && deviceName.contains(QStringLiteral("锁")))
        {
            return QStringList{QStringLiteral("解锁"), QStringLiteral("上锁")};
        }
        if (typeText.contains(QStringLiteral("安防")) && deviceName.contains(QStringLiteral("摄像头")))
        {
            return QStringList{QStringLiteral("开启"), QStringLiteral("关闭")};
        }
        if (typeText.contains(QStringLiteral("传感")) || deviceName.contains(QStringLiteral("传感器")))
        {
            return QStringList{QStringLiteral("开启"), QStringLiteral("关闭")};
        }
        return defaultActions;
    };

    auto applyParamEditorForAction = [=]() {
        const QString actionText = cmbAction->currentText().trimmed();

        editParam->setVisible(false);
        spinParam->setVisible(false);
        cmbParam->setVisible(false);

        if (actionText == QStringLiteral("调节亮度") || actionText == QStringLiteral("设置开合度") || actionText == QStringLiteral("设置音量"))
        {
            spinParam->setRange(0, 100);
            spinParam->setSuffix(QStringLiteral("%"));
            int value = 0;
            const QRegularExpression re(QStringLiteral("(-?\\d+)"));
            const QRegularExpressionMatch match = re.match(action->paramText);
            if (match.hasMatch())
            {
                value = match.captured(1).toInt();
            }
            value = qBound(0, value, 100);
            spinParam->setValue(value);
            spinParam->setVisible(true);
            return;
        }

        if (actionText == QStringLiteral("设置温度"))
        {
            spinParam->setRange(16, 30);
            spinParam->setSuffix(QStringLiteral("C"));
            int value = 24;
            const QRegularExpression re(QStringLiteral("(-?\\d+)"));
            const QRegularExpressionMatch match = re.match(action->paramText);
            if (match.hasMatch())
            {
                value = match.captured(1).toInt();
            }
            value = qBound(16, value, 30);
            spinParam->setValue(value);
            spinParam->setVisible(true);
            return;
        }

        if (actionText == QStringLiteral("设置色温"))
        {
            int index = 2;
            const QString currentParam = action->paramText;
            if (currentParam.contains(QStringLiteral("6500")) || currentParam.contains(QStringLiteral("亮")))
            {
                index = 0;
            }
            else if (currentParam.contains(QStringLiteral("3000")) || currentParam.contains(QStringLiteral("暖")))
            {
                index = 1;
            }
            cmbParam->setCurrentIndex(index);
            cmbParam->setVisible(true);
            return;
        }

        const bool needParam = !(actionText == QStringLiteral("开启") ||
                                 actionText == QStringLiteral("关闭") ||
                                 actionText == QStringLiteral("打开") ||
                                 actionText == QStringLiteral("解锁") ||
                                 actionText == QStringLiteral("上锁") ||
                                 actionText.compare(QStringLiteral("on"), Qt::CaseInsensitive) == 0 ||
                                 actionText.compare(QStringLiteral("off"), Qt::CaseInsensitive) == 0 ||
                                 actionText.compare(QStringLiteral("open"), Qt::CaseInsensitive) == 0 ||
                                 actionText.compare(QStringLiteral("unlock"), Qt::CaseInsensitive) == 0 ||
                                 actionText.compare(QStringLiteral("lock"), Qt::CaseInsensitive) == 0);

        editParam->setEnabled(needParam);
        editParam->setPlaceholderText(needParam ? textParamHint : textNoParam);
        if (!needParam)
        {
            editParam->clear();
        }
        editParam->setVisible(true);
    };

    auto repopulateActionsByDevice = [=]() {
        const QString currentAction = cmbAction->currentText().trimmed().isEmpty() ? action->actionText.trimmed() : cmbAction->currentText().trimmed();
        const QStringList actions = actionsForCurrentDevice();

        QSignalBlocker blocker(cmbAction);
        cmbAction->clear();
        for (const QString &actionText : actions)
        {
            cmbAction->addItem(actionText);
        }

        int index = cmbAction->findText(currentAction);
        if (index < 0)
        {
            index = 0;
        }
        cmbAction->setCurrentIndex(index);
        applyParamEditorForAction();
    };

    connect(cmbDevice, &QComboBox::currentIndexChanged, &dialog, [=](int) {
        repopulateActionsByDevice();
    });
    connect(cmbAction, &QComboBox::currentTextChanged, &dialog, [=](const QString &) {
        applyParamEditorForAction();
    });
    repopulateActionsByDevice();

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    if (cmbDevice->currentText().trimmed().isEmpty())
    {
        QMessageBox::warning(this, kTextHint, QStringLiteral("\u8bf7\u9009\u62e9\u8bbe\u5907\u3002"));
        return false;
    }

    action->deviceName = cmbDevice->currentText();
    action->deviceId = cmbDevice->currentData().toString();
    action->actionText = cmbAction->currentText().trimmed();
    if (cmbParam->isVisible())
    {
        const QString label = cmbParam->currentText().trimmed();
        const QString value = cmbParam->currentData().toString();
        action->paramText = label + QStringLiteral("(") + value + QStringLiteral(")");
    }
    else if (spinParam->isVisible())
    {
        action->paramText = QString::number(spinParam->value()) + spinParam->suffix();
    }
    else
    {
        action->paramText = editParam->text().trimmed();
    }
    return true;
}

int SceneWidget::findSceneRowByCode(const QString &sceneCode) const
{
    for (int index = 0; index < m_scenes.size(); ++index)
    {
        if (m_scenes.at(index).id == sceneCode)
        {
            return index;
        }
    }

    return -1;
}

int SceneWidget::findActionRowById(const SceneDefinition &scene, qint64 actionId) const
{
    for (int index = 0; index < scene.actions.size(); ++index)
    {
        if (scene.actions.at(index).recordId == actionId)
        {
            return index;
        }
    }

    return -1;
}

void SceneWidget::updateSceneDetails(int row)
{
    if (row < 0 || row >= m_scenes.size())
    {
        return;
    }

    renderSceneDetails(m_scenes.at(row));
}

void SceneWidget::updateSceneExecutionResult(const QJsonObject &resultData)
{
    qDebug() << "Scene execution result:" << resultData;
}

void SceneWidget::on_btnAddScene_clicked()
{
    SceneDefinition scene;
    scene.icon = ":/icons/scene.svg";
    if (!openSceneDialog(&scene, QStringLiteral("\u6dfb\u52a0\u573a\u666f")))
    {
        return;
    }

    const SceneDefinition createdScene = m_sceneService.createScene(scene.name, scene.description, scene.icon);
    if (createdScene.id.isEmpty())
    {
        QMessageBox::critical(this, kTextFailed, QStringLiteral("\u573a\u666f\u4fdd\u5b58\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u6570\u636e\u5e93\u8fde\u63a5\u72b6\u6001\u3002"));
        return;
    }

    loadScenesFromDatabase(createdScene.id);
    QMessageBox::information(this, kTextSuccess, QStringLiteral("\u573a\u666f\u5df2\u6dfb\u52a0\u3002"));
}

void SceneWidget::on_btnDeleteScene_clicked()
{
    const int currentRow = ui->listWidget_scenes->currentRow();
    if (currentRow < 0 || currentRow >= m_scenes.size())
    {
        QMessageBox::warning(this, kTextHint, QStringLiteral("\u8bf7\u5148\u9009\u62e9\u573a\u666f\u3002"));
        return;
    }

    const SceneDefinition selectedScene = m_scenes.at(currentRow);
    if (QMessageBox::question(this,
                              QStringLiteral("\u786e\u8ba4\u5220\u9664"),
                              QStringLiteral("\u786e\u5b9a\u8981\u5220\u9664\u573a\u666f\u201c") + selectedScene.name + QStringLiteral("\u201d\u5417\uff1f")) != QMessageBox::Yes)
    {
        return;
    }

    if (!m_sceneService.deleteScene(selectedScene))
    {
        QMessageBox::critical(this, kTextFailed, QStringLiteral("\u573a\u666f\u5220\u9664\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u6570\u636e\u5e93\u8fde\u63a5\u72b6\u6001\u3002"));
        return;
    }

    loadScenesFromDatabase();
}

void SceneWidget::on_btnActivateScene_clicked()
{
    const int currentRow = ui->listWidget_scenes->currentRow();
    if (currentRow < 0 || currentRow >= m_scenes.size())
    {
        QMessageBox::warning(this, kTextHint, QStringLiteral("\u8bf7\u5148\u9009\u62e9\u573a\u666f\u3002"));
        return;
    }

    const SceneDefinition scene = m_scenes.at(currentRow);
    QMessageBox::information(this,
                             QStringLiteral("\u573a\u666f\u6fc0\u6d3b"),
                             QStringLiteral("\u6b63\u5728\u6267\u884c\u573a\u666f\uff1a") + scene.name + QStringLiteral("\n\n\u8bf7\u6c42\u6307\u4ee4\u5df2\u53d1\u9001\u81f3\u7f51\u7edc\u5c42\u3002"));

    const QJsonObject sceneCmd = m_sceneService.buildTriggerSceneCommand(scene);
    emit requestTriggerScene(sceneCmd);
}

void SceneWidget::on_btnAddDeviceToScene_clicked()
{
    const int currentRow = ui->listWidget_scenes->currentRow();
    if (currentRow < 0 || currentRow >= m_scenes.size())
    {
        QMessageBox::warning(this, kTextHint, QStringLiteral("\u8bf7\u5148\u9009\u62e9\u4e00\u4e2a\u573a\u666f\u3002"));
        return;
    }

    SceneDeviceAction action;
    if (!openActionDialog(&action, QStringLiteral("\u6dfb\u52a0\u8bbe\u5907\u5230\u573a\u666f")))
    {
        return;
    }

    const SceneDefinition scene = m_scenes.at(currentRow);
    if (!m_sceneService.addDeviceAction(scene, action))
    {
        QMessageBox::critical(this, kTextFailed, QStringLiteral("\u6dfb\u52a0\u8bbe\u5907\u52a8\u4f5c\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u6570\u636e\u5e93\u8fde\u63a5\u6216\u8bbe\u5907\u4fe1\u606f\u3002"));
        return;
    }

    loadScenesFromDatabase(scene.id);
}

void SceneWidget::on_btnRemoveDevice_clicked()
{
    const int sceneRow = ui->listWidget_scenes->currentRow();
    if (sceneRow < 0 || sceneRow >= m_scenes.size())
    {
        QMessageBox::warning(this, kTextHint, QStringLiteral("\u8bf7\u5148\u9009\u62e9\u573a\u666f\u3002"));
        return;
    }

    const int actionRow = ui->tableWidget_devices->currentRow();
    if (actionRow < 0 || actionRow >= m_scenes.at(sceneRow).actions.size())
    {
        QMessageBox::warning(this, kTextHint, QStringLiteral("\u8bf7\u5148\u5728\u8868\u683c\u4e2d\u9009\u62e9\u8bbe\u5907\u52a8\u4f5c\u3002"));
        return;
    }

    const SceneDefinition scene = m_scenes.at(sceneRow);
    const SceneDeviceAction action = scene.actions.at(actionRow);
    if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4\u79fb\u9664"), QStringLiteral("\u786e\u5b9a\u79fb\u9664\u8be5\u8bbe\u5907\u52a8\u4f5c\u5417\uff1f")) != QMessageBox::Yes)
    {
        return;
    }

    if (!m_sceneService.removeDeviceAction(scene, action))
    {
        QMessageBox::critical(this, kTextFailed, QStringLiteral("\u79fb\u9664\u8bbe\u5907\u52a8\u4f5c\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u6570\u636e\u5e93\u8fde\u63a5\u72b6\u6001\u3002"));
        return;
    }

    loadScenesFromDatabase(scene.id);
}

void SceneWidget::on_btnEditDeviceInScene_clicked()
{
    const int row = ui->tableWidget_devices->currentRow();
    if (row < 0)
    {
        QMessageBox::warning(this, kTextHint, QStringLiteral("请先在表格中选择要修改的设备动作。"));
        return;
    }

    editSelectedAction(row, 0);
}

void SceneWidget::editSelectedScene()
{
    const int currentRow = ui->listWidget_scenes->currentRow();
    if (currentRow < 0 || currentRow >= m_scenes.size())
    {
        return;
    }

    SceneDefinition scene = m_scenes.at(currentRow);
    if (!openSceneDialog(&scene, QStringLiteral("\u7f16\u8f91\u573a\u666f")))
    {
        return;
    }

    if (!m_sceneService.updateScene(scene))
    {
        QMessageBox::critical(this, kTextFailed, QStringLiteral("\u573a\u666f\u66f4\u65b0\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u6570\u636e\u5e93\u8fde\u63a5\u72b6\u6001\u3002"));
        return;
    }

    loadScenesFromDatabase(scene.id);
}

void SceneWidget::editSelectedAction(int row, int column)
{
    Q_UNUSED(column);

    const int sceneRow = ui->listWidget_scenes->currentRow();
    if (sceneRow < 0 || sceneRow >= m_scenes.size())
    {
        return;
    }

    const SceneDefinition scene = m_scenes.at(sceneRow);
    if (row < 0 || row >= scene.actions.size())
    {
        return;
    }

    const SceneDeviceAction originalAction = scene.actions.at(row);
    SceneDeviceAction updatedAction = originalAction;
    if (!openActionDialog(&updatedAction, QStringLiteral("\u7f16\u8f91\u8bbe\u5907\u52a8\u4f5c")))
    {
        return;
    }

    if (!m_sceneService.updateDeviceAction(scene, originalAction, updatedAction))
    {
        QMessageBox::critical(this, kTextFailed, QStringLiteral("\u8bbe\u5907\u52a8\u4f5c\u66f4\u65b0\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u6570\u636e\u5e93\u8fde\u63a5\u6216\u8bbe\u5907\u4fe1\u606f\u3002"));
        return;
    }

    loadScenesFromDatabase(scene.id, originalAction.recordId);
}
