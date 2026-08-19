#include "WeatherCode.h"
#include <QStringList>

namespace Util {


QString WeatherCode::getIconByWeather(const QString& weatherStr) {
    const QString basePath = "qrc:/resources/icons/weather/";

    if (weatherStr.contains(QString::fromUtf8("晴"))) {
        return basePath + "sunny.png";
    } else if (weatherStr.contains(QString::fromUtf8("多云"))) {
        return basePath + "cloudy.png";
    } else if (weatherStr.contains(QString::fromUtf8("阴"))) {
        return basePath + "overcast.png";
    } else if (weatherStr.contains(QString::fromUtf8("雷阵雨"))) {
        return basePath + "thunderstorm.png";
    } else if (weatherStr.contains(QString::fromUtf8("雪"))) {
        return basePath + "snow.png";
    } else if (weatherStr.contains(QString::fromUtf8("雨"))) {
        return basePath + "rain.png";
    } else if (weatherStr.contains(QString::fromUtf8("雾")) || weatherStr.contains(QString::fromUtf8("霾"))) {
        return basePath + "fog.png";
    } else if (weatherStr.contains(QString::fromUtf8("沙")) || weatherStr.contains(QString::fromUtf8("尘"))) {
        return basePath + "sandstorm.png";
    }

    return basePath + "unknown.png";
}

bool WeatherCode::isSevereWeather(const QString& weatherStr) {
    const QString weather = weatherStr.trimmed();
    if (weather.isEmpty()) return false;

    static const QStringList benignWeather = {
        QStringLiteral("晴"),
        QStringLiteral("多云"),
        QStringLiteral("晴间多云"),
        QStringLiteral("多云间晴")
    };
    return !benignWeather.contains(weather);
}

} // namespace Util
