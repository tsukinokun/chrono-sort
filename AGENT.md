# ChronoSort — エージェント向けガイド

歴史の出来事カードを年代順に並べるカードゲーム（ボードゲーム「Timeline」準拠）。
TsukinoEngine（C++20 / DX11 / EnTT）上で、3D の卓にカードを置いて遊ぶ。

応答とコメントは日本語、ログとコミットメッセージは英語。

## 構成

| パス | 内容 |
|---|---|
| `External/TsukinoEngine/` | エンジン（submodule）。API は `Docs/api-index.md` から引く。`External/TsukinoEngine/External/` は読まない |
| `premake5.lua` | ワークスペース定義。エンジンの `Tools/premake/tsukino.lua` のヘルパーを使う |
| `ChronoSort/include/ChronoSort/`, `ChronoSort/src/` | ゲーム本体（下記） |
| `ChronoSort/Assets/` | 実行時アセット。パスはワークスペースルートからの相対（`ChronoSort/Assets/...`） |
| `Tools/art/` | カード面・メッシュ・UI 素材を生成する Python スクリプト |
| `Tools/audio/` | 効果音の合成と BGM の取得 |

ゲーム本体のモジュール:

| 名前空間 / フォルダ | 役割 |
|---|---|
| `Rules/` | ECS に依存しないルール（`Timeline` / `Deck` / `GameRules` / `RulesSelfTest`）。判定はここだけで行う |
| `Board/` | 3D の卓の都合（`CardFactory`、カメラのレイ等） |
| `ECS/Component`, `ECS/System` | 見た目と入力。カードの移動は `CardMotionComponent` の目標を書き換え、`CardMotionSystem` が補間する |
| `Scene/` | `TitleScene` / `SoloGameScene` など |

## ビルドと実行

```
build.bat            Debug をビルド（成功時は出力 0 行）
build.bat Release
open.bat             premake を回して VS を開く
bin\Debug\ChronoSort.exe          タイトルから
bin\Debug\ChronoSort.exe solo     対局画面から直接（確認用）
```

- 作業ディレクトリはワークスペースルート（Debug）。ログは `Logs/ChronoSort.log`。
- Debug 起動時に `Rules::RunSelfTest()` が走り、`[RulesSelfTest] All tests passed.` がログに出る。
  ルールを変えたら必ずここにテストを足す。
- `build.bat` は毎回 premake を回すので、ソースを追加しても再生成は不要。

## アセット生成

```
python Tools/art/make_meshes.py   card.obj / table.obj
python Tools/art/bake_cards.py    デッキ JSON → カード面 PNG と .tmat、卓の布
```

- カード面は 512x768。表（`<id>_front`）は年号を伏せ、裏（`<id>_back`）に年号を焼き込む。
- 絵の元画像は `Tools/art/raw/<id>.*`（gitignore）。無いカードは地域色の仮の絵柄になる。
- エンジンは 3D 空間に文字を描けないため、カード上の文字はすべて焼き込み。

## 規約

- エンジンの `CODING_GUIDELINES.md` と `tsukino-doc-comment` スキルに従う（ファイルヘッダは 76 文字の罫線、
  `@brief` はファイルヘッダのみ、関数要約は です・ます調）。
- ソースは UTF-8 / CRLF。
- エンジンを直すのは汎用機能だけ。ゲーム固有のものはゲーム側に置く。
- コミット: 英語・`[Add]` 等のプレフィックス・main へ直接・push しない・Co-Authored-By を付けない。

## ハマりどころ

- **assimp と .obj**: `.mtl` のマテリアルより前に `DefaultMaterial` が 0 番に入る。
  `ModelComponent::materials` で差し替えるときは 1 番から（`CardFactory.cpp` の `kFirstObjMaterialSlot`）。
- **build.bat の文字コード**: cmd は CP932 で読むため、日本語コメントを含む `.bat` は Shift_JIS で保存する。
  UTF-8 のままだとコメント行が分断されてコマンドとして実行される。
- **SpriteComponent の World 空間はビルボード専用**。傾けたい板は ModelComponent で描く。
- **FontComponent は Overlay（画面）専用**。3D に文字を出したいときは画像に焼くか、`WorldAnchorComponent` で画面 UI を追従させる。
- **ModelComponent の opacity < 1 は影が消える**。角丸などの抜きは `.tmat` の `AlphaCutoff`。
- **カメラは逆 Z**（near が z=1）。レイを作るときに注意。
- **hlslpp のクォータニオン合成は行列と逆順**: 「A のあと B」は `mul(B, A)`。
- Entity の既定値は `entt::null`（`{}` は有効なエンティティ 0 番になる）。
