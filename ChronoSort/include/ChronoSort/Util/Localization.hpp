//----------------------------------------------------------------------------
//! @file   Localization.hpp
//! @brief  表示言語と、キーから画面の文字列を引く表
//! @detail 画面に出す文字列は Assets/Data/Text/<言語>.json に「キー → 文字列」で置き、Text("キー") で引きます。
//!         カードの題名と説明文は Assets/Data/Text/Cards/<言語>.json に、カードの id ごとに置きます
//!         （日本語はデッキ JSON がそのまま原文なので、カードの表は日本語以外の言語にだけある）。
//!         表は起動時と、タイトルで言語を選び直したときに LoadLanguage で読み直します。
//!         対局の途中では言語を変えないので、一度引いた文字列を持ち続けても食い違いません。
//!         エンジンの資源を持たないので、Rules からも Util の関数として呼べます。
//----------------------------------------------------------------------------
#pragma once
#include <initializer_list>
#include <map>
#include <string>
#include <string_view>

namespace ChronoSort::Util {

    //! 表示言語です（値は設定の札の値にもなる）。
    enum class Language : int {
        Japanese = 0,    // 日本語（原文）
        English  = 1,    // 英語
    };

    //! カード1枚の訳です。
    struct CardText {
        std::string title;      // 出来事名（UTF-8）
        std::string caption;    // 説明文（UTF-8）
    };

    //! 言語を保存用のコード（"ja" / "en"）に変えます。
    //! @param  [in] language 言語。
    //! @return コード。
    const char* LanguageCode(Language language);

    //! 保存用のコードから言語を返します。知らないコードなら日本語にします。
    //! @param  [in] code コード。
    //! @return 言語。
    Language LanguageFromCode(const std::string& code);

    //! 言語の表を読み込み、今の言語にします。日本語の表は足りないキーの代わりにいつも読んでおきます。
    //! @param  [in] language 言語。
    void LoadLanguage(Language language);

    //! 今の言語を返します。
    //! @return 言語。
    Language CurrentLanguage();

    //! キーの文字列を今の言語で返します。無ければ日本語、それも無ければキーそのものを返します。
    //! @param  [in] key キー（"options.title" など）。
    //! @return 文字列。
    const std::wstring& Text(std::string_view key);

    //! キーの文字列の {0} {1} … を args で置き換えて返します。
    //! @param  [in] key  キー。
    //! @param  [in] args 置き換える文字列（{0} から順に）。
    //! @return 文字列。
    std::wstring Format(std::string_view key, std::initializer_list<std::wstring> args);

    //! 今の言語でのカードの訳を返します。日本語のとき、または訳が無いときは nullptr。
    //! @param  [in] id カードの id。
    //! @return 訳。
    const CardText* FindCardText(const std::string& id);

    //! 言語の表をそのまま読みます（今の言語は変えない。自己テストで表を見比べるため）。
    //! @param  [in] language 言語。
    //! @return キーと文字列（UTF-8）。読めなければ空。
    std::map<std::string, std::string> ReadTextTable(Language language);

    //! 言語のカードの表をそのまま読みます（今の言語は変えない）。
    //! @param  [in] language 言語。
    //! @return カードの id と訳。日本語や読めないときは空。
    std::map<std::string, CardText> ReadCardTable(Language language);

}    // namespace ChronoSort::Util
