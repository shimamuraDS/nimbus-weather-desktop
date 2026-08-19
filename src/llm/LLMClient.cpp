#include "LLMClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QDebug>
#include <memory>
#include <utility>

namespace LLM {

namespace {

struct RequestState {
    bool completed = false;
    bool timedOut = false;
};

} // namespace

LLMClient::LLMClient(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
}

void LLMClient::chat(const QString& apiUrl, const QString& apiKey,
                     const QString& model, const QString& userMessage,
                     std::function<void(const QString&)> callback,
                     int timeoutMs) {
    auto failFast = [&callback](const QString& reason) {
        qWarning() << "[LLMClient]" << reason;
        if (callback) callback(QString());
    };

    QUrl baseUrl(apiUrl.trimmed());
    const QHostAddress hostAddress(baseUrl.host());
    const bool localHttp = baseUrl.scheme() == QStringLiteral("http")
        && (baseUrl.host().compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0
            || hostAddress.isLoopback());
    if (!baseUrl.isValid()
        || (baseUrl.scheme() != QStringLiteral("https") && !localHttp)
        || baseUrl.host().isEmpty()) {
        failFast(QStringLiteral("Invalid API URL; HTTPS is required for remote hosts"));
        return;
    }
    if (apiKey.trimmed().isEmpty() || model.trimmed().isEmpty()) {
        failFast(QStringLiteral("API key or model is empty"));
        return;
    }

    QString path = baseUrl.path();
    while (path.endsWith(QLatin1Char('/'))) path.chop(1);
    if (!path.endsWith(QStringLiteral("/chat/completions"))) {
        path += QStringLiteral("/chat/completions");
    }
    baseUrl.setPath(path);

    QUrl url(baseUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    QJsonObject body;
    body["model"] = model;
    body["stream"] = false;

    QJsonObject thinking;
    thinking["type"] = "enabled";
    body["thinking"] = thinking;
    body["reasoning_effort"] = "high";

    QJsonArray messages;
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = QString::fromUtf8(
        "你是一个简洁实用的天气助手。请用中文回复，语气友好自然，给出实用的出行建议。不要使用夸张或华丽的辞藻。");
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    messages.append(userMsg);

    body["messages"] = messages;

    QByteArray bodyData = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = m_manager->post(request, bodyData);
    auto state = std::make_shared<RequestState>();

    // 超时定时器
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply, state]() {
        if (state->completed) return;
        state->timedOut = true;
        qWarning() << "[LLMClient] Request timed out";
        reply->abort();
    });
    timer->start(qMax(1, timeoutMs));

    connect(reply, &QNetworkReply::finished, this,
            [reply, timer, state, callback = std::move(callback)]() mutable {
        if (state->completed) return;
        state->completed = true;
        timer->stop();
        reply->deleteLater();

        if (state->timedOut) {
            if (callback) callback(QString());
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[LLMClient] Network error:" << reply->errorString();
            if (callback) callback(QString());
            return;
        }

        QByteArray responseData = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "[LLMClient] JSON parse error:" << parseError.errorString();
            if (callback) callback(QString());
            return;
        }

        QJsonObject root = doc.object();
        QJsonArray choices = root["choices"].toArray();
        if (choices.isEmpty()) {
            qWarning() << "[LLMClient] No choices in response";
            if (callback) callback(QString());
            return;
        }

        QString content = choices[0].toObject()["message"].toObject()["content"].toString().trimmed();
        if (content.isEmpty()) {
            qWarning() << "[LLMClient] Empty content in response";
        }
        if (callback) callback(content);
    });
}

} // namespace LLM
