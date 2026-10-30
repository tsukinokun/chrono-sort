# ChronoSort — エージェント向けガイド

歴史の出来事カードを年代順に並べるカードゲーム。
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
| `Rules/` | ECS に依存しないルール。`Match`（対局の進行：ソロ/対戦/延長戦/得点・`SoloRank`）、`CpuPlayer`（CPU の手）、`Timeline` / `Deck`、`RulesTuning`（数値。`MatchConfig::rules` で受け取る）、`RulesSelfTest` |
| `Board/` | 3D の卓。`CardFactory`（Prefab からカード・山札の裏・目印・卓を出す）、`BoardState`（卓全体の状態。コンテキスト）、`BoardLayout`（並べ方の計算・`SeatPosition`）、`BoardView`（置き場所を変える窓口。`Board::View(registry)` で受け取る）、`BoardTuning` |
| `Match/` | `MatchSession`（1回の対局の状態。レジストリのコンテキストが値で持つ。`Session` / `Events`）、`MatchActions`（配る・手番を始める・カードを出す・終える）、`MatchEvents`（対局の出来事）、`MatchTuning`（公開・入力・CPU・配り方・出来事への返し方の数値） |
| `ECS/Component` | `CardPlacement`、`CardMotion`、`CardGlow`、`Reveal`（公開と判定の演出の段階）、`CameraRig`、`CountUpText`（数字の数え上げ）、`ChoiceTag` / `ChoiceLift`（選択肢の札の見た目と持ち上がり） |
| `ECS/System` | 1ファイル1システム：`MatchFlow`・`CpuTurn`・`HumanInput`・`Reveal`・`CameraRig`（と `ShakeCamera`）・`MatchFeedback`・`BoardLayout`・`CardMotion`・`CardGlow`・`CountUpText`・`ChoiceTag` |
| `ECS/SystemSetup` | シーンに並べるシステムの組み合わせ（`AddTableSystems` / `AddMatchSystems` / `AddUIOnlySystems`）と更新の順番の名前（`Order::kInput` 〜 `kPostProcess`）。**システムはシーンで手書きせず、ここに足す** |
| `Effects/` | `ParticleEmitter`（`Emit("dust", 位置)`。撒き方は `Particles.json`、粒子そのものはエンジンの `BurstEmitter` / `BurstParticleSystem`。`Effects::Particles(registry)` で受け取る） |
| `Game/` | `GameServices`（シーンをまたぐ調整値・音・設定・記録）、`TuningSet`（`Data/Tuning/*.json` を読み、Debug では保存し直すと読み直す。`Game::Tuning()`）、`TuningTypes`（追従・飛び方・弾み・光り方の組）、`SoundBank`（合図の名前で鳴らす。BGM と効果音はエンジンの別のバス）、`SoundTuning`、`Prefabs`、`Options`、`SaveData`、`AssetManifest`、`FrameMonitor` |
| `UI/` | `PaperUI`（紙片の部品。ボタンには重なったときの音が付く）、`Widgets`（`NoteButton`・`ChoiceTag`・`InkLine`）、`TweenTags`（トゥイーンの目印の一覧）、`PaperLayout`（`Data/UI/*.json`）、`DeskHud`・`PlayPopups`・`ResultSheet`・`PauseSheet`（対局の画面 UI。出来事を受けて動く）、`CardAlbum`（結果の紙から開く、出たカードの一覧と拡大）、`TutorialGuide`（慣れるまでの対局で、最初の手番に遊び方の紙を出して止め、閉じた後は手札から隙間へ運ぶ手本を見せる）、`OptionsSheet` |
| `Scene/` | `SceneRouter`（画面の切り替えはすべてここ。タイトルとクレジットはエンジンの名前付きシーン）、`TitleScene`、`GameScene`（組み立てと画面の切り替えだけ）、`CreditsScene`、`LoadingScene`、`DeskStage`、`StageTuning` |
| `Util/` | `AssetPaths`（アセットのフォルダとパスはここだけで決める）、`TextUtil`、`Credits`（`Data/Credits.json` の出典。クレジット画面と `CardAlbum` が使う） |
| `Game/Options` | 設定（音量3本・演出の速さ・画面の揺れ）の読み書き。`Saves/Options.json`。スライダーの位置は2乗して音量にする（`VolumeGain`） |
| `Game/SaveData` | 記録を `Saves/ChronoSort.json` に保存（あとから足した項目は `LoadOptional` で読み、古い記録も読めるようにする。`tutorialMatches` は遊び方の説明つきで正しく置けた対局の数）。`GameServices::Save()` の1つをタイトルと対局の終わりが書き換え、`StoreSave()` で保存する（それぞれが読み書きしない） |

### 起動と読み込み

`WinMain` → `LoadingScene` → `TitleScene`。`LoadingScene` はエンジンの `AssetLoadQueue`（裏スレッド）で
`Game::CollectStartupAssets()` の一覧（カードの .tmat・メッシュ・UI/粒子のテクスチャ・効果音・BGM）を読み、
読み終えたテクスチャを本体スレッドで 1 フレーム 4ms までずつ GPU へ送ってからタイトルへ進む。
**新しいアセットを足したら `AssetManifest.cpp` の一覧に入るか確かめる**（入っていないと、初めて使う瞬間に同期で読んで止まる）。

