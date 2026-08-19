# CLAUDE.md

## Project: Nimbus Weather

Desktop weather alert app for Windows. System tray resident, auto-start, dual alert mode (official warnings + hourly analysis), dark cyberpunk UI.

## Tech Stack
- C++17, Qt 6.8 LTS (QML + C++), CMake 3.16+
- Compiler: MinGW 13.1.0, Build tool: Ninja
- 3rd-party: Tencent LBS WebService API
- Packaging: WiX Toolset v7 (MSI)

## Build

```bash
# Standard version (fixed templates)
cmake -DWITH_LLM=OFF -DCMAKE_BUILD_TYPE=Release -B build-standard
cmake --build build-standard

# AI version (LLM notifications + template fallback)
cmake -DWITH_LLM=ON -DCMAKE_BUILD_TYPE=Release -B build-ai
cmake --build build-ai

# Tests
ctest --output-on-failure
```

## Architecture

MVVM + 3-tier service architecture. See `docs/architecture.md` for full details.

Key conventions:
- `src/util/` — Config (QSettings + DPAPI), TimeUtil, WeatherCode, ScreenHelper
- `src/data/` — WeatherCacheManager (JSON cache, hourly rolling storage)
- `src/network/` — HttpClient → TencentApiClient (Tencent LBS API), GitHubReleaseClient (update check)
- `src/service/` — WeatherService, LocationService, AlertService, NotificationManager
- `src/viewmodel/` — WeatherViewModel, SettingsViewModel, TrayViewModel
- `src/llm/` — LLMClient, LLMAlertGenerator (only when WITH_LLM=ON)
- `qml/views/` — TodayView, PastView, FutureView, SettingsView
- `qml/components/` — Reusable QML components
- `tests/` — QtTest + CTest (tst_HourlyMerge, tst_AlertCondition, tst_HttpService)

## Two Build Variants

Conditional compilation via `#ifdef WITH_LLM`:
- Standard (`WITH_LLM=OFF`): Fixed Chinese template notifications
- AI (`WITH_LLM=ON`): LLM-generated natural language alerts via OpenAI-compatible API (DeepSeek)

## Notes
- Auto-update check on startup (GitHub Releases API); red dot indicator on toolbar GitHub icon when new version available
- API keys stored via Windows DPAPI encryption in QSettings; `config.ini` remains as fallback
- LLM API Key stored via Windows DPAPI encryption (crypt32.dll loaded dynamically)
- Window size ≤ 1/12 screen area, positioned bottom-right above taskbar
- No comments needed in code unless WHY is non-obvious
- All QML visual constants centralized in `Theme.qml`
