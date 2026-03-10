#include "sceneservice.h"

#include "database/dao/DeviceDao.h"
#include "database/dao/SceneDao.h"

#include <QJsonArray>
#include <QRegularExpression>

namespace
{
    const QString kActionEnable = QStringLiteral("\u5f00\u542f");
    const QString kActionDisable = QStringLiteral("\u5173\u95ed");
    const QString kActionOpen = QStringLiteral("\u6253\u5f00");
    const QString kActionUnlock = QStringLiteral("\u89e3\u9501");
    const QString kActionLock = QStringLiteral("\u4e0a\u9501");

    SceneDeviceAction makeAction(const QString &deviceId, const QString &deviceName, const QString &actionText, const QString &paramText = QString())
    {
        SceneDeviceAction action;
        action.deviceId = deviceId;
        action.deviceName = deviceName;
        action.actionText = actionText;
        action.paramText = paramText;
        return action;
    }

    SceneList builtInScenes()
    {
        return {
            {"scene_go_home", QStringLiteral("\u56de\u5bb6\u6a21\u5f0f"), ":/icons/home.svg", QStringLiteral("\u6b22\u8fce\u56de\u5bb6\uff0c\u81ea\u52a8\u5f00\u542f\u7167\u660e\u3001\u7a7a\u8c03\u5e76\u6253\u5f00\u7a97\u5e18"),
             {makeAction("light_living", QStringLiteral("\u5ba2\u5385\u4e3b\u706f"), kActionEnable, "80%"),
              makeAction("curtain_living", QStringLiteral("\u5ba2\u5385\u7a97\u5e18"), kActionEnable, "100%"),
              makeAction("ac_living", QStringLiteral("\u5ba2\u5385\u7a7a\u8c03"), kActionEnable, "24C")}},

            {"scene_sleep", QStringLiteral("\u7761\u7720\u6a21\u5f0f"), ":/icons/bedtime.svg", QStringLiteral("\u5173\u95ed\u4e3b\u7167\u660e\uff0c\u62c9\u4e0a\u7a97\u5e18\uff0c\u5e76\u628a\u5367\u5ba4\u7a7a\u8c03\u8c03\u5230\u8212\u9002\u6e29\u5ea6"),
             {makeAction("light_bedroom", QStringLiteral("\u5367\u5ba4\u706f"), kActionDisable),
              makeAction("light_living", QStringLiteral("\u5ba2\u5385\u4e3b\u706f"), kActionDisable),
              makeAction("curtain_bedroom", QStringLiteral("\u5367\u5ba4\u7a97\u5e18"), kActionDisable, "0%"),
              makeAction("ac_bedroom", QStringLiteral("\u5367\u5ba4\u7a7a\u8c03"), kActionEnable, "26C")}},

            {"scene_movie", QStringLiteral("\u5f71\u9662\u6a21\u5f0f"), ":/icons/movie.svg", QStringLiteral("\u8c03\u6697\u706f\u5149\u3001\u5173\u95ed\u7a97\u5e18\uff0c\u8425\u9020\u89c2\u5f71\u73af\u5883"),
             {makeAction("light_living", QStringLiteral("\u5ba2\u5385\u4e3b\u706f"), kActionDisable),
              makeAction("curtain_living", QStringLiteral("\u5ba2\u5385\u7a97\u5e18"), kActionDisable, "0%"),
              makeAction("tv_living", QStringLiteral("\u5ba2\u5385\u7535\u89c6"), kActionEnable)}},

            {"scene_party", QStringLiteral("\u6d3e\u5bf9\u6a21\u5f0f"), ":/icons/celebration.svg", QStringLiteral("\u5f00\u542f\u4e3b\u8981\u7167\u660e\u548c\u5a31\u4e50\u8bbe\u5907"),
             {makeAction("light_living", QStringLiteral("\u5ba2\u5385\u4e3b\u706f"), kActionEnable, "100%"),
              makeAction("light_bedroom", QStringLiteral("\u5367\u5ba4\u706f"), kActionEnable, "80%"),
              makeAction("light_kitchen", QStringLiteral("\u53a8\u623f\u706f"), kActionEnable, "100%"),
              makeAction("tv_living", QStringLiteral("\u5ba2\u5385\u7535\u89c6"), kActionEnable)}},

            {"scene_wakeup", QStringLiteral("\u8d77\u5e8a\u6a21\u5f0f"), ":/icons/wb_sunny.svg", QStringLiteral("\u9010\u6b65\u5f00\u542f\u7167\u660e\u548c\u7a97\u5e18\uff0c\u5e2e\u52a9\u81ea\u7136\u5524\u9192"),
             {makeAction("light_bedroom", QStringLiteral("\u5367\u5ba4\u706f"), kActionEnable, "60%"),
              makeAction("curtain_bedroom", QStringLiteral("\u5367\u5ba4\u7a97\u5e18"), kActionEnable, "80%"),
              makeAction("ac_bedroom", QStringLiteral("\u5367\u5ba4\u7a7a\u8c03"), kActionDisable)}},

            {"scene_leave_home", QStringLiteral("\u79bb\u5bb6\u6a21\u5f0f"), ":/icons/flight_takeoff.svg", QStringLiteral("\u5173\u95ed\u5173\u952e\u8bbe\u5907\u5e76\u9501\u95e8"),
             {makeAction("light_living", QStringLiteral("\u5ba2\u5385\u4e3b\u706f"), kActionDisable),
              makeAction("light_bedroom", QStringLiteral("\u5367\u5ba4\u706f"), kActionDisable),
              makeAction("curtain_living", QStringLiteral("\u5ba2\u5385\u7a97\u5e18"), kActionDisable, "0%"),
              makeAction("ac_living", QStringLiteral("\u5ba2\u5385\u7a7a\u8c03"), kActionDisable),
              makeAction("lock_door", QStringLiteral("\u524d\u95e8\u667a\u80fd\u9501"), kActionLock)}}};
    }

