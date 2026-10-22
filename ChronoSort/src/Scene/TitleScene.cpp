//----------------------------------------------------------------------------
//! @file   TitleScene.cpp
//! @brief  タイトル画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/TitleScene.hpp>

#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/UITweenSystem.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Renderer/Renderer.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>

namespace ChronoSort::Scene {

    namespace {
        constexpr float kScreenCenterX = 800.0f;    // 画面中央 X（1600x900 前提）
    }    // namespace

    //----------------------------------------------------------------------------
    //! シーンを初期化します。
    //----------------------------------------------------------------------------
    void TitleScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI&) {
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        // 古地図風の落ち着いた背景色
        context->renderer->SetClearColor(0.16f, 0.13f, 0.10f, 1.0f);

        // 2D のカードゲームなので、画面 UI に必要なシステムだけを登録する
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::InteractionSystem>(), -2);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::UITweenSystem>(), -1);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), 9);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), 10);

        CreateText(L"ChronoSort", kScreenCenterX, 220.0f, 3.0f);
        CreateText(L"歴史の出来事を、年代順に並べよう", kScreenCenterX, 330.0f, 1.0f);
        CreateText(L"Enter で次の見本カード", kScreenCenterX, 800.0f, 0.7f);

        // 日本史と世界史を混ぜた山札
        m_deck.LoadFromFile("ChronoSort/Assets/Data/Decks/japan.json");
        m_deck.LoadFromFile("ChronoSort/Assets/Data/Decks/world.json");
        m_rng.seed(std::random_device{}());
        m_deck.Shuffle(m_rng);

        m_sampleTitle = CreateText(L"", kScreenCenterX, 520.0f, 1.6f);
        m_sampleYear  = CreateText(L"", kScreenCenterX, 600.0f, 1.2f);
        ShowNextSampleCard();
    }

    //----------------------------------------------------------------------------
    //! シーンを更新します。
    //----------------------------------------------------------------------------
    void TitleScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI&, float deltaTime) {
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Enter)) {
            ShowNextSampleCard();
        }

        m_scene.Update(deltaTime);
    }

    //----------------------------------------------------------------------------
    //! シーンの終了処理を行います。
    //----------------------------------------------------------------------------
    void TitleScene::OnExit() {
    }

    //----------------------------------------------------------------------------
    //! 画面にテキストを1つ置きます。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity TitleScene::CreateText(const std::wstring& text, float x, float y, float scale) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Tsukino::ECS::Entity    entity   = m_scene.CreateEntity();

        auto& transform    = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
        transform.position = hlslpp::float3(x, y, 0.0f);

        auto& font           = registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(entity);
        font.text            = text;
        font.scale           = scale;
        font.color           = hlslpp::float4(0.96f, 0.90f, 0.78f, 1.0f);    // 羊皮紙色
        font.horizontalAlign = Tsukino::BuiltIn::ECS::HorizontalAlign::Center;
        font.verticalAlign   = Tsukino::BuiltIn::ECS::VerticalAlign::Middle;
        return entity;
    }

    //----------------------------------------------------------------------------
    //! 山札から1枚引き、見本カードの表示を差し替えます。
    //----------------------------------------------------------------------------
    void TitleScene::ShowNextSampleCard() {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto&                   title    = registry.GetComponent<Tsukino::BuiltIn::ECS::FontComponent>(m_sampleTitle);
        auto&                   year     = registry.GetComponent<Tsukino::BuiltIn::ECS::FontComponent>(m_sampleYear);

        const std::optional<Rules::CardData> card = m_deck.Draw();
        if(!card) {
            title.text = L"（山札がありません）";
            year.text  = L"";
            return;
        }
        title.text = Util::Utf8ToWide(card->title);
        year.text  = Util::Utf8ToWide(card->yearLabel);
    }

}    // namespace ChronoSort::Scene
