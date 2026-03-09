-- SmartHome MySQL 8.0.36 演示数据脚本
-- 目标：保证当前 Qt 前端页面可以直接展示首页、设备、场景、历史、报警和系统设置数据。

USE smarthome;

SET NAMES utf8mb4;

START TRANSACTION;

SET @txt_admin = _utf8mb4 0xe7aea1e79086e59198;
SET @txt_zhangsan = _utf8mb4 0xe5bca0e4b889;
SET @txt_lisi = _utf8mb4 0xe69d8ee59b9b;
SET @cat_light = _utf8mb4 0xe781afe58589e8aebee5a487;
SET @cat_ac = _utf8mb4 0xe7a9bae8b083e8aebee5a487;
SET @cat_curtain = _utf8mb4 0xe7aa97e5b898e8aebee5a487;
SET @cat_lock = _utf8mb4 0xe997a8e99481e8aebee5a487;
SET @cat_media = _utf8mb4 0xe5bdb1e99fb3e8aebee5a487;
SET @cat_other = _utf8mb4 0xe585b6e4bb96e8aebee5a487;
SET @dev_living_light = _utf8mb4 0xe5aea2e58e85e4b8bbe781af;
SET @dev_bedroom_light = _utf8mb4 0xe58da7e5aea4e5908ae781af;
SET @dev_living_ac = _utf8mb4 0xe5aea2e58e85e7a9bae8b083;
SET @dev_living_curtain = _utf8mb4 0xe5aea2e58e85e7aa97e5b898;
SET @dev_door_lock = _utf8mb4 0xe699bae883bde997a8e99481;
SET @dev_living_tv = _utf8mb4 0xe5aea2e58e85e794b5e8a786;
SET @dev_entry_light = _utf8mb4 0xe78e84e585b3e6849fe5ba94e781af;
SET @dev_purifier = _utf8mb4 0xe5aea2e58e85e7a9bae6b094e58780e58c96e599a8;
SET @dev_speaker = _utf8mb4 0xe58da7e5aea4e99fb3e7aeb1;
SET @dev_air_sensor = _utf8mb4 0xe7a9bae6b094e8b4a8e9878fe4bca0e6849fe599a8;
SET @type_light = _utf8mb4 0xe781afe58589;
SET @type_ac = _utf8mb4 0xe7a9bae8b083;
SET @type_curtain = _utf8mb4 0xe7aa97e5b898;
SET @type_lock = _utf8mb4 0xe997a8e99481;
SET @type_media = _utf8mb4 0xe5bdb1e99fb3;
SET @type_other = _utf8mb4 0xe585b6e4bb96;
SET @unit_level = _utf8mb4 0xe7baa7;
SET @remark_home_quick = _utf8mb4 0xe9a696e9a1b5e5bfabe68db7e68ea7e588b6e8aebee5a487;
SET @remark_bedroom_demo = _utf8mb4 0xe58da7e5aea4e781afe58589e6bc94e7a4bae8aebee5a487;
SET @remark_lock = _utf8mb4 0xe9a696e9a1b5e5bfabe68db7e68ea7e588b6e8aebee5a487efbc8c6f6e20e8a1a8e7a4bae5b7b2e4b88ae99481;
SET @remark_offline = _utf8mb4 0xe794a8e4ba8ee6bc94e7a4bae7a6bbe7babfe8aebee5a487;
SET @remark_other_demo = _utf8mb4 0xe794a8e4ba8ee6bc94e7a4bae585b6e4bb96e8aebee5a487e58886e7b1bb;
SET @remark_error = _utf8mb4 0xe794a8e4ba8ee6bc94e7a4bae69585e99a9ce8aebee5a487;
SET @remark_env_demo = _utf8mb4 0xe794a8e4ba8ee78eafe5a283e695b0e68daee88194e58aa8e6bc94e7a4ba;
SET @scene_home = _utf8mb4 0xe59b9ee5aeb6e6a8a1e5bc8f;
SET @scene_sleep = _utf8mb4 0xe79da1e79ca0e6a8a1e5bc8f;
SET @scene_movie = _utf8mb4 0xe8a782e5bdb1e6a8a1e5bc8f;
SET @scene_away = _utf8mb4 0xe7a6bbe5aeb6e6a8a1e5bc8f;
SET @scene_desc_home = _utf8mb4 0xe68993e5bc80e5aea2e58e85e4b8bbe781afe38081e7a9bae8b083e38081e794b5e8a786efbc8ce7aa97e5b898e58d8ae5bc80efbc8ce98082e59088e588b0e5aeb6e5908ee79a84e9bb98e8aea4e59cbae699af;
SET @scene_desc_sleep = _utf8mb4 0xe585b3e997ade4b8bbe8a681e785a7e6988eefbc8ce99481e997a8e5b9b6e8b083e695b4e7a9bae8b083e588b0e88892e98082e79da1e79ca0e6b8a9e5baa6;
SET @scene_desc_movie = _utf8mb4 0xe8b083e69a97e781afe58589efbc8ce585b3e997ade7aa97e5b898efbc8ce68993e5bc80e794b5e8a786efbc8ce890a5e980a0e8a782e5bdb1e6b09be59bb4;
SET @scene_desc_away = _utf8mb4 0xe585b3e997ade781afe58589e5928ce7a9bae8b083efbc8ce585b3e997ade7aa97e5b898e5b9b6e99481e997a8;
SET @action_on = _utf8mb4 0xe5bc80e590af;
SET @action_off = _utf8mb4 0xe585b3e997ad;
SET @action_set_open = _utf8mb4 0xe8aebee7bdaee5bc80e59088e5baa6;
SET @param_b80 = _utf8mb4 0xe4baaee5baa63a383025;
SET @param_t24 = _utf8mb4 0xe6b8a9e5baa63a323443;
SET @param_o60 = _utf8mb4 0xe5bc80e59088e5baa63a363025;
SET @param_o0 = _utf8mb4 0xe5bc80e59088e5baa63a3025;
SET @param_v20 = _utf8mb4 0xe99fb3e9878f3a323025;
SET @param_b0 = _utf8mb4 0xe4baaee5baa63a3025;
SET @param_t26 = _utf8mb4 0xe6b8a9e5baa63a323643;
SET @param_lock_on = _utf8mb4 0xe997a8e994813ae5b7b2e4b88ae99481;
SET @param_b20 = _utf8mb4 0xe4baaee5baa63a323025;
SET @param_o10 = _utf8mb4 0xe5bc80e59088e5baa63a313025;
SET @param_v35 = _utf8mb4 0xe99fb3e9878f3a333525;
SET @param_t23 = _utf8mb4 0xe6b8a9e5baa63a323343;
SET @param_t_none = _utf8mb4 0xe6b8a9e5baa63a2d2d;
SET @op_adjust = _utf8mb4 0xe8b083e88a82e58f82e695b0;
SET @op_activate_scene = _utf8mb4 0xe6bf80e6b4bbe59cbae699af;
SET @log_home_light = _utf8mb4 0xe9a696e9a1b5e5bfabe68db7e68ea7e588b6e5bc80e590afe5aea2e58e85e4b8bbe781af;
SET @result_success = _utf8mb4 0xe68890e58a9f;
SET @log_set_ac = _utf8mb4 0xe5b086e5aea2e58e85e7a9bae8b083e8aebee7bdaee4b8ba20323443;
SET @log_activate_home = _utf8mb4 0xe4b880e994aee6bf80e6b4bbe59b9ee5aeb6e6a8a1e5bc8f;
SET @alarm_temp_high = _utf8mb4 0xe6b8a9e5baa6e8bf87e9ab98;
SET @alarm_temp_content = _utf8mb4 0xe5bd93e5898de6b8a9e5baa6e8b685e8bf87e99888e580bc;
SET @handled = _utf8mb4 0xe5b7b2e5a484e79086;
SET @alarm_device_offline = _utf8mb4 0xe8aebee5a487e7a6bbe7babf;
SET @alarm_device_content = _utf8mb4 0xe78e84e585b3e6849fe5ba94e781afe9809ae4bfa1e4b8ade696adefbc8ce5b7b2e8aeb0e5bd95e5be85e5b7a1e6a380;
SET @theme_default = _utf8mb4 0xe9bb98e8aea4e4b8bbe9a298;
SET @lang_zh_cn = _utf8mb4 0xe7ae80e4bd93e4b8ade69687;
SET @status_text_normal = _utf8mb4 0xe7b3bbe7bb9fe8bf90e8a18ce6ada3e5b8b8;

