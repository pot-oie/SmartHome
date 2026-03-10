#include "scenewidget.h"
#include "ui_scenewidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>

SceneWidget::SceneWidget(QWidget *parent) : QWidget(parent),
                                            ui(new Ui::SceneWidget)
{
    ui->setupUi(this);
    ui->tableWidget_devices->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_devices->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_devices->setSelectionMode(QAbstractItemView::SingleSelection);
    loadScenesFromDatabase();
}

SceneWidget::~SceneWidget()
{
    delete ui;
}

void SceneWidget::loadScenesFromDatabase()
{
    m_scenes = m_sceneService.loadDefaultScenes();
    renderSceneList();

    disconnect(ui->listWidget_scenes, &QListWidget::currentRowChanged, this, &SceneWidget::updateSceneDetails);
    connect(ui->listWidget_scenes, &QListWidget::currentRowChanged, this, &SceneWidget::updateSceneDetails);

    if (ui->listWidget_scenes->count() > 0)
    {
        ui->listWidget_scenes->setCurrentRow(0);
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
    ui->label_sceneName->setText("场景名称：" + scene.name);
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
    qDebug() << "场景执行结果：" << resultData;
}

void SceneWidget::on_btnAddScene_clicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("添加场景");
    dialog.resize(360, 240);

    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *editName = new QLineEdit(&dialog);
    editName->setPlaceholderText("例如：晚餐模式");
    form->addRow("场景名称:", editName);

    QLineEdit *editDesc = new QLineEdit(&dialog);
    editDesc->setPlaceholderText("例如：开启餐厅灯并关闭客厅电视");
    form->addRow("场景描述:", editDesc);

    QComboBox *cmbIcon = new QComboBox(&dialog);
    cmbIcon->addItem(QIcon(":/icons/home.svg"), "回家", ":/icons/home.svg");
    cmbIcon->addItem(QIcon(":/icons/bedtime.svg"), "睡眠", ":/icons/bedtime.svg");
    cmbIcon->addItem(QIcon(":/icons/movie.svg"), "观影", ":/icons/movie.svg");
    cmbIcon->addItem(QIcon(":/icons/flight_takeoff.svg"), "离家", ":/icons/flight_takeoff.svg");
    cmbIcon->addItem(QIcon(":/icons/celebration.svg"), "派对", ":/icons/celebration.svg");
    cmbIcon->addItem(QIcon(":/icons/wb_sunny.svg"), "起床", ":/icons/wb_sunny.svg");
    cmbIcon->addItem(QIcon(":/icons/restaurant.svg"), "用餐", ":/icons/restaurant.svg");
    cmbIcon->addItem(QIcon(":/icons/self_improvement.svg"), "冥想", ":/icons/self_improvement.svg");
    cmbIcon->addItem(QIcon(":/icons/sports_esports.svg"), "游戏", ":/icons/sports_esports.svg");
    cmbIcon->addItem(QIcon(":/icons/cleaning_services.svg"), "清洁", ":/icons/cleaning_services.svg");
    cmbIcon->addItem(QIcon(":/icons/pets.svg"), "宠物", ":/icons/pets.svg");
    cmbIcon->addItem(QIcon(":/icons/music.svg"), "音乐", ":/icons/music.svg");
    cmbIcon->addItem(QIcon(":/icons/scene.svg"), "通用", ":/icons/scene.svg");
    form->addRow("场景图标:", cmbIcon);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        const QString sceneName = editName->text().trimmed();
        const QString sceneDesc = editDesc->text().trimmed();
        const QString sceneIcon = cmbIcon->currentData().toString();

        if (sceneName.isEmpty() || sceneDesc.isEmpty())
        {
            QMessageBox::warning(this, "提示", "场景名称、场景描述均为必填项。");
            return;
        }

        const SceneDefinition createdScene = m_sceneService.createScene(sceneName, sceneDesc, sceneIcon);
        if (createdScene.id.isEmpty())
        {
            QMessageBox::critical(this, "失败", "场景保存失败，请检查数据库连接状态。");
            return;
        }

        m_scenes.push_back(createdScene);
        renderSceneList();
        ui->listWidget_scenes->setCurrentRow(m_scenes.size() - 1);
        QMessageBox::information(this, "成功", "场景 \"" + sceneName + "\" 已添加！");
    }
}

