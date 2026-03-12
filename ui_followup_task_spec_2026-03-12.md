# SmartHome UI 第二轮修复任务规格（2026-03-12）

## 1. 文档目标

本版用于承接第一轮改造后的残留问题，只做以下两件事：

1. 明确现状：哪些问题已修，哪些问题仍存在，哪些改动被回退。
2. 提供可直接执行的分步方案：先诊断、再修复、再回归。

本版不改变既有架构原则：UI -> Service -> DAO。

---

## 2. 现状与结论

### 2.1 项目框架现状

1. 场景页依然走 SceneWidget -> SceneService -> SceneDao。
2. 场景参数显示依然来源于 SceneDeviceAction.paramText。
3. 场景参数持久化字段已存在兼容读取逻辑：SceneDao 使用 COALESCE(action_param, param_value)。

相关文件：

- [ui/scenewidget.cpp](ui/scenewidget.cpp)
- [services/sceneservice.cpp](services/sceneservice.cpp)
- [database/dao/SceneDao.cpp](database/dao/SceneDao.cpp)

### 2.2 已确认仍存在的问题

1. 快捷控制卡片开关态对比不足，图标和文本层级不清晰。
2. 场景参数仍有漏显案例：卧室窗帘 -> 设置开合度 -> 5%，表格参数列为空。
3. 场景动作参数单位仍可能被用户侧间接改变，不符合“单位不可改”要求。
4. 三个列表行高仍不够大：主导航、设备分类、场景列表。
5. 暗色主题下，设备分类 icon 与场景新建弹窗中的场景图标，在未选中状态可读性差。
6. 编译告警仍在：populateActionCombo 未使用。

---

## 3. 根因分析（基于当前代码）

### R1 快捷卡片视觉对比不足

现象点：

- [ui/homewidget.cpp](ui/homewidget.cpp) 中 quickControlCardStyle 与 quickCardIconColor 的开启/关闭态配色接近。
- 当前按钮布局采用 ToolButtonTextUnderIcon，但图标尺寸、内边距、文本字号权重未形成明显层级。

结论：

- 这是 UI 设计参数问题，不涉及业务逻辑。

### R2 窗帘参数 5% 漏显

现象点：

- DAO 读取侧已做 COALESCE，说明“只读不到旧字段”不是唯一原因。
- 编辑流程中，参数控件切换依赖动作码和设备类型识别；一旦识别不到 set_openness，会回落到无需参数分支。
- updateSceneAction 的定位仍有历史匹配路径（findSceneActionRecordId），属于高风险点。

结论：

- 该问题大概率是“动作识别/回写一致性 + 更新定位策略”组合问题，不是单点 SQL 问题。

### R3 参数单位可被修改

现象点：

- 场景动作弹窗里不同参数编辑器可产生不同文本格式。
- 对 set_openness、set_brightness、set_volume、set_temperature 的输出仍是字符串拼接，缺少统一格式约束函数。

结论：

- 应在 UI 层统一参数格式生成，不允许自由编辑单位文本。

### R4 三个列表行高不足

现象点：

- [resources/style.qss](resources/style.qss) 和 [resources/style_dark.qss](resources/style_dark.qss) 里 listCategory/listWidget_scenes 仍是 min-height: 42。
- 主导航虽然已提升，但用户体感仍偏小，需要统一再上调。

结论：

- 需要统一提升到同一目标区间，建议 52 到 60。

### R5 暗色主题 icon 可读性差

现象点：

- 设备分类列表 icon 使用原始 QIcon，没有暗色着色。
- 新建场景弹窗图标下拉使用原始 icon，同样缺少暗色自适应。

结论：

- 需要给列表 icon 与弹窗图标都加同一套暗色着色策略。

### R6 未使用函数告警

现象点：

- [ui/scenewidget.cpp](ui/scenewidget.cpp) 中 populateActionCombo 已无调用。

结论：

- 直接删除即可，不影响功能。

---

## 4. 第二轮任务拆解（按优先级）

## P0 场景参数可靠性闭环（最高优先）

目标：修复“窗帘 5% 漏显”并防止类似问题。

实施项：

1. 场景动作更新优先使用 recordId 精确更新，避免依赖旧文本匹配。
2. 动作参数格式统一输出：
   - set_openness/set_brightness/set_volume 固定为整数 + %。
   - set_temperature 固定为整数 + C。
   - set_color_temp 固定为文本 + 数值组合，但由固定选项生成。