-- 默认用户：1 个超级管理员 + 2 个普通用户
INSERT INTO users (id, username, password_hash, display_name, role, status, created_at, updated_at)
VALUES
    (1, 'admin', 'admin123', @txt_admin, 'super_admin', 'active', '2026-03-09 09:00:00', '2026-03-09 09:00:00'),
    (2, 'zhangsan', 'zhangsan123', @txt_zhangsan, 'user', 'active', '2026-03-09 09:05:00', '2026-03-09 09:05:00'),
    (3, 'lisi', 'lisi123', @txt_lisi, 'user', 'active', '2026-03-09 09:10:00', '2026-03-09 09:10:00')
ON DUPLICATE KEY UPDATE
    password_hash = VALUES(password_hash),
    display_name = VALUES(display_name),
    role = VALUES(role),
    status = VALUES(status),
    updated_at = VALUES(updated_at);

-- 固定设备分类：直接匹配设备控制页左侧列表
INSERT INTO device_categories (id, category_code, category_name, display_order, is_enabled, created_at, updated_at)
VALUES
    (1, 'light', @cat_light, 1, 1, '2026-03-09 09:20:00', '2026-03-09 09:20:00'),
    (2, 'air_conditioner', @cat_ac, 2, 1, '2026-03-09 09:20:00', '2026-03-09 09:20:00'),
    (3, 'curtain', @cat_curtain, 3, 1, '2026-03-09 09:20:00', '2026-03-09 09:20:00'),
    (4, 'door_lock', @cat_lock, 4, 1, '2026-03-09 09:20:00', '2026-03-09 09:20:00'),
    (5, 'media', @cat_media, 5, 1, '2026-03-09 09:20:00', '2026-03-09 09:20:00'),
    (6, 'other', @cat_other, 6, 1, '2026-03-09 09:20:00', '2026-03-09 09:20:00')