### ECS の作り方（2026-10 のリファクタリングで決めたこと）

- **シーンには進行のロジックを書かない**。シーンは組み立て（Prefab・コンテキスト・UI・システム）と画面の切り替えだけ。
  切り替えは `Scene::Router`（`ToTitle` / `ToCredits` / `ToMatch(Solo(...) / Versus(...))` / `Quit`）を使い、シーン同士は include しない。
- **状態はコンポーネントかコンテキストに置く**。カード1枚ごとのことはコンポーネント、卓や対局で1つのことはコンテキスト。
  `MatchSession`・`BoardView`・`CardFactory`・`ParticleEmitter` はレジストリが値で持つ（シーンと一緒に片付く。生ポインタで渡さない）。
- **演出の段階は時間で進むコンポーネントにする**（`RevealComponent` を `RevealSystem` が進める）。
  `MatchSession::sequencer`（エンジンの `Tsukino::Core::Sequencer`）は「見た目だけの時間差」に限る。
- **画面 UI・音・粒子は出来事（`Match/MatchEvents`）で受ける**。進行の側は `Match::Events(registry).Publish(...)` するだけ。
  規則の値が変わったら `MatchStateChangedEvent` を出す（UI は毎フレーム Match を読まない）。受け取った側で同じ型を出し直さない（必要ならエンジンの `EventBus::Enqueue`）。
- **数値はコードに書かず `ChronoSort/Assets/Data/Tuning/*.json` に置く**。構造体（`XxxTuning`）のメンバの初期値が JSON を書く前の値で、
  `load` は `Tsukino::IO::LoadOptional` で全項目を省略可にする。JSON は `TuningSet` が `HotReloadable` で持ち、Debug では保存すると読み直す。
  円周率・配列の大きさ・モデルの寸法（`kTableSurfaceY`・カードの大きさ）・更新の順番などはコードに残す。
- **音は合図の名前で鳴らす**（`Sounds().Play("play.flip")`）。どのファイルをどの音量かは `Sounds.json`。
- **エンティティはコードで組み立てず Prefab から出す**（`Game/Prefabs`、`ChronoSort/Assets/Prefabs/`）。
  ゲームのコンポーネントを Prefab に書くなら `Prefabs::RegisterComponents` に登録し、cereal の load/save を書く
  （エンジンの組み込みコンポーネントの load は項目を順に読むものがあるので、Prefab の JSON には全項目を順に書く）。
- **紙片の置き方は `ChronoSort/Assets/Data/UI/*.json`**（`PaperLayout`）。その場で作る紙片（帯・朱印・結果・一時停止・設定）も名前で引く。
  ボタン・選択肢・伸び縮みする線は `UI/Widgets` を使い、選択肢はエンジンの `UIToggleComponent`、スライダーは `UISliderComponent` に乗せる。
  しばらく見せて消す紙片は `PaperUI::DestroyAfter`（エンジンの `LifetimeSystem`）。トゥイーンの目印は `UI/TweenTags` に足す。
- アセットのパスは `Util/AssetPaths` だけに書く。シーンをまたぐもの（調整値・音・設定・記録）は `GameServices`。
- **エンジンに足すのは、どのゲームでも使える機能だけ**（ゲームの名前・アセット・見た目を前提にしない）。足したら CHANGELOG と `generate-docs.bat`。

### 対局画面の流れ

`GameScene` が組み立て、`MatchFlow`（段取り）→ `CpuTurn` → `HumanInput` → `Reveal` の各システムが `Rules::Match` を進める。
ルールの正否は必ず `Match::Play()` の戻り値で判断し、画面側で判定し直さない。
対局で山札から出たカードは `Match::AppearedCards()` に1度ずつ残り、行方（場・外れ・手札）は `Match::FateOf()` で引く。
対局中の Esc は一時停止の紙（続ける／設定／タイトルへ）。一時停止中は `MatchSession::paused` で段取り・入力・演出を止め、カードの動きと UI は回し続ける。
「演出を速く」は、人間の手番以外のあいだだけ deltaTime に `kFastEffectsScale` を掛ける。
画面 UI は `DeskHud`（机の紙片）・`PlayPopups`（朱印と得点）・`ResultSheet`・`PauseSheet` が、それぞれ出来事を購読して動く。
カードを出したあとの演出は `RevealComponent` の段階（運ぶ → 裏返す → 叩きつける → 着地 → 判定 → 列へ／正しい位置を見せて捨て札へ）。
場の列のカードは寝かせず、手前の辺を卓に付けたまま `BoardTuning::rowTilt` だけカメラへ起こす（題名と年号を読ませるため）。
列に着地させるときは `BoardView::SlotRestPose` を使う。`SlotPosition` は卓の上の点なので、粒子・目印用。
場の列は注目点（カーソル・隙間）の周りを大きく広げ、遠いカードほど細く詰める（`BoardLayout` の `BuildLayout`）。
それでも `BoardTuning::rowHalfSpan` に収まらなければ、カメラではなく列だけを流し（`ScrollBy` / `ScrollToSlot`）、範囲の外は端に畳んで重ねる（`FoldX`）。
山札・捨て札は列より手前の左右の角にあり、列が長くなっても奥行きでぶつからない。CPU の肖像は席の内側の横に置く（席の手前は起こした列と重なる）。