void SceneWidget::on_btnDeleteScene_clicked()
{
    const int currentRow = ui->listWidget_scenes->currentRow();
    if (currentRow < 0 || currentRow >= m_scenes.size())
    {
        return;
    }

    const QString sceneName = m_scenes.at(currentRow).name;
    const SceneDefinition selectedScene = m_scenes.at(currentRow);
    if (QMessageBox::question(this, "确认删除", "确定要删除场景 \"" + sceneName + "\" 吗？") == QMessageBox::Yes)
    {
        if (!m_sceneService.deleteScene(selectedScene))
        {
            QMessageBox::critical(this, "失败", "场景删除失败，请检查数据库连接状态。");
            return;
        }

        m_scenes.removeAt(currentRow);
        renderSceneList();
        if (!m_scenes.isEmpty())
        {
            const int nextRow = (currentRow < m_scenes.size()) ? currentRow : (m_scenes.size() - 1);
            ui->listWidget_scenes->setCurrentRow(nextRow);
        }
    }
}

void SceneWidget::on_btnActivateScene_clicked()
{
    const int currentRow = ui->listWidget_scenes->currentRow();
    if (currentRow < 0 || currentRow >= m_scenes.size())
        return;

    const SceneDefinition scene = m_scenes.at(currentRow);

    QMessageBox::information(this, "场景激活", "正在执行场景：" + scene.name + "\n\n请求指令已发送至网络层！");

    QJsonObject sceneCmd = m_sceneService.buildTriggerSceneCommand(scene);
    emit requestTriggerScene(sceneCmd);
}

void SceneWidget::on_btnAddDeviceToScene_clicked()
{
    // 动态创建并弹出“设备绑定”弹窗
    QListWidgetItem *item = ui->listWidget_scenes->currentItem();
    if (!item)
    {
        QMessageBox::warning(this, "提示", "请先选择一个场景！");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("添加设备到场景 - " + item->text());
    dialog.resize(320, 200);

    QFormLayout *form = new QFormLayout(&dialog);

    QComboBox *cmbDevice = new QComboBox(&dialog);
    cmbDevice->addItems({"客厅主灯", "卧室灯", "客厅空调", "卧室窗帘", "前门智能锁", "客厅电视"});
    form->addRow("选择设备:", cmbDevice);

    QComboBox *cmbAction = new QComboBox(&dialog);
    cmbAction->addItems({"开启", "关闭"});
    form->addRow("执行动作:", cmbAction);

    QLineEdit *editParam = new QLineEdit(&dialog);
    editParam->setPlaceholderText("如: 24°C 或 80% (选填)");
    form->addRow("动作参数:", editParam);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        const int currentRow = ui->listWidget_scenes->currentRow();
        if (currentRow < 0 || currentRow >= m_scenes.size())
        {
            return;
        }

        SceneDefinition &scene = m_scenes[currentRow];
        SceneDeviceAction action;
        action.deviceName = cmbDevice->currentText();
        action.deviceId = action.deviceName;
        action.actionText = cmbAction->currentText();
        action.paramText = editParam->text();

        if (!m_sceneService.addDeviceAction(scene, action))
        {
            QMessageBox::critical(this, "失败", "添加设备动作失败，请检查数据库连接或设备映射。");
            return;
        }

        scene.actions.push_back(action);

        renderSceneDetails(scene);
    }
}

void SceneWidget::on_btnRemoveDevice_clicked()
{
    const int sceneRow = ui->listWidget_scenes->currentRow();
    if (sceneRow < 0 || sceneRow >= m_scenes.size())
    {
        QMessageBox::warning(this, "提示", "请先选择场景。");
        return;
    }

    const int actionRow = ui->tableWidget_devices->currentRow();
    if (actionRow < 0 || actionRow >= m_scenes[sceneRow].actions.size())
    {
        QMessageBox::warning(this, "提示", "请先在表格中选择要移除的设备动作。");
        return;
    }

    SceneDefinition &scene = m_scenes[sceneRow];
    const SceneDeviceAction action = scene.actions.at(actionRow);
    if (QMessageBox::question(this, "确认移除", "确定移除该设备动作吗？") != QMessageBox::Yes)
    {
        return;
    }

    if (!m_sceneService.removeDeviceAction(scene, action))
    {
        QMessageBox::critical(this, "失败", "移除设备动作失败，请检查数据库连接状态。");
        return;
    }

    scene.actions.removeAt(actionRow);
    renderSceneDetails(scene);
}