ON DUPLICATE KEY UPDATE
    category_name = VALUES(category_name),
    display_order = VALUES(display_order),
    is_enabled = VALUES(is_enabled),
    updated_at = VALUES(updated_at);

-- 默认设备：共 10 台，其中 8 台在线，首页可直接统计为在线 8/10
INSERT INTO devices (
    id, device_id, category_id, device_name, device_type, ip_address, online_status, switch_status,
    current_value, value_unit, supports_slider, slider_min, slider_max, display_order,
    is_home_quick_control, quick_control_key, remarks, created_at, updated_at
)
VALUES
    (1, 'light_living_main', 1, @dev_living_light, @type_light, '192.168.1.101', 'online', 'on', 80.00, '%', 1, 0.00, 100.00, 1, 1, 'btnQuickLight', @remark_home_quick, '2026-03-09 09:30:00', '2026-03-09 09:30:00'),
    (2, 'light_bedroom_ceiling', 1, @dev_bedroom_light, @type_light, '192.168.1.102', 'online', 'off', 50.00, '%', 1, 0.00, 100.00, 2, 0, NULL, @remark_bedroom_demo, '2026-03-09 09:31:00', '2026-03-09 09:31:00'),
    (3, 'ac_living_room', 2, @dev_living_ac, @type_ac, '192.168.1.103', 'online', 'on', 24.00, 'C', 1, 16.00, 30.00, 1, 1, 'btnQuickAC', @remark_home_quick, '2026-03-09 09:32:00', '2026-03-09 09:32:00'),
    (4, 'curtain_living_room', 3, @dev_living_curtain, @type_curtain, '192.168.1.104', 'online', 'on', 60.00, '%', 1, 0.00, 100.00, 1, 1, 'btnQuickCurtain', @remark_home_quick, '2026-03-09 09:33:00', '2026-03-09 09:33:00'),
    (5, 'door_lock_main', 4, @dev_door_lock, @type_lock, '192.168.1.105', 'online', 'on', 1.00, NULL, 0, NULL, NULL, 1, 1, 'btnQuickDoor', @remark_lock, '2026-03-09 09:34:00', '2026-03-09 09:34:00'),
    (6, 'tv_living_room', 5, @dev_living_tv, @type_media, '192.168.1.106', 'online', 'off', 35.00, '%', 1, 0.00, 100.00, 1, 1, 'btnQuickTV', @remark_home_quick, '2026-03-09 09:35:00', '2026-03-09 09:35:00'),
    (7, 'light_entry_sensor', 1, @dev_entry_light, @type_light, '192.168.1.107', 'offline', 'off', 0.00, '%', 1, 0.00, 100.00, 3, 0, NULL, @remark_offline, '2026-03-09 09:36:00', '2026-03-09 09:36:00'),
    (8, 'purifier_living_room', 6, @dev_purifier, @type_other, '192.168.1.108', 'online', 'on', 2.00, @unit_level, 1, 1.00, 3.00, 1, 0, NULL, @remark_other_demo, '2026-03-09 09:37:00', '2026-03-09 09:37:00'),
    (9, 'speaker_bedroom', 5, @dev_speaker, @type_media, '192.168.1.109', 'error', 'off', 15.00, '%', 1, 0.00, 100.00, 2, 0, NULL, @remark_error, '2026-03-09 09:38:00', '2026-03-09 09:38:00'),
    (10, 'sensor_air_quality', 6, @dev_air_sensor, @type_other, '192.168.1.110', 'online', 'on', 0.00, NULL, 0, NULL, NULL, 2, 0, NULL, @remark_env_demo, '2026-03-09 09:39:00', '2026-03-09 09:39:00')
