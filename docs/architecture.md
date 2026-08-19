# Nimbus Weather — 系统架构设计文档

## 一、项目概述

| 属性 | 说明 |
|------|------|
| **项目名称** | Nimbus Weather |
| **技术栈** | C++17, Qt 6.8 LTS (QML + C++), CMake 3.30+ |
| **运行平台** | Windows 桌面端 (系统托盘驻留应用) |
| **构建环境** | CMake + Ninja (MinGW 13.1.0) |
| **第三方服务** | 腾讯位置服务 WebService API |
| **打包格式** | CPack WiX 4 MSI + portable ZIP (Standard / AI 双版本) |

### 核心功能

1. **天气查看** — 当日逐小时、过去 7 天早晚、未来 7 天早晚
2. **智能定位** — IP 自动定位 + 98 城市手动选择双轨机制
3. **天气提醒** — 双重判定（官方预警 + 逐小时分析），定时/自动两种模式
4. **隐式运行** — 系统托盘驻留，开机自启，窗口面积 ≤ 屏幕的 1/12
5. **LLM 增强** — AI 版本通过大模型生成自然语言提醒（可选）
6. **自动更新检测** — 启动时静默检查 GitHub Releases，有新版时工具栏 GitHub 图标显示红点提示

---

## 二、项目目录结构

```text
nimbus-weather-desktop/
├── CMakeLists.txt                  # 核心构建脚本 (含 WITH_LLM 双版本开关)
├── .gitignore                      # Git 忽略规则 (排除敏感配置文件)
├── README.md
├── LICENSE
├── CLAUDE.md                       # AI 辅助开发上下文
├── docs/
│   ├── architecture.md             # 本文件 — 系统架构设计
│   ├── api-design.md               # API 接口文档
│   ├── requirements.md             # 需求规格说明
│   ├── dev-guide.md                # 开发指南
│   ├── user-guide.md               # 用户手册
│   ├── llm-alert-design.md         # LLM 告警功能详设
│   └── timepicker-design.md        # TimePickerDialog 组件详设
├── src/                            # C++ 源代码
│   ├── main.cpp                    # 应用程序入口
│   ├── data/
│   │   └── WeatherCacheManager.h/.cpp    # JSON 本地缓存 (hourly_data 滚动存储)
│   ├── network/
│   │   ├── HttpClient.h/.cpp             # QNetworkAccessManager 封装
│   │   ├── TencentApiClient.h/.cpp       # 腾讯 LBS API 调用
│   │   └── GitHubReleaseClient.h/.cpp    # GitHub Releases API 版本检测
│   ├── service/
│   │   ├── WeatherService.h/.cpp         # 天气数据获取与处理
│   │   ├── LocationService.h/.cpp        # IP 自动 + 手动城市定位
│   │   ├── AlertService.h/.cpp           # 双重模式异常天气检测与提醒
│   │   └── NotificationManager.h/.cpp    # Windows 原生通知
│   ├── viewmodel/
│   │   ├── WeatherViewModel.h/.cpp       # 天气数据 QML 绑定
│   │   ├── SettingsViewModel.h/.cpp      # 设置与 LLM 配置绑定
│   │   └── TrayViewModel.h/.cpp          # 托盘与通知管理
│   ├── llm/                              # [AI 版本独有]
│   │   ├── LLMClient.h/.cpp              # OpenAI 兼容 HTTP 客户端
│   │   └── LLMAlertGenerator.h/.cpp      # 天气数据 → LLM Prompt → 提醒文本
│   └── util/
│       ├── Config.h/.cpp                 # QSettings + DPAPI 安全存储
│       ├── TimeUtil.h/.cpp               # 时间处理工具
│       ├── WeatherCode.h/.cpp            # 天气代码 → 图标/描述映射
│       └── ScreenHelper.h/.cpp           # 屏幕几何 + 任务栏检测
├── qml/                            # UI 层 (View)
│   ├── MainWindow.qml              # 无边框主窗口 (StackView 路由 + 过渡动画)
│   ├── views/
│   │   ├── TodayView.qml           # 当日逐小时天气
│   │   ├── PastView.qml            # 过去 7 天
│   │   ├── FutureView.qml          # 未来 7 天预报
│   │   └── SettingsView.qml        # 设置与提醒配置
│   └── components/
│       ├── Theme.qml               # 全局深色赛博朋克视觉主题
│       ├── Toolbar.qml             # 顶部导航栏
│       ├── WeatherCard.qml         # 天气卡片 (早晚, 玻璃态)
│       ├── HourlyCard.qml          # 逐小时竖条卡片
│       ├── NavigationButton.qml    # 圆形导航按钮
│       ├── TimePickerDialog.qml    # 提醒时间弹窗 (Popup)
│       ├── TimeComboBox.qml        # 主题化时间下拉选择器
│       ├── CitySelector.qml        # 98 城市选择器 (省市两列)
│       ├── APISettingsPane.qml     # API 配置可折叠面板
│       └── LLMSettingsPane.qml     # LLM 设置面板 [AI 版本独有]
├── resources/
│   ├── resources.qrc               # Qt 资源集合
│   ├── app.rc                      # Windows 资源脚本 (图标)
│   ├── NimbusWeather.ico           # 应用图标
│   └── icons/                      # 天气图标 PNG
├── tests/                          # 单元测试 (QtTest + CTest)
│   ├── CMakeLists.txt
│   ├── tst_HourlyMerge.cpp
│   ├── tst_AlertCondition.cpp
│   └── tst_HttpService.cpp
├── scripts/
│   └── build_release.py            # 可复现发布编排（编译/测试/MSI/ZIP/SHA-256）
└── dist/                           # CPack 发布产物（Git 忽略）
    ├── NimbusWeather-*-win64-*.msi
    ├── NimbusWeather-*-win64-*.zip
    └── SHA256SUMS.txt
```

