<p align="center">
  <img src="resources/icons/NimbusWeather.ico" width="96" alt="Nimbus Weather Logo" />
</p>

<h1 align="center" style="font-size: 2.5em; font-weight: bold; margin-bottom: 0.2em; color: #00f0ff;">Nimbus Weather</h1>

<p align="center">
  <strong>Windows デスクトップ天気アラートアプリ</strong>
</p>

<p align="center">
  <a href="README.md">English</a> ·
  <a href="README_zh.md">中文</a> ·
  <b>日本語</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++17" />
  <img src="https://img.shields.io/badge/Qt-6.8%20LTS-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="Qt 6.8 LTS" />
  <img src="https://img.shields.io/badge/CMake-3.30%2B-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake 3.30+" />
  <img src="https://img.shields.io/badge/Security-Windows%20DPAPI-ff7b90?style=for-the-badge&logo=windows&logoColor=white" alt="Windows DPAPI" />
  <img src="https://img.shields.io/badge/AI-DeepSeek%20%2F%20LLM-00f0ff?style=for-the-badge&logo=openai&logoColor=white" alt="DeepSeek LLM" />
  <img src="https://img.shields.io/badge/UI-Cyberpunk%20QML-a78bfa?style=for-the-badge&logo=qt&logoColor=white" alt="Cyberpunk QML" />
  <img src="https://img.shields.io/badge/Platform-Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows" />
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="MIT License" />
</p>

<p align="center" style="font-size: 1.1em; color: #cbd5e1; max-width: 750px; margin: 0 auto; line-height: 1.6;">
  Nimbus Weatherは、ダークサイバーパンクなGlassmorphism UIとLLM搭載のインテリジェント通知を特徴とするWindowsデスクトップ天気アプリです。システムトレイ常駐型で動作し、1時間ごとのタイムライン、柔軟なマルチポイントアラート、公式災害警報とスマートな時間別モニタリングを組み合わせたデュアル警告システムを提供します。
</p>

---

## スクリーンショット

<table align="center" style="border-collapse: collapse; border: none; width: 100%; max-width: 1000px;">
  <tr style="border: none;">
    <td width="50%" align="center" style="border: none; padding: 12px; vertical-align: top;">
      <div style="border: 1px solid rgba(0,240,255,0.25); border-radius: 12px; padding: 6px; background: rgba(17,17,36,0.5); box-shadow: 0 8px 32px rgba(0,240,255,0.12);">
        <img src="docs/images/screenshot-today.png" width="100%" style="border-radius: 8px;" alt="本日の天気"/>
      </div>
      <br/><sub><b>24時間の1時間ごとタイムライン</b><br/>現在時刻をシアンでハイライト、未来予測と過去データを横スクロール</sub>
    </td>
    <td width="50%" align="center" style="border: none; padding: 12px; vertical-align: top;">
      <div style="border: 1px solid rgba(0,240,255,0.25); border-radius: 12px; padding: 6px; background: rgba(17,17,36,0.5); box-shadow: 0 8px 32px rgba(0,240,255,0.12);">
        <img src="docs/images/screenshot-future.png" width="100%" style="border-radius: 8px;" alt="7日間予報"/>
      </div>
      <br/><sub><b>7日間の天気予報</b><br/>エレクトリックシアンのGlassmorphismカードで朝夕の温湿度と風力を表示</sub>
    </td>
  </tr>
  <tr style="border: none;">
    <td width="50%" align="center" style="border: none; padding: 12px; vertical-align: top;">
      <div style="border: 1px solid rgba(255,123,144,0.25); border-radius: 12px; padding: 6px; background: rgba(17,17,36,0.5); box-shadow: 0 8px 32px rgba(255,123,144,0.12);">
        <img src="docs/images/screenshot-past.png" width="100%" style="border-radius: 8px;" alt="過去7日間"/>
      </div>
      <br/><sub><b>過去7日間の自動アーカイブ</b><br/>サンセットコーラルの暖色テーマ、ローカルの1時間ごとキャッシュから自動归档</sub>
    </td>
    <td width="50%" align="center" style="border: none; padding: 12px; vertical-align: top;">
      <div style="border: 1px solid rgba(0,240,255,0.25); border-radius: 12px; padding: 6px; background: rgba(17,17,36,0.5); box-shadow: 0 8px 32px rgba(0,240,255,0.12);">
        <img src="docs/images/screenshot-alerm.png" width="100%" style="border-radius: 8px;" alt="アラート設定"/>
      </div>
      <br/><sub><b>定時天気モニタリングアラート</b><br/>カスタム時間ポイントと事前モニタリング時間、編集・削除に対応</sub>
    </td>
  </tr>
  <tr style="border: none;">
    <td width="50%" align="center" style="border: none; padding: 12px; vertical-align: top;">
      <div style="border: 1px solid rgba(255,255,255,0.1); border-radius: 12px; padding: 6px; background: rgba(17,17,36,0.5); box-shadow: 0 8px 32px rgba(255,255,255,0.05);">
        <img src="docs/images/screenshot-standard.png" width="100%" style="border-radius: 8px;" alt="標準版"/>
      </div>
      <br/><sub><b>標準版（固定テンプレート通知）</b><br/>組み込みの中国語ロジックアラートテンプレート、追加APIコストなし</sub>
    </td>
    <td width="50%" align="center" style="border: none; padding: 12px; vertical-align: top;">
      <div style="border: 1px solid rgba(50,205,80,0.25); border-radius: 12px; padding: 6px; background: rgba(17,17,36,0.5); box-shadow: 0 8px 32px rgba(50,205,80,0.12);">
        <img src="docs/images/screenshot-ai.png" width="100%" style="border-radius: 8px;" alt="AI版"/>
      </div>
      <br/><sub><b>AI版（LLM自然言語通知）</b><br/>DeepSeek気象診断と服装・通勤アドバイス、オフライン時に自動テンプレートフォールバック</sub>
    </td>
  </tr>
