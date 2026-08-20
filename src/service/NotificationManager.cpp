#include "NotificationManager.h"
#include <QIcon>
#include <QApplication>
#include <QDebug>

namespace Service {

NotificationManager& NotificationManager::getInstance() {
    static NotificationManager instance;
    return instance;
}

NotificationManager::NotificationManager() {
    m_trayIcon = new QSystemTrayIcon(this);
    QIcon trayIcon(QCoreApplication::applicationDirPath() + "/NimbusWeather.ico");
    if (trayIcon.isNull())
        trayIcon = QIcon(":/resources/icons/NimbusWeather.ico");
    m_trayIcon->setIcon(trayIcon);
    m_trayIcon->setToolTip("Nimbus Weather");

    m_trayMenu = std::make_unique<QMenu>();
    QAction* showAction = m_trayMenu->addAction(QString::fromUtf8("显示窗口"));
    QAction* quitAction = m_trayMenu->addAction(QString::fromUtf8("退出"));

    connect(showAction, &QAction::triggered, this, &NotificationManager::showWindowRequested);
    connect(quitAction, &QAction::triggered, this, &NotificationManager::quitRequested);

    m_trayIcon->setContextMenu(m_trayMenu.get());
    m_trayIcon->show();

    if (qApp) {
        connect(qApp, &QCoreApplication::aboutToQuit,
                this, &NotificationManager::shutdown);
    }
}

QSystemTrayIcon* NotificationManager::getTrayIcon() const {
    return m_trayIcon;
}

bool NotificationManager::showWeatherAlert(const QString& title, const QString& content) {
    if (!m_trayIcon) {
        qWarning() << "[NotificationManager] Notification dropped: tray icon is unavailable";
        return false;
    }

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "[NotificationManager] Notification deferred: system tray is unavailable";
        return false;
    }

    // Explorer may not have been ready when the application started. Re-show
    // the icon before dispatching instead of silently dropping the alert.
    if (!m_trayIcon->isVisible()) m_trayIcon->show();
    if (!m_trayIcon->isVisible() || !QSystemTrayIcon::supportsMessages()) {
        qWarning() << "[NotificationManager] Notification deferred: tray messages are unavailable";
        return false;
    }

    m_trayIcon->showMessage(title, content, QSystemTrayIcon::Warning, 10000);
    qInfo() << "[NotificationManager] Weather alert dispatched:" << title;
    return true;
}

void NotificationManager::shutdown() {
    if (m_trayIcon) {
        m_trayIcon->setContextMenu(nullptr);
        m_trayIcon->hide();
        delete m_trayIcon;
        m_trayIcon = nullptr;
    }
    m_trayMenu.reset();
}

} // namespace Service