---

## 三、分层架构

项目严格遵循 **MVVM (Model-View-ViewModel)** 及经典三层服务架构：

```
QML View Layer  →  ViewModel Layer  →  Service Layer  →  Network Layer
                               ↓                  ↓
                          Data Layer          Util Layer
```

### 各层职责

| 层 | 目录 | 职责 |
|----|------|------|
| **View** | `qml/` | QML 声明式 UI，通过属性绑定驱动界面刷新 |
| **ViewModel** | `src/viewmodel/` | QObject 派生类，Q_PROPERTY 暴露数据，Q_INVOKABLE 提供交互 |
| **Service** | `src/service/` | 业务逻辑：天气获取、定位切换、告警检测、通知发送 |
| **Network** | `src/network/` | HTTP 请求封装，腾讯 API 调用及 JSON 解析 |
| **Data** | `src/data/` | 本地 JSON 缓存 (1 小时粒度滚动存储，去重合并) |
| **Util** | `src/util/` | 配置管理 (QSettings + DPAPI)、时间工具、天气代码映射、屏幕几何 |

### 关键设计决策

- **单例模式** — `Config`, `WeatherCacheManager`, `NotificationManager` 使用单例，全局唯一
- **信号槽驱动** — 网络请求异步完成 → 发射信号 → ViewModel 更新属性 → QML 自动刷新
- **定时器调度** — `AlertService` 内部 60s 定时器轮询，匹配提醒时间点
- **编译时多版本** — `WITH_LLM` CMake 选项，同一代码库产出标准版/AI 版两个独立构建

---

## 四、API 接口设计

详见 [`docs/api-design.md`](api-design.md)。核心要点：

- **IP 定位** — `GET /ws/location/v1/ip` → 提取 `ad_info.adcode`（归一化到市级精度）
- **逐小时天气** — `GET /ws/weather/v1/?type=hours` → 滚动存入本地缓存
- **未来预报** — `GET /ws/weather/v1/?type=future&get_md=1` → 7 天早晚数据
- **实时预警** — `GET /ws/weather/v1/?type=now&added_fields=alarm` → 高优触发通知

---

## 五、数据模型

### JSON 缓存 (`weather_cache.json`)

```json
{
  "adcode": 130681,
  "last_fetch_time": "2026-05-12T16:00:00",
  "hourly_data": [
    {
      "hour": "2026-05-11T08:00:00",
      "info": {
        "weather": "晴",
        "temperature": 25,
        "wind_direction": "东北风",
        "wind_power": "1-2级"
      }
    }
  ],
  "future_forecast": [
    {
      "date": "2026-05-12",
      "day": { "weather": "晴", "temperature": 26, "humidity": 40 },
      "night": { "weather": "多云", "temperature": 19, "humidity": 55 }
    }
  ],
  "current_alarms": []
}
```

