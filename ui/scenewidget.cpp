#include "scenewidget.h"
#include "ui_scenewidget.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QTableWidgetItem>

namespace
{
    void populateSceneIconCombo(QComboBox *comboBox)
    {
        comboBox->addItem(QIcon(":/icons/home.svg"), QString::fromUtf8("回家"), ":/icons/home.svg");
        comboBox->addItem(QIcon(":/icons/bedtime.svg"), QString::fromUtf8("睡眠"), ":/icons/bedtime.svg");
        comboBox->addItem(QIcon(":/icons/movie.svg"), QString::fromUtf8("观影"), ":/icons/movie.svg");
        comboBox->addItem(QIcon(":/icons/flight_takeoff.svg"), QString::fromUtf8("离家"), ":/icons/flight_takeoff.svg");
        comboBox->addItem(QIcon(":/icons/celebration.svg"), QString::fromUtf8("派对"), ":/icons/celebration.svg");
        comboBox->addItem(QIcon(":/icons/wb_sunny.svg"), QString::fromUtf8("起床"), ":/icons/wb_sunny.svg");
        comboBox->addItem(QIcon(":/icons/restaurant.svg"), QString::fromUtf8("用餐"), ":/icons/restaurant.svg");
        comboBox->addItem(QIcon(":/icons/self_improvement.svg"), QString::fromUtf8("冥想"), ":/icons/self_improvement.svg");
        comboBox->addItem(QIcon(":/icons/sports_esports.svg"), QString::fromUtf8("游戏"), ":/icons/sports_esports.svg");
        comboBox->addItem(QIcon(":/icons/cleaning_services.svg"), QString::fromUtf8("清洁"), ":/icons/cleaning_services.svg");
        comboBox->addItem(QIcon(":/icons/pets.svg"), QString::fromUtf8("宠物"), ":/icons/pets.svg");
        comboBox->addItem(QIcon(":/icons/music.svg"), QString::fromUtf8("音乐"), ":/icons/music.svg");
        comboBox->addItem(QIcon(":/icons/scene.svg"), QString::fromUtf8("通用"), ":/icons/scene.svg");
    }

    void populateActionCombo(QComboBox *comboBox)
    {
        comboBox->addItem(QString::fromUtf8("开启"));
        comboBox->addItem(QString::fromUtf8("关闭"));
        comboBox->addItem(QString::fromUtf8("打开"));
        comboBox->addItem(QString::fromUtf8("解锁"));
        comboBox->addItem(QString::fromUtf8("上锁"));
    }
}

SceneWidget::SceneWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SceneWidget)
{
    ui->setupUi(this);
    ui->tableWidget_devices->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_devices->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_devices->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(ui->listWidget_scenes, &QListWidget::currentRowChanged, this, &SceneWidget::updateSceneDetails);
    connect(ui->listWidget_scenes, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
        editSelectedScene();
    });
    connect(ui->tableWidget_devices, &QTableWidget::cellDoubleClicked, this, &SceneWidget::editSelectedAction);

    loadScenesFromDatabase();
}

SceneWidget::~SceneWidget()
{
    delete ui;
}

