# Ars Gladii（アルス・グラディ）

現在のバージョン: **Ver. 0.2.0**（[更新履歴](CHANGELOG.md)）

C++ と DxLib で作る 3D MMORPG のプロトタイプです。
現在は「第1層のフィールド・雑魚敵・ボス1体・装備ドロップ」まで遊べる最小構成です。

- 対象: Windows / 1920x1080
- 開発環境: Visual Studio 2022（2026 でも開けます）
- 言語: C++17 / DxLib

## 遊べる内容

- はじまりの町（安全地帯・回復量UP）と第1層「灰風の草原」
- 雑魚敵 3 種（草原ウルフ・岩角イノシシ・ゴブリン兵）。倒すと画面左下のログに経験値・ドロップ品を表示
- ボス「灰牙の獣王 グラウゼル」。3種の攻撃パターン＋HP50%で激昂。倒すと画面中央にリザルト表示
- ボスから武器・防具がドロップ（個体値によって性能・耐久値が変化）
- 武器種 5 種（片手剣・片手棍・短剣・細剣・槍）、ソードスキル 11 種（クールダウン制）
- ガード・パリィ・ジャンプ・回避
- 左上にキャラアイコン＋HP/MP/EXP バー、ボス HP バー、ミニマップ、スキルホットバー
- ESC メニュー: ステータス / 装備・所持品 / スキル選択 / ゲーム設定 / システム（セーブ・ロード）
- セーブ/ロード（F5 / F9 またはメニュー）
- 3Dモデルの差し替え（未配置ならプリミティブのサンプルモデルで動作）

## 操作

| キー | 動作 |
|---|---|
| W A S D | 移動（カメラ基準） |
| マウス移動 / ホイール | 視点移動 / ズーム（Alt を押している間はカーソル表示） |
| 左クリック | 通常攻撃（3段コンボ） |
| 右クリック長押し | ガード（正面の攻撃を軽減） |
| ガード中に左クリック | パリィ（敵の攻撃が当たる瞬間に合わせる） |
| Shift | 回避ステップ（無敵時間あり） |
| Space | ジャンプ |
| 左 Ctrl（長押し） | ダッシュ |
| 1 ～ 4 | ソードスキル |
| R | 回復薬を使う（メニューで登録アイテムを変更可） |
| ESC | メニュー |
| F5 / F9 | クイックセーブ / ロード |
| F1 | 操作説明の表示切替 |

キー配置は ESC メニュー →「ゲーム設定」で変更できます（マウス操作・ESC・Enter は固定）。

## ビルド方法

### 1. DxLib のセットアップ

1. [DXライブラリ置き場](https://dxlib.xsrv.jp/) から「Visual C++ 用」をダウンロードして展開します。
2. 展開したフォルダ内の **`プロジェクトに追加すべきファイル_VC用`** フォルダを、
   このリポジトリの直下に **`DxLib`** という名前でコピーします。

```
mmorpg/
├─ ArsGladii.sln
├─ DxLib/              ← ここに DxLib.h や各種 .lib が入る
│   ├─ DxLib.h
│   └─ ...
└─ ArsGladii/
```

> 別の場所に置きたい場合は、環境変数 `DXLIB_DIR` に DxLib のフォルダを設定してください（例: `C:\DxLib\プロジェクトに追加すべきファイル_VC用`）。
> DxLib が見つからないときは、ビルド時にエラーメッセージで案内が出ます。

### 2. ビルドと実行

1. `ArsGladii.sln` を Visual Studio で開きます。
   - VS2026 で開いた場合、プラットフォームツールセットの再ターゲットを聞かれたら「OK」で構いません。
2. 構成を `Debug | x64`（または `Release | x64`）にしてビルドします（x86 も可）。
3. F5 で実行します。作業ディレクトリは `ArsGladii/` に設定済みなので、`Assets` がそのまま読まれます。
   ビルド後、`Assets` は `bin/<Platform>/<Configuration>/` にもコピーされるので、exe 単体でも起動できます。

プロジェクト設定（参考）: 文字セット「マルチバイト」、ランタイム `/MT`・`/MTd`（DxLib の要件）、`/utf-8`、C++17、`NOMINMAX`。

## 素材の差し替え

すべて任意です。置かなくてもサンプル表示で動きます。

| 素材 | 置き場所 | 説明 |
|---|---|---|
| 3Dモデル | `ArsGladii/Assets/Models/<キー>/model.cfg` | [Models/README.md](ArsGladii/Assets/Models/README.md) |
| フォント | `ArsGladii/Assets/Fonts/` | [Fonts/README.md](ArsGladii/Assets/Fonts/README.md)（既定はメイリオ） |
| アイコン・タイトル背景 | `ArsGladii/Assets/UI/` | [UI/README.md](ArsGladii/Assets/UI/README.md) |
| BGM・効果音 | `ArsGladii/Assets/Sounds/` | [Sounds/README.md](ArsGladii/Assets/Sounds/README.md) |
| 地面テクスチャ | `ArsGladii/Assets/Textures/ground.png` | |

itch.io などの `.fbx` モデルは、`model.cfg` にファイル名とアニメ番号（待機・移動・攻撃・被弾・死亡）を書くだけで使えます。

## ゲームデザイン

ステータス・ダメージ式・個体値・武器種・スキル・ボスの行動などは [docs/GameDesign.md](docs/GameDesign.md) にまとめています。

## ソース構成

```
ArsGladii/Source/
├─ main.cpp
├─ Core/     Game（メインループ）, Input, Camera, AssetManager, FontManager, Config, KeyValueFile, Random, MathUtil
├─ Scene/    Scene, TitleScene, FieldScene
├─ Entity/   Character, Player, Enemy, EnemyDatabase, Animator, PrimitiveRenderer, AnimState
├─ Combat/   Stats（ダメージ式）, WeaponType, Skill
├─ Item/     Item（個体値）, ItemDatabase, Inventory/Equipment, DropTable
├─ World/    World（湧き・戦闘判定）, Field（地形）, Floor（層定義）, GameEvent
├─ UI/       HUD, MessageLog, FloatingText, ResultScreen, Menu, UIDraw
└─ Save/     SaveData
```

セーブデータ: `ArsGladii/save/save01.dat`（テキスト形式）、設定: `ArsGladii/save/config.ini`
