#include "scenewidget.h"
#include "ui_scenewidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QListWidgetItem>
#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>

SceneWidget::SceneWidget(QWidget *parent) : QWidget(parent),
                                            ui(new Ui::SceneWidget)
{
    ui->setupUi(this);
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
    bool ok;
    QString sceneName = QInputDialog::getText(this, "添加场景", "请输入场景名称：", QLineEdit::Normal, "", &ok);
    if (ok && !sceneName.isEmpty())
    {
        m_scenes.push_back(m_sceneService.createCustomScene(sceneName));
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
    if (QMessageBox::question(this, "确认删除", "确定要删除场景 \"" + sceneName + "\" 吗？") == QMessageBox::Yes)
    {
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
        scene.actions.push_back(action);

        renderSceneDetails(scene);
    }
}