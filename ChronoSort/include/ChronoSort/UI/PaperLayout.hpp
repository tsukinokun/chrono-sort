//----------------------------------------------------------------------------
//! @file   PaperLayout.hpp
//! @brief  画面に常に出しておく紙片の置き方（JSON から読む）
//! @detail ChronoSort/Assets/Data/UI/<画面>.json に、名前ごとの PaperSpec（テクスチャ・位置・大きさ・傾き・揺れ・描画順）を書きます。
//!         画面のコードは Spec("名前") で受け取り、文字・親・動きだけを足して紙片を作ります。
//!         位置や傾きを直したいときはコードではなく JSON を直します。書いていない項目は PaperSpec の既定値です。
//!
//!         書き方：
//!             { "papers": [ { "key": "seal", "value": { "texture": "wax_seal.png", "center": { "x": 112, "y": 118 }, ... } } ] }
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/UI/PaperUI.hpp>

#include <map>
#include <string>

namespace ChronoSort::UI {

    //! 画面に常に出しておく紙片の置き方です。
    class PaperLayout {
    public:

        PaperLayout() = default;

        //! 置き方の JSON を読み込みます。読めなければ空のまま（Spec は既定値を返す）にし、警告を出します。
        //! @param  [in] path ワークスペースルートからのパス。
        explicit PaperLayout(const std::string& path);

        //! 名前の紙片の作り方を返します。無い名前なら警告を出して既定値を返します。
        //! @param  [in] name 名前。
        //! @return 作り方（parent は入っていない）。
        PaperSpec Spec(const std::string& name) const;

    private:
        std::string                      m_path;      // 読んだファイル（警告に使う）
        std::map<std::string, PaperSpec> m_papers;    // 名前 → 作り方
    };

}    // namespace ChronoSort::UI
