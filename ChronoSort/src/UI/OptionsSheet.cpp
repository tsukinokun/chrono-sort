//----------------------------------------------------------------------------
//! @file   OptionsSheet.cpp
//! @brief  設定の紙の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/OptionsSheet.hpp>

#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/UI/Widgets.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>
#include <ChronoSort/Util/Localization.hpp>

#include <Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UISliderComponent.hpp>
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

        constexpr float kTrackLeft    = -95.0f;    // 線の始点（紙の中心からの X）
        constexpr float kTrackWidth   = 300.0f;    // 線の長さ
        constexpr float kTrackHeight  = 20.0f;     // 線の太さ
        constexpr float kPreviewEvery = 0.12f;     // 効果音の試し鳴りの間隔（秒）
        constexpr float kCloseSeconds = 0.35f;     // 滑り出す時間
        constexpr float kLabelX       = -250.0f;   // 行の名前の X
    }    // namespace

    //----------------------------------------------------------------------------
    //! 作り先のシーンと描画順を受け取ります。
    //----------------------------------------------------------------------------
    OptionsSheet::OptionsSheet(Tsukino::ECS::Scene& scene, Tsukino::EngineIntegration::EngineContext& context, int sortOrder, bool allowLanguage)
        : m_scene(scene)
        , m_context(context)
        , m_ui(scene, *context.assetManager)
        , m_layout(Util::AssetPaths::UILayout("Options"))
        , m_sortOrder(sortOrder)
        , m_allowLanguage(allowLanguage) {
    }

    //----------------------------------------------------------------------------
    //! 紙を開きます。
    //----------------------------------------------------------------------------
    void OptionsSheet::Open() {
        if(m_sheet != entt::null) {
            return;
        }
        PaperSpec sheet = m_layout.Spec("sheet");
        sheet.sortOrder = m_sortOrder;
        m_sheet         = m_ui.Paper(sheet);
        m_ui.SlideIn(m_sheet, hlslpp::float2(0.0f, 620.0f), 0.0f, 0.5f);

        const int layer = m_sortOrder + 1;
        m_ui.Text(Util::Text("options.title"), hlslpp::float2(0.0f, float(m_layout.Spec("heading").center.y)), 1.15f, kInkColor, layer, m_sheet);

        Game::Options& options = Game::Services().Settings();
        AddSlider(Util::Text("options.master"), "rowMaster", &options.master, true);
        AddSlider(Util::Text("options.bgm"), "rowBgm", &options.bgm, false);
        AddSlider(Util::Text("options.sfx"), "rowSfx", &options.sfx, true);
        AddToggle(Util::Text("options.speed"), "rowFast", {Util::Text("options.speedNormal"), false}, {Util::Text("options.speedFast"), true}, &options.fastEffects);
        AddToggle(Util::Text("options.shake"), "rowShake", {Util::Text("options.shakeOn"), true}, {Util::Text("options.shakeOff"), false}, &options.cameraShake);
        AddQualityChoice(Util::Text("options.quality"), "rowQuality", &options.graphicsQuality);
        m_openedLanguage = options.language;
        AddLanguageChoice(Util::Text("options.language"), "rowLanguage");

        PaperSpec close = m_layout.Spec("close");
        close.sortOrder = layer + 1;
        close.parent    = m_sheet;
        m_close         = NoteButton(m_ui, close, Util::Text("common.close"), 0.72f);
        m_previewTimer  = 0.0f;
    }

    //----------------------------------------------------------------------------
    //! スライダーを1本作ります。
    //----------------------------------------------------------------------------
    void OptionsSheet::AddSlider(const std::wstring& label, const std::string& row, float* value, bool preview) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const int               layer    = m_sortOrder + 1;
        const float             y        = float(m_layout.Spec(row).center.y);
        m_ui.Text(label, hlslpp::float2(kLabelX, y), 0.62f, kFadedInkColor, layer, m_sheet, false);

        Slider slider;
        slider.value   = value;
        slider.preview = preview;
        slider.shown   = *value;

        // 線の始点を原点にした入れ物。紙の傾き・揺れに付いていく
        const Tsukino::ECS::Entity track = m_scene.CreateEntity();
        auto&                      at    = registry.AddComponent<BuiltIn::TransformComponent>(track);
        at.position                      = hlslpp::float3(kTrackLeft, y, 0.0f);
        at.parent                        = m_sheet;

        // 薄い下書きの線と、その上に引いた墨の線
        InkLine(m_ui, track, "ink_line.png", hlslpp::float2(kTrackWidth, kTrackHeight * 0.7f), layer, hlslpp::float4(1.0f, 1.0f, 1.0f, 0.22f), 1.0f);
        slider.fill = InkLine(m_ui, track, "ink_line.png", hlslpp::float2(kTrackWidth, kTrackHeight), layer + 1, hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f), slider.shown);

        // 線より少し広い透明の当たり判定（細い線でもつかみやすく）。つかんで動かすのはエンジンの UISliderComponent
        slider.hit = InkLine(m_ui, track, "glow.png", hlslpp::float2(kTrackWidth + 36.0f, 44.0f), layer + 4, hlslpp::float4(1.0f, 1.0f, 1.0f, 0.0f), 1.0f);
        registry.GetComponent<BuiltIn::TransformComponent>(slider.hit).position = hlslpp::float3(-18.0f, 0.0f, 0.0f);
        registry.AddComponent<BuiltIn::PointerTargetComponent>(slider.hit);
        auto& input           = registry.AddComponent<BuiltIn::UISliderComponent>(slider.hit);
        input.track           = track;
        input.trackLength     = kTrackWidth;
        input.changeThreshold = 0.004f;    // マウスの細かい揺れで音を鳴らし続けない
        input.value           = *value;

        PaperSpec knob = m_layout.Spec("knob");
        knob.center    = hlslpp::float2(kTrackWidth * slider.shown, 0.0f);
        knob.sortOrder = layer + 2;
        knob.parent    = track;
        slider.knob    = m_ui.Paper(knob);

        slider.text = m_ui.Text(std::to_wstring(static_cast<int>(std::lround(*value * 100.0f))), hlslpp::float2(kTrackWidth + 44.0f, 0.0f), 0.5f, kFadedInkColor,
                                layer, track);
        m_sliders.push_back(slider);
    }

    //----------------------------------------------------------------------------
    //! 札2枚の切り替えを1つ作ります。
    //----------------------------------------------------------------------------
    void OptionsSheet::AddToggle(const std::wstring& label, const std::string& row, const std::pair<std::wstring, bool>& first,
                                 const std::pair<std::wstring, bool>& second, bool* value) {
        const int   layer = m_sortOrder + 1;
        const float y     = float(m_layout.Spec(row).center.y);
        m_ui.Text(label, hlslpp::float2(kLabelX, y), 0.62f, kFadedInkColor, layer, m_sheet, false);

        Toggle toggle;
        toggle.value = value;
        toggle.group = ChoiceGroup(m_ui);

        const std::pair<std::wstring, bool> choices[] = {first, second};
        for(int i = 0; i < 2; ++i) {
            ChoiceSpec spec;
            spec.paper              = m_layout.Spec("toggleTag");
            spec.paper.center       = hlslpp::float2(float(spec.paper.center.x) + 160.0f * static_cast<float>(i), y + m_ui.Jitter() * 5.0f);
            spec.paper.angleDegrees = m_ui.Jitter() * 4.0f;
            spec.paper.sortOrder    = layer + 1;
            spec.paper.parent       = m_sheet;
            spec.label              = choices[i].first;
            spec.value              = choices[i].second ? 1 : 0;
            ChoiceTag(m_ui, toggle.group, spec, choices[i].second == *value);
        }
        m_toggles.push_back(toggle);
    }

    //----------------------------------------------------------------------------
    //! 画質の選択を作ります。
    //----------------------------------------------------------------------------
    void OptionsSheet::AddQualityChoice(const std::wstring& label, const std::string& row, int* value) {
        const int   layer = m_sortOrder + 1;
        const float y     = float(m_layout.Spec(row).center.y);
        m_ui.Text(label, hlslpp::float2(kLabelX, y), 0.62f, kFadedInkColor, layer, m_sheet, false);

        QualityChoice quality;
        quality.value = value;
        quality.group = ChoiceGroup(m_ui);

        const PaperSpec tag      = m_layout.Spec("qualityTag");
        const float     interval = float(tag.size.x) + 10.0f;
        const std::pair<std::wstring, Game::GraphicsQualityChoice> choices[] = {
            {Util::Text("options.qualityAuto"), Game::GraphicsQualityChoice::Auto},
            {Util::Text("options.qualityHigh"), Game::GraphicsQualityChoice::High},
            {Util::Text("options.qualityMedium"), Game::GraphicsQualityChoice::Medium},
            {Util::Text("options.qualityLow"), Game::GraphicsQualityChoice::Low},
        };
        for(int i = 0; i < 4; ++i) {
            ChoiceSpec spec;
            spec.paper              = tag;
            spec.paper.center       = hlslpp::float2(float(tag.center.x) + interval * static_cast<float>(i), y + m_ui.Jitter() * 5.0f);
            spec.paper.angleDegrees = m_ui.Jitter() * 4.0f;
            spec.paper.sortOrder    = layer + 1;
            spec.paper.parent       = m_sheet;
            spec.label              = choices[i].first;
            spec.value              = static_cast<int>(choices[i].second);
            ChoiceTag(m_ui, quality.group, spec, spec.value == *value);
        }
        m_qualities.push_back(quality);

        // カード面の細かさだけは読み込みのときに決まるので、次の起動から変わる
        m_ui.Text(Util::Text("options.qualityNote"), hlslpp::float2(kLabelX, float(m_layout.Spec("noteQuality").center.y)), 0.42f, kFadedInkColor, layer, m_sheet, false);
    }

    //----------------------------------------------------------------------------
    //! 言語の選択を作ります。
    //----------------------------------------------------------------------------
    void OptionsSheet::AddLanguageChoice(const std::wstring& label, const std::string& row) {
        const int   layer = m_sortOrder + 1;
        const float y     = float(m_layout.Spec(row).center.y);
        m_ui.Text(label, hlslpp::float2(kLabelX, y), 0.62f, kFadedInkColor, layer, m_sheet, false);

        // 対局中に言語を変えると、場のカードの面や名前が途中で食い違うので、タイトルで変えてもらう
        if(!m_allowLanguage) {
            m_ui.Text(Util::Text("options.languageNote"), hlslpp::float2(float(m_layout.Spec("languageTag").center.x) - 60.0f, y), 0.42f, kFadedInkColor, layer, m_sheet,
                      false, true);
            return;
        }

        m_languageGroup = ChoiceGroup(m_ui);
        // 札の文字は、読めない言語のときでも自分の言語を見つけられるよう、その言語自身の書き方にする
        const std::pair<const wchar_t*, Util::Language> choices[] = {
            {L"日本語", Util::Language::Japanese},
            {L"English", Util::Language::English},
        };
        const PaperSpec tag     = m_layout.Spec("languageTag");
        const int       current = static_cast<int>(Util::LanguageFromCode(Game::Services().Settings().language));
        for(int i = 0; i < 2; ++i) {
            ChoiceSpec spec;
            spec.paper              = tag;
            spec.paper.center       = hlslpp::float2(float(tag.center.x) + (float(tag.size.x) + 30.0f) * static_cast<float>(i), y + m_ui.Jitter() * 5.0f);
            spec.paper.angleDegrees = m_ui.Jitter() * 4.0f;
            spec.paper.sortOrder    = layer + 1;
            spec.paper.parent       = m_sheet;
            spec.label              = choices[i].first;
            spec.value              = static_cast<int>(choices[i].second);
            ChoiceTag(m_ui, m_languageGroup, spec, spec.value == current);
        }
    }

    //----------------------------------------------------------------------------
    //! 開いたときと違う言語になったかを返します。
    //----------------------------------------------------------------------------
    bool OptionsSheet::LanguageChanged() const {
        return !m_openedLanguage.empty() && m_openedLanguage != Game::Services().Settings().language;
    }

    //----------------------------------------------------------------------------
    //! 紙を閉じ、設定を保存します。
    //----------------------------------------------------------------------------
    void OptionsSheet::Close() {
        if(m_sheet == entt::null) {
            return;
        }
        Game::Services().StoreOptions();
        // 滑り出し終わったら LifetimeSystem が子ごと片付ける。こちらは先に手を離し、すぐ次を開けるようにする
        m_ui.SlideOut(m_sheet, hlslpp::float2(0.0f, 620.0f), 0.0f, kCloseSeconds);
        m_ui.DestroyAfter(m_sheet, kCloseSeconds);
        m_sheet = entt::null;
        m_close = entt::null;
        m_sliders.clear();
        m_toggles.clear();
        m_qualities.clear();
        m_languageGroup = entt::null;
    }

    //----------------------------------------------------------------------------
    //! 毎フレームの入力と見た目の更新を行います。
    //----------------------------------------------------------------------------
    bool OptionsSheet::Update(float deltaTime) {
        if(m_sheet == entt::null) {
            return false;
        }
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        m_previewTimer -= deltaTime;

        // スライダー：値を設定へ写し、音量に反映して試し鳴りする（つかんだ瞬間はすぐ鳴らせるようにする）
        const float rate = 1.0f - std::exp(-18.0f * deltaTime);
        for(Slider& slider : m_sliders) {
            const auto& input = registry.GetComponent<BuiltIn::UISliderComponent>(slider.hit);
            if(input.dragStarted) {
                m_previewTimer = 0.0f;
                m_ui.Punch(slider.knob, 0.25f);
            }
            if(input.changed) {
                *slider.value = input.value;
                m_ui.SetText(slider.text, std::to_wstring(static_cast<int>(std::lround(input.value * 100.0f))));
                Game::Services().ApplyOptions();
                if(slider.preview && m_previewTimer <= 0.0f) {
                    Game::Services().Sounds().Play("ui.preview");
                    m_previewTimer = kPreviewEvery;
                }
            }

            // 線とつまみは値をなめらかに追う。つかんでいる間はつまみを少し大きく
            slider.shown += (*slider.value - slider.shown) * rate;
            SetInkLineFill(registry, slider.fill, slider.shown);

            auto&       knob  = registry.GetComponent<BuiltIn::TransformComponent>(slider.knob);
            const float scale = input.dragging ? 1.3f : (m_ui.Hovered(slider.hit) ? 1.12f : 1.0f);
            knob.position     = hlslpp::float3(kTrackWidth * slider.shown, 0.0f, 0.0f);
            knob.scale += (hlslpp::float3(scale, scale, 1.0f) - knob.scale) * rate;
            knob.dirty = true;
        }

        // 切り替えの札（選んだ札の見た目と音は ChoiceTagSystem）
        for(Toggle& toggle : m_toggles) {
            if(ChoiceChanged(registry, toggle.group)) {
                *toggle.value = SelectedChoice(registry, toggle.group, *toggle.value ? 1 : 0) != 0;
            }
        }

        // 画質の札（選んだらすぐ描画へ反映する）
        for(QualityChoice& quality : m_qualities) {
            if(ChoiceChanged(registry, quality.group)) {
                *quality.value = SelectedChoice(registry, quality.group, *quality.value);
                Game::Services().Graphics().SetChoice(static_cast<Game::GraphicsQualityChoice>(*quality.value));
            }
        }

        // 言語の札（表の読み直しと画面の作り直しは、閉じた後にタイトルが行う）
        if(m_languageGroup != entt::null && ChoiceChanged(registry, m_languageGroup)) {
            const int current = static_cast<int>(Util::LanguageFromCode(Game::Services().Settings().language));
            Game::Services().Settings().language = Util::LanguageCode(static_cast<Util::Language>(SelectedChoice(registry, m_languageGroup, current)));
        }

        if(m_ui.Clicked(m_close) || m_context.inputSystem->IsKeyPressed(KeyCode::Escape)) {
            Game::Services().Sounds().Play("ui.close");
            Close();
            return true;
        }
        return false;
    }

}    // namespace ChronoSort::UI
