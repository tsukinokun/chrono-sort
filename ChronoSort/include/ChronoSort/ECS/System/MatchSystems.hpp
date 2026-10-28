//----------------------------------------------------------------------------
//! @file   MatchSystems.hpp
//! @brief  対局を進めるシステム
//! @detail どれも MatchSession（レジストリのコンテキスト）を読み書きし、無いシーンでは何もしません。
//!         SystemSetup の Order::kMatch に、ここに並べた順で登録します
//!         （段取り → CPU → 人間の入力 → 公開の演出 → カメラ。どれも並べ方より前に動く）。
//!         一時停止中は、段取り・CPU・入力・公開の演出を進めません（カメラの漂いは止めない）。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>
#include <Tsukino/Core/ECS/System/ISystem.hpp>

#include <vector>

namespace Tsukino::ECS {
    class EventBus;
}    // namespace Tsukino::ECS

namespace ChronoSort::ECS {

    //! 時間を置いた段取り（手番を始める・回す・結果を出す）と、見た目だけの時間差を進めます。
    class MatchFlowSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //! CPU の手番：手札を手に取って迷い、置き先を決めて出します。
    class CpuTurnSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //! 人間の入力：手札のホバー・ドラッグ・クリックで選ぶ・置き先の予告、列のホイール送り、放置時間。
    class HumanInputSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //! 出したカードの公開と判定の演出を、RevealComponent の段階に沿って進めます。
    class RevealSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //! 卓のカメラに、揺れと漂いを足します（CameraRigComponent）。
    class CameraRigSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //! 対局の出来事を受けて、音・粒子・画面 UI・発光・カメラの揺れで返します。
    //! 自分では何も進めません（Update は空）。購読は自分の寿命の間だけ続きます。
    class MatchFeedbackSystem : public Tsukino::ECS::ISystem {
    public:

        //! 出来事の購読を始めます。
        //! @param  [in] registry 対局のシーンのレジストリ（MatchSession を持っていること）。
        //! @param  [in] events   対局のシーンの EventBus。
        MatchFeedbackSystem(Tsukino::ECS::Registry& registry, Tsukino::ECS::EventBus& events);

        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        std::vector<Tsukino::ECS::ScopedConnection> m_connections;    // 購読（破棄で解除される）
    };

    //! カメラを揺らします（設定で揺れを切っていれば何もしない）。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     strength 揺れの強さ。
    void ShakeCamera(Tsukino::ECS::Registry& registry, float strength);

}    // namespace ChronoSort::ECS
