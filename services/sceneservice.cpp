#include "sceneservice.h"

#include "database/dao/DeviceDao.h"
#include "database/dao/SceneDao.h"
#include "deviceservice.h"

#include <QHash>
#include <QRegularExpression>

namespace
{
    const QString kActionEnable = QStringLiteral("开启");
    const QString kActionOpen = QStringLiteral("打开");
    const QString kActionUnlock = QStringLiteral("解锁");

    int parseParamValue(const QString &paramText)
    {
        QString raw = paramText;
        raw.remove(QRegularExpression("[^\\d-]"));
        return raw.toInt();
    }

    bool isTurnOnAction(const QString &actionText)
    {
        const QString trimmed = actionText.trimmed();
        const QString lower = trimmed.toLower();
        return trimmed == kActionEnable || trimmed == kActionOpen || trimmed == kActionUnlock || lower == "on" || lower == "open" || lower == "unlock";
    }
}

SceneList SceneService::loadScenes() const
{
    SceneDao dao;
    return dao.listScenesWithActions();
}

SettingsDeviceList SceneService::loadAvailableDevices() const
{
    DeviceDao dao;
    return dao.listSettingsDevices();
}

SceneDefinition SceneService::createCustomScene(const QString &sceneName) const
{
    return createScene(sceneName, QStringLiteral("自定义场景"), QStringLiteral(":/icons/scene.svg"));
}

SceneDefinition SceneService::createScene(const QString &sceneName, const QString &sceneDescription, const QString &iconPath) const
{
    SceneDefinition scene;
    scene.id.clear();
    scene.name = sceneName.trimmed();
    scene.icon = iconPath.trimmed().isEmpty() ? QStringLiteral(":/icons/scene.svg") : iconPath.trimmed();
    scene.description = sceneDescription.trimmed();

    if (scene.name.isEmpty() || scene.description.isEmpty())
    {
        scene.id.clear();
        return scene;
    }

    SceneDao dao;
    if (!dao.insertScene(scene) || scene.id.isEmpty())
    {
        scene.id.clear();
    }

    return scene;
}

bool SceneService::updateScene(const SceneDefinition &scene) const
{
    if (scene.id.trimmed().isEmpty() || scene.name.trimmed().isEmpty() || scene.description.trimmed().isEmpty())
    {
        return false;
    }

    SceneDao dao;
    return dao.updateScene(scene);
}

bool SceneService::addDeviceAction(const SceneDefinition &scene, const SceneDeviceAction &action) const
{
    if (scene.id.trimmed().isEmpty())
    {
        return false;
    }

    SceneDao dao;
    return dao.insertSceneAction(scene.id.trimmed(), action);
}

bool SceneService::updateDeviceAction(const SceneDefinition &scene, const SceneDeviceAction &oldAction, const SceneDeviceAction &newAction) const
{
    if (scene.id.trimmed().isEmpty())
    {
        return false;
    }

    SceneDao dao;
    return dao.updateSceneAction(scene.id.trimmed(), oldAction, newAction);
}

bool SceneService::removeDeviceAction(const SceneDefinition &scene, const SceneDeviceAction &action) const
{
    if (scene.id.trimmed().isEmpty())
    {
        return false;
    }

    SceneDao dao;
    return dao.deleteSceneAction(scene.id.trimmed(), action);
}

bool SceneService::deleteScene(const SceneDefinition &scene) const
{
    if (scene.id.trimmed().isEmpty())
    {
        return false;
    }

    SceneDao dao;
    return dao.deleteSceneByCode(scene.id.trimmed());
}

SceneExecutionResult SceneService::executeScene(const SceneDefinition &scene) const
{
    SceneExecutionResult result;
    result.sceneId = scene.id;
    result.sceneName = scene.name;

    DeviceService deviceService;
    const DeviceList devices = deviceService.loadDevices();
    QHash<QString, DeviceDefinition> deviceIndex;
    for (const DeviceDefinition &device : devices)
    {
        deviceIndex.insert(device.id, device);
    }

    for (const SceneDeviceAction &action : scene.actions)
    {
        SceneActionExecutionResult actionResult;
        actionResult.deviceId = action.deviceId;
        actionResult.deviceName = action.deviceName;

        if (!deviceIndex.contains(action.deviceId))
        {
            actionResult.message = QStringLiteral("设备不存在或未加载到当前数据库结果中");
            result.actionResults.push_back(actionResult);
            ++result.failureCount;
            continue;
        }

        const DeviceDefinition device = deviceIndex.value(action.deviceId);
        if (!device.isOnline)
        {
            actionResult.message = QStringLiteral("设备离线，已跳过");
            result.actionResults.push_back(actionResult);
            ++result.failureCount;
            continue;
        }

        QString errorMessage;
        QString warningMessage;
        bool ok = false;

        if (!action.paramText.trimmed().isEmpty())
        {
            if (!device.supportsSlider)
            {
                actionResult.message = QStringLiteral("设备不支持参数调节");
                result.actionResults.push_back(actionResult);
                ++result.failureCount;
                continue;
            }

            ok = deviceService.updateDeviceValue(device,
                                                 parseParamValue(action.paramText),
                                                 &errorMessage,
                                                 &warningMessage);
        }
        else
        {
            ok = deviceService.updateSwitchState(device.id,
                                                 isTurnOnAction(action.actionText),
                                                 &errorMessage,
                                                 &warningMessage);
        }

        actionResult.success = ok;
        if (ok)
        {
            actionResult.message = warningMessage.trimmed().isEmpty()
                                       ? QStringLiteral("执行成功")
                                       : warningMessage.trimmed();
            ++result.successCount;
        }
        else
        {
            actionResult.message = errorMessage.trimmed().isEmpty()
                                       ? QStringLiteral("执行失败")
                                       : errorMessage.trimmed();
            ++result.failureCount;
        }

        result.actionResults.push_back(actionResult);
    }

    return result;
}
