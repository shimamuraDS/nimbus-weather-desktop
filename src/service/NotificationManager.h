#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <memory>

namespace Service {

class NotificationManager : public QObject {
    Q_OBJECT
public:
    static NotificationManager& getInstance();

    NotificationManager(const NotificationManager&) = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;

    QSystemTrayIcon* getTrayIcon() const;
    void showWeatherAlert(const QString& title, const QString& content);

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
