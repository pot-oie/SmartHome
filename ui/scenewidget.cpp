#include "scenewidget.h"
#include "ui_scenewidget.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIcon>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QTableWidgetItem>

namespace
{
    const QString kTextHint = QStringLiteral("\u63d0\u793a");
    const QString kTextFailed = QStringLiteral("\u5931\u8d25");
    const QString kTextSuccess = QStringLiteral("\u6210\u529f");
    const QString kTextSceneNamePrefix = QStringLiteral("\u573a\u666f\u540d\u79f0\uff1a");

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
    for (const SceneDefinition &scene : m_scenes)
    {
        QListWidgetItem *item = new QListWidgetItem(scene.name);
        item->setIcon(QIcon(scene.icon));
        ui->listWidget_scenes->addItem(item);
    }
}

void SceneWidget::renderSceneDetails(const SceneDefinition &scene)
{
    ui->label_sceneName->setText(kTextSceneNamePrefix + scene.name);
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
    form->addRow(QStringLiteral("\u9009\u62e9\u8bbe\u5907:"), cmbDevice);

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
    form->addRow(QStringLiteral("\u6267\u884c\u52a8\u4f5c:"), cmbAction);

    QLineEdit *editParam = new QLineEdit(action->paramText, &dialog);
    editParam->setPlaceholderText(QStringLiteral("\u4f8b\u5982\uff1a24C \u6216 80%"));
    form->addRow(QStringLiteral("\u52a8\u4f5c\u53c2\u6570:"), editParam);

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
                              QStringLiteral("\u786e\u5b9a\u8981\u5220\u9664\u573a\u666f\u201c") + selectedScene.name + QStringLiteral("\u201d\u5417\uff1f"))
        != QMessageBox::Yes)
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
