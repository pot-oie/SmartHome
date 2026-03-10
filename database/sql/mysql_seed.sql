-- SmartHome MySQL 8.0 演示数据脚本
USE smarthome;

SET NAMES utf8mb4;

START TRANSACTION;

INSERT INTO users (id, username, password_hash, display_name, role, status, created_at, updated_at)
VALUES
    (1, 'admin', '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9', '管理员', 'super_admin', 'active', '2026-03-09 09:00:00', '2026-03-09 09:00:00'),
    (2, 'zhangsan', 'd4ca56bd2f7d836cdf3830ad6e2eb9627a9c5b771a4a797fe418f3a59dde0a4c', '张三', 'user', 'active', '2026-03-09 09:05:00', '2026-03-09 09:05:00'),
    (3, 'lisi', '51ea015c9749e0ee553be2296ed036bfafc871e1fb2f8691896fef17f3e952fa', '李四', 'user', 'active', '2026-03-09 09:10:00', '2026-03-09 09:10:00')
ON DUPLICATE KEY UPDATE
    password_hash = VALUES(password_hash),
    display_name = VALUES(display_name),
    role = VALUES(role),
    status = VALUES(status),
    updated_at = VALUES(updated_at);

INSERT INTO device_categories (id, category_code, category_name, display_order, is_enabled, created_at, updated_at)
VALUES
    (1, 'light', '照明设备', 1, 1, now(), now()),
    (2, 'air_conditioner', '空调设备', 2, 1, now(), now()),
    (3, 'curtain', '窗帘设备', 3, 1, now(), now()),
    (4, 'security', '安防设备', 4, 1, now(), now()),
    (5, 'media', '影音设备', 5, 1, now(), now()),
    (6, 'sensor', '传感设备', 6, 1, now(), now())
ON DUPLICATE KEY UPDATE
    category_name = VALUES(category_name),
    display_order = VALUES(display_order),
    is_enabled = VALUES(is_enabled),
    updated_at = VALUES(updated_at);

INSERT INTO devices (
    id, device_id, category_id, device_name, device_type, room_name, protocol_type, manufacturer,
    ip_address, online_status, switch_status, current_value, value_unit, supports_slider,
    slider_min, slider_max, display_order, is_home_quick_control, quick_control_key, remarks,
    last_seen_at, last_control_at, created_at, updated_at
)
VALUES
    (1, 'light_hallway', 1, '玄关灯', '照明设备', '玄关', 'simulator', 'SmartHome Lab', '192.168.1.101', 'online', 'on', 80.00, '%', 1, 0.00, 100.00, 1, 1, 'btnQuickLight', '首页快捷控制', now(), now(), now(), now()),
    (2, 'light_bedroom', 1, '卧室灯', '照明设备', '卧室', 'simulator', 'SmartHome Lab', '192.168.1.102', 'online', 'off', 0.00, '%', 1, 0.00, 100.00, 2, 0, null, '睡眠场景设备', now(), now(), now(), now()),
    (3, 'ac_living_room', 2, '客厅空调', '空调设备', '客厅', 'simulator', 'SmartHome Lab', '192.168.1.103', 'online', 'on', 24.00, '°C', 1, 16.00, 30.00, 3, 1, 'btnQuickAC', '首页快捷控制', now(), now(), now(), now()),
    (4, 'curtain_living_room', 3, '客厅窗帘', '窗帘设备', '客厅', 'simulator', 'SmartHome Lab', '192.168.1.104', 'online', 'on', 100.00, '%', 1, 0.00, 100.00, 4, 1, 'btnQuickCurtain', '首页快捷控制', now(), now(), now(), now()),
    (5, 'lock_door', 4, '前门智能锁', '安防设备', '玄关', 'simulator', 'SmartHome Lab', '192.168.1.105', 'online', 'on', 1.00, null, 0, null, null, 5, 1, 'btnQuickDoor', '门锁设备', now(), now(), now(), now()),
    (6, 'camera_01', 4, '客厅摄像头', '安防设备', '客厅', 'simulator', 'SmartHome Lab', '192.168.1.106', 'offline', 'off', 0.00, null, 0, null, null, 6, 0, null, '用于状态推送演示', now(), now(), now(), now()),
    (7, 'tv_living', 5, '客厅电视', '影音设备', '客厅', 'simulator', 'SmartHome Lab', '192.168.1.107', 'online', 'off', 20.00, '%', 1, 0.00, 100.00, 7, 1, 'btnQuickTV', '影音设备', now(), now(), now(), now()),
    (8, 'sensor_env_living', 6, '客厅环境传感器', '传感设备', '客厅', 'simulator', 'SmartHome Lab', '192.168.1.108', 'online', 'on', 25.50, '°C', 0, null, null, 8, 0, null, '环境监控', now(), now(), now(), now()),
    (9, 'curtain_bedroom', 3, '卧室窗帘', '窗帘设备', '卧室', 'simulator', 'SmartHome Lab', '192.168.1.109', 'online', 'on', 80.00, '%', 1, 0.00, 100.00, 9, 0, null, '起床场景设备', now(), now(), now(), now()),
    (10, 'ac_bedroom', 2, '卧室空调', '空调设备', '卧室', 'simulator', 'SmartHome Lab', '192.168.1.110', 'offline', 'off', 26.00, '°C', 1, 16.00, 30.00, 10, 0, null, '离线演示设备', now(), now(), now(), now()),
    (11, 'sensor_bedroom_01', 6, '卧室传感器', '传感设备', '卧室', 'simulator', 'SmartHome Lab', '192.168.1.111', 'online', 'on', 23.80, '°C', 0, null, null, 11, 0, null, 'Ping 测试设备', now(), now(), now(), now())
