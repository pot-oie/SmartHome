# Repository Guidelines

## 项目结构与模块组织

本仓库是一个基于 Qt Widgets 和 qmake 的智能家居桌面应用。根目录包含入口与主窗口文件，如 `main.cpp`、`mainwindow.cpp`、`loginwidget.cpp`。`ui/` 存放各业务页面的 `.h/.cpp/.ui`，例如 `ui/homewidget.*`、`ui/settingswidget.*`。`network/` 放网络协议与管理逻辑，`database/` 放数据库访问层，`resources/` 保存 `resources.qrc`、图标与 `style.qss`，`translations/` 保存 `.ts` 翻译文件。`build/` 和 Qt Creator 生成内容属于构建产物，不应提交。

## 构建、测试与开发命令

优先使用 Qt Creator 打开 `SmartHome.pro` 进行本地开发。命令行可使用：

```powershell
qmake SmartHome.pro
mingw32-make
./release/SmartHome.exe
```

`qmake` 生成 Makefile，`mingw32-make` 编译项目，可执行文件通常输出到 `release/` 或 Qt Creator 指定的构建目录。更新翻译时可运行 `lrelease SmartHome.pro` 生成 `.qm`。

## 代码风格与命名约定

统一使用 C++17，延续现有 4 空格缩进和大括号换行风格。类名使用 `PascalCase`，成员函数使用 `camelCase`，Qt 槽函数保持 `on_<控件名>_<信号名>` 形式。UI 类、资源路径和翻译键名应与现有文件保持一致，避免随意重命名 `.ui` 对应类。样式改动优先放在 `resources/style.qss`，不要把大量样式硬编码进控件逻辑。

## 测试指南

仓库当前没有独立自动化测试目录，也未发现 Qt Test 用例。提交前至少应完成手工验证：登录流程、主窗口切换、核心页面显示、资源加载和翻译切换。若新增复杂逻辑，建议补充可独立验证的测试代码，并按模块命名，例如 `tests/test_networkmanager.cpp`。

## 提交与合并请求规范

现有提交历史已使用 `feat:` 前缀，建议继续采用 `type: 简短说明`，例如 `feat: 完成设备控制页联动`、`fix: 修复登录窗口空指针`。Pull Request 应说明变更目的、影响模块、手工验证步骤；涉及 UI 调整时附截图，涉及接口或数据库变更时同步更新 [接口文档.md](/C:/Users/Lenovo/Desktop/QT/SmartHome/接口文档.md)。

## 配置与资源注意事项

不要提交 `build/`、`release/`、Qt Creator 用户配置或生成的 `.qm` 以外的临时文件。新增图标请放入 `resources/icons/` 并更新 `resources/resources.qrc`。修改数据库或网络协议前，先确认对应页面是否已有依赖，避免界面与后端字段不一致。

## 第三方库处理约定

`qcustomplot.cpp` 和 `qcustomplot.h` 为第三方库文件，默认无需阅读或修改；仅在明确涉及图表库升级、兼容性修复或 API 用法核对时再处理。
