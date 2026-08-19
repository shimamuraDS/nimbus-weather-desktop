#include "Config.h"
#include <windows.h>
#include <wincrypt.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QTime>
#include <algorithm>

namespace Util {

namespace {

bool isValidAlertTime(const QString& value) {
    return value.size() == 5
        && QTime::fromString(value, QStringLiteral("HH:mm")).isValid();
}

} // namespace

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

Config::Config() {
    QString iniPath = QCoreApplication::applicationDirPath()
                      + QDir::separator() + "config.ini";

    // Search parent directories for config.ini (handles debug build subdirectories)
    if (!QFile::exists(iniPath)) {
        QDir dir = QCoreApplication::applicationDirPath();
        for (int i = 0; i < 5 && !QFile::exists(dir.filePath("config.ini")); ++i) {
            if (!dir.cdUp()) break;
        }
        QString found = dir.filePath("config.ini");
        if (QFile::exists(found)) {
            iniPath = found;
        }
    }

    m_iniSettings = std::make_unique<QSettings>(iniPath, QSettings::IniFormat);
    m_userSettings = std::make_unique<QSettings>();

    // Nimbus Weather uses new application metadata. Copy the previous
    // EnterpriseCorp/Nimbus settings once so existing API keys and alert
    // preferences survive the brand migration.
    const bool isNimbusWeather =
        QCoreApplication::organizationName() == QStringLiteral("shimamuraDS")
        && QCoreApplication::applicationName() == QStringLiteral("NimbusWeather");
    if (isNimbusWeather
        && !m_userSettings->contains(QStringLiteral("General/BrandMigrationCompleted"))) {
        if (m_userSettings->allKeys().isEmpty()) {
            QSettings legacySettings(QStringLiteral("EnterpriseCorp"),
                                     QStringLiteral("Nimbus"));
            const QStringList legacyKeys = legacySettings.allKeys();
            for (const QString& key : legacyKeys) {
                m_userSettings->setValue(key, legacySettings.value(key));
            }
        }
        m_userSettings->setValue(QStringLiteral("General/BrandMigrationCompleted"), true);
        m_userSettings->sync();
    }

    setAutoStart(isAutoStart());
}

// DPAPI via runtime dynamic loading — no linker dependency on crypt32
static bool initDPAPI(DATA_BLOB& out, const DATA_BLOB& in, bool protect, LPCWSTR entropy) {
    HMODULE hLib = LoadLibraryW(L"crypt32.dll");
    if (!hLib) return false;

    using ProtectFunc = BOOL (WINAPI *)(DATA_BLOB*, LPCWSTR, DATA_BLOB*, PVOID,
                                        CRYPTPROTECT_PROMPTSTRUCT*, DWORD, DATA_BLOB*);
    using UnprotectFunc = BOOL (WINAPI *)(DATA_BLOB*, LPWSTR*, DATA_BLOB*, PVOID,
                                          CRYPTPROTECT_PROMPTSTRUCT*, DWORD, DATA_BLOB*);

    bool ok = false;
    if (protect) {
        auto fn = (ProtectFunc)GetProcAddress(hLib, "CryptProtectData");
        if (fn) ok = fn(const_cast<DATA_BLOB*>(&in), entropy, nullptr,
                        nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out);
    } else {
        auto fn = (UnprotectFunc)GetProcAddress(hLib, "CryptUnprotectData");
        if (fn) ok = fn(const_cast<DATA_BLOB*>(&in), nullptr, nullptr, nullptr,
                        nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out);
    }
    FreeLibrary(hLib);
    return ok && out.pbData;
}

static QByteArray encryptDPAPI(const QByteArray& plain, LPCWSTR entropy) {
    DATA_BLOB in, out = {};
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plain.data()));
    in.cbData = static_cast<DWORD>(plain.size());
    if (!initDPAPI(out, in, true, entropy)) return {};
    QByteArray result(reinterpret_cast<char*>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return result;
}

static QByteArray decryptDPAPI(const QByteArray& cipher, LPCWSTR entropy) {
    DATA_BLOB in, out = {};
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(cipher.data()));
    in.cbData = static_cast<DWORD>(cipher.size());
    if (!initDPAPI(out, in, false, entropy)) return {};
    QByteArray result(reinterpret_cast<char*>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return result;
}