ON DUPLICATE KEY UPDATE
    category_id = VALUES(category_id),
    device_name = VALUES(device_name),
    device_type = VALUES(device_type),
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
    updated_at = VALUES(updated_at);

-- 默认场景：直接匹配场景管理页左侧列表
INSERT INTO scenes (id, scene_code, scene_name, scene_description, is_default, display_order, created_at, updated_at)
VALUES
    (1, 'go_home', @scene_home, @scene_desc_home, 1, 1, '2026-03-09 09:45:00', '2026-03-09 09:45:00'),
    (2, 'sleep', @scene_sleep, @scene_desc_sleep, 1, 2, '2026-03-09 09:46:00', '2026-03-09 09:46:00'),
    (3, 'movie', @scene_movie, @scene_desc_movie, 1, 3, '2026-03-09 09:47:00', '2026-03-09 09:47:00'),
    (4, 'away', @scene_away, @scene_desc_away, 1, 4, '2026-03-09 09:48:00', '2026-03-09 09:48:00')
ON DUPLICATE KEY UPDATE
    scene_name = VALUES(scene_name),
    scene_description = VALUES(scene_description),
    is_default = VALUES(is_default),
    display_order = VALUES(display_order),
    updated_at = VALUES(updated_at);

-- 场景动作：回家模式详情页至少可直接显示 客厅主灯 -> 开启 -> 亮度:80%
INSERT INTO scene_actions (id, scene_id, device_id, action_name, action_param, action_order, created_at)
VALUES
    (1, 1, 1, @action_on, @param_b80, 1, '2026-03-09 09:50:00'),
    (2, 1, 3, @action_on, @param_t24, 2, '2026-03-09 09:50:00'),
    (3, 1, 4, @action_set_open, @param_o60, 3, '2026-03-09 09:50:00'),
    (4, 1, 6, @action_on, @param_v20, 4, '2026-03-09 09:50:00'),
    (5, 2, 2, @action_off, @param_b0, 1, '2026-03-09 09:51:00'),
    (6, 2, 3, @action_on, @param_t26, 2, '2026-03-09 09:51:00'),
    (7, 2, 4, @action_off, @param_o0, 3, '2026-03-09 09:51:00'),
    (8, 2, 5, @action_on, @param_lock_on, 4, '2026-03-09 09:51:00'),
    (9, 3, 1, @action_on, @param_b20, 1, '2026-03-09 09:52:00'),
    (10, 3, 4, @action_set_open, @param_o10, 2, '2026-03-09 09:52:00'),
    (11, 3, 6, @action_on, @param_v35, 3, '2026-03-09 09:52:00'),
    (12, 3, 3, @action_on, @param_t23, 4, '2026-03-09 09:52:00'),
    (13, 4, 1, @action_off, @param_b0, 1, '2026-03-09 09:53:00'),
    (14, 4, 3, @action_off, @param_t_none, 2, '2026-03-09 09:53:00'),
    (15, 4, 4, @action_off, @param_o0, 3, '2026-03-09 09:53:00'),
    (16, 4, 5, @action_on, @param_lock_on, 4, '2026-03-09 09:53:00')
ON DUPLICATE KEY UPDATE
    action_name = VALUES(action_name),
    action_param = VALUES(action_param),
    action_order = VALUES(action_order);

-- 环境数据：最后一条固定为 25.5 和 60.2，首页可直接显示
INSERT INTO env_records (id, temperature, humidity, pm25, co2, created_at)
VALUES
    (1, 24.80, 58.10, 16.00, 430.00, '2026-03-09 13:55:00'),
    (2, 25.00, 58.90, 16.50, 438.00, '2026-03-09 14:00:00'),
    (3, 25.10, 59.20, 17.00, 442.00, '2026-03-09 14:05:00'),
    (4, 25.20, 59.60, 17.20, 448.00, '2026-03-09 14:10:00'),
    (5, 25.30, 59.90, 17.50, 452.00, '2026-03-09 14:15:00'),
    (6, 25.40, 60.00, 17.80, 458.00, '2026-03-09 14:20:00'),
    (7, 25.45, 60.10, 18.00, 462.00, '2026-03-09 14:25:00'),
    (8, 25.50, 60.20, 18.30, 465.00, '2026-03-09 14:30:00')