每次 API 请求成功后按 `hour` 时间戳去重合并，清理超过 7 天的历史记录。

### QSettings 键值

| 键 | 类型 | 说明 |
|----|------|------|
| `General/AutoStart` | bool | 开机自启动 (默认 true) |
| `Location/IsAuto` | bool | 自动定位模式 |
| `Location/ManualAdcode` | int | 手动选择的城市 adcode |
| `Location/ManualCityName` | string | 手动选择的城市名 |
| `Alerts/Times` | QStringList | 提醒时间点 ("HH:mm") |
| `Alerts/AdvanceMinutes` | QStringList | 对应提前监测时长 |
| `API/WeatherKey` | string | 天气 API Key (DPAPI 加密存储，设置界面填入) |
| `LLM/Enabled` | bool | AI 功能开关 [AI 版本] |
| `LLM/ApiUrl` | string | LLM API 地址 [AI 版本] |
| `LLM/ApiKey` | string | LLM API Key (DPAPI 加密存储) [AI 版本] |
| `LLM/ModelName` | string | 模型名称 [AI 版本] |

### C++ 数据结构

```cpp
// 日常早晚天气记录 (WeatherCacheManager.h)
struct DailyWeather {
    QString date;
    QString dayWeather;
    int dayTemp;
    int dayHumidity;
    QString nightWeather;
    int nightTemp;
    int nightHumidity;
};
```

---

## 六、组件详细设计

### C++ 核心类

| 类 | 层 | 职责 |
|----|-----|------|
| `HttpClient` | Network | QNetworkAccessManager 封装，异步 HTTP GET/POST |
| `TencentApiClient` | Network | 继承 HttpClient，封装腾讯 LBS 三种天气 API + IP 定位 |
| `GitHubReleaseClient` | Network | GitHub Releases API 调用，获取最新版本号与发布页 URL |
| `WeatherService` | Service | 天气数据获取协调，触发缓存更新 |
| `LocationService` | Service | 自动/手动定位逻辑，adcode 归一化 |
| `AlertService` | Service | 60s 定时器，双重判定（官方预警 + 逐小时），触发通知 |
| `NotificationManager` | Service | Windows 原生气泡通知 |
| `WeatherCacheManager` | Data | JSON 缓存读写，过去 7 天聚合 |
| `WeatherViewModel` | ViewModel | 天气数据 QML 绑定 |
| `SettingsViewModel` | ViewModel | 设置数据绑定 + LLM 配置绑定 + 更新检测 |
| `TrayViewModel` | ViewModel | 系统托盘与窗口显隐 |
| `Config` | Util | QSettings 持久化 + DPAPI 加密 |

### QML 组件树

| 组件 | 路径 | 职责 |
|------|------|------|
| `MainWindow` | `qml/MainWindow.qml` | 无边框主窗口，StackView 路由，方向感知过渡动画 |
| `Theme` | `qml/components/Theme.qml` | 全局视觉常量（色板、间距、圆角、字体） |
| `Toolbar` | `qml/components/Toolbar.qml` | 标题 + 定位控件 + 设置/最小化按钮 |
| `WeatherCard` | `qml/components/WeatherCard.qml` | 玻璃态天气卡片 (240×170)，isPast 冷暖色切换 |
| `HourlyCard` | `qml/components/HourlyCard.qml` | 逐小时竖条卡片 (80×170)，isNow 高亮 |
| `NavigationButton` | `qml/components/NavigationButton.qml` | 圆形导航按钮，hover 发光边框 |
| `CitySelector` | `qml/components/CitySelector.qml` | 省市两列弹出面板，hover 高亮 |
| `TimePickerDialog` | `qml/components/TimePickerDialog.qml` | 提醒时间弹窗 (Popup)，进出缩放动画 |
| `TimeComboBox` | `qml/components/TimeComboBox.qml` | 主题化数字下拉选择器 |
| `APISettingsPane` | `qml/components/APISettingsPane.qml` | API Key 密码输入 + 帮助图标(获取密钥链接) + LLM Loader |
| `LLMSettingsPane` | `qml/components/LLMSettingsPane.qml` | 模型选择(DeepSeek/自定义) + API地址/模型名(可编辑ComboBox含预设) + 测试连接 [AI 版本独有] |

