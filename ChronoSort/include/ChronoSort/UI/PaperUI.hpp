//----------------------------------------------------------------------------
//! @file   PaperUI.hpp
//! @brief  「机の上の紙片」でできた画面 UI の部品
//! @detail UI は定規で揃えず、紙片ごとに少しずつ傾け、位相をずらしてゆっくり揺らします。
//!         ホバーで持ち上がり、押すと沈み、クリックで弾み、重なると紙の擦れる音がするボタンもここで作ります。
//!         座標は基準解像度（kReferenceWidth × kReferenceHeight）の画面ピクセルです。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/Math/Color.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <hlsl++.h>

#include <random>
#include <string>

namespace Tsukino::ECS {
    class Scene;
}    // namespace Tsukino::ECS

namespace Tsukino::Asset {
    class AssetManager;
}    // namespace Tsukino::Asset

namespace ChronoSort::UI {

    constexpr float kReferenceWidth  = 1600.0f;    // UI の基準解像度（幅）
    constexpr float kReferenceHeight = 900.0f;     // UI の基準解像度（高さ）

    // 文字の色。見た目の色（sRGB の 0〜255）で書き、エンジンが受け取る線形の色にして持つ
    const hlslpp::float4 kInkColor       = Tsukino::Core::Math::Color::FromSrgb8(46, 31, 20);      // 墨の色
    const hlslpp::float4 kFadedInkColor  = Tsukino::Core::Math::Color::FromSrgb8(107, 76, 51);     // 薄墨（補足の文字）
    const hlslpp::float4 kParchmentColor = Tsukino::Core::Math::Color::FromSrgb8(247, 232, 199);   // 羊皮紙の色（暗い所の文字）
    const hlslpp::float4 kSealRedColor   = Tsukino::Core::Math::Color::FromSrgb8(158, 26, 26);     // 朱

    //! 紙片1枚の作り方です。
    struct PaperSpec {
        std::string    texture;                                  // ChronoSort/Assets/Textures/UI/ からのファイル名
        hlslpp::float2 center = hlslpp::float2(0.0f, 0.0f);      // 中心の位置
        hlslpp::float2 size   = hlslpp::float2(0.0f, 0.0f);      // 大きさ（0 ならテクスチャの大きさ）
        float          angleDegrees = 0.0f;                      // 置いたときの傾き（度）
        float          swayDegrees  = 0.8f;                      // 揺れの幅（度）。0 で揺れない
        float          swaySeconds  = 3.2f;                      // 揺れの周期（秒）
        float          bobPixels    = 1.5f;                      // 上下の漂いの幅
        int            sortOrder    = 0;                         // 描画順（大きいほど手前）
        Tsukino::ECS::Entity parent = entt::null;                // 親（指定すると一緒に動く）
    };

    //! 紙片の UI を作る道具です。
    class PaperUI {
    public:

        //! 作り先のシーンとアセット管理を受け取ります。
        //! @param  [in] scene        UI を作るシーン。
        //! @param  [in] assetManager アセット管理。
        PaperUI(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager);

        //! 紙片を1枚置きます（傾けて置き、ゆっくり揺らす）。
        //! @param  [in] spec 作り方。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity Paper(const PaperSpec& spec);

        //! 文字を置きます。parent を指定すると、その紙片の上に貼り付けたように一緒に傾き・揺れます。
        //! @param  [in] text      文字列。
        //! @param  [in] position  位置（parent があればその中心からの相対）。
        //! @param  [in] scale     文字の大きさ（1 で 32px 相当）。
        //! @param  [in] color     色（linear。見た目の色は Color::FromSrgb8 などで変換して渡す）。
        //! @param  [in] sortOrder 描画順。
        //! @param  [in] parent    貼り付ける紙片。
        //! @param  [in] centered  中央揃えにするなら true（false は左揃え）。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity Text(const std::wstring& text, const hlslpp::float2& position, float scale, const hlslpp::float4& color, int sortOrder,
                                  Tsukino::ECS::Entity parent = entt::null, bool centered = true);