3. DAO 查询继续兼容 action_param 和 param_value，同时对空字符串做回退处理。

涉及文件：

- [ui/scenewidget.cpp](ui/scenewidget.cpp)
- [database/dao/SceneDao.cpp](database/dao/SceneDao.cpp)

验收：

1. 修改卧室窗帘为 5%，保存后参数列立即显示 5%。
2. 重新进入页面后参数仍显示 5%。
3. 历史数据中仅有 param_value 的记录也可见。

## P1 参数单位不可修改（场景页）

目标：添加/编辑场景动作时，用户不能改单位，只能改数值或固定选项。

实施项：

1. 禁止 set_openness 等动作走自由文本单位输入。
2. 所有带单位动作走控件化输入并由代码生成最终字符串。

涉及文件：

- [ui/scenewidget.cpp](ui/scenewidget.cpp)

验收：

1. 用户无法将 % 改成其他单位。
2. 用户无法将 C 改成其他单位。

## P2 列表行高统一提升

目标：三个列表在视觉上明显更大，点击更稳。

实施项：

1. 主导航、设备分类、场景列表统一上调单行高度到 56 左右。
2. 保证 item sizeHint 与 QSS min-height 一致。

涉及文件：

- [mainwindow.cpp](mainwindow.cpp)
- [resources/style.qss](resources/style.qss)
- [resources/style_dark.qss](resources/style_dark.qss)
- [ui/devicecontrolwidget.cpp](ui/devicecontrolwidget.cpp)
- [ui/scenewidget.cpp](ui/scenewidget.cpp)

验收：

1. 三个列表单行高度视觉一致。
2. 中英文下文本不挤压。

## P3 暗色主题 icon 可读性修复

目标：未选中状态下 icon 不再发黑难辨。

实施项：

1. 设备分类列表 icon 增加暗色着色。
2. 场景新建弹窗的场景图标下拉增加暗色着色。

涉及文件：

- [ui/devicecontrolwidget.cpp](ui/devicecontrolwidget.cpp)
- [ui/scenewidget.cpp](ui/scenewidget.cpp)

验收：

1. 暗色主题下未选中 icon 仍有足够对比度。

## P4 快捷卡片视觉重排

目标：开启和关闭状态有明确差异，图标与文本位置比例更合理。

实施项：

1. 调整开启/关闭态背景、边框、文本、图标色差。
2. 调整图标尺寸、文字字号和内边距，形成稳定信息层级。

涉及文件：

- [ui/homewidget.cpp](ui/homewidget.cpp)

验收：

1. 同一主题下开启和关闭可一眼区分。
2. 图标与文案在 1366x768 和常见窗口缩放下均不拥挤。

## P5 编译告警清理

目标：移除未使用函数告警。

实施项：

1. 删除未使用函数 populateActionCombo。

涉及文件：

- [ui/scenewidget.cpp](ui/scenewidget.cpp)

验收：

1. 不再出现该告警。

---

## 5. 实施顺序

1. P0 场景参数可靠性闭环
2. P1 参数单位不可修改
3. P5 告警清理
4. P2 列表行高统一
5. P3 暗色 icon 可读性
6. P4 快捷卡片视觉重排

说明：先做功能正确性，再做视觉体验。

---

## 6. 回归矩阵（第二轮）

1. 场景参数

- 添加窗帘动作 set_openness 5%，参数列显示 5%。
- 编辑已存在窗帘动作 5% 到 35%，参数列刷新为 35%。
- 切换中英文后参数列仍正确。

2. 参数单位约束

- set_openness 只能输出 %。
- set_temperature 只能输出 C。
- set_color_temp 只能从固定选项选择。

3. 列表与图标

- 三个列表行高明显提升。
- 暗色主题下分类 icon 和场景图标清晰可读。

4. 首页快捷卡片

- 开启/关闭/离线状态可区分。
- 图标与文本位置、大小协调。

5. 编译质量

- 不再出现 populateActionCombo 未使用告警。

---

## 7. 执行备注

1. 第二轮仍保持最小改动原则，不做核心流程重构。
2. 对数据库兼容修复优先放在 DAO，不在 UI 做临时兜底。