### 页面路由与动画

| 操作 | 动画 | 时长/缓动 |
|------|------|-----------|
| 今日 ↔ 过去 | 水平滑动，方向感知 | pushEnter/popExit: 200ms OutCubic |
| 今日 ↔ 未来 | 水平滑动，方向感知 | pushEnter/popExit: 200ms OutCubic |
| 主页 ↔ 设置 | 垂直滑动 | 200ms OutCubic / 150ms InCubic |

---

## 七、构建系统

### 双版本机制

```cmake
option(WITH_LLM "Build with LLM-powered weather alerts" OFF)
```

| 构建命令 | 产物 | 通知方式 |
|----------|------|---------|
| `cmake -DWITH_LLM=OFF ..` | NimbusWeather.exe (标准版) | 固定中文模板 |
| `cmake -DWITH_LLM=ON ..` | NimbusWeather.exe (AI 版) | LLM 自然语言 + 模板降级 |

条件编译通过 `#ifdef WITH_LLM` 宏控制：
- `src/llm/` 模块的编译和链接
- `Config` 中 LLM 配置方法的编译
- `AlertService` 中 LLM 路径 vs 固定模板路径
- `LLMSettingsPane.qml` 的条件打包（标准版 Loader 静默失败）

### 依赖库

| 库 | 用途 |
|----|------|
| `Qt6::Core` | 基础框架 |
| `Qt6::Gui` | 图形渲染 |
| `Qt6::Qml` / `Qt6::Quick` | QML 引擎与声明式 UI |
| `Qt6::Network` | HTTPS 通信 |
| `Qt6::Widgets` | `QSystemTrayIcon` 系统托盘 |
| `Shell32` | `SHAppBarMessage` (任务栏检测) |

DPAPI (crypt32.dll) 通过 `LoadLibrary`/`GetProcAddress` 动态加载，无需链接。

---

## 八、测试策略

### 单元测试 (QtTest + CTest)

| 测试 | 覆盖范围 |
|------|---------|
| `tst_HourlyMerge` | `WeatherCacheManager` 逐小时数据去重合并、超期清理 |
| `tst_AlertCondition` | `AlertService` 恶劣天气判定逻辑 (模拟数据断言) |
| `tst_HttpService` | Mock 网络层，验证天气/定位 JSON 解析正确性 |

### 集成测试场景

1. **自动定位成功流** — IP → adcode → 加载天气 → 渲染视图全流程
2. **断网容灾** — 断网启动，加载本地缓存，界面提示
3. **时间命中提醒** — 调系统时间验证 AlertService 触发
4. **手动切换定位** — 自动 → 手动城市选择 → 数据刷新

---

## 九、打包部署

### CMake / Qt / CPack 发布链路

发布目录由 CMake Install 规则统一定义；`qt_generate_deploy_qml_app_script()` 调用与当前 Qt kit 匹配的部署工具，自动解析 QML imports、Qt 插件和 MinGW runtime。CPack 再从同一安装清单生成 portable ZIP 与 WiX 4 MSI，避免手工同步 deploy 目录或从 PATH 混入旧版 DLL。

```powershell
python scripts/build_release.py
```

MSI 特性：
- 安装向导支持自定义安装路径（WixUI_InstallDir，Browse 选择目录）
- Standard / AI 使用独立、稳定的 UpgradeCode
- 默认按 per-machine 方式安装到 `%ProgramFiles%`
- 支持 `msiexec /quiet` 静默部署与 Windows Installer 回滚
- MajorUpgrade 自动升级旧版本并阻止降级
- 开始菜单快捷方式由 CPack 统一生成
- 开机自启由应用设置页管理，不由安装器强制开启

发布脚本固定使用 WiX 4.0.4，并将 WiX 二进制与扩展缓存隔离在 `.tools/`；最终产物写入 `dist/` 并生成 SHA-256 校验文件。

### 卸载清理

- `%LOCALAPPDATA%\shimamuraDS\NimbusWeather\weather_cache.json`
- `QSettings` 注册表残留 (`HKCU\Software\shimamuraDS\NimbusWeather`)
- `HKCU\...\Run` 开机自启项
