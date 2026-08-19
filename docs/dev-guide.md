# 开发指南

## 1. 项目结构与分层架构

项目严格遵循 **MVVM (Model-View-ViewModel)** 及经典三层服务架构：

*   **Data Layer (`src/data`)**：`WeatherCacheManager` 负责应用本地数据目录下的 1 小时粒度 JSON 缓存读写。它接收 `type=hours` 返回的逐小时数据并滚动存储，提供过去 7 天聚合读取能力。
*   **Network Layer (`src/network`)**：封装 `QNetworkAccessManager`，`TencentApiClient` 处理 IP 定位与三种天气 API 的并发抓取及 JSON 结构反序列化。IP 定位返回的 adcode 会归一化到市级精度（区县代码 → 市级代码）。
*   **Service Layer (`src/service`)**：`WeatherService`, `LocationService`, `AlertService`, `NotificationManager`，负责业务逻辑流转、Windows 原生弹窗唤起以及定时任务调度。
*   **ViewModel Layer (`src/viewmodel`)**：继承自 `QObject`，通过 `Q_PROPERTY` 暴露 `todayWeather`, `pastWeatherList` 等属性，供前端 UI 数据绑定。
*   **View Layer (`qml/`)**：分为 `components` (高复用组件，如 `WeatherCard`, `Toolbar`, `Theme`) 和 `views` (页面容器，如 `TodayView`, `SettingsView`)。
*   **Util Layer (`src/util`)**：`Config`, `TimeUtil`, `WeatherCode`, `ScreenHelper` 等工具类，提供配置管理、时间计算、天气代码映射、屏幕几何动态监听（含 Windows 自动隐藏任务栏检测）等基础能力。

### 分层依赖关系

```
QML View Layer  →  ViewModel Layer  →  Service Layer  →  Network Layer
                                ↓                  ↓
                           Data Layer          Util Layer
```

## 2. QML 组件清单

| 组件 | 路径 | 职责 |
|------|------|------|
| `MainWindow` | `qml/MainWindow.qml` | 无边框主窗口，深色赛博朋克渐变背景，双层辉光边框，StackView 页面路由，方向感知过渡动画 |
| `Theme` | `qml/components/Theme.qml` | 全局视觉主题定义（深色赛博朋克色板：电光青/珊瑚/紫罗兰强调色、玻璃态辉光边框、间距、圆角、字体），各组件通过 `Theme { id: theme }` 实例化使用 |
| `Toolbar` | `qml/components/Toolbar.qml` | 顶部工具栏，标题“Nimbus Weather”后跟定位控件（城市名+定位不准？/城市选择器+自动定位），设置/返回按钮，最小化(-)按钮，离线提示横幅 |
| `WeatherCard` | `qml/components/WeatherCard.qml` | 玻璃态天气卡片（240×170），`isPast` 属性控制暖珊瑚/冷青背景色变体，hover 缩放效果，图标浮动动画 |
| `HourlyCard` | `qml/components/HourlyCard.qml` | 逐小时天气卡片（竖条 80×170），内置天气图标，图标悬停浮动动画（transform Translate），`isNow` 属性高亮当前小时（青色渐变边框+发光效果） |
| `NavigationButton` | `qml/components/NavigationButton.qml` | 圆形导航按钮，hover 发光边框，按压缩放动画 |
| `CitySelector` | `qml/components/CitySelector.qml` | 城市选择器，内置 98 个全国城市，左侧省份列表 + 右侧城市列表两列弹出面板，带弹出/收起动画和 hover 高亮 |
| `APISettingsPane` | `qml/components/APISettingsPane.qml` | 可折叠 API 设置面板，天气 API Key 输入 + 帮助图标(获取密钥链接) + LLM 设置 Loader |
| `LLMSettingsPane` | `qml/components/LLMSettingsPane.qml` | [AI 版本] 模型选择器(DeepSeek/自定义) + 可编辑 ComboBox(API地址/模型名预设) + 测试连接 |
| `TodayView` | `qml/views/TodayView.qml` | 当日逐小时天气时间线，顶部"今日天气"标题（柔和紫色），横向滑动卡片区，默认居中显示当前小时卡片，`isNow` 高亮 |
| `PastView` | `qml/views/PastView.qml` | 过去 7 天天气列表，暖珊瑚色卡片，水平居中，两侧对称间距，默认滚动至末尾（最近一天） |
| `FutureView` | `qml/views/FutureView.qml` | 未来 7 天预报列表，冷青色卡片，水平居中，两侧对称间距 |
| `SettingsView` | `qml/views/SettingsView.qml` | 开机自启动开关 + 提醒时间管理（Flickable 可滚动，提醒时间+时长双行选择器，支持修改和删除，每个提醒显示对应未来时长，定位已移至 Toolbar） |

## 3. 页面过渡动画

StackView 使用方向感知过渡动画（pushEnter/popExit: 200ms OutCubic, pushExit/popEnter: 150ms InCubic）：

| 操作 | 动画 | 方向控制 |
|------|------|---------|
| 今日 → 过去 (◀) | 新页面从左侧滑入 | `navGoingLeft = true`, `navIsSettings = false` |
| 过去 → 今日 | 页面从右往左滑 | `navGoingLeft = false`, `navIsSettings = false` |
| 今日 → 未来 (▶) | 新页面从右侧滑入 | `navGoingLeft = false`, `navIsSettings = false` |
| 未来 → 今日 | 页面从左往右滑 | `navGoingLeft = true`, `navIsSettings = false` |
| 主页 → 设置页 | 设置页从顶部向下滑入 | `navIsSettings = true` |
| 设置页 → 主页 | 设置页从下向上滑出 | `navIsSettings = true` |

## 4. 编码规范

### C++ 规范
*   使用 C++17 标准
*   启用 `explicit` 防止隐式转换
*   采用命名空间隔离（`namespace Service`, `namespace ViewModel`）
*   内存管理首选 `std::unique_ptr` 及 Qt 的对象树自动释放机制
*   单例模式用于全局服务（Config, WeatherCacheManager, NotificationManager）

### QML 规范
*   使用 QtQuick.Layouts 代替绝对定位
*   组件的对外交互需通过定义 `signal` 进行事件抛出
*   严格遵循声明式 UI 编程思想
*   所有文案使用中文
*   视觉效果统一使用 `Theme` 组件定义的常量，避免硬编码颜色/间距/字体

## 5. 如何添加新功能

1.  在腾讯位置服务 API 文档中查阅所需新增的参数（如增加空气质量 `added_fields=air`）。
2.  在 `TencentApiClient` 补充请求逻辑并定义新的 `signals`。
3.  在 `WeatherCacheManager` 中扩展 JSON 缓存树节点。
4.  在 `WeatherViewModel` 新增对应的 `Q_PROPERTY` 以通知 QML 引擎刷新界面。
5.  在对应的 QML 视图层绑定新属性。
6.  如需新增视觉样式常量，在 `Theme.qml` 中添加。

## 6. 编译与调试

```bash
# Debug 构建
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# 运行测试
ctest --output-on-failure
```