ON DUPLICATE KEY UPDATE
    temperature = VALUES(temperature),
    humidity = VALUES(humidity),
    pm25 = VALUES(pm25),
    co2 = VALUES(co2),
    created_at = VALUES(created_at);

-- 操作日志：直接适配历史记录页的时间、用户、设备名称、操作类型和操作结果列
INSERT INTO operation_logs (
    id, msg_id, user_id, device_id, module_name, operation_type, operation_content,
    result, result_code, request_payload, response_payload, created_at
)
VALUES
    (
        1, 'msg-202603091430250001', 1, 1, 'home',
        @action_on, @log_home_light,
        @result_success, 200,
        JSON_OBJECT('action', 'control_single_device', 'device_id', 'light_living_main', 'command', 'turn_on'),
        JSON_OBJECT('code', 200, 'message', 'success', 'current_state', 'on', 'current_value', 80),
        '2026-03-09 14:30:25'
    ),
    (
        2, 'msg-202603091432100001', 2, 3, 'device_control',
        @op_adjust, @log_set_ac,
        @result_success, 200,
        JSON_OBJECT('action', 'control_single_device', 'device_id', 'ac_living_room', 'command', 'set_param', 'param_name', 'temperature', 'param_value', 24),
        JSON_OBJECT('code', 200, 'message', 'success', 'current_state', 'on', 'current_value', 24),
        '2026-03-09 14:32:10'
    ),
    (
        3, 'msg-202603091435500001', 1, NULL, 'scene',
        @op_activate_scene, @log_activate_home,
        @result_success, 200,
        JSON_OBJECT('action', 'trigger_scene', 'scene_code', 'go_home'),
        JSON_OBJECT('code', 200, 'message', 'Scene executed successfully'),
        '2026-03-09 14:35:50'
    )
ON DUPLICATE KEY UPDATE
    user_id = VALUES(user_id),
    device_id = VALUES(device_id),
    module_name = VALUES(module_name),
    operation_type = VALUES(operation_type),
    operation_content = VALUES(operation_content),
    result = VALUES(result),
    result_code = VALUES(result_code),
    request_payload = VALUES(request_payload),
    response_payload = VALUES(response_payload),
    created_at = VALUES(created_at);

-- 报警记录：保留已处理历史报警，这样页面既能显示历史，又不影响系统状态显示为正常
INSERT INTO alarm_records (id, alarm_type, alarm_content, handled_status, handled_at, created_at)
VALUES
    (1, @alarm_temp_high, @alarm_temp_content, @handled, '2026-03-09 12:20:00', '2026-03-09 12:15:30'),
    (2, @alarm_device_offline, @alarm_device_content, @handled, '2026-03-09 11:10:00', '2026-03-09 11:00:00')
ON DUPLICATE KEY UPDATE
    alarm_type = VALUES(alarm_type),
    alarm_content = VALUES(alarm_content),
    handled_status = VALUES(handled_status),
    handled_at = VALUES(handled_at),
    created_at = VALUES(created_at);

-- 系统配置：直接支持主题、语言和温湿度阈值页面
INSERT INTO system_configs (
    id, config_name, theme, language,
    temperature_low_threshold, temperature_high_threshold,
    humidity_low_threshold, humidity_high_threshold,
    system_status_level, system_status_text, updated_by, created_at, updated_at
)
VALUES
    (
        1, 'default', @theme_default, @lang_zh_cn,
        15.00, 35.00,
        30.00, 80.00,
        'normal', @status_text_normal,
        1, '2026-03-09 10:00:00', '2026-03-09 10:00:00'
    )
ON DUPLICATE KEY UPDATE
    theme = VALUES(theme),
    language = VALUES(language),
    temperature_low_threshold = VALUES(temperature_low_threshold),
    temperature_high_threshold = VALUES(temperature_high_threshold),
    humidity_low_threshold = VALUES(humidity_low_threshold),
    humidity_high_threshold = VALUES(humidity_high_threshold),
    system_status_level = VALUES(system_status_level),
    system_status_text = VALUES(system_status_text),
    updated_by = VALUES(updated_by),
    updated_at = VALUES(updated_at);

COMMIT;