ON DUPLICATE KEY UPDATE
    category_id = VALUES(category_id),
    device_name = VALUES(device_name),
    device_type = VALUES(device_type),
    room_name = VALUES(room_name),
    protocol_type = VALUES(protocol_type),
    manufacturer = VALUES(manufacturer),
    ip_address = VALUES(ip_address),
    online_status = VALUES(online_status),
    switch_status = VALUES(switch_status),
    current_value = VALUES(current_value),
    value_unit = VALUES(value_unit),
    supports_slider = VALUES(supports_slider),
    slider_min = VALUES(slider_min),
    slider_max = VALUES(slider_max),
    display_order = VALUES(display_order),
    is_home_quick_control = VALUES(is_home_quick_control),
    quick_control_key = VALUES(quick_control_key),
    remarks = VALUES(remarks),
    last_seen_at = VALUES(last_seen_at),
    last_control_at = VALUES(last_control_at),
    updated_at = VALUES(updated_at);

INSERT INTO device_state_snapshots (
    id, device_id, online_status, switch_status, current_value, value_unit, mode_text,
    battery_level, signal_rssi, power_watt, last_reported_at, extra_payload, created_at, updated_at
)
VALUES
    (1, 1, 'online', 'on', 80.00, '%', '常亮', 98.00, -42, 18.50, now(), JSON_OBJECT('brightness', 80), now(), now()),
    (2, 2, 'online', 'off', 0.00, '%', '关闭', 97.00, -48, 0.00, now(), JSON_OBJECT('brightness', 0), now(), now()),
    (3, 3, 'online', 'on', 24.00, '°C', '制冷', 99.00, -40, 920.00, now(), JSON_OBJECT('target_temperature', 24), now(), now()),
    (4, 4, 'online', 'on', 100.00, '%', '全开', 95.00, -44, 42.00, now(), JSON_OBJECT('open_percent', 100), now(), now()),
    (5, 5, 'online', 'on', 1.00, null, '已上锁', 88.00, -52, 1.50, now(), JSON_OBJECT('lock_state', 'locked'), now(), now()),
    (6, 6, 'offline', 'off', 0.00, null, '离线', 55.00, null, 0.00, now(), JSON_OBJECT('reason', 'connection_lost'), now(), now()),
    (7, 7, 'online', 'off', 20.00, '%', '待机', 100.00, -41, 12.00, now(), JSON_OBJECT('volume', 20), now(), now()),
    (8, 8, 'online', 'on', 25.50, '°C', '空气质量良好', 100.00, -38, 2.00, now(), JSON_OBJECT('humidity', 60.2, 'pm25', 28.0, 'co2', 620.0), now(), now()),
    (9, 9, 'online', 'on', 80.00, '%', '已开启', 96.00, -43, 26.00, now(), JSON_OBJECT('open_percent', 80), now(), now()),
    (10, 10, 'offline', 'off', 26.00, '°C', '离线', 70.00, null, 0.00, now(), JSON_OBJECT('last_target_temperature', 26), now(), now()),
    (11, 11, 'online', 'on', 23.80, '°C', '正常', 100.00, -39, 1.00, now(), JSON_OBJECT('humidity', 55.0, 'pm25', 20.0, 'co2', 650.0), now(), now())
