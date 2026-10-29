//----------------------------------------------------------------------------
//! @file   CardAlbum.cpp
//! @brief  対局の後に、出たカードを見る紙の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/CardAlbum.hpp>

#include <ChronoSort/ECS/Component/PaperComponent.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/UI/Widgets.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>
#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <algorithm>
#include <string>

namespace ChronoSort::UI {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;

        constexpr int   kColumns      = 6;                                   // 1頁の列の数
        constexpr int   kRows         = 2;                                   // 1頁の段の数
        constexpr int   kCardsPerPage = kColumns * kRows;                    // 1頁のカードの数
        const hlslpp::float2 kCardSize = hlslpp::float2(150.0f, 225.0f);     // 一覧のカードの大きさ（絵と同じ 2:3）
        constexpr float kColumnStep   = 200.0f;                              // 列の間隔
        constexpr float kRowY[kRows]  = {-135.0f, 135.0f};                   // 段の中心の高さ（紙の中心から）
        constexpr float kLabelGap     = 136.0f;                              // カードの中心から、行方の札の文字まで
        const hlslpp::float2 kZoomCardCenter = hlslpp::float2(-245.0f, 0.0f);     // 拡大したカードの位置（拡大の紙の中心から）
        const hlslpp::float2 kZoomCardSize   = hlslpp::float2(300.0f, 450.0f);    // 拡大したカードの大きさ
        constexpr float kZoomTextX    = -60.0f;                              // 拡大の紙の、文字の書き出し

        //! 行方の短い言い方です。
        const wchar_t* FateLabel(Rules::CardFate fate) {
            switch(fate) {
            case Rules::CardFate::Placed:
                return L"正解";
            case Rules::CardFate::Missed:
                return L"外れ";
            case Rules::CardFate::InHand:
                return L"手札に残った";
            }
            return L"";
        }

