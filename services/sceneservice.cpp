#include "sceneservice.h"

#include "database/dao/SceneDao.h"

#include <QJsonArray>

namespace
{
    SceneDeviceAction makeAction(const QString &deviceId, const QString &deviceName, const QString &actionText, const QString &paramText = QString())
    {
        return {deviceId, deviceName, actionText, paramText};
    }

    int parseParamValue(const QString &paramText)
    {
        QString raw = paramText;
        raw.remove('%').remove("°C").remove("亮度:");
        return raw.trimmed().toInt();
    }
}

SceneList SceneService::loadDefaultScenes() const
{
    SceneDao dao;
    const SceneList scenesFromDb = dao.listScenesWithActions();
    if (!scenesFromDb.isEmpty())
    {
        return scenesFromDb;
    }

    return {
        {"scene_go_home", "回家模式", ":/icons/home.svg", "欢迎回家！自动开启照明、空调并打开窗帘", {makeAction("light_living", "客厅主灯", "开启", "80%"), makeAction("curtain_living", "客厅窗帘", "开启", "100%"), makeAction("ac_living", "客厅空调", "开启", "24°C"), makeAction("lock_door", "前门智能锁", "解锁")}},

        {"scene_sleep", "睡眠模式", ":/icons/bedtime.svg", "晚安！关闭所有照明，窗帘合上，空调调至舒适温度", {makeAction("light_bedroom", "卧室灯", "关闭"), makeAction("light_living", "客厅主灯", "关闭"), makeAction("curtain_bedroom", "卧室窗帘", "关闭", "0%"), makeAction("ac_bedroom", "卧室空调", "开启", "26°C")}},

        {"scene_movie", "影院模式", ":/icons/movie.svg", "享受电影时光！调暗灯光，关闭窗帘", {makeAction("light_living", "客厅主灯", "关闭"), makeAction("curtain_living", "客厅窗帘", "关闭", "0%"), makeAction("tv_living", "客厅电视", "开启")}},

        {"scene_party", "派对模式", ":/icons/celebration.svg", "派对时间！所有灯光全开", {makeAction("light_living", "客厅主灯", "开启", "100%"), makeAction("light_bedroom", "卧室灯", "开启", "80%"), makeAction("light_kitchen", "厨房灯", "开启", "100%"), makeAction("tv_living", "客厅电视", "开启")}},

        {"scene_wakeup", "起床模式", ":/icons/wb_sunny.svg", "早安！缓缓开启灯光和窗帘，帮助您自然苏醒", {makeAction("light_bedroom", "卧室灯", "开启", "60%"), makeAction("curtain_bedroom", "卧室窗帘", "开启", "80%"), makeAction("ac_bedroom", "卧室空调", "关闭")}},

        {"scene_leave_home", "离家模式", ":/icons/flight_takeoff.svg", "安全离家！关闭所有电器，开启安防监控", {makeAction("light_all", "所有灯光", "关闭"), makeAction("curtain_all", "所有窗帘", "关闭"), makeAction("ac_living", "客厅空调", "关闭"), makeAction("ac_bedroom", "卧室空调", "关闭"), makeAction("lock_door", "前门智能锁", "上锁")}}};
}

SceneDefinition SceneService::createCustomScene(const QString &sceneName) const
{
    return createScene(sceneName, "自定义场景", ":/icons/scene.svg");
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

bool SceneService::addDeviceAction(const SceneDefinition &scene, const SceneDeviceAction &action) const
{
    if (scene.id.trimmed().isEmpty())
    {
        return false;
    }

    SceneDao dao;
    return dao.insertSceneAction(scene.id.trimmed(), action);
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

        if (!action.paramText.isEmpty())
        {
            cmd["command"] = "set_param";
            cmd["param_value"] = parseParamValue(action.paramText);
        }
        else
        {
            cmd["command"] = (action.actionText == "开启" || action.actionText == "解锁") ? "turn_on" : "turn_off";
        }

        commandsArray.append(cmd);
    }

    dataObj["commands"] = commandsArray;
    sceneCmd["data"] = dataObj;
    return sceneCmd;
}