ON DUPLICATE KEY UPDATE
    online_status = VALUES(online_status),
    switch_status = VALUES(switch_status),
    current_value = VALUES(current_value),
    value_unit = VALUES(value_unit),
    mode_text = VALUES(mode_text),
    battery_level = VALUES(battery_level),
    signal_rssi = VALUES(signal_rssi),
    power_watt = VALUES(power_watt),
    last_reported_at = VALUES(last_reported_at),
    extra_payload = VALUES(extra_payload),
    updated_at = VALUES(updated_at);

INSERT INTO scenes (
    id, scene_code, scene_name, scene_description, scene_icon, welcome_text, trigger_key,
    is_default, is_enabled, display_order, created_at, updated_at
)
VALUES
    (1, 'scene_go_home', '回家模式', '开启玄关灯、客厅空调并打开客厅窗帘', 'home', '欢迎回家', 'btnQuickGoHome', 1, 1, 1, now(), now()),
    (2, 'sleep', '睡眠模式', '关闭照明并调整卧室设备', 'sleep', '晚安', null, 0, 1, 2, now(), now()),
    (3, 'movie', '影院模式', '关闭窗帘并打开电视', 'movie', '影院模式已准备就绪', null, 0, 1, 3, now(), now()),
    (4, 'party', '派对模式', '开启照明和影音设备', 'party', '派对模式已激活', null, 0, 1, 4, now(), now()),
    (5, 'wake_up', '起床模式', '打开卧室灯和卧室窗帘', 'sun', '早安', null, 0, 1, 5, now(), now()),
    (6, 'away', '离家模式', '关闭灯光空调并上锁', 'away', '离家模式已启动', null, 0, 1, 6, now(), now())
ON DUPLICATE KEY UPDATE
    scene_name = VALUES(scene_name),
    scene_description = VALUES(scene_description),
    scene_icon = VALUES(scene_icon),
    welcome_text = VALUES(welcome_text),
    trigger_key = VALUES(trigger_key),
    is_default = VALUES(is_default),
    is_enabled = VALUES(is_enabled),
    display_order = VALUES(display_order),
    updated_at = VALUES(updated_at);

