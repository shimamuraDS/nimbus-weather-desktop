#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QScreen>
#include <QVariantMap>
#include <QTimer>

#include "service/WeatherService.h"
#include "service/LocationService.h"
#include "service/AlertService.h"
#include "service/NotificationManager.h"
#include "viewmodel/WeatherViewModel.h"
#include "viewmodel/SettingsViewModel.h"
#include "viewmodel/TrayViewModel.h"
#include "util/ScreenHelper.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QApplication app(argc, argv);

    app.setOrganizationName("shimamuraDS");
    app.setOrganizationDomain("github.com/shimamuraDS");
    app.setApplicationName("NimbusWeather");
    app.setApplicationDisplayName("Nimbus Weather");
    QIcon appIcon(QCoreApplication::applicationDirPath() + "/NimbusWeather.ico");
    if (appIcon.isNull())
        appIcon = QIcon(":/resources/icons/NimbusWeather.ico");
    app.setWindowIcon(appIcon);

    // 确保程序关闭最后一个窗口时不退出（托盘常驻）
    QApplication::setQuitOnLastWindowClosed(false);

    // 初始化服务层
    auto* weatherService = new Service::WeatherService(&app);
    auto* locationService = new Service::LocationService(&app);
    auto& notificationMgr = Service::NotificationManager::getInstance();
    auto* alertService = new Service::AlertService(notificationMgr, &app);

    // 初始化 ViewModel 层
    auto* weatherViewModel = new ViewModel::WeatherViewModel(weatherService, locationService, &app);
    auto* settingsViewModel = new ViewModel::SettingsViewModel(locationService, &app);
    auto* trayViewModel = new ViewModel::TrayViewModel(&app);

    // Never leave the application inaccessible when Explorer's system tray is
    // unavailable during startup. Manual launches are visible by default;
    // auto-start remains hidden only when a working tray exists.
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        trayViewModel->showWindow();
    }

    // API 密钥更新后自动刷新天气数据
    QObject::connect(settingsViewModel, &ViewModel::SettingsViewModel::weatherApiKeyChanged,
                     weatherViewModel, &ViewModel::WeatherViewModel::requestData);

    // 启动预警监控
    alertService->startMonitoring();

    // 初始化 QML 引擎
    QQmlApplicationEngine engine;

    // 动态监听屏幕可用区域变化（任务栏自动隐藏/显示时更新）
    auto* screenHelper = new Util::ScreenHelper(&app);

    // 注册 C++ 对象为 QML 上下文属性
    engine.rootContext()->setContextProperty("weatherViewModel", weatherViewModel);
    engine.rootContext()->setContextProperty("settingsViewModel", settingsViewModel);
    engine.rootContext()->setContextProperty("trayViewModel", trayViewModel);
    engine.rootContext()->setContextProperty("trayIcon", notificationMgr.getTrayIcon());
    engine.rootContext()->setContextProperty("primaryScreen", screenHelper);

    // 托盘图标点击 → 切换窗口显示
    QObject::connect(notificationMgr.getTrayIcon(), &QSystemTrayIcon::activated,
                     trayViewModel, [trayViewModel](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            trayViewModel->showWindow();
        }
    });

    // 托盘菜单：显示窗口 / 退出
    QObject::connect(&notificationMgr, &Service::NotificationManager::showWindowRequested,
                     trayViewModel, [trayViewModel]() {
        trayViewModel->showWindow();
    });
    QObject::connect(&notificationMgr, &Service::NotificationManager::quitRequested,
                     &app, &QApplication::quit);

    const QUrl url(QStringLiteral("qrc:/NimbusWeather/qml/MainWindow.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);

    QTimer::singleShot(5000, settingsViewModel, &ViewModel::SettingsViewModel::checkForUpdates);

    return app.exec();
}
