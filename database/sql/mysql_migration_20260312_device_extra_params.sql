-- SmartHome 增量迁移脚本（已有库）
-- 日期：2026-03-12
-- 目标：为空调风速/定时、灯光色温等扩展参数提供实时读写存储

USE smarthome;
SET NAMES utf8mb4;

START TRANSACTION;

-- 1) 新增扩展参数表（幂等）
CREATE TABLE IF NOT EXISTS device_control_params (
    id BIGINT NOT NULL AUTO_INCREMENT,
    device_id BIGINT NOT NULL,
    param_code VARCHAR(50) NOT NULL,
    param_name VARCHAR(100) NOT NULL,
    param_type ENUM('int', 'decimal', 'text', 'bool') NOT NULL DEFAULT 'text',
    param_value_int INT DEFAULT NULL,
    param_value_decimal DECIMAL(12,2) DEFAULT NULL,
    param_value_text VARCHAR(100) DEFAULT NULL,
    param_unit VARCHAR(20) DEFAULT NULL,
    min_value DECIMAL(12,2) DEFAULT NULL,
    max_value DECIMAL(12,2) DEFAULT NULL,
    options_json JSON DEFAULT NULL,
    is_realtime TINYINT(1) NOT NULL DEFAULT 1,
    is_enabled TINYINT(1) NOT NULL DEFAULT 1,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uq_device_control_params_device_code (device_id, param_code),
    KEY idx_device_control_params_device_enabled (device_id, is_enabled),
    KEY idx_device_control_params_code (param_code),
    CONSTRAINT fk_device_control_params_device
        FOREIGN KEY (device_id) REFERENCES devices (id)
        ON UPDATE CASCADE
        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 2) 初始化空调参数（按 device_id 字符串匹配；设备不存在则自动跳过）
INSERT INTO device_control_params (
    device_id, param_code, param_name, param_type,
    param_value_int, param_value_decimal, param_value_text, param_unit,
    min_value, max_value, options_json, is_realtime, is_enabled, created_at, updated_at
)
SELECT d.id, t.param_code, t.param_name, t.param_type,
       t.param_value_int, t.param_value_decimal, t.param_value_text, t.param_unit,
       t.min_value, t.max_value, t.options_json, 1, 1, NOW(), NOW()
FROM devices d
JOIN (
    SELECT 'ac_living_room' AS device_key, 'ac_mode' AS param_code, '空调模式' AS param_name, 'text' AS param_type,
           NULL AS param_value_int, NULL AS param_value_decimal, 'cool' AS param_value_text, NULL AS param_unit,
           NULL AS min_value, NULL AS max_value, JSON_ARRAY('cool', 'heat', 'dry', 'fan') AS options_json
    UNION ALL
    SELECT 'ac_living_room', 'fan_speed', '空调风速', 'text', NULL, NULL, 'medium', NULL, NULL, NULL, JSON_ARRAY('low', 'medium', 'high')
    UNION ALL
    SELECT 'ac_living_room', 'timer_minutes', '空调定时', 'int', 30, NULL, '30', 'min', 0, 120, NULL
    UNION ALL
    SELECT 'ac_bedroom', 'ac_mode', '空调模式', 'text', NULL, NULL, 'heat', NULL, NULL, NULL, JSON_ARRAY('cool', 'heat', 'dry', 'fan')
    UNION ALL
    SELECT 'ac_bedroom', 'fan_speed', '空调风速', 'text', NULL, NULL, 'low', NULL, NULL, NULL, JSON_ARRAY('low', 'medium', 'high')
    UNION ALL
    SELECT 'ac_bedroom', 'timer_minutes', '空调定时', 'int', 0, NULL, '0', 'min', 0, 120, NULL
) t ON d.device_id = t.device_key
ON DUPLICATE KEY UPDATE
    param_name = VALUES(param_name),
    param_type = VALUES(param_type),
    param_value_int = VALUES(param_value_int),
    param_value_decimal = VALUES(param_value_decimal),
    param_value_text = VALUES(param_value_text),
    param_unit = VALUES(param_unit),
    min_value = VALUES(min_value),
    max_value = VALUES(max_value),
    options_json = VALUES(options_json),
    is_realtime = 1,
    is_enabled = 1,
    updated_at = NOW();

-- 3) 初始化灯光色温参数（按 device_id 字符串匹配；设备不存在则自动跳过）
INSERT INTO device_control_params (
    device_id, param_code, param_name, param_type,
    param_value_int, param_value_decimal, param_value_text, param_unit,
    min_value, max_value, options_json, is_realtime, is_enabled, created_at, updated_at
)
SELECT d.id, 'color_temp', '色温', 'int',
       CASE d.device_id
           WHEN 'light_hallway' THEN 4200
           WHEN 'light_bedroom' THEN 3500
           ELSE 4000
       END AS param_value_int,
       NULL,
       CASE d.device_id
           WHEN 'light_hallway' THEN '4200'
           WHEN 'light_bedroom' THEN '3500'
           ELSE '4000'
       END AS param_value_text,
       'K',
       2700,
       6500,
       NULL,
       1,
       1,
       NOW(),
       NOW()
FROM devices d
WHERE d.device_id IN ('light_hallway', 'light_bedroom')
ON DUPLICATE KEY UPDATE
    param_name = VALUES(param_name),
    param_type = VALUES(param_type),
    param_value_int = VALUES(param_value_int),
    param_value_decimal = VALUES(param_value_decimal),
    param_value_text = VALUES(param_value_text),
    param_unit = VALUES(param_unit),
    min_value = VALUES(min_value),
    max_value = VALUES(max_value),
    options_json = VALUES(options_json),
    is_realtime = 1,
    is_enabled = 1,
    updated_at = NOW();

COMMIT;

-- 4) 验证查询（可单独执行）
-- SELECT d.device_id, d.device_name, p.param_code, p.param_value_int, p.param_value_text, p.param_unit, p.updated_at
-- FROM device_control_params p
-- JOIN devices d ON d.id = p.device_id
-- ORDER BY d.device_id, p.param_code;