    void seedBuiltInScenes(SceneDao &dao)
    {
        const SceneList defaults = builtInScenes();
        for (const SceneDefinition &defaultScene : defaults)
        {
            const SceneList currentScenes = dao.listScenesWithActions();
            bool exists = false;
            for (const SceneDefinition &current : currentScenes)
            {
                if (current.id == defaultScene.id)
                {
                    exists = true;
                    break;
                }
            }

            if (exists)
            {
                continue;
            }

            SceneDefinition scene = defaultScene;
            if (!dao.insertScene(scene) || scene.id.isEmpty())
            {
                continue;
            }

            for (const SceneDeviceAction &action : defaultScene.actions)
            {
                dao.insertSceneAction(scene.id, action);
            }
        }
    }

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
        return trimmed == kActionEnable
               || trimmed == kActionOpen
               || trimmed == kActionUnlock
               || lower == "on"
               || lower == "open"
               || lower == "unlock";
    }
}

SceneList SceneService::loadDefaultScenes() const
{
    SceneDao dao;
    SceneList scenesFromDb = dao.listScenesWithActions();
    if (!scenesFromDb.isEmpty())
    {
        return scenesFromDb;
    }

    seedBuiltInScenes(dao);
    scenesFromDb = dao.listScenesWithActions();
    return scenesFromDb;
}

SettingsDeviceList SceneService::loadAvailableDevices() const
{
    DeviceDao dao;
    return dao.listSettingsDevices();
}

SceneDefinition SceneService::createCustomScene(const QString &sceneName) const
{
    return createScene(sceneName, QStringLiteral("\u81ea\u5b9a\u4e49\u573a\u666f"), ":/icons/scene.svg");
}

SceneDefinition SceneService::createScene(const QString &sceneName, const QString &sceneDescription, const QString &iconPath) const
{
    SceneDefinition scene;
    scene.id.clear();
    scene.name = sceneName.trimmed();
    scene.icon = iconPath.trimmed().isEmpty() ? QString(":/icons/scene.svg") : iconPath.trimmed();
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

QJsonObject SceneService::buildTriggerSceneCommand(const SceneDefinition &scene) const
{
    QJsonObject sceneCmd;
    sceneCmd["action"] = "trigger_scene";

    QJsonObject dataObj;
    dataObj["scene_id"] = scene.id;

    QJsonArray commandsArray;
    for (const SceneDeviceAction &action : scene.actions)
    {
        QJsonObject cmd;
        cmd["device_id"] = action.deviceId;

        if (!action.paramText.trimmed().isEmpty())
        {
            cmd["command"] = "set_param";
            cmd["param_value"] = parseParamValue(action.paramText);
        }
        else
        {
            cmd["command"] = isTurnOnAction(action.actionText) ? "turn_on" : "turn_off";
        }

        commandsArray.append(cmd);
    }

    dataObj["commands"] = commandsArray;
    sceneCmd["data"] = dataObj;
    return sceneCmd;
}
