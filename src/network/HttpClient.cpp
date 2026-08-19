#include "HttpClient.h"
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>
#include <QDebug>
#include <memory>
#include <utility>

namespace Network {

namespace {

struct RequestState {
    bool completed = false;
    bool timedOut = false;
};

QString redactedUrl(const QUrl& url) {
    QUrl safeUrl(url);
    QUrlQuery query(safeUrl);
    const auto items = query.queryItems();
    query.clear();
    for (const auto& item : items) {
        const bool sensitive = item.first.compare(QStringLiteral("key"), Qt::CaseInsensitive) == 0
            || item.first.contains(QStringLiteral("token"), Qt::CaseInsensitive)
            || item.first.contains(QStringLiteral("secret"), Qt::CaseInsensitive);
        query.addQueryItem(item.first, sensitive ? QStringLiteral("<redacted>") : item.second);
    }
    safeUrl.setQuery(query);
    return safeUrl.toString(QUrl::FullyEncoded);
}

} // namespace

HttpClient::HttpClient(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
}

void HttpClient::sendGetRequest(const QUrl& url,
                                std::function<void(const QJsonObject&)> onSuccess,
                                std::function<void(const QString&)> onError,
                                int timeoutMs) {
    if (!url.isValid() || (url.scheme() != QStringLiteral("https")
                           && url.scheme() != QStringLiteral("http"))) {
        if (onError) onError(QStringLiteral("Invalid request URL"));
        return;
    }

    QNetworkRequest request(url);
    QNetworkReply* reply = m_manager->get(request);
    auto state = std::make_shared<RequestState>();

    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply, state]() {
        if (state->completed) return;
        state->timedOut = true;
        reply->abort();
    });
    timer->start(qMax(1, timeoutMs));

    connect(reply, &QNetworkReply::finished, this,
            [reply, timer, state, onSuccess = std::move(onSuccess),
             onError = std::move(onError), url]() {
        if (state->completed) return;
        state->completed = true;
        timer->stop();
        reply->deleteLater();

        if (state->timedOut) {
            qWarning() << "[HttpClient] Request timed out:" << redactedUrl(url);
            if (onError) onError(QStringLiteral("Request timed out"));
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[HttpClient] Network error:" << redactedUrl(url)
                       << reply->errorString();
            if (onError) onError(reply->errorString());
            return;
        }

        QByteArray responseData = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "[HttpClient] Invalid JSON response from" << redactedUrl(url)
                       << parseError.errorString();
            if (onError) onError(QStringLiteral("Invalid JSON response"));
            return;
        }

        QJsonObject rootObj = doc.object();
        if (rootObj.contains("status") && rootObj["status"].toInt() != 0) {
            QString errMsg = rootObj["message"].toString();
            if (errMsg.isEmpty()) errMsg = QStringLiteral("Remote API returned an error");
            qWarning() << "[HttpClient] API error:" << redactedUrl(url) << errMsg;
            if (onError) onError(errMsg);
            return;
        }

        if (onSuccess) onSuccess(rootObj);
    });
}

} // namespace Network
