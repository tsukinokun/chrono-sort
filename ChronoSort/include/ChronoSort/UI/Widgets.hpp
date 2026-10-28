//----------------------------------------------------------------------------
//! @file   Widgets.hpp
//! @brief  紙片の UI の部品（ボタンの札・選択肢の札・墨の線）
//! @detail どの画面でも同じ形で作る部品です。置き方（PaperSpec）は画面ごとの Data/UI/*.json から受け取ります。
//!         選択肢はエンジンの UIToggleComponent（組で1つだけ入）に乗せ、見た目は ChoiceTagSystem が合わせます。
//!         墨の線はエンジンのスプライトの左右の塗り（SpriteFillMode::Horizontal）で伸び縮みさせます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <hlsl++.h>

#include <string>

namespace ChronoSort::UI {

    //! 文字を書いた札のボタンを作ります。文字は札より1つ手前に描きます。
    //! @param  [in] ui          紙片の部品。
    //! @param  [in] paper       札の置き方。
    //! @param  [in] label       文字。
    //! @param  [in] labelScale  文字の大きさ。
    //! @param  [in] labelOffset 文字の位置（札の中心から）。
    //! @return 札（クリックは PaperUI::Clicked で受け取る）。
    Tsukino::ECS::Entity NoteButton(PaperUI& ui, const PaperSpec& paper, const std::wstring& label, float labelScale,
                                    const hlslpp::float2& labelOffset = hlslpp::float2(6.0f, 2.0f));

    //! 選択肢の組を作ります（この中の札から1つだけが選ばれる）。
    //! @param  [in] ui 紙片の部品。
    //! @return 組。
    Tsukino::ECS::Entity ChoiceGroup(PaperUI& ui);

    //! 選択肢の札1枚の作り方です。
    struct ChoiceSpec {
        PaperSpec      paper;                                          // 札の置き方
        std::wstring   label;                                          // 文字
        float          labelScale    = 0.6f;                           // 文字の大きさ
        hlslpp::float2 labelOffset   = hlslpp::float2(4.0f, 2.0f);     // 文字の位置（札の中心から）
        int            value         = 0;                              // この札を選んだときの値
        hlslpp::float4 chosenColor   = kSealRedColor;                  // 選ばれているときの文字の色
        hlslpp::float4 otherColor    = kFadedInkColor;                 // 選ばれていないときの文字の色
        float          punch         = 0.2f;                           // 選ばれたときに弾む強さ
        bool           punchOnCreate = false;                          // 作った直後も、選ばれている札を弾ませるか
    };

    //! 選択肢の札を1枚作ります。
    //! @param  [in] ui       紙片の部品。
    //! @param  [in] group    属する組（ChoiceGroup で作ったもの）。
    //! @param  [in] spec     作り方。
    //! @param  [in] selected 最初から選んでおくなら true。
    //! @return 札。文字は ChoiceTagComponent::text に入っている。
    Tsukino::ECS::Entity ChoiceTag(PaperUI& ui, Tsukino::ECS::Entity group, const ChoiceSpec& spec, bool selected);

    //! 組の中で選ばれている札の値を返します。
    //! @param  [in] registry レジストリ。
    //! @param  [in] group    組。
    //! @param  [in] fallback どれも選ばれていないときの値。
    //! @return 値。
    int SelectedChoice(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity group, int fallback);

    //! 組の選ばれている札が、このフレームにクリックで変わったかを返します。
    //! @param  [in] registry レジストリ。
    //! @param  [in] group    組。
    //! @return 変わったなら true。
    bool ChoiceChanged(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity group);

    //! 左端から伸び縮みする墨の線を作ります。
    //! @param  [in] ui        紙片の部品。
    //! @param  [in] parent    親（線の左端がこの原点に来る）。
    //! @param  [in] texture   テクスチャ。
    //! @param  [in] size      いちばん伸びたときの大きさ。
    //! @param  [in] sortOrder 描画順。
    //! @param  [in] tint      色。
    //! @param  [in] fill      伸びている割合（0〜1）。
    //! @return 線。
    Tsukino::ECS::Entity InkLine(PaperUI& ui, Tsukino::ECS::Entity parent, const std::string& texture, const hlslpp::float2& size, int sortOrder,
                                 const hlslpp::float4& tint, float fill);

    //! 墨の線の伸びている割合を変えます。
    //! @param  [in] registry レジストリ。
    //! @param  [in] line     線。
    //! @param  [in] fill     伸びている割合（0〜1）。
    void SetInkLineFill(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity line, float fill);

}    // namespace ChronoSort::UI