## ビルドと実行

```
build.bat            Debug をビルド（成功時は出力 0 行）
build.bat Release
open.bat             premake を回して VS を開く
bin\Debug\ChronoSort.exe          タイトルから
bin\Debug\ChronoSort.exe solo     ソロの対局画面から直接（確認用）
bin\Debug\ChronoSort.exe versus   CPU 2人との対戦から直接（確認用）
bin\Debug\ChronoSort.exe solo japan   出題を日本史に絞る（world で世界史。versus にも付けられる）
```

- 作業ディレクトリはワークスペースルート（Debug）。ログは `Logs/ChronoSort.log`。
- `Logs/ChronoSort.log` の `[Hitch]` は 34ms を超えたフレームと、その直前の出来事（`FrameMonitor::Mark`）。
  終了時に件数と最長の集計が出る。重さを調べるときはまずここを見る。
- Debug ビルドでは対局中に **F8** で人間の手番を CPU（やさしい）が代わりに打つ。最後まで通しで確かめるときに使う。
- 自動操作でクリックを送るときは、押してから離すまで 200ms 以上空ける（Debug はフレームが遅く、短いクリックを取りこぼす）。
- Debug 起動時に `Rules::RunSelfTest()` が走り、`[RulesSelfTest] All tests passed.` がログに出る。
  ルールを変えたら必ずここにテストを足す。
- `build.bat` は毎回 premake を回すので、ソースを追加しても再生成は不要。

## アセット生成

```
python Tools/art/make_meshes.py   card.obj / table.obj
python Tools/art/bake_cards.py    デッキ JSON → カード面 PNG と .tmat、卓の布、山札の裏、挿入位置の目印
python Tools/art/make_ui.py       画面 UI の素材（紙片・封蝋・インク壺・朱印・リボンなど）
python Tools/art/make_fx.py       粒子のテクスチャ
python Tools/art/make_icon.py     exe とウィンドウのアイコン（ChronoSort/Assets/Icon/App.ico）
```

カードを増やす・直すときは `Tools/art/card_list.py` を編集し、次の順に流す:

```
python Tools/art/card_list.py         デッキ JSON（年号・検索語）を書き出す
python Tools/art/fetch_art.py         Commons から絵を取得し Credits.json に出典を記録（--only id1,id2 で取り直し）
python Tools/art/contact_sheet.py X   取得した絵の一覧を X に出して目で確かめる。外れは card_list.py の file で差し替え
python Tools/art/bake_cards.py        カード面を焼き直す
python Tools/art/fetch_portraits.py   CPU の名札の肖像
python Tools/audio/synth_sfx.py       効果音を合成
python Tools/audio/fetch_bgm.py       BGM（PD/CC0 の演奏）を取得して mp3 に（ffmpeg が要る）
```

画像のライセンスは PD / CC0 / CC BY のみ（BY-SA・NC・ND は fetch_art.py が弾く）。Commons は連続で叩くと 429 を返すので、
スクリプトは間隔を空けて Retry-After に従う。

テクスチャを作り直したのに見た目が変わらないときは `Cache/ChronoSort/` を消す
（キャッシュはソースの更新日時でしか作り直されない）。

- カード面は 512x768 で描き、384x576・256 色に減色して保存する（容量対策。見た目はほぼ変わらない）。
  表（`<id>_front`）は年号を伏せ、裏（`<id>_back`）に年号を焼き込む。
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
- **`far` / `near` は Windows.h のマクロ**。変数名に使うとコンパイルエラーになる。
- **テクスチャの色空間**: エンジンの TextureImporter は色のテクスチャを sRGB、`_n` / `_normal` / `_mr` など
  末尾の名前のものを線形として DDS キャッシュに書く（2026-10 にエンジン側で修正。それ以前のキャッシュは白っぽくなる）。
- **画面 UI の座標**は UI カメラの基準解像度 1600x900。マウス座標（ウィンドウのピクセル）とは別物なので、
  3D のピックには `CameraUtility::ScreenPointToRay` にウィンドウの大きさを渡す。
- **UI の子の位置**: `TransformComponent::parent` を付けた子は親の回転・UITween の動きに一緒に乗る。
  親のスケールも掛かるので、紙片は `UIRectComponent` で大きさを決め、scale は 1 のままにする。
- **PowerShell 5.1 のスクリプト**に UTF-8 の日本語を書くと、BOM が無ければ CP932 として読まれて行が壊れる。
  `.ps1` は ASCII で書く。
