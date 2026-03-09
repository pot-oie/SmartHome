-- SmartHome MySQL 8.0.36 初始化脚本
-- 设计原则：优先适配当前 Qt 前端页面演示，其次兼顾后续扩展。
-- 说明：当前阶段不建 rooms 表，设备归类以 device_categories 为主，便于直接贴合现有 UI 的设备分类结构。

CREATE DATABASE IF NOT EXISTS smarthome
    DEFAULT CHARACTER SET utf8mb4
    COLLATE utf8mb4_0900_ai_ci;

USE smarthome;

SET NAMES utf8mb4;

-- 用户表：支持登录、角色区分和操作日志联表。
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

-- 设备分类表：直接对应设备控制页左侧分类列表。
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

-- 设备表：适配首页快捷控制、设备控制页和系统设置页设备管理表。
CREATE TABLE IF NOT EXISTS devices (
    id BIGINT NOT NULL AUTO_INCREMENT,
    device_id VARCHAR(64) NOT NULL,
    category_id BIGINT NOT NULL,
    device_name VARCHAR(100) NOT NULL,
    device_type VARCHAR(50) NOT NULL,
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
    CONSTRAINT fk_devices_category
        FOREIGN KEY (category_id) REFERENCES device_categories (id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 场景表：适配场景列表、默认场景和首页回家模式按钮。
CREATE TABLE IF NOT EXISTS scenes (
    id BIGINT NOT NULL AUTO_INCREMENT,
    scene_code VARCHAR(50) NOT NULL,
    scene_name VARCHAR(100) NOT NULL,
    scene_description VARCHAR(255) NOT NULL,
    is_default TINYINT(1) NOT NULL DEFAULT 0,
    display_order INT NOT NULL DEFAULT 0,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_scenes_code (scene_code),
    UNIQUE KEY uq_scenes_name (scene_name),
    KEY idx_scenes_default_order (is_default, display_order)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 场景动作表：当前优先适配场景详情页表格展示，action_param 可直接存页面展示文案。
CREATE TABLE IF NOT EXISTS scene_actions (
    id BIGINT NOT NULL AUTO_INCREMENT,
    scene_id BIGINT NOT NULL,
    device_id BIGINT NOT NULL,
    action_name VARCHAR(50) NOT NULL,
    action_param VARCHAR(100) DEFAULT NULL,
    action_order INT NOT NULL DEFAULT 1,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
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

-- 操作日志表：适配历史记录页操作日志列表，并为后续联调预留 JSON 字段。
CREATE TABLE IF NOT EXISTS operation_logs (
    id BIGINT NOT NULL AUTO_INCREMENT,
    msg_id VARCHAR(64) NOT NULL,
    user_id BIGINT DEFAULT NULL,
    device_id BIGINT DEFAULT NULL,
    module_name VARCHAR(50) NOT NULL,
    operation_type VARCHAR(50) NOT NULL,
    operation_content VARCHAR(255) DEFAULT NULL,
    result VARCHAR(20) NOT NULL,
    result_code INT DEFAULT NULL,
    request_payload JSON DEFAULT NULL,
    response_payload JSON DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_operation_logs_msg_id (msg_id),
    KEY idx_operation_logs_user_id (user_id),
    KEY idx_operation_logs_device_id (device_id),
    KEY idx_operation_logs_created_at (created_at),
    KEY idx_operation_logs_module_created (module_name, created_at),
    CONSTRAINT fk_operation_logs_user
        FOREIGN KEY (user_id) REFERENCES users (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL,
    CONSTRAINT fk_operation_logs_device
        FOREIGN KEY (device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 环境数据表：适配首页最新温湿度展示和历史页环境折线图。
CREATE TABLE IF NOT EXISTS env_records (
    id BIGINT NOT NULL AUTO_INCREMENT,
    temperature DECIMAL(5,2) NOT NULL,
    humidity DECIMAL(5,2) NOT NULL,
    pm25 DECIMAL(8,2) NOT NULL DEFAULT 0.00,
    co2 DECIMAL(8,2) NOT NULL DEFAULT 0.00,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_env_records_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 报警记录表：适配首页最近报警、报警设置页历史表格和当前报警状态判断。
CREATE TABLE IF NOT EXISTS alarm_records (
    id BIGINT NOT NULL AUTO_INCREMENT,
    alarm_type VARCHAR(50) NOT NULL,
    alarm_content VARCHAR(255) NOT NULL,
    handled_status VARCHAR(20) NOT NULL DEFAULT 'pending',
    handled_at DATETIME DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_alarm_records_created_at (created_at),
    KEY idx_alarm_records_handled_status (handled_status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 系统配置表：采用单行配置方案，直接适配系统设置页和报警阈值页。
CREATE TABLE IF NOT EXISTS system_configs (
    id BIGINT NOT NULL AUTO_INCREMENT,
    config_name VARCHAR(50) NOT NULL DEFAULT 'default',
    theme VARCHAR(50) NOT NULL DEFAULT 'default_theme',
    language VARCHAR(50) NOT NULL DEFAULT 'zh_CN',
    temperature_low_threshold DECIMAL(5,2) NOT NULL DEFAULT 15.00,
    temperature_high_threshold DECIMAL(5,2) NOT NULL DEFAULT 35.00,
    humidity_low_threshold DECIMAL(5,2) NOT NULL DEFAULT 30.00,
    humidity_high_threshold DECIMAL(5,2) NOT NULL DEFAULT 80.00,
    system_status_level VARCHAR(20) NOT NULL DEFAULT 'normal',
    system_status_text VARCHAR(100) NOT NULL DEFAULT 'system_normal',
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