INSERT INTO scene_actions (
    id, scene_id, device_id, action_name, action_param, command_code, param_name, param_value, action_order, created_at, updated_at
)
VALUES
    (1, 1, 1, '开启', '80%', 'turn_on', 'brightness', '80', 1, now(), now()),
    (2, 1, 4, '开启', '100%', 'set_param', 'open_percent', '100', 2, now(), now()),
    (3, 1, 3, '开启', '24°C', 'set_param', 'temperature', '24', 3, now(), now()),
    (4, 2, 2, '关闭', null, 'turn_off', null, null, 1, now(), now()),
    (5, 2, 9, '关闭', '0%', 'set_param', 'open_percent', '0', 2, now(), now()),
    (6, 2, 10, '开启', '26°C', 'set_param', 'temperature', '26', 3, now(), now()),
    (7, 3, 4, '关闭', '0%', 'set_param', 'open_percent', '0', 1, now(), now()),
    (8, 3, 7, '开启', null, 'turn_on', null, null, 2, now(), now()),
    (9, 4, 1, '开启', '100%', 'set_param', 'brightness', '100', 1, now(), now()),
    (10, 4, 7, '开启', '35%', 'set_param', 'volume', '35', 2, now(), now()),
    (11, 5, 2, '开启', '60%', 'set_param', 'brightness', '60', 1, now(), now()),
    (12, 5, 9, '开启', '80%', 'set_param', 'open_percent', '80', 2, now(), now()),
    (13, 6, 1, '关闭', null, 'turn_off', null, null, 1, now(), now()),
    (14, 6, 3, '关闭', null, 'turn_off', null, null, 2, now(), now()),
    (15, 6, 4, '关闭', '0%', 'set_param', 'open_percent', '0', 3, now(), now()),
    (16, 6, 5, '上锁', null, 'lock', 'lock_state', 'locked', 4, now(), now())
ON DUPLICATE KEY UPDATE
    action_name = VALUES(action_name),
    action_param = VALUES(action_param),
    command_code = VALUES(command_code),
    param_name = VALUES(param_name),
    param_value = VALUES(param_value),
    action_order = VALUES(action_order),
    updated_at = VALUES(updated_at);

INSERT INTO env_realtime_snapshots (
    id, location_code, location_name, source_device_id, temperature, humidity, pm25, co2,
    status_level, record_source, updated_at, created_at
)
VALUES
    (1, 'living_room', '客厅', 8, 25.50, 60.20, 28.00, 620.00, 'normal', 'simulator', now(), now()),
    (2, 'bedroom', '卧室', 11, 23.80, 55.00, 20.00, 650.00, 'normal', 'simulator', now(), now())
ON DUPLICATE KEY UPDATE
    temperature = VALUES(temperature),
    humidity = VALUES(humidity),
    pm25 = VALUES(pm25),
    co2 = VALUES(co2),
    status_level = VALUES(status_level),
    record_source = VALUES(record_source),
    updated_at = VALUES(updated_at);

INSERT INTO env_records (
    id, location_code, location_name, source_device_id, temperature, humidity, pm25, co2, record_source, created_at
)
VALUES
    (1, 'living_room', '客厅', 8, 24.90, 58.10, 24.00, 580.00, 'simulator', '2026-03-09 14:00:00'),
    (2, 'living_room', '客厅', 8, 25.20, 59.20, 26.00, 600.00, 'simulator', '2026-03-09 15:00:00'),
    (3, 'living_room', '客厅', 8, 25.50, 60.20, 28.00, 620.00, 'simulator', '2026-03-09 15:30:00'),
    (4, 'bedroom', '卧室', 11, 23.40, 54.00, 19.00, 640.00, 'simulator', '2026-03-09 14:00:00'),
    (5, 'bedroom', '卧室', 11, 23.80, 55.00, 20.00, 650.00, 'simulator', '2026-03-09 15:30:00')
ON DUPLICATE KEY UPDATE
    temperature = VALUES(temperature),
    humidity = VALUES(humidity),
    pm25 = VALUES(pm25),
    co2 = VALUES(co2),
    created_at = VALUES(created_at);

INSERT INTO device_telemetry_records (
    id, device_id, metric_code, metric_name, metric_value_decimal, metric_value_text, metric_unit, quality_flag, source_type, recorded_at, created_at
)
VALUES
    (1, 1, 'brightness', '灯光亮度', 80.00, 'on', '%', 'good', 'simulator', now(), now()),
    (2, 3, 'temperature', '空调设定温度', 24.00, 'on', '°C', 'good', 'simulator', now(), now()),
    (3, 4, 'open_percent', '窗帘开合度', 100.00, 'on', '%', 'good', 'simulator', now(), now()),
    (4, 6, 'online_status', '在线状态', 0.00, 'offline', null, 'bad', 'simulator', now(), now()),
    (5, 11, 'temperature', '环境温度', 23.80, 'normal', '°C', 'good', 'simulator', now(), now())