void SceneWidget::loadScenesFromDatabase(const QString &sceneCodeToSelect, qint64 actionIdToSelect)
{
    m_scenes = m_sceneService.loadDefaultScenes();
    renderSceneList();

    if (m_scenes.isEmpty())
    {
        ui->label_sceneName->setText(QString::fromUtf8("场景名称："));
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
    for (const SceneDefinition &scene : m_scenes)
    {
        QListWidgetItem *item = new QListWidgetItem(scene.name);
        item->setIcon(QIcon(scene.icon));
        ui->listWidget_scenes->addItem(item);
    }
}

void SceneWidget::renderSceneDetails(const SceneDefinition &scene)
{
    ui->label_sceneName->setText(QString::fromUtf8("场景名称：") + scene.name);
    ui->label_sceneDesc->setText(scene.description);

    ui->tableWidget_devices->clearContents();
    ui->tableWidget_devices->setRowCount(0);

    for (const SceneDeviceAction &action : scene.actions)
    {
        const int row = ui->tableWidget_devices->rowCount();
        ui->tableWidget_devices->insertRow(row);
        ui->tableWidget_devices->setItem(row, 0, new QTableWidgetItem(action.deviceName));
        ui->tableWidget_devices->setItem(row, 1, new QTableWidgetItem(action.actionText));
        ui->tableWidget_devices->setItem(row, 2, new QTableWidgetItem(action.paramText));
    }
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
    editName->setPlaceholderText(QString::fromUtf8("例如：晚餐模式"));
    form->addRow(QString::fromUtf8("场景名称:"), editName);

    QLineEdit *editDesc = new QLineEdit(scene->description, &dialog);
    editDesc->setPlaceholderText(QString::fromUtf8("例如：开启餐厅灯并关闭客厅电视"));
    form->addRow(QString::fromUtf8("场景描述:"), editDesc);

    QComboBox *cmbIcon = new QComboBox(&dialog);
    populateSceneIconCombo(cmbIcon);
    const int iconIndex = cmbIcon->findData(scene->icon);
    cmbIcon->setCurrentIndex(iconIndex >= 0 ? iconIndex : cmbIcon->count() - 1);
    form->addRow(QString::fromUtf8("场景图标:"), cmbIcon);

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
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("场景名称和场景描述均为必填项。"));
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
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("当前没有可用设备，请先到系统设置中维护设备。"));
        return false;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.resize(340, 200);

    QFormLayout *form = new QFormLayout(&dialog);

    QComboBox *cmbDevice = new QComboBox(&dialog);
    for (const SettingsDeviceEntry &device : devices)
    {
        cmbDevice->addItem(device.name, device.id);
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
    form->addRow(QString::fromUtf8("选择设备:"), cmbDevice);

    QComboBox *cmbAction = new QComboBox(&dialog);
    populateActionCombo(cmbAction);
    int actionIndex = cmbAction->findText(action->actionText);
    if (actionIndex < 0 && !action->actionText.trimmed().isEmpty())
    {
        cmbAction->addItem(action->actionText);
        actionIndex = cmbAction->count() - 1;
    }
    if (actionIndex >= 0)
    {
        cmbAction->setCurrentIndex(actionIndex);
    }
    form->addRow(QString::fromUtf8("执行动作:"), cmbAction);

    QLineEdit *editParam = new QLineEdit(action->paramText, &dialog);
    editParam->setPlaceholderText(QString::fromUtf8("例如：24°C 或 80%"));
    form->addRow(QString::fromUtf8("动作参数:"), editParam);

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
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请选择设备。"));
        return false;
    }

    action->deviceName = cmbDevice->currentText();
    action->deviceId = cmbDevice->currentData().toString();
    action->actionText = cmbAction->currentText().trimmed();
    action->paramText = editParam->text().trimmed();
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
    qDebug() << "场景执行结果:" << resultData;
}

void SceneWidget::on_btnAddScene_clicked()
{
    SceneDefinition scene;
    scene.icon = ":/icons/scene.svg";
    if (!openSceneDialog(&scene, QString::fromUtf8("添加场景")))
    {
        return;
    }

    const SceneDefinition createdScene = m_sceneService.createScene(scene.name, scene.description, scene.icon);
    if (createdScene.id.isEmpty())
    {
        QMessageBox::critical(this, QString::fromUtf8("失败"), QString::fromUtf8("场景保存失败，请检查数据库连接状态。"));
        return;
    }

    loadScenesFromDatabase(createdScene.id);
    QMessageBox::information(this, QString::fromUtf8("成功"), QString::fromUtf8("场景已添加。"));
}