QString Config::getTencentApiKey() const {
    // contains() 区分「用户主动清空」和「从未设置」
    if (m_userSettings->contains("API/WeatherKey")) {
        QString stored = m_userSettings->value("API/WeatherKey", "").toString();
        if (stored.isEmpty()) return QString(); // 用户主动清空
        QByteArray cipher = QByteArray::fromBase64(stored.toUtf8());
        if (!cipher.isEmpty()) {
            QByteArray plain = decryptDPAPI(cipher, L"Nimbus Weather");
            if (!plain.isEmpty()) return QString::fromUtf8(plain);
        }
        return QString();
    }
    return m_iniSettings->value("API/DeveloperKey", "").toString();
}

QString Config::getWeatherApiKey() const {
    // 仅返回用户自行填入的值（UI 绑定），不回退 config.ini
    if (m_userSettings->contains("API/WeatherKey")) {
        QString stored = m_userSettings->value("API/WeatherKey", "").toString();
        if (stored.isEmpty()) return QString();
        QByteArray cipher = QByteArray::fromBase64(stored.toUtf8());
        if (!cipher.isEmpty()) {
            QByteArray plain = decryptDPAPI(cipher, L"Nimbus Weather");
            return QString::fromUtf8(plain);
        }
    }
    return QString();
}

void Config::setWeatherApiKey(const QString& key) {
    if (key.isEmpty()) {
        m_userSettings->setValue("API/WeatherKey", "");
        return;
    }
    QByteArray encrypted = encryptDPAPI(key.toUtf8(), L"Nimbus Weather");
    if (!encrypted.isEmpty()) {
        m_userSettings->setValue("API/WeatherKey",
            QString::fromLatin1(encrypted.toBase64()));
    } else {
        qWarning() << "[Config] Failed to encrypt weather API key";
    }
}

bool Config::isAutoLocation() const {
    return m_userSettings->value("Location/IsAuto", true).toBool();
}

void Config::setAutoLocation(bool autoLoc) {
    m_userSettings->setValue("Location/IsAuto", autoLoc);
}

int Config::getManualAdcode() const {
    return m_userSettings->value("Location/ManualAdcode", 110000).toInt();
}

void Config::setManualAdcode(int adcode) {
    m_userSettings->setValue("Location/ManualAdcode", adcode);
}

QString Config::getManualCityName() const {
    return m_userSettings->value("Location/ManualCityName", QString::fromUtf8("北京市")).toString();
}

bool Config::isAutoStart() const {
    return m_userSettings->value("General/AutoStart", true).toBool();
}