ON DUPLICATE KEY UPDATE
    metric_value_decimal = VALUES(metric_value_decimal),
    metric_value_text = VALUES(metric_value_text),
    recorded_at = VALUES(recorded_at),
    created_at = VALUES(created_at);

INSERT INTO scene_executions (
    id, scene_id, triggered_by, trigger_source, execution_status, result_summary,
    request_payload, response_payload, started_at, finished_at, created_at, updated_at
)
VALUES
    (1, 1, 3, 'manual', 'success', '回家模式执行成功',
     JSON_OBJECT('scene_id', 'scene_go_home'),
     JSON_OBJECT('code', 200, 'message', 'Scene executed successfully'),
     now(), now(), now(), now())
ON DUPLICATE KEY UPDATE
    execution_status = VALUES(execution_status),
    result_summary = VALUES(result_summary),
    request_payload = VALUES(request_payload),
    response_payload = VALUES(response_payload),
    finished_at = VALUES(finished_at),
    updated_at = VALUES(updated_at);

INSERT INTO scene_execution_details (
    id, execution_id, device_id, action_name, action_param, execution_status, result_message, executed_at, created_at
)
VALUES
    (1, 1, 1, '开启', '80%', 'success', '玄关灯已开启', now(), now()),
    (2, 1, 4, '开启', '100%', 'success', '客厅窗帘已打开', now(), now()),
    (3, 1, 3, '开启', '24°C', 'success', '客厅空调已设为24°C', now(), now())
ON DUPLICATE KEY UPDATE
    execution_status = VALUES(execution_status),
    result_message = VALUES(result_message),
    executed_at = VALUES(executed_at);

INSERT INTO operation_logs (
    id, msg_id, user_id, scene_id, device_id, operator_name, device_name_snapshot,
    module_name, operation_type, operation_content, result, result_code,
    request_payload, response_payload, created_at
)
VALUES
    (1, 'msg-202603091530000001', 3, 1, null, '李四', null, 'scene', 'trigger_scene', '触发回家模式', 'success', 200,
     JSON_OBJECT('scene_id', 'scene_go_home'),
     JSON_OBJECT('code', 200, 'message', 'Scene executed successfully'), now()),
    (2, 'msg-202603091520000002', 2, null, 3, '张三', '客厅空调', 'device_control', 'set_param', '设置客厅空调为24°C', 'success', 200,
     JSON_OBJECT('device_id', 'ac_living_room', 'command', 'set_param', 'param_name', 'temperature', 'param_value', 24),
     JSON_OBJECT('code', 200, 'message', 'success'), now()),
    (3, 'msg-202603091510000003', 1, null, 6, '管理员', '客厅摄像头', 'device_control', 'ping_device', '测试客厅摄像头连通性', 'failed', 400,
     JSON_OBJECT('device_id', 'camera_01'),
     JSON_OBJECT('code', 400, 'latency', 'timeout'), now())
ON DUPLICATE KEY UPDATE
    result = VALUES(result),
    result_code = VALUES(result_code),
    request_payload = VALUES(request_payload),
    response_payload = VALUES(response_payload),
    created_at = VALUES(created_at);

