#include "LLMAlertGenerator.h"
#include "LLMClient.h"
#include "../util/Config.h"
#include "../util/TimeUtil.h"
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>

namespace LLM {

namespace {

QString jsonScalarToString(const QJsonValue& value) {
    if (value.isString()) return value.toString();
    if (value.isDouble()) return QString::number(value.toDouble());
    if (value.isBool()) return value.toBool() ? QStringLiteral("true")
                                               : QStringLiteral("false");
    return QString();
}

} // namespace

LLMAlertGenerator::LLMAlertGenerator(QObject* parent) : QObject(parent) {}

void LLMAlertGenerator::generateAlert(const QJsonArray& hourlyData,
                                      const QString& currentWeather,
                                      int advanceMinutes,
                                      std::function<void(const QString&)> callback) {
    // 构建天气数据摘要
    QJsonObject weatherSummary;
    weatherSummary["currentWeather"] = currentWeather;

    // 未来数小时的逐小时预报
    int futureHours = qMax(1, (advanceMinutes + 59) / 60);
    QJsonArray forecastArray;
    for (int i = 0; i < hourlyData.size() && forecastArray.size() < qMax(futureHours, 3); ++i) {
        QJsonObject hourObj = hourlyData[i].toObject();
        QString hourStr = hourObj["hour"].toString();
        QDateTime dt = Util::TimeUtil::parseTencentHour(hourStr);
        if (dt.isValid() && Util::TimeUtil::isWithinFutureHours(dt, futureHours)) {
            QJsonObject info = hourObj["info"].toObject();
            QJsonObject entry;
            entry["time"] = hourStr.mid(11, 5);
            entry["weather"] = info["weather"].toString();
            entry["temperature"] = jsonScalarToString(info["temperature"]);
            entry["humidity"] = jsonScalarToString(info["humidity"]);
            QString wind = info["wind"].toString();
            if (wind.isEmpty()) {
                wind = info["wind_direction"].toString();
                const QString power = info["wind_power"].toString();
                if (!power.isEmpty()) wind += QStringLiteral(" ") + power;
            }
            entry["wind"] = wind.trimmed();
            forecastArray.append(entry);
        }
    }
    weatherSummary["forecast"] = forecastArray;
    weatherSummary["advanceMinutes"] = advanceMinutes;

    QString prompt = QString::fromUtf8(
        "以下是未来一段时间的天气数据（JSON格式）：\n\n") +
        QString::fromUtf8(QJsonDocument(weatherSummary).toJson(QJsonDocument::Compact)) +
        QString::fromUtf8("\n\n请根据以上天气数据，为用户生成一条简洁的天气提醒通知。"
                          "要求：1-2句话，包含：1) 当前天气概况 2) 未来变化趋势 3) 实用的出行建议。"
                          "语气友好自然，像朋友在提醒你。不要用任何格式标记。");

    auto* client = new LLMClient(this);
    auto& config = Util::Config::getInstance();
    client->chat(config.getLLMApiUrl(), config.getLLMApiKey(),
                 config.getLLMModelName(), prompt, callback, 10000);
}

} // namespace LLM