void Config::setAutoStart(bool autoStart) {
    m_userSettings->setValue("General/AutoStart", autoStart);

    QSettings reg("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                  QSettings::NativeFormat);
    if (autoStart) {
        QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        reg.remove(QStringLiteral("Nimbus"));
        reg.setValue(QStringLiteral("Nimbus Weather"),
                     QStringLiteral("\"%1\" -hidden").arg(appPath));
    } else {
        reg.remove(QStringLiteral("Nimbus"));
        reg.remove(QStringLiteral("Nimbus Weather"));
    }
}

void Config::setManualCityName(const QString& cityName) {
    m_userSettings->setValue("Location/ManualCityName", cityName);
}

QStringList Config::getAlertTimes() const {
    return m_userSettings->value("Alerts/Times", QStringList()).toStringList();
}

void Config::setAlertTimes(const QStringList& times) {
    m_userSettings->setValue("Alerts/Times", times);
}

QStringList Config::getAlertAdvanceMinutes() const {
    return m_userSettings->value("Alerts/AdvanceMinutes", QStringList()).toStringList();
}

void Config::setAlertAdvanceMinutes(const QStringList& minutes) {
    m_userSettings->setValue("Alerts/AdvanceMinutes", minutes);
}

int Config::getAdvanceMinutesFor(const QString& alertTime) const {
    QStringList times = getAlertTimes();
    QStringList advances = getAlertAdvanceMinutes();
    int idx = times.indexOf(alertTime);
    if (idx >= 0 && idx < advances.size()) {
        return advances[idx].toInt();
    }
    return 0;
}

void Config::setAdvanceMinutesFor(const QString& alertTime, int minutes) {
    if (!isValidAlertTime(alertTime)) return;
    minutes = qBound(0, minutes, 24 * 60);
    QStringList times = getAlertTimes();
    QStringList advances = getAlertAdvanceMinutes();
    int idx = times.indexOf(alertTime);
    if (idx >= 0) {
        if (advances.size() != times.size()) {
            advances.clear();
            for (int i = 0; i < times.size(); ++i) advances.append("0");
        }
        advances[idx] = QString::number(minutes);
        setAlertAdvanceMinutes(advances);
    }
}

void Config::sortAlertsTogether(QStringList& times, QStringList& advances) {
    QList<QPair<QString, QString>> pairs;
    for (int i = 0; i < times.size(); ++i) {
        pairs.append({times[i], i < advances.size() ? advances[i] : "0"});
    }
    std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    times.clear();
    advances.clear();
    for (const auto& p : pairs) {
        times.append(p.first);
        advances.append(p.second);
    }
}

void Config::addAlertTime(const QString& time) {
    if (!isValidAlertTime(time)) return;
    QStringList times = getAlertTimes();
    if (!times.contains(time)) {
        QStringList advances = getAlertAdvanceMinutes();
        times.append(time);
        advances.append("0");
        sortAlertsTogether(times, advances);
        setAlertTimes(times);
        setAlertAdvanceMinutes(advances);
    }
}

void Config::removeAlertTime(const QString& time) {
    QStringList times = getAlertTimes();
    QStringList advances = getAlertAdvanceMinutes();
    int idx = times.indexOf(time);
    if (idx >= 0) {
        times.removeAt(idx);
        if (idx < advances.size()) advances.removeAt(idx);
        setAlertTimes(times);
        setAlertAdvanceMinutes(advances);
    }
}

void Config::updateAlertTime(const QString& oldTime, const QString& newTime) {
    if (!isValidAlertTime(oldTime) || !isValidAlertTime(newTime)) return;
    QStringList times = getAlertTimes();
    QStringList advances = getAlertAdvanceMinutes();
    int idx = times.indexOf(oldTime);
    if (idx >= 0 && oldTime != newTime && !times.contains(newTime)) {
        if (advances.size() != times.size()) {
            advances.clear();
            for (int i = 0; i < times.size(); ++i) advances.append("0");
        }
        times[idx] = newTime;
        sortAlertsTogether(times, advances);
        setAlertTimes(times);
        setAlertAdvanceMinutes(advances);
    }
}
#ifdef WITH_LLM

bool Config::isLLMEnabled() const {
    return m_userSettings->value("LLM/Enabled", false).toBool();
}

void Config::setLLMEnabled(bool enabled) {
    m_userSettings->setValue("LLM/Enabled", enabled);
}

QString Config::getLLMApiUrl() const {
    return m_userSettings->value("LLM/ApiUrl", "https://api.deepseek.com").toString();
}

void Config::setLLMApiUrl(const QString& url) {
    m_userSettings->setValue("LLM/ApiUrl", url);
}

QString Config::getLLMApiKey() const {
    QByteArray encrypted = QByteArray::fromBase64(
        m_userSettings->value("LLM/ApiKey", "").toString().toUtf8());
    if (encrypted.isEmpty()) return QString();
    QByteArray plain = decryptDPAPI(encrypted, L"Nimbus Weather LLM");
    if (plain.isEmpty()) {
        // Compatibility with API keys saved before the application rename.
        plain = decryptDPAPI(encrypted, L"Nimbus LLM");
    }
    return QString::fromUtf8(plain);
}

void Config::setLLMApiKey(const QString& key) {
    if (key.isEmpty()) {
        m_userSettings->setValue("LLM/ApiKey", "");
        return;
    }
    QByteArray encrypted = encryptDPAPI(key.toUtf8(), L"Nimbus Weather LLM");
    if (!encrypted.isEmpty()) {
        m_userSettings->setValue("LLM/ApiKey", QString::fromLatin1(encrypted.toBase64()));
    } else {
        qWarning() << "[Config] Failed to encrypt LLM API key";
    }
}

QString Config::getLLMModelName() const {
    return m_userSettings->value("LLM/ModelName", "deepseek-v4-pro").toString();
}

void Config::setLLMModelName(const QString& model) {
    m_userSettings->setValue("LLM/ModelName", model);
}

#endif

} // namespace Util