        //! 紙片をボタンにします（ホバーで持ち上がり、押すと沈み、クリックで弾み、重なると "ui.hover" の音が鳴る）。
        //! @param  [in] entity 紙片。
        void MakeButton(Tsukino::ECS::Entity entity);

        //! 紙片が押されたかを返します（このフレームにクリックされたら true）。
        //! @param  [in] entity 紙片。
        //! @return クリックされたら true。
        bool Clicked(Tsukino::ECS::Entity entity);

        //! 紙片にカーソルが重なっているかを返します。
        //! @param  [in] entity 紙片。
        //! @return 重なっていれば true。
        bool Hovered(Tsukino::ECS::Entity entity);

        //! 画面外から滑り込ませます。
        //! @param  [in] entity  紙片。
        //! @param  [in] from    滑り込み始めのずれ。
        //! @param  [in] delay   始めるまでの待ち（秒）。
        //! @param  [in] seconds かける時間（秒）。
        void SlideIn(Tsukino::ECS::Entity entity, const hlslpp::float2& from, float delay = 0.0f, float seconds = 0.45f);

        //! 画面外へ滑り出させ、透明にします。
        //! @param  [in] entity  紙片。
        //! @param  [in] to      滑り出す先のずれ。
        //! @param  [in] delay   始めるまでの待ち（秒）。
        //! @param  [in] seconds かける時間（秒）。
        void SlideOut(Tsukino::ECS::Entity entity, const hlslpp::float2& to, float delay = 0.0f, float seconds = 0.35f);

        //! 弾ませます（数字が変わったときなど）。
        //! @param  [in] entity 紙片や文字。
        //! @param  [in] amount 弾む強さ。
        void Punch(Tsukino::ECS::Entity entity, float amount = 0.25f);

        //! 揺れと上下の漂いを止めます（倒れた物など、もう動かさない紙片に）。
        //! @param  [in] entity 紙片。
        void StopIdle(Tsukino::ECS::Entity entity);

        //! 文字列を書き換えます。
        //! @param  [in] entity 文字のエンティティ。
        //! @param  [in] text   新しい文字列。
        void SetText(Tsukino::ECS::Entity entity, const std::wstring& text);

        //! スプライトのテクスチャを差し替えます。
        //! @param  [in] entity  紙片。
        //! @param  [in] texture ChronoSort/Assets/Textures/UI/ からのファイル名。
        void SetTexture(Tsukino::ECS::Entity entity, const std::string& texture);

        //! エンティティと、その子の UI をまとめて破棄します。
        //! @param  [in] entity 破棄する UI。
        void Destroy(Tsukino::ECS::Entity entity);

        //! しばらく置いてから、子の UI ごと破棄させます（LifetimeSystem が片付ける）。
        //! @param  [in] entity  破棄する UI。
        //! @param  [in] seconds 何秒後に破棄するか。
        void DestroyAfter(Tsukino::ECS::Entity entity, float seconds);

        //! -1〜1 の乱数を返します（紙片ごとの傾きのばらつきに使う）。
        //! @return 乱数。
        float Jitter();

        //! UI を作るシーンを返します（部品の関数が、紙片にコンポーネントを足すときに使う）。
        //! @return シーン。
        Tsukino::ECS::Scene& Scene() { return m_scene; }

        //! テクスチャを読み込みます。
        //! @param  [in] texture ChronoSort/Assets/Textures/UI/ からのファイル名。
        //! @return ハンドル。
        Tsukino::Asset::AssetHandle Texture(const std::string& texture);

    private:
        Tsukino::ECS::Scene&          m_scene;           // UI を作るシーン
        Tsukino::Asset::AssetManager& m_assetManager;    // アセット管理
        std::mt19937                  m_rng;             // ばらつき用の乱数
    };

}    // namespace ChronoSort::UI