INSERT INTO alarm_rules (
    id, rule_code, rule_name, metric_code, comparator, threshold_value, threshold_unit,
    scope_type, device_id, severity, cooldown_seconds, is_enabled, description, created_at, updated_at
)
VALUES
    (1, 'temp_high', '温度过高', 'temperature', 'gt', 30.00, '°C', 'global', null, 'warning', 300, 1, '环境温度高于30°C时触发', now(), now()),
    (2, 'temp_low', '温度过低', 'temperature', 'lt', 18.00, '°C', 'global', null, 'warning', 300, 1, '环境温度低于18°C时触发', now(), now()),
    (3, 'humidity_high', '湿度过高', 'humidity', 'gt', 70.00, '%', 'global', null, 'warning', 300, 1, '环境湿度高于70%时触发', now(), now()),
    (4, 'humidity_low', '湿度过低', 'humidity', 'lt', 30.00, '%', 'global', null, 'warning', 300, 1, '环境湿度低于30%时触发', now(), now()),
    (5, 'pm25_high', 'PM2.5超标', 'pm25', 'gt', 75.00, 'ug/m3', 'global', null, 'critical', 300, 1, 'PM2.5 高于 75 时触发', now(), now()),
    (6, 'co2_high', 'CO2浓度过高', 'co2', 'gt', 1000.00, 'ppm', 'global', null, 'critical', 300, 1, 'CO2 高于 1000ppm 时触发', now(), now()),
    (7, 'device_offline', '设备离线', 'online_status', 'eq', 0.00, null, 'global', null, 'critical', 60, 1, '设备离线时触发', now(), now())
ON DUPLICATE KEY UPDATE
    threshold_value = VALUES(threshold_value),
    severity = VALUES(severity),
    cooldown_seconds = VALUES(cooldown_seconds),
    is_enabled = VALUES(is_enabled),
    description = VALUES(description),
    updated_at = VALUES(updated_at);

INSERT INTO alarm_records (
    id, alarm_code, alarm_type, alarm_content, severity, source_device_id, source_location,
    trigger_metric, trigger_value_decimal, trigger_display_text, trigger_unit, handled_status,
    is_active, alarm_source, extra_payload, handled_at, cleared_at, created_at
)
VALUES
    (1, 'device_offline', '设备离线', '客厅摄像头当前离线', 'critical', 6, '客厅',
     'online_status', 0.00, 'offline', null, 'pending', 1, 'simulator', JSON_OBJECT('rule_code', 'device_offline'), null, null, now()),
    (2, 'temp_high', '温度过高', '客厅温度达到31.5°C', 'warning', 8, '客厅',
     'temperature', 31.50, '31.5', '°C', 'handled', 0, 'simulator', JSON_OBJECT('rule_code', 'temp_high'), now(), now(), now())
ON DUPLICATE KEY UPDATE
    severity = VALUES(severity),
    handled_status = VALUES(handled_status),
    is_active = VALUES(is_active),
    handled_at = VALUES(handled_at),
    cleared_at = VALUES(cleared_at),
    created_at = VALUES(created_at);

INSERT INTO system_configs (
    id, config_name, theme, language,
    temperature_low_threshold, temperature_high_threshold,
    humidity_low_threshold, humidity_high_threshold,
    pm25_high_threshold, co2_high_threshold,
    simulator_enabled, simulator_interval_seconds, history_retention_days,
    default_scene_code, system_status_level, system_status_text,
    updated_by, created_at, updated_at
)
VALUES
    (1, 'default', '浅色主题', '简体中文',
     18.00, 30.00, 30.00, 70.00, 75.00, 1000.00,
     1, 5, 30, 'scene_go_home', 'normal', '系统正常，无报警',
     1, now(), now())
ON DUPLICATE KEY UPDATE
    theme = VALUES(theme),
    language = VALUES(language),
    temperature_low_threshold = VALUES(temperature_low_threshold),
    temperature_high_threshold = VALUES(temperature_high_threshold),
    humidity_low_threshold = VALUES(humidity_low_threshold),
    humidity_high_threshold = VALUES(humidity_high_threshold),
    pm25_high_threshold = VALUES(pm25_high_threshold),
    co2_high_threshold = VALUES(co2_high_threshold),
    simulator_enabled = VALUES(simulator_enabled),
    simulator_interval_seconds = VALUES(simulator_interval_seconds),
    history_retention_days = VALUES(history_retention_days),
    default_scene_code = VALUES(default_scene_code),
    system_status_level = VALUES(system_status_level),
    system_status_text = VALUES(system_status_text),
    updated_by = VALUES(updated_by),
    updated_at = VALUES(updated_at);

COMMIT;
