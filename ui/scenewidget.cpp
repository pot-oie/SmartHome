#include "scenewidget.h"
#include "ui_scenewidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QListWidgetItem>
#include <QJsonObject>
#include <QJsonArray>
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
    ui->listWidget_scenes->clear();

    struct SceneData
    {
        QString name;
        QString icon;
    };
    QVector<SceneData> scenes = {
        {"回家模式", ":/icons/home.svg"},
        {"睡眠模式", ":/icons/bedtime.svg"},
        {"影院模式", ":/icons/movie.svg"},
        {"派对模式", ":/icons/celebration.svg"},
        {"起床模式", ":/icons/wb_sunny.svg"},
        {"离家模式", ":/icons/flight_takeoff.svg"}};

    for (const auto &scene : scenes)
    {
        QListWidgetItem *item = new QListWidgetItem(scene.name);
        item->setIcon(QIcon(scene.icon));
        ui->listWidget_scenes->addItem(item);
    }

    if (ui->listWidget_scenes->count() > 0)
    {
        ui->listWidget_scenes->setCurrentRow(0);
        updateSceneDetails(0);
    }

    connect(ui->listWidget_scenes, &QListWidget::currentRowChanged, this, &SceneWidget::updateSceneDetails);
}

void SceneWidget::updateSceneDetails(int row)
{
    if (row < 0)
        return;

    // 动态读取当前场景名称
    QString currentSceneName = ui->listWidget_scenes->item(row)->text();
    ui->label_sceneName->setText("场景名称：" + currentSceneName);

    ui->tableWidget_devices->clearContents();
    ui->tableWidget_devices->setRowCount(0);

    QStringList devices;
    QString description;

    switch (row)
    {
    case 0: // 回家模式
        devices << "客厅主灯,开启,80%" << "客厅窗帘,开启,100%" << "客厅空调,开启,24°C" << "前门智能锁,解锁,";
        description = "欢迎回家！自动开启照明、空调并打开窗帘";
        break;
    case 1: // 睡眠模式
        devices << "卧室灯,关闭," << "客厅主灯,关闭," << "卧室窗帘,关闭,0%" << "卧室空调,开启,26°C";
        description = "晚安！关闭所有照明，窗帘合上，空调调至舒适温度";
        break;
    case 2: // 影院模式
        devices << "客厅主灯,关闭," << "客厅窗帘,关闭,0%" << "客厅电视,开启,";
        description = "享受电影时光！调暗灯光，关闭窗帘";
        break;
    case 3: // 派对模式
        devices << "客厅主灯,开启,100%" << "卧室灯,开启,80%" << "厨房灯,开启,100%" << "客厅电视,开启,";
        description = "派对时间！所有灯光全开";
        break;
    case 4: // 起床模式
        devices << "卧室灯,开启,60%" << "卧室窗帘,开启,80%" << "卧室空调,关闭,";
        description = "早安！缓缓开启灯光和窗帘，帮助您自然苏醒";
        break;
    case 5: // 离家模式
        devices << "所有灯光,关闭," << "所有窗帘,关闭," << "客厅空调,关闭," << "卧室空调,关闭," << "前门智能锁,上锁,";
        description = "安全离家！关闭所有电器，开启安防监控";
        break;
    default:
        description = "请选择一个场景";
    }

    ui->label_sceneDesc->setText(description);

    for (const QString &deviceStr : devices)
    {
        QStringList parts = deviceStr.split(',');
        if (parts.size() >= 2)
        {
            int r = ui->tableWidget_devices->rowCount();
            ui->tableWidget_devices->insertRow(r);
            ui->tableWidget_devices->setItem(r, 0, new QTableWidgetItem(parts[0]));
            ui->tableWidget_devices->setItem(r, 1, new QTableWidgetItem(parts[1]));
            ui->tableWidget_devices->setItem(r, 2, new QTableWidgetItem(parts.size() > 2 ? parts[2] : ""));
        }
    }
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
        QListWidgetItem *item = new QListWidgetItem(sceneName);
        item->setIcon(QIcon(":/icons/scene.svg"));
        ui->listWidget_scenes->addItem(item);
        QMessageBox::information(this, "成功", "场景 \"" + sceneName + "\" 已添加！");
    }
}

void SceneWidget::on_btnDeleteScene_clicked()
{
    QListWidgetItem *item = ui->listWidget_scenes->currentItem();
    if (item)
    {
        if (QMessageBox::question(this, "确认删除", "确定要删除场景 \"" + item->text() + "\" 吗？") == QMessageBox::Yes)
        {
            delete item;
        }
    }
}

void SceneWidget::on_btnActivateScene_clicked()
{
    QListWidgetItem *item = ui->listWidget_scenes->currentItem();
    if (!item)
        return;

    QMessageBox::information(this, "场景激活", "正在执行场景：" + item->text() + "\n\n请求指令已发送至网络层！");

    QJsonObject sceneCmd;
    sceneCmd["action"] = "trigger_scene";
    QJsonObject dataObj;
    dataObj["scene_id"] = "scene_" + item->text();

    QJsonArray commandsArray;
    for (int i = 0; i < ui->tableWidget_devices->rowCount(); ++i)
    {
        QJsonObject cmd;
        QTableWidgetItem *nameItem = ui->tableWidget_devices->item(i, 0);
        QTableWidgetItem *actionItem = ui->tableWidget_devices->item(i, 1);
        QTableWidgetItem *paramItem = ui->tableWidget_devices->item(i, 2);

        if (nameItem && actionItem)
        {
            cmd["device_id"] = nameItem->text();
            cmd["command"] = (actionItem->text() == "开启") ? "turn_on" : "turn_off";
            if (paramItem && !paramItem->text().isEmpty())
            {
                cmd["command"] = "set_param";
                QString rawParam = paramItem->text();
                rawParam.remove("%").remove("°C").remove("亮度: ");
                cmd["param_value"] = rawParam.toInt();
            }
            commandsArray.append(cmd);
        }
    }
    dataObj["commands"] = commandsArray;
    sceneCmd["data"] = dataObj;
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
        int row = ui->tableWidget_devices->rowCount();
        ui->tableWidget_devices->insertRow(row);
        ui->tableWidget_devices->setItem(row, 0, new QTableWidgetItem(cmbDevice->currentText()));
        ui->tableWidget_devices->setItem(row, 1, new QTableWidgetItem(cmbAction->currentText()));
        ui->tableWidget_devices->setItem(row, 2, new QTableWidgetItem(editParam->text()));
    }
}