</table>

---

## 主な機能

### UI/UX
- **ダークサイバーパンクスタイル**: グローバルダークグラデーション背景に、エレクトリックシアン、サンセットコーラル、パステルパープルの3つのコントラストカラースキーム。
- **Glassmorphismカード**: すりガラス効果のカード、ホバー時のエッジライトマイクロアニメーションとスムーズな減衰スクロール。
- **デスクトップドックデザイン**: ウィンドウサイズを画面の1/12に制限、タスクバー通知領域の上に配置し、フォーカス喪失時に自動非表示。

### 気象観測と履歴
- **24時間タイムライン**: 当日の1時間ごとの天気を水平スクロール、現在時刻をハイライトしシステム時間に合わせて自動スライド。
- **未来と過去のデュアルカバレッジ**: 7日間予報 + 過去7日間の天気カード、ローカルJSONの1時間ごとローリングキャッシュに基づく — オフラインでも利用可能。
- **adcode都市レベル測位**: IP自動位置特定、または全国98都市からの手動選択、市区町村コードに正規化。

### デュアルアラートとLLM
- **Tencent公式災害 + 1時間ごとスマート監視**: デュアルアラート融合アルゴリズムにより重複通知を排除し、降雨確率と極端な温湿度を予測。
- **DeepSeek気象診断**（AI版のみ）: アラート発生時、DeepSeekがリアルタイムデータに基づいて会話的な服装と通勤のヒントを生成。
- **フォールバックメカニズム**: DeepSeek APIが利用できない場合、自動的にローカルの標準中国語テンプレート通知に切り替え。

### セキュリティ統合
- **トレイ常駐と自動起動**: システムトレイ右クリックメニュー、Windowsレジストリ`Run`キーによる自動起動。
- **Windows DPAPI暗号化**: APIキーとLLMトークンをWindows DPAPIで暗号化、現在のユーザーにバインド — 設定ファイルは他のデバイスで復号化できません。
- **再現可能なWindowsリリース**: CMake InstallとQt Deployment APIで対応するQt/QML/MinGW依存関係を収集し、CPackからインストール先を選択できるWiX 4 MSIとポータブルZIPを生成。
- **自動アップデートチェック**: 起動時にGitHub Releasesを静かに確認し、新しいバージョンがある場合、ツールバーのGitHubアイコンに赤いドットが表示されます。

---

## バージョン比較とダウンロード

Nimbus Weatherは単一コードベース、デュアルコンパイル条件分岐方式を採用し、2つの独立したインストーラーを生成します。

| 項目 | Standard 標準版 | AI 智能版 |
|:---|:---:|:---:|
| **CMakeビルドフラグ** | `-DWITH_LLM=OFF` | `-DWITH_LLM=ON` |
| **通知ロジック** | 固定中国語テンプレート | DeepSeek自然言語 + APIオフライン時自動テンプレートフォールバック |
| **外部API依存** | Tencent LBS WebService APIのみ | Tencent LBS API + DeepSeek (OpenAI互換) API |
| **セキュアストレージ** | DPAPI暗号化Tencent開発キー | DPAPIデュアルキー暗号化（Tencentキー + LLMキー） |
| **パッケージ成果物** | `NimbusWeather-1.0.2-win64-Standard.msi` | `NimbusWeather-1.0.2-win64-AI.msi` |
| **ポータブルアーカイブ** | `NimbusWeather-1.0.2-win64-Standard.zip` | `NimbusWeather-1.0.2-win64-AI.zip` |