        //! 行方の文字の色です。
        hlslpp::float4 FateColor(Rules::CardFate fate) {
            switch(fate) {
            case Rules::CardFate::Missed:
                return kSealRedColor;
            case Rules::CardFate::InHand:
                return kFadedInkColor;
            default:
                return kInkColor;
            }
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 作り先のシーン・アセット管理・見せる対局を受け取ります。
    //----------------------------------------------------------------------------
    CardAlbum::CardAlbum(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager, const Rules::Match& match)
        : m_ui(scene, assetManager)
        , m_match(match)
        , m_layout(Util::AssetPaths::UILayout("CardAlbum")) {
    }

    //----------------------------------------------------------------------------
    //! 紙を出します。
    //----------------------------------------------------------------------------
    void CardAlbum::Open() {
        if(m_sheet != entt::null) {
            return;
        }
        if(!m_loaded) {
            m_credits = Util::LoadCredits();
            m_loaded  = true;
        }
        // 年代順に並べる（同じ年は出た順のまま）
        m_cards = m_match.AppearedCards();
        std::stable_sort(m_cards.begin(), m_cards.end(), [](const Rules::CardData& a, const Rules::CardData& b) { return a.year < b.year; });
        m_pageCount = std::max(1, static_cast<int>((m_cards.size() + kCardsPerPage - 1) / kCardsPerPage));

        const PaperSpec sheet = m_layout.Spec("albumSheet");
        m_sheet               = m_ui.Paper(sheet);
        m_ui.SlideIn(m_sheet, hlslpp::float2(0.0f, -kReferenceHeight), 0.0f, 0.55f);
        m_ui.Text(L"この対局に出たカード（" + std::to_wstring(m_cards.size()) + L" 枚）", hlslpp::float2(0.0f, -338.0f), 0.85f, kInkColor,
                  sheet.sortOrder + 1, m_sheet);
        m_pageNo = m_ui.Text(L"", hlslpp::float2(0.0f, 334.0f), 0.5f, kFadedInkColor, sheet.sortOrder + 1, m_sheet);

        auto button = [&](const char* name, const std::wstring& label) {
            PaperSpec spec = m_layout.Spec(name);
            spec.parent    = m_sheet;
            return NoteButton(m_ui, spec, label, 0.62f);
        };
        m_prev  = button("albumPrev", L"前の頁");
        m_next  = button("albumNext", L"次の頁");
        m_close = button("albumClose", L"閉じる");
        ShowPage(0, 0);
    }

    //----------------------------------------------------------------------------
    //! 1段戻ります。
    //----------------------------------------------------------------------------
    void CardAlbum::Back() {
        if(m_zoom != entt::null) {
            CloseZoom();
            return;
        }
        if(m_sheet == entt::null) {
            return;
        }
        // 滑り出してから片付ける（滑り出しの間に札を押されないよう、先に参照を外す）
        m_ui.SlideOut(m_sheet, hlslpp::float2(0.0f, -kReferenceHeight), 0.0f, 0.4f);
        m_ui.DestroyAfter(m_sheet, 0.45f);
        m_sheet = m_page = m_pageNo = m_prev = m_next = m_close = entt::null;
        m_pageCards.clear();
    }

    //----------------------------------------------------------------------------
    //! 札・カードのクリックと、頁送りのキー・ホイールを受けます。
    //----------------------------------------------------------------------------
    void CardAlbum::Update(const Tsukino::Input::InputSystem& input) {
        if(m_sheet == entt::null) {
            return;
        }
        Game::SoundBank& sounds = Game::Services().Sounds();

        // 拡大中は、拡大の紙の札か紙の外を押すと一覧に戻る
        if(m_zoom != entt::null) {
            if(m_ui.Clicked(m_zoomBack) || m_ui.Clicked(m_catcher)) {
                sounds.Play("ui.close");
                CloseZoom();
            }
            return;
        }

        if(m_ui.Clicked(m_close)) {
            sounds.Play("ui.close");
            Back();
            return;
        }
        const bool next = m_ui.Clicked(m_next) || input.IsKeyPressed(Tsukino::Input::KeyCode::Right) || input.GetWheelDelta() < 0.0f;
        const bool prev = m_ui.Clicked(m_prev) || input.IsKeyPressed(Tsukino::Input::KeyCode::Left) || input.GetWheelDelta() > 0.0f;
        if(next && m_current + 1 < m_pageCount) {
            sounds.Play("ui.close");
            ShowPage(m_current + 1, 1);
            return;
        }
        if(prev && m_current > 0) {
            sounds.Play("ui.close");
            ShowPage(m_current - 1, -1);
            return;
        }
        for(size_t i = 0; i < m_pageCards.size(); ++i) {
            if(m_ui.Clicked(m_pageCards[i])) {
                sounds.Play("ui.open");
                ShowZoom(static_cast<size_t>(m_current) * kCardsPerPage + i);
                return;
            }
        }
    }

    //----------------------------------------------------------------------------
    //! 指定した頁を出します。
    //----------------------------------------------------------------------------
    void CardAlbum::ShowPage(int page, int direction) {
        m_current = std::clamp(page, 0, m_pageCount - 1);

        // 新しい頁は、めくる向きの反対側から滑り込ませる
        if(m_page != entt::null) {
            m_ui.Destroy(m_page);
        }
        Tsukino::ECS::Registry& registry = m_ui.Scene().GetRegistry();
        m_page                           = m_ui.Scene().CreateEntity();
        auto& transform                  = registry.AddComponent<BuiltIn::TransformComponent>(m_page);
        transform.parent                 = m_sheet;
        // 頁は紙と同じ位置に重ねる入れ物なので、紙の内側の四角を写し、文字を紙の内側に収める
        registry.AddComponent<ECS::PaperComponent>(m_page, registry.GetComponent<ECS::PaperComponent>(m_sheet));
        m_pageCards.clear();

        const int layer = m_layout.Spec("albumSheet").sortOrder + 2;
        const size_t first = static_cast<size_t>(m_current) * kCardsPerPage;
        for(size_t i = first; i < std::min(m_cards.size(), first + kCardsPerPage); ++i) {
            const int            slot   = static_cast<int>(i - first);
            const float          x      = (static_cast<float>(slot % kColumns) - (kColumns - 1) * 0.5f) * kColumnStep;
            const float          y      = kRowY[slot / kColumns];
            const Rules::CardFate fate  = m_match.FateOf(m_cards[i]);

            // カードは机に並べたように、少しずつ傾けて置く
            PaperSpec card;
            card.center       = hlslpp::float2(x, y);
            card.size         = kCardSize;
            card.angleDegrees = m_ui.Jitter() * 2.5f;
            card.swayDegrees  = 0.0f;
            card.bobPixels    = 0.0f;
            card.sortOrder    = layer;
            card.parent       = m_page;
            const auto picture = m_ui.Picture(Util::AssetPaths::CardRevealedTexture(m_cards[i].id), card);
            m_ui.MakeButton(picture);
            m_pageCards.push_back(picture);

            if(fate == Rules::CardFate::Missed) {
                PaperSpec stamp;
                stamp.texture      = "stamp_wrong.png";
                stamp.center       = hlslpp::float2(38.0f, -70.0f);
                stamp.size         = hlslpp::float2(78.0f, 78.0f);
                stamp.angleDegrees = -14.0f + m_ui.Jitter() * 6.0f;
                stamp.swayDegrees  = 0.0f;
                stamp.bobPixels    = 0.0f;
                stamp.sortOrder    = layer + 1;
                stamp.parent       = picture;
                m_ui.Paper(stamp);
            }
            m_ui.Text(FateLabel(fate), hlslpp::float2(x, y + kLabelGap), 0.48f, FateColor(fate), layer + 1, m_page);
        }
        if(direction != 0) {
            m_ui.SlideIn(m_page, hlslpp::float2(260.0f * static_cast<float>(direction), 0.0f), 0.0f, 0.35f);
        }
        m_ui.SetText(m_pageNo, std::to_wstring(m_current + 1) + L" / " + std::to_wstring(m_pageCount));
    }

    //----------------------------------------------------------------------------
    //! カードを1枚大きく出します。
    //----------------------------------------------------------------------------
    void CardAlbum::ShowZoom(size_t index) {
        if(index >= m_cards.size() || m_zoom != entt::null) {
            return;
        }
        const Rules::CardData&  card     = m_cards[index];
        const Rules::CardFate   fate     = m_match.FateOf(card);
        Tsukino::ECS::Registry& registry = m_ui.Scene().GetRegistry();

        // 紙の外のクリックを受ける透明な板（一覧の札やカードにも届かないよう、画面いっぱいに重ねる）
        PaperSpec catcher  = m_layout.Spec("zoomCatcher");
        m_catcher          = m_ui.Paper(catcher);
        registry.GetComponent<BuiltIn::SpriteComponent>(m_catcher).tintColor = hlslpp::float4(0.0f, 0.0f, 0.0f, 0.0f);
        registry.AddComponent<BuiltIn::PointerTargetComponent>(m_catcher);

        const PaperSpec sheet = m_layout.Spec("zoomSheet");
        m_zoom                = m_ui.Paper(sheet);
        m_ui.SlideIn(m_zoom, hlslpp::float2(0.0f, 40.0f), 0.0f, 0.25f);
        const int layer = sheet.sortOrder + 1;

        PaperSpec picture;
        picture.center       = kZoomCardCenter;
        picture.size         = kZoomCardSize;
        picture.angleDegrees = -1.5f;
        picture.swayDegrees  = 0.0f;
        picture.bobPixels    = 0.0f;
        picture.sortOrder    = layer;
        picture.parent       = m_zoom;
        m_ui.Picture(Util::AssetPaths::CardRevealedTexture(card.id), picture);

        // 右側に、カードの中身と絵の出典を書く
        float y    = -205.0f;
        auto  line = [&](const std::wstring& text, float scale, const hlslpp::float4& color, float advance) {
            m_ui.Text(text, hlslpp::float2(kZoomTextX, y), scale, color, layer, m_zoom, false);
            y += advance;
        };
        line(Util::Utf8ToWide(card.title), 0.85f, kInkColor, 62.0f);
        line(Util::Utf8ToWide(card.yearLabel), 0.8f, kInkColor, 60.0f);
        line(std::wstring(card.region == "JP" ? L"日本" : L"世界") + L"の出来事  ・  " + Util::Utf8ToWide(card.category), 0.55f, kFadedInkColor, 46.0f);
        line(std::wstring(L"この対局では  ") + FateLabel(fate), 0.55f, FateColor(fate), 70.0f);
        if(const Util::CreditEntry* credit = m_credits.FindImage(card.id)) {
            line(L"絵の出典", 0.5f, kSealRedColor, 38.0f);
            line(Util::Utf8ToWide(credit->artist), 0.44f, kInkColor, 32.0f);
            line(Util::Utf8ToWide(credit->license), 0.44f, kFadedInkColor, 32.0f);
            line(Util::Utf8ToWide(credit->file), 0.38f, kFadedInkColor, 32.0f);
        }

        PaperSpec back = m_layout.Spec("zoomBack");
        back.parent    = m_zoom;
        m_zoomBack     = NoteButton(m_ui, back, L"一覧に戻る", 0.62f);
    }

    //----------------------------------------------------------------------------
    //! 大きく出したカードを引っ込めます。
    //----------------------------------------------------------------------------
    void CardAlbum::CloseZoom() {
        if(m_zoom == entt::null) {
            return;
        }
        m_ui.SlideOut(m_zoom, hlslpp::float2(0.0f, 40.0f), 0.0f, 0.2f);
        m_ui.DestroyAfter(m_zoom, 0.25f);
        m_ui.Destroy(m_catcher);
        m_zoom = m_zoomBack = m_catcher = entt::null;
    }

}    // namespace ChronoSort::UI
