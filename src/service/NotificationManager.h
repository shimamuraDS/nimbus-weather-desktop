#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <memory>

#include "AlertNotifier.h"

namespace Service {

class NotificationManager : public QObject, public AlertNotifier {
    Q_OBJECT
public:
    static NotificationManager& getInstance();

    NotificationManager(const NotificationManager&) = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;

    QSystemTrayIcon* getTrayIcon() const;
    bool showWeatherAlert(const QString& title, const QString& content) override;

signals:
    void quitRequested();
    void showWindowRequested();

private:
    NotificationManager();
    ~NotificationManager() = default;
    void shutdown();

    QSystemTrayIcon* m_trayIcon = nullptr;
    std::unique_ptr<QMenu> m_trayMenu;
};

} // namespace Service

#endif // NOTIFICATIONMANAGER_H