void SceneWidget::on_btnDeleteScene_clicked()
{
    const int currentRow = ui->listWidget_scenes->currentRow();
    if (currentRow < 0 || currentRow >= m_scenes.size())
    {
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择场景。"));
        return;
    }

    const SceneDefinition selectedScene = m_scenes.at(currentRow);
    if (QMessageBox::question(this,
                              QString::fromUtf8("确认删除"),
                              QString::fromUtf8("确定要删除场景“") + selectedScene.name + QString::fromUtf8("”吗？"))
        != QMessageBox::Yes)
    {
        return;
    }

    if (!m_sceneService.deleteScene(selectedScene))
    {
        QMessageBox::critical(this, QString::fromUtf8("失败"), QString::fromUtf8("场景删除失败，请检查数据库连接状态。"));
        return;
    }

    loadScenesFromDatabase();
}

void SceneWidget::on_btnActivateScene_clicked()
{
    const int currentRow = ui->listWidget_scenes->currentRow();
    if (currentRow < 0 || currentRow >= m_scenes.size())
    {
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择场景。"));
        return;
    }

    const SceneDefinition scene = m_scenes.at(currentRow);
    QMessageBox::information(this,
                             QString::fromUtf8("场景激活"),
                             QString::fromUtf8("正在执行场景：") + scene.name + QString::fromUtf8("\n\n请求指令已发送至网络层。"));

    const QJsonObject sceneCmd = m_sceneService.buildTriggerSceneCommand(scene);
    emit requestTriggerScene(sceneCmd);
}

void SceneWidget::on_btnAddDeviceToScene_clicked()
{
    const int currentRow = ui->listWidget_scenes->currentRow();
    if (currentRow < 0 || currentRow >= m_scenes.size())
    {
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择一个场景。"));
        return;
    }

    SceneDeviceAction action;
    if (!openActionDialog(&action, QString::fromUtf8("添加设备到场景")))
    {
        return;
    }

    const SceneDefinition scene = m_scenes.at(currentRow);
    if (!m_sceneService.addDeviceAction(scene, action))
    {
        QMessageBox::critical(this, QString::fromUtf8("失败"), QString::fromUtf8("添加设备动作失败，请检查数据库连接或设备信息。"));
        return;
    }

    loadScenesFromDatabase(scene.id);
}

void SceneWidget::on_btnRemoveDevice_clicked()
{
    const int sceneRow = ui->listWidget_scenes->currentRow();
    if (sceneRow < 0 || sceneRow >= m_scenes.size())
    {
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择场景。"));
        return;
    }

    const int actionRow = ui->tableWidget_devices->currentRow();
    if (actionRow < 0 || actionRow >= m_scenes.at(sceneRow).actions.size())
    {
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先在表格中选择设备动作。"));
        return;
    }

    const SceneDefinition scene = m_scenes.at(sceneRow);
    const SceneDeviceAction action = scene.actions.at(actionRow);
    if (QMessageBox::question(this, QString::fromUtf8("确认移除"), QString::fromUtf8("确定移除该设备动作吗？")) != QMessageBox::Yes)
    {
        return;
    }

    if (!m_sceneService.removeDeviceAction(scene, action))
    {
        QMessageBox::critical(this, QString::fromUtf8("失败"), QString::fromUtf8("移除设备动作失败，请检查数据库连接状态。"));
        return;
    }

    loadScenesFromDatabase(scene.id);
}

void SceneWidget::editSelectedScene()
{
    const int currentRow = ui->listWidget_scenes->currentRow();
    if (currentRow < 0 || currentRow >= m_scenes.size())
    {
        return;
    }

    SceneDefinition scene = m_scenes.at(currentRow);
    if (!openSceneDialog(&scene, QString::fromUtf8("编辑场景")))
    {
        return;
    }

    if (!m_sceneService.updateScene(scene))
    {
        QMessageBox::critical(this, QString::fromUtf8("失败"), QString::fromUtf8("场景更新失败，请检查数据库连接状态。"));
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
    if (!openActionDialog(&updatedAction, QString::fromUtf8("编辑设备动作")))
    {
        return;
    }

    if (!m_sceneService.updateDeviceAction(scene, originalAction, updatedAction))
    {
        QMessageBox::critical(this, QString::fromUtf8("失败"), QString::fromUtf8("设备动作更新失败，请检查数据库连接或设备信息。"));
        return;
    }

    loadScenesFromDatabase(scene.id, originalAction.recordId);
}