> [!NOTE]
> AI版はLLMスイッチが無効の場合、実行時オーバーヘッドと基盤依存関係は標準版と同じです。

[GitHub Releasesで最新バージョンをダウンロード](https://github.com/shimamuraDS/nimbus-weather-desktop/releases)

---

## 技術スタック

```
┌───────────────────────────────────────────────────────┐
│                    QML View Layer                     │
│   MainWindow · TodayView · PastView · FutureView      │
│   SettingsView · 11の再利用可能コンポーネント          │
├───────────────────────────────────────────────────────┤
│                ViewModel Layer (C++)                  │
│   WeatherViewModel · SettingsViewModel · TrayVM       │
├───────────────────────────────────────────────────────┤
│                  Service Layer                        │
│   Weather · Location · Alert · Notification           │
├───────────────────┬───────────────────────────────────┤
│   Network Layer   │        Data / Util Layer          │
│   Tencent LBS API │  Cache Manager · DPAPI · Config   │
│  (3 weather APIs) │  TimeUtil · WeatherCode · Screen  │
├───────────────────┴───────────────────────────────────┤
│               LLM Module (AI build only)              │
│        LLMClient (OpenAI compat) · LLMAlertGenerator  │
└───────────────────────────────────────────────────────┘
```

| レイヤー | 技術 | 説明 |
|:---|:---|:---|
| **開発言語** | C++17 · QML (Qt Quick) | ネイティブ実行効率 + GPU加速宣言型UI |
| **コアフレームワーク** | Qt 6.8 LTS | Core / Gui / Qml / Quick / Network / Widgets |
| **ビルドシステム** | CMake 3.30+ · Ninja | モダンC++ビルド、Ninjaインクリメンタルコンパイル |
| **デザインパターン** | MVVM + 3層サービスアーキテクチャ | UI双方向データバインディング、Viewにビジネスロジックなし |
| **外部サービス** | Tencent LBS API + OpenAI互換ネットワーク層 | IP測位、天気警報、リアルタイム/毎時/複数日天気 |
| **暗号化** | Windows DPAPI (crypt32.dll動的ロード) | 静的依存なし、Windowsディストリビューション間互換 |
| **パッケージング** | Qt Deployment API · CPack · WiX 4.0.4 | 単一マニフェストからMSI/ZIPを生成、インストール先選択・サイレント導入・メジャーアップグレード対応 |
| **テスト** | QtTest + CTest | 時間枠マージ、マルチソースアラート判定、HTTP非同期リトライをカバー |

---

## アーキテクチャ

```mermaid
graph TD
    %% Define Styles
    classDef qml fill:#00f0ff,stroke:#00b8d4,stroke-width:2px,color:#0b0f19;
    classDef vm fill:#a78bfa,stroke:#7c3aed,stroke-width:2px,color:#ffffff;
    classDef svc fill:#ff7b90,stroke:#e11d48,stroke-width:2px,color:#ffffff;
    classDef data fill:#38bdf8,stroke:#0284c7,stroke-width:2px,color:#ffffff;
    classDef llm fill:#4ade80,stroke:#16a34a,stroke-width:2px,color:#ffffff;

    %% View Layer
    subgraph UI_Layer [QML View Layer - UI]
        Theme["Theme.qml<br>(グローバルスタイル定義)"]
        Main["MainWindow.qml<br>(ベースウィンドウ)"]
        T_View["TodayView.qml<br>(1時間ごとタイムライン)"]
        P_View["PastView.qml<br>(過去7日間)"]
        F_View["FutureView.qml<br>(未来7日間)"]
        S_View["SettingsView.qml<br>(設定とセキュリティ)"]
    end

    %% ViewModel Layer
    subgraph VM_Layer [ViewModel Layer - 双方向データバインディング]
        WVM["WeatherViewModel<br>(天気状態ブリッジ)"]
        SVM["SettingsViewModel<br>(設定連動とDPAPIブリッジ)"]
        TVM["TrayViewModel<br>(Windowsトレイメニュー連動)"]
    end

    %% Service Layer
    subgraph SVC_Layer [Service Layer - コアビジネスロジック]
        WS["WeatherService<br>(3ソースAPIデータ融合)"]
        LS["LocationService<br>(IP測位/adcode)"]
        AS["AlertService<br>(重複排除+災害分析)"]
        NM["NotificationManager<br>(システムトレイプッシュ)"]
    end

    %% Data & Infrastructure Layer
    subgraph Infrastructure [Data & Infrastructure - 低レベルドライバ]
        HC["HttpClient<br>(QNetworkAccessManager)"]
        TAC["TencentApiClient<br>(WebAPIラッパー)"]
        WCM["WeatherCacheManager<br>(ローカルJSONローリングアーカイブ)"]
        CONF["Config & DPAPI<br>(crypt32.dll 動的セキュリティライブラリ)"]
    end

    %% Conditional LLM Block
    subgraph AI_Module [LLM AI Module - AIビルドのみ]
        LC["LLMClient<br>(OpenAIプロトコル互換クライアント)"]
        AG["LLMAlertGenerator<br>(気象コンテキスト診断)"]
    end

    %% Relations & Flow
    Main --> T_View & P_View & F_View & S_View
    T_View & P_View & F_View -.-> WVM
    S_View -.-> SVM
    Main -.-> TVM

    WVM <--> WS & AS
    SVM <--> CONF
    TVM <--> NM

    WS --> TAC & WCM
    LS --> TAC
    AS --> NM
    TAC --> HC

    %% LLM Connection
    AS -.-> AG
    AG --> LC
    LC --> HC
    AG -.-> CONF

    %% Applying Classes
    class Theme,Main,T_View,P_View,F_View,S_View qml;
    class WVM,SVM,TVM vm;
    class WS,LS,AS,NM svc;
    class HC,TAC,WCM,CONF data;
    class LC,AG llm;
```

---

## ビルドガイド

### 1. 前提条件

* **Qt SDK**: Qt 6.8+ (MinGW 64-bitビルドキット)
* **CMake**: v3.30以上
* **Ninja**: CMakeジェネレーターとして推奨
* **Python**: 3.10+（リリーススクリプト）
* **.NET SDK**: リポジトリ内に固定版WiX 4.0.4を導入するために使用

### 2. ビルド

```bash
git clone https://github.com/shimamuraDS/nimbus-weather-desktop.git
cd nimbus-weather-desktop

# 標準版 (LLM無効)
cmake -G "Ninja" -DWITH_LLM=OFF -DCMAKE_BUILD_TYPE=Release -B build-standard
cmake --build build-standard --config Release

# AI版 (LLM有効)
cmake -G "Ninja" -DWITH_LLM=ON -DCMAKE_BUILD_TYPE=Release -B build-ai
cmake --build build-ai --config Release
```

### 3. テスト

```bash
ctest --test-dir build-standard --output-on-failure
```

---

## MSIとポータブル版のパッケージング

```powershell
# AI版とStandard版をビルド・テスト・パッケージング
python scripts/build_release.py

# 片方のエディションのみも指定可能
python scripts/build_release.py --variant ai
python scripts/build_release.py --variant standard
```

スクリプトは固定版WiX 4.0.4とUI拡張を`.tools/`に分離して導入し、他のWiXバージョンによる影響を防ぎます。Qt依存関係は`qt_generate_deploy_qml_app_script()`が解決し、MSI、ZIP、`SHA256SUMS.txt`は`dist/`に出力されます。

---

## よくある質問

> [!WARNING]
> **コンパイル時に`crypt32`リンカーライブラリが見つかりませんか？**
> Nimbus Weatherは`LoadLibrary`による動的ロードを採用しています。CMakeで`crypt32`を静的にリンクしないでください。古いバージョンのWindowsで互換性の問題が発生する可能性があります。

> [!TIP]
> **手動で位置都市を追加する方法は？**
> `src/util/WeatherCode.h`に都市のadcodeと名前のマッピングを追加し、再コンパイルすると、UIの都市選択メニューが自動的に更新されます。

> [!CAUTION]
> **LLMが異常な天気ヒントを返しますか？**
> 設定ページで正しいAPI Base URL（例：`https://api.deepseek.com`）と有効なAPI KEYが入力されていることを確認してください。API設定画面の「接続テスト」ボタンで接続状態を確認できます。

---

## ライセンス

本プロジェクトは[MITライセンス](LICENSE)の下でオープンソース公開されています。

---

<p align="center">
  by <b>shimamuraDS</b>
</p>
