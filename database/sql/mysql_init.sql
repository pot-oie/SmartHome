-- SmartHome MySQL 8.0 初始化脚本
-- 设计目标：
-- 1. 直接适配当前 Qt 前端页面的首页、设备控制、场景、历史、报警和系统设置页面。
-- 2. 为后续“模拟服务端”预留稳定的数据写入入口，支持实时快照、历史时序、报警规则与场景执行落库。
-- 3. 不破坏现有 DAO 已使用的表名，优先扩展而不是替换。

CREATE DATABASE IF NOT EXISTS smarthome
    DEFAULT CHARACTER SET utf8mb4
    COLLATE utf8mb4_0900_ai_ci;

USE smarthome;

SET NAMES utf8mb4;

-- 用户表：支持登录、角色和操作日志归属
CREATE TABLE IF NOT EXISTS users (
    id BIGINT NOT NULL AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    display_name VARCHAR(50) NOT NULL,
    role ENUM('super_admin', 'user') NOT NULL DEFAULT 'user',
    status ENUM('active', 'disabled') NOT NULL DEFAULT 'active',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_users_username (username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 设备分类表：对应设备控制页和系统设置页中的设备类型
CREATE TABLE IF NOT EXISTS device_categories (
    id BIGINT NOT NULL AUTO_INCREMENT,
    category_code VARCHAR(50) NOT NULL,
    category_name VARCHAR(50) NOT NULL,
    display_order INT NOT NULL DEFAULT 0,
    is_enabled TINYINT(1) NOT NULL DEFAULT 1,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_device_categories_code (category_code),
    UNIQUE KEY uq_device_categories_name (category_name),
    KEY idx_device_categories_order (display_order)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 设备基础表：保留前端首页/设置页直接展示需要的当前状态字段
CREATE TABLE IF NOT EXISTS devices (
    id BIGINT NOT NULL AUTO_INCREMENT,
    device_id VARCHAR(64) NOT NULL,
    category_id BIGINT NOT NULL,
    device_name VARCHAR(100) NOT NULL,
    device_type VARCHAR(50) NOT NULL,
    room_name VARCHAR(50) DEFAULT NULL,
    protocol_type VARCHAR(30) NOT NULL DEFAULT 'simulator',
    manufacturer VARCHAR(100) DEFAULT NULL,
    ip_address VARCHAR(45) NOT NULL,
    online_status ENUM('online', 'offline', 'error') NOT NULL DEFAULT 'online',
    switch_status ENUM('on', 'off') NOT NULL DEFAULT 'off',
    current_value DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    value_unit VARCHAR(20) DEFAULT NULL,
    supports_slider TINYINT(1) NOT NULL DEFAULT 0,
    slider_min DECIMAL(10,2) DEFAULT NULL,
    slider_max DECIMAL(10,2) DEFAULT NULL,
    display_order INT NOT NULL DEFAULT 0,
    is_home_quick_control TINYINT(1) NOT NULL DEFAULT 0,
    quick_control_key VARCHAR(50) DEFAULT NULL,
    remarks VARCHAR(255) DEFAULT NULL,
    last_seen_at DATETIME DEFAULT NULL,
    last_control_at DATETIME DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_devices_device_id (device_id),
    UNIQUE KEY uq_devices_quick_control_key (quick_control_key),
    UNIQUE KEY uq_devices_ip_address (ip_address),
    KEY idx_devices_category_id (category_id),
    KEY idx_devices_type (device_type),
    KEY idx_devices_online_status (online_status),
    KEY idx_devices_switch_status (switch_status),
    KEY idx_devices_home_quick (is_home_quick_control, display_order),
    KEY idx_devices_display_order (display_order),
    KEY idx_devices_room_name (room_name),
    CONSTRAINT fk_devices_category
        FOREIGN KEY (category_id) REFERENCES device_categories (id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 设备当前快照表：给模拟服务端提供“最新状态”写入位点
CREATE TABLE IF NOT EXISTS device_state_snapshots (
    id BIGINT NOT NULL AUTO_INCREMENT,
    device_id BIGINT NOT NULL,
    online_status ENUM('online', 'offline', 'error') NOT NULL DEFAULT 'online',
    switch_status ENUM('on', 'off') NOT NULL DEFAULT 'off',
    current_value DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    value_unit VARCHAR(20) DEFAULT NULL,
    mode_text VARCHAR(50) DEFAULT NULL,
    battery_level DECIMAL(5,2) DEFAULT NULL,
    signal_rssi INT DEFAULT NULL,
    power_watt DECIMAL(10,2) DEFAULT NULL,
    last_reported_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    extra_payload JSON DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_device_state_snapshots_device (device_id),
    KEY idx_device_state_snapshots_reported_at (last_reported_at),
    CONSTRAINT fk_device_state_snapshots_device
        FOREIGN KEY (device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 设备遥测历史表：保存亮度、温度设定值、窗帘开合度、在线状态等时序数据
CREATE TABLE IF NOT EXISTS device_telemetry_records (
    id BIGINT NOT NULL AUTO_INCREMENT,
    device_id BIGINT NOT NULL,
    metric_code VARCHAR(50) NOT NULL,
    metric_name VARCHAR(100) NOT NULL,
    metric_value_decimal DECIMAL(12,2) DEFAULT NULL,
    metric_value_text VARCHAR(100) DEFAULT NULL,
    metric_unit VARCHAR(20) DEFAULT NULL,
    quality_flag VARCHAR(20) NOT NULL DEFAULT 'good',
    source_type VARCHAR(20) NOT NULL DEFAULT 'simulator',
    recorded_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_device_telemetry_device_time (device_id, recorded_at),
    KEY idx_device_telemetry_metric_time (metric_code, recorded_at),
    KEY idx_device_telemetry_source_time (source_type, recorded_at),
    CONSTRAINT fk_device_telemetry_device
        FOREIGN KEY (device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 场景表：支持场景列表、首页快捷场景和场景详情
CREATE TABLE IF NOT EXISTS scenes (
    id BIGINT NOT NULL AUTO_INCREMENT,
    scene_code VARCHAR(50) NOT NULL,
    scene_name VARCHAR(100) NOT NULL,
    scene_description VARCHAR(255) NOT NULL,
    scene_icon VARCHAR(100) DEFAULT NULL,
    welcome_text VARCHAR(255) DEFAULT NULL,
    trigger_key VARCHAR(50) DEFAULT NULL,
    is_default TINYINT(1) NOT NULL DEFAULT 0,
    is_enabled TINYINT(1) NOT NULL DEFAULT 1,
    display_order INT NOT NULL DEFAULT 0,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_scenes_code (scene_code),
    UNIQUE KEY uq_scenes_name (scene_name),
    UNIQUE KEY uq_scenes_trigger_key (trigger_key),
    KEY idx_scenes_default_order (is_default, display_order)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 场景动作表：同时保留前端展示文本和服务端可直接执行的参数字段
CREATE TABLE IF NOT EXISTS scene_actions (
    id BIGINT NOT NULL AUTO_INCREMENT,
    scene_id BIGINT NOT NULL,
    device_id BIGINT NOT NULL,
    action_name VARCHAR(50) NOT NULL,
    action_param VARCHAR(100) DEFAULT NULL,
    command_code VARCHAR(50) NOT NULL DEFAULT 'device_command',
    param_name VARCHAR(50) DEFAULT NULL,
    param_value VARCHAR(100) DEFAULT NULL,
    action_order INT NOT NULL DEFAULT 1,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_scene_actions_scene_device_order (scene_id, device_id, action_order),
    KEY idx_scene_actions_scene (scene_id),
    KEY idx_scene_actions_device (device_id),
    KEY idx_scene_actions_order (scene_id, action_order),
    CONSTRAINT fk_scene_actions_scene
        FOREIGN KEY (scene_id) REFERENCES scenes (id)
        ON UPDATE CASCADE
        ON DELETE CASCADE,
    CONSTRAINT fk_scene_actions_device
        FOREIGN KEY (device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 场景执行记录：用于服务端记录“一键激活场景”的执行结果
CREATE TABLE IF NOT EXISTS scene_executions (
    id BIGINT NOT NULL AUTO_INCREMENT,
    scene_id BIGINT NOT NULL,
    triggered_by BIGINT DEFAULT NULL,
    trigger_source VARCHAR(20) NOT NULL DEFAULT 'manual',
    execution_status ENUM('pending', 'success', 'partial_success', 'failed') NOT NULL DEFAULT 'success',
    result_summary VARCHAR(255) DEFAULT NULL,
    request_payload JSON DEFAULT NULL,
    response_payload JSON DEFAULT NULL,
    started_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    finished_at DATETIME DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_scene_executions_scene_time (scene_id, started_at),
    KEY idx_scene_executions_user_time (triggered_by, started_at),
    CONSTRAINT fk_scene_executions_scene
        FOREIGN KEY (scene_id) REFERENCES scenes (id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,
    CONSTRAINT fk_scene_executions_user
        FOREIGN KEY (triggered_by) REFERENCES users (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 场景执行明细：每条记录对应一次场景下发到某个设备的动作
CREATE TABLE IF NOT EXISTS scene_execution_details (
    id BIGINT NOT NULL AUTO_INCREMENT,
    execution_id BIGINT NOT NULL,
    device_id BIGINT DEFAULT NULL,
    action_name VARCHAR(50) NOT NULL,
    action_param VARCHAR(100) DEFAULT NULL,
    execution_status ENUM('pending', 'success', 'failed') NOT NULL DEFAULT 'success',
    result_message VARCHAR(255) DEFAULT NULL,
    executed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_scene_execution_details_execution (execution_id),
    KEY idx_scene_execution_details_device (device_id),
    CONSTRAINT fk_scene_execution_details_execution
        FOREIGN KEY (execution_id) REFERENCES scene_executions (id)
        ON UPDATE CASCADE
        ON DELETE CASCADE,
    CONSTRAINT fk_scene_execution_details_device
        FOREIGN KEY (device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 操作日志表：适配历史记录页，同时为服务端保留请求/响应和快照字段
CREATE TABLE IF NOT EXISTS operation_logs (
    id BIGINT NOT NULL AUTO_INCREMENT,
    msg_id VARCHAR(64) NOT NULL,
    user_id BIGINT DEFAULT NULL,
    scene_id BIGINT DEFAULT NULL,
    device_id BIGINT DEFAULT NULL,
    operator_name VARCHAR(50) DEFAULT NULL,
    device_name_snapshot VARCHAR(100) DEFAULT NULL,
    module_name VARCHAR(50) NOT NULL,
    operation_type VARCHAR(50) NOT NULL,
    operation_content VARCHAR(255) DEFAULT NULL,
    result VARCHAR(20) NOT NULL,
    result_code INT DEFAULT NULL,
    request_payload JSON DEFAULT NULL,
    response_payload JSON DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_operation_logs_msg_id (msg_id),
    KEY idx_operation_logs_user_id (user_id),
    KEY idx_operation_logs_scene_id (scene_id),
    KEY idx_operation_logs_device_id (device_id),
    KEY idx_operation_logs_created_at (created_at),
    KEY idx_operation_logs_module_created (module_name, created_at),
    CONSTRAINT fk_operation_logs_user
        FOREIGN KEY (user_id) REFERENCES users (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL,
    CONSTRAINT fk_operation_logs_scene
        FOREIGN KEY (scene_id) REFERENCES scenes (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL,
    CONSTRAINT fk_operation_logs_device
        FOREIGN KEY (device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 环境当前快照表：首页的温度、湿度和空气质量卡片优先读这里
CREATE TABLE IF NOT EXISTS env_realtime_snapshots (
    id BIGINT NOT NULL AUTO_INCREMENT,
    location_code VARCHAR(50) NOT NULL,
    location_name VARCHAR(50) NOT NULL,
    source_device_id BIGINT DEFAULT NULL,
    temperature DECIMAL(5,2) NOT NULL,
    humidity DECIMAL(5,2) NOT NULL,
    pm25 DECIMAL(8,2) NOT NULL DEFAULT 0.00,
    co2 DECIMAL(8,2) NOT NULL DEFAULT 0.00,
    status_level VARCHAR(20) NOT NULL DEFAULT 'normal',
    record_source VARCHAR(20) NOT NULL DEFAULT 'simulator',
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_env_realtime_location (location_code),
    KEY idx_env_realtime_updated_at (updated_at),
    CONSTRAINT fk_env_realtime_source_device
        FOREIGN KEY (source_device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 环境历史表：兼容现有 EnvRecordDao，同时补充位置与来源字段
CREATE TABLE IF NOT EXISTS env_records (
    id BIGINT NOT NULL AUTO_INCREMENT,
    location_code VARCHAR(50) NOT NULL DEFAULT 'living_room',
    location_name VARCHAR(50) NOT NULL DEFAULT '客厅',
    source_device_id BIGINT DEFAULT NULL,
    temperature DECIMAL(5,2) NOT NULL,
    humidity DECIMAL(5,2) NOT NULL,
    pm25 DECIMAL(8,2) NOT NULL DEFAULT 0.00,
    co2 DECIMAL(8,2) NOT NULL DEFAULT 0.00,
    record_source VARCHAR(20) NOT NULL DEFAULT 'simulator',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_env_records_created_at (created_at),
    KEY idx_env_records_location_time (location_code, created_at),
    CONSTRAINT fk_env_records_source_device
        FOREIGN KEY (source_device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 报警规则表：服务端定时模拟时直接读取这些规则判断是否触发报警
CREATE TABLE IF NOT EXISTS alarm_rules (
    id BIGINT NOT NULL AUTO_INCREMENT,
    rule_code VARCHAR(50) NOT NULL,
    rule_name VARCHAR(100) NOT NULL,
    metric_code VARCHAR(50) NOT NULL,
    comparator ENUM('gt', 'gte', 'lt', 'lte', 'eq', 'neq') NOT NULL,
    threshold_value DECIMAL(12,2) NOT NULL,
    threshold_unit VARCHAR(20) DEFAULT NULL,
    scope_type ENUM('global', 'device') NOT NULL DEFAULT 'global',
    device_id BIGINT DEFAULT NULL,
    severity ENUM('info', 'warning', 'critical') NOT NULL DEFAULT 'warning',
    cooldown_seconds INT NOT NULL DEFAULT 300,
    is_enabled TINYINT(1) NOT NULL DEFAULT 1,
    description VARCHAR(255) DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_alarm_rules_code (rule_code),
    KEY idx_alarm_rules_metric_enabled (metric_code, is_enabled),
    KEY idx_alarm_rules_device (device_id),
    CONSTRAINT fk_alarm_rules_device
        FOREIGN KEY (device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 报警记录表：兼容首页最近报警、报警页历史表和服务端事件闭环
CREATE TABLE IF NOT EXISTS alarm_records (
    id BIGINT NOT NULL AUTO_INCREMENT,
    alarm_code VARCHAR(50) DEFAULT NULL,
    alarm_type VARCHAR(50) NOT NULL,
    alarm_content VARCHAR(255) NOT NULL,
    severity ENUM('info', 'warning', 'critical') NOT NULL DEFAULT 'warning',
    source_device_id BIGINT DEFAULT NULL,
    source_location VARCHAR(50) DEFAULT NULL,
    trigger_metric VARCHAR(50) DEFAULT NULL,
    trigger_value_decimal DECIMAL(12,2) DEFAULT NULL,
    trigger_display_text VARCHAR(50) DEFAULT NULL,
    trigger_unit VARCHAR(20) DEFAULT NULL,
    handled_status VARCHAR(20) NOT NULL DEFAULT 'pending',
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    alarm_source VARCHAR(20) NOT NULL DEFAULT 'simulator',
    extra_payload JSON DEFAULT NULL,
    handled_at DATETIME DEFAULT NULL,
    cleared_at DATETIME DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_alarm_records_created_at (created_at),
    KEY idx_alarm_records_handled_status (handled_status),
    KEY idx_alarm_records_active_created (is_active, created_at),
    KEY idx_alarm_records_device (source_device_id),
    CONSTRAINT fk_alarm_records_device
        FOREIGN KEY (source_device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 系统配置表：首页系统状态、报警阈值、主题语言和模拟服务开关都放在这里
CREATE TABLE IF NOT EXISTS system_configs (
    id BIGINT NOT NULL AUTO_INCREMENT,
    config_name VARCHAR(50) NOT NULL DEFAULT 'default',
    theme VARCHAR(50) NOT NULL DEFAULT '浅色主题',
    language VARCHAR(50) NOT NULL DEFAULT '简体中文',
    temperature_low_threshold DECIMAL(5,2) NOT NULL DEFAULT 18.00,
    temperature_high_threshold DECIMAL(5,2) NOT NULL DEFAULT 30.00,
    humidity_low_threshold DECIMAL(5,2) NOT NULL DEFAULT 30.00,
    humidity_high_threshold DECIMAL(5,2) NOT NULL DEFAULT 70.00,
    pm25_high_threshold DECIMAL(8,2) NOT NULL DEFAULT 75.00,
    co2_high_threshold DECIMAL(8,2) NOT NULL DEFAULT 1000.00,
    simulator_enabled TINYINT(1) NOT NULL DEFAULT 1,
    simulator_interval_seconds INT NOT NULL DEFAULT 5,
    history_retention_days INT NOT NULL DEFAULT 30,
    default_scene_code VARCHAR(50) DEFAULT NULL,
    system_status_level VARCHAR(20) NOT NULL DEFAULT 'normal',
    system_status_text VARCHAR(100) NOT NULL DEFAULT '系统正常，无报警',
    updated_by BIGINT DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_system_configs_name (config_name),
    KEY idx_system_configs_updated_by (updated_by),
    CONSTRAINT fk_system_configs_updated_by
        FOREIGN KEY (updated_by) REFERENCES users (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
