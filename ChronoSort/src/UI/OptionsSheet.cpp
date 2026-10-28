//----------------------------------------------------------------------------
//! @file   OptionsSheet.cpp
//! @brief  設定の紙の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/OptionsSheet.hpp>

#include <ChronoSort/Game/Options.hpp>
#include <ChronoSort/Game/Sounds.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIRectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UICanvas.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace ChronoSort::UI {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
        using Tsukino::Input::KeyCode;

        constexpr float kTrackLeft     = -95.0f;    // 線の始点（紙の中心からの X）
        constexpr float kTrackWidth    = 300.0f;    // 線の長さ
        constexpr float kTrackHeight   = 20.0f;     // 線の太さ
        constexpr float kPreviewEvery  = 0.12f;     // 効果音の試し鳴りの間隔（秒）
        constexpr float kCloseSeconds  = 0.35f;     // 滑り出す時間

        //----------------------------------------------------------------------------
        //! 左端を基準に伸ばす線のスプライトを作ります。
        //! @param  [in] scene     作り先のシーン。
        //! @param  [in] ui        紙片の部品（テクスチャの読み込みに使う）。
        //! @param  [in] parent    親。
        //! @param  [in] texture   テクスチャ。
        //! @param  [in] size      大きさ。
        //! @param  [in] sortOrder 描画順。
        //! @param  [in] tint      色。
        //! @return 作成したエンティティ。
        //----------------------------------------------------------------------------
        Tsukino::ECS::Entity CreateLine(Tsukino::ECS::Scene& scene, PaperUI& ui, Tsukino::ECS::Entity parent, const char* texture, const hlslpp::float2& size,
                                        int sortOrder, const hlslpp::float4& tint) {
            Tsukino::ECS::Registry& registry = scene.GetRegistry();
            const Tsukino::ECS::Entity entity = scene.CreateEntity();
            registry.AddComponent<BuiltIn::TransformComponent>(entity).parent = parent;
            auto& sprite         = registry.AddComponent<BuiltIn::SpriteComponent>(entity);
            sprite.textureHandle = ui.Texture(texture);
            sprite.sortOrder     = sortOrder;
            sprite.tintColor     = tint;
            auto& rect           = registry.AddComponent<BuiltIn::UIRectComponent>(entity);
            rect.size            = size;
            rect.pivot           = hlslpp::float2(0.0f, 0.5f);
            return entity;
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 作り先のシーンと描画順を受け取ります。
    //----------------------------------------------------------------------------
    OptionsSheet::OptionsSheet(Tsukino::ECS::Scene& scene, Tsukino::EngineIntegration::EngineContext& context, int sortOrder)
        : m_scene(scene)
        , m_context(context)
        , m_ui(scene, *context.assetManager)
        , m_sortOrder(sortOrder) {
    }

    //----------------------------------------------------------------------------
    //! 紙を開きます。
    //----------------------------------------------------------------------------
    void OptionsSheet::Open() {
        if(m_sheet != entt::null) {
            return;
        }
        PaperSpec sheet;
        sheet.texture      = "scrap_result.png";
        sheet.center       = hlslpp::float2(kReferenceWidth * 0.5f + 20.0f, kReferenceHeight * 0.5f);
        sheet.size         = hlslpp::float2(620.0f, 520.0f);
        sheet.angleDegrees = 1.5f;
        sheet.swayDegrees  = 0.3f;
        sheet.sortOrder    = m_sortOrder;
        m_sheet            = m_ui.Paper(sheet);
        m_ui.SlideIn(m_sheet, hlslpp::float2(0.0f, 620.0f), 0.0f, 0.5f);

        const int layer = m_sortOrder + 1;
        m_ui.Text(L"設定", hlslpp::float2(0.0f, -200.0f), 1.15f, kInkColor, layer, m_sheet);

        Game::Options& options = Game::CurrentOptions();
        AddSlider(L"全体", -118.0f, &options.master);
        AddSlider(L"BGM", -58.0f, &options.bgm);
        AddSlider(L"効果音", 2.0f, &options.sfx);
        AddToggle(L"演出の速さ", 78.0f, L"普通", L"速い", &options.fastEffects);
        // 「揺れ」は true が1枚目の「あり」（札の並びを「あり／なし」にしたいので、ほかの切り替えと向きが逆）
        AddToggle(L"画面の揺れ", 140.0f, L"あり", L"なし", &options.cameraShake);

        PaperSpec close;
        close.texture      = "scrap_note.png";
        close.center       = hlslpp::float2(150.0f, 212.0f);
        close.size         = hlslpp::float2(170.0f, 62.0f);
        close.angleDegrees = -3.0f;
        close.sortOrder    = layer + 1;
        close.parent       = m_sheet;
        m_close            = m_ui.Paper(close);
        m_ui.Text(L"閉じる", hlslpp::float2(6.0f, 2.0f), 0.72f, kInkColor, layer + 2, m_close);
        m_ui.MakeButton(m_close);

        m_closing  = false;
        m_dragging = -1;
    }

    //----------------------------------------------------------------------------
    //! スライダーを1本作ります。
    //----------------------------------------------------------------------------
    void OptionsSheet::AddSlider(const std::wstring& label, float y, float* value) {
        const int layer = m_sortOrder + 1;
        m_ui.Text(label, hlslpp::float2(-250.0f, y), 0.62f, kFadedInkColor, layer, m_sheet, false);

        Slider slider;
        slider.value = value;
        slider.shown = *value;

        // 線の始点を原点にした入れ物。紙の傾き・揺れに付いていく
        slider.track = m_scene.CreateEntity();
        auto& track  = m_scene.GetRegistry().AddComponent<BuiltIn::TransformComponent>(slider.track);
        track.position = hlslpp::float3(kTrackLeft, y, 0.0f);
        track.parent   = m_sheet;

        // 薄い下書きの線と、その上に引いた墨の線
        CreateLine(m_scene, m_ui, slider.track, "ink_line.png", hlslpp::float2(kTrackWidth, kTrackHeight * 0.7f), layer, hlslpp::float4(1.0f, 1.0f, 1.0f, 0.22f));
        slider.fill = CreateLine(m_scene, m_ui, slider.track, "ink_line.png", hlslpp::float2(kTrackWidth * slider.shown, kTrackHeight), layer + 1,
                                 hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f));

        // 線より少し広い透明の当たり判定（細い線でもつかみやすく）
        slider.hit = CreateLine(m_scene, m_ui, slider.track, "glow.png", hlslpp::float2(kTrackWidth + 36.0f, 44.0f), layer + 4, hlslpp::float4(1.0f, 1.0f, 1.0f, 0.0f));
        m_scene.GetRegistry().GetComponent<BuiltIn::TransformComponent>(slider.hit).position = hlslpp::float3(-18.0f, 0.0f, 0.0f);
        m_scene.GetRegistry().AddComponent<BuiltIn::PointerTargetComponent>(slider.hit);

        PaperSpec knob;
        knob.texture     = "wax_seal.png";
        knob.center      = hlslpp::float2(kTrackWidth * slider.shown, 0.0f);
        knob.size        = hlslpp::float2(34.0f, 34.0f);
        knob.swayDegrees = 4.0f;
        knob.bobPixels   = 0.0f;
        knob.sortOrder   = layer + 2;
        knob.parent      = slider.track;
        slider.knob      = m_ui.Paper(knob);

        slider.text = m_ui.Text(std::to_wstring(static_cast<int>(std::lround(*value * 100.0f))), hlslpp::float2(kTrackWidth + 44.0f, 0.0f), 0.5f, kFadedInkColor,
                                layer, slider.track);
        m_sliders.push_back(slider);
    }

    //----------------------------------------------------------------------------
    //! 札2枚の切り替えを1つ作ります。
    //----------------------------------------------------------------------------
    void OptionsSheet::AddToggle(const std::wstring& label, float y, const std::wstring& off, const std::wstring& on, bool* value) {
        const int layer = m_sortOrder + 1;
        m_ui.Text(label, hlslpp::float2(-250.0f, y), 0.62f, kFadedInkColor, layer, m_sheet, false);

        Toggle toggle;
        toggle.value = value;
        const std::wstring names[] = {off, on};
        for(int i = 0; i < 2; ++i) {
            PaperSpec spec;
            spec.texture      = "scrap_note.png";
            spec.center       = hlslpp::float2(-10.0f + 160.0f * static_cast<float>(i), y + m_ui.Jitter() * 5.0f);
            spec.size         = hlslpp::float2(130.0f, 54.0f);
            spec.angleDegrees = m_ui.Jitter() * 4.0f;
            spec.swayDegrees  = 0.6f;
            spec.sortOrder    = layer + 1;
            spec.parent       = m_sheet;
            const auto paper  = m_ui.Paper(spec);
            toggle.texts.push_back(m_ui.Text(names[i], hlslpp::float2(4.0f, 2.0f), 0.6f, kInkColor, layer + 2, paper));
            m_ui.MakeButton(paper);
            toggle.tags.push_back(paper);
        }
        m_toggles.push_back(toggle);
        RefreshToggle(m_toggles.back(), false);
    }

    //----------------------------------------------------------------------------
    //! 切り替えの札の見た目を、今の値に合わせます。
    //----------------------------------------------------------------------------
    void OptionsSheet::RefreshToggle(Toggle& toggle, bool punch) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        // 画面の揺れは true が1枚目（あり）なので、どちらの札を選んでいるかは値の向きで変わる
        const bool inverted = toggle.value == &Game::CurrentOptions().cameraShake;
        const int  selected = (*toggle.value != inverted) ? 1 : 0;
        for(size_t i = 0; i < toggle.texts.size(); ++i) {
            const bool chosen = static_cast<int>(i) == selected;
            registry.GetComponent<BuiltIn::FontComponent>(toggle.texts[i]).color = chosen ? kSealRedColor : kFadedInkColor;
            if(chosen && punch) {
                m_ui.Punch(toggle.tags[i], 0.2f);
            }
        }
    }

    //----------------------------------------------------------------------------
    //! 紙を閉じ、設定を保存します。
    //----------------------------------------------------------------------------
    void OptionsSheet::Close() {
        if(m_sheet == entt::null || m_closing) {
            return;
        }
        Game::StoreOptions();
        m_ui.SlideOut(m_sheet, hlslpp::float2(0.0f, 620.0f), 0.0f, kCloseSeconds);
        m_closing    = true;
        m_closeTimer = kCloseSeconds;
        m_dragging   = -1;
    }

    //----------------------------------------------------------------------------
    //! マウスの位置を、スライダーの線のローカル座標の X にします。
    //----------------------------------------------------------------------------
    float OptionsSheet::MouseAlong(const Slider& slider) {
        int x = 0;
        int y = 0;
        m_context.inputSystem->GetMousePosition(&x, &y);
        const hlslpp::float2 ui = BuiltIn::GetUICanvas(m_scene.GetRegistry()).ToUI(hlslpp::float2(static_cast<float>(x), static_cast<float>(y)));

        // 紙は傾いて揺れているので、線のワールド行列で線のローカル空間へ戻す
        const auto&          transform = m_scene.GetRegistry().GetComponent<BuiltIn::TransformComponent>(slider.track);
        const hlslpp::float4 local     = hlslpp::mul(hlslpp::float4(ui, 0.0f, 1.0f), hlslpp::inverse(transform.worldMatrix));
        return float(local.x);
    }

    //----------------------------------------------------------------------------
    //! スライダーの値を音量に反映し、試し鳴りします。
    //----------------------------------------------------------------------------
    void OptionsSheet::OnSliderChanged(size_t index) {
        Game::ApplyOptions(m_context);
        // BGM は流れている曲がそのまま試し鳴りになる。全体と効果音は小さな音で確かめさせる
        if(index != 1 && m_previewTimer <= 0.0f) {
            Game::Sounds(m_context).Play("tick", 0.8f);
            m_previewTimer = kPreviewEvery;
        }
    }

    //----------------------------------------------------------------------------
    //! 毎フレームの入力と見た目の更新を行います。
    //----------------------------------------------------------------------------
    bool OptionsSheet::Update(float deltaTime) {
        if(m_sheet == entt::null) {
            return false;
        }
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        // 滑り出し終わったら片付ける
        if(m_closing) {
            m_closeTimer -= deltaTime;
            if(m_closeTimer <= 0.0f) {
                m_ui.Destroy(m_sheet);
                m_sheet = entt::null;
                m_close = entt::null;
                m_sliders.clear();
                m_toggles.clear();
                m_closing = false;
            }
            return false;
        }

        Tsukino::Input::InputSystem& input = *m_context.inputSystem;
        m_previewTimer -= deltaTime;

        // スライダー：押したらつかみ、押している間は位置に合わせる
        for(size_t i = 0; i < m_sliders.size(); ++i) {
            if(m_ui.Clicked(m_sliders[i].hit)) {
                m_dragging     = static_cast<int>(i);
                m_previewTimer = 0.0f;
                m_ui.Punch(m_sliders[i].knob, 0.25f);
            }
        }
        if(m_dragging >= 0) {
            Slider&     slider = m_sliders[static_cast<size_t>(m_dragging)];
            const float value  = std::clamp(MouseAlong(slider) / kTrackWidth, 0.0f, 1.0f);
            if(std::abs(value - *slider.value) > 0.004f) {
                *slider.value = value;
                m_ui.SetText(slider.text, std::to_wstring(static_cast<int>(std::lround(value * 100.0f))));
                OnSliderChanged(static_cast<size_t>(m_dragging));
            }
            if(!input.IsKeyDown(KeyCode::LButton)) {
                m_dragging = -1;
            }
        }

        // 線とつまみは値をなめらかに追う。つかんでいる間はつまみを少し大きく
        const float rate = 1.0f - std::exp(-18.0f * deltaTime);
        for(size_t i = 0; i < m_sliders.size(); ++i) {
            Slider& slider = m_sliders[i];
            slider.shown += (*slider.value - slider.shown) * rate;
            registry.GetComponent<BuiltIn::UIRectComponent>(slider.fill).size.x  = kTrackWidth * slider.shown;
            registry.GetComponent<BuiltIn::SpriteComponent>(slider.fill).uvRect.z = slider.shown;

            auto&       knob  = registry.GetComponent<BuiltIn::TransformComponent>(slider.knob);
            const float scale = (m_dragging == static_cast<int>(i)) ? 1.3f : (m_ui.Hovered(slider.hit) ? 1.12f : 1.0f);
            knob.position     = hlslpp::float3(kTrackWidth * slider.shown, 0.0f, 0.0f);
            knob.scale += (hlslpp::float3(scale, scale, 1.0f) - knob.scale) * rate;
            knob.dirty = true;
        }

        // 切り替えの札
        for(Toggle& toggle : m_toggles) {
            const bool inverted = toggle.value == &Game::CurrentOptions().cameraShake;
            for(size_t i = 0; i < toggle.tags.size(); ++i) {
                if(!m_ui.Clicked(toggle.tags[i])) {
                    continue;
                }
                const bool next = (i == 1) != inverted;
                if(next != *toggle.value) {
                    *toggle.value = next;
                    Game::Sounds(m_context).Play("stamp", 0.6f);
                    RefreshToggle(toggle, true);
                }
            }
        }

        if(m_ui.Clicked(m_close) || input.IsKeyPressed(KeyCode::Escape)) {
            Game::Sounds(m_context).Play("page", 0.7f);
            Close();
            return true;
        }
        return false;
    }

}    // namespace ChronoSort::UI
