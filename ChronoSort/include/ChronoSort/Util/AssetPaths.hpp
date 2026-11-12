//----------------------------------------------------------------------------
//! @file   AssetPaths.hpp
//! @brief  ゲームのアセットの置き場所
//! @detail ChronoSort/Assets の下のどのフォルダに何を置くかを、ここ1か所で決めます。
//!         コードはファイル名や名前だけを持ち、フォルダはここの関数で付けます（Prefab の目次は Game/Prefabs.hpp）。
//!         読込画面で先に読むアセットの一覧（Game/AssetManifest）も、ここのフォルダを使います。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Util/Localization.hpp>

#include <filesystem>
#include <string>
#include <system_error>

namespace ChronoSort::Util::AssetPaths {

    //! アセットの置き場所の根元
    inline constexpr const char* kRoot = "ChronoSort/Assets/";

    // フォルダ（読込画面でまとめて読むときにも使う）
    inline constexpr const char* kUITextureDir   = "ChronoSort/Assets/Textures/UI";       // 紙片・印などの画面 UI の画像
    inline constexpr const char* kFxTextureDir   = "ChronoSort/Assets/Textures/Fx";       // 粒子の画像
    inline constexpr const char* kCardTextureDir = "ChronoSort/Assets/Textures/Cards";    // カードの面の画像（カードの id ごとのフォルダ）
    inline constexpr const char* kSfxDir         = "ChronoSort/Assets/Audio/Sfx";         // 効果音（.wav）
    inline constexpr const char* kBgmDir         = "ChronoSort/Assets/Audio/Bgm";         // BGM（.mp3）

    // 1つだけのファイル
    inline constexpr const char* kCardMesh             = "ChronoSort/Assets/Meshes/card.obj";
    inline constexpr const char* kTableMesh            = "ChronoSort/Assets/Meshes/table.obj";
    inline constexpr const char* kTableMaterial        = "ChronoSort/Assets/Textures/Table/table.tmat";
    inline constexpr const char* kCoverMaterial        = "ChronoSort/Assets/Textures/Board/cover.tmat";
    inline constexpr const char* kMarkerMaterial       = "ChronoSort/Assets/Textures/Board/marker.tmat";
    inline constexpr const char* kMarkerWarnMaterial   = "ChronoSort/Assets/Textures/Board/marker_warning.tmat";
    inline constexpr const char* kCredits              = "ChronoSort/Assets/Data/Credits.json";    // 出典の一覧

    //! 今の言語の画面 UI の画像のフォルダを返します（日本語は Textures/UI/、ほかの言語は Textures/UI/<言語>/）。
    //! 字を焼き込んだ画像（題字・判定の印）だけが言語のフォルダにあります。
    //! @return フォルダのパス（末尾に / は付けない）。言語のフォルダが無い言語は空。
    inline std::string LocalizedUITextureDir() {
        const Language language = CurrentLanguage();
        return language == Language::Japanese ? std::string() : std::string(kUITextureDir) + "/" + LanguageCode(language);
    }

    //! 画面 UI の画像のパスを返します。字を焼き込んだ画像は、今の言語のものがあればそちらを返します。
    //! @param  [in] file Textures/UI/ からのファイル名。
    //! @return パス。
    inline std::string UITexture(const std::string& file) {
        const std::string localized = LocalizedUITextureDir();
        if(!localized.empty()) {
            std::error_code error;
            if(std::filesystem::exists(localized + "/" + file, error)) {
                return localized + "/" + file;
            }
        }
        return std::string(kUITextureDir) + "/" + file;
    }

    //! 粒子の画像のパスを返します。
    //! @param  [in] file Textures/Fx/ からのファイル名。
    //! @return パス。
    inline std::string FxTexture(const std::string& file) {
        return std::string(kFxTextureDir) + "/" + file;
    }

    //! 今の言語のカードの面の画像のフォルダを返します（日本語は Cards/、ほかの言語は Cards/<言語>/）。
    //! @return フォルダのパス（末尾に / は付けない）。
    inline std::string CardFaceDir() {
        const Language language = CurrentLanguage();
        return language == Language::Japanese ? std::string(kCardTextureDir) : std::string(kCardTextureDir) + "/" + LanguageCode(language);
    }

    //! カードの年代を伏せた面（手札で見える面）の画像のパスを返します。
    //! @param  [in] id カードの id。
    //! @return パス。
    inline std::string CardHiddenTexture(const std::string& id) {
        return CardFaceDir() + "/" + id + "_front.png";
    }

    //! カードの年代が書かれた面（めくった後の面）の画像のパスを返します。
    //! @param  [in] id カードの id。
    //! @return パス。
    inline std::string CardRevealedTexture(const std::string& id) {
        return CardFaceDir() + "/" + id + "_back.png";
    }

    //! 効果音のパスを返します。
    //! @param  [in] name 拡張子を除いたファイル名。
    //! @return パス。
    inline std::string Sfx(const std::string& name) {
        return std::string(kSfxDir) + "/" + name + ".wav";
    }

    //! BGM のパスを返します。
    //! @param  [in] name 拡張子を除いたファイル名。
    //! @return パス。
    inline std::string Bgm(const std::string& name) {
        return std::string(kBgmDir) + "/" + name + ".mp3";
    }

    //! 画面 UI の紙片の置き方の JSON のパスを返します。
    //! @param  [in] screen 画面の名前（"GameHud" など）。
    //! @return パス。
    inline std::string UILayout(const std::string& screen) {
        return std::string(kRoot) + "Data/UI/" + screen + ".json";
    }

    //! 調整値の JSON のパスを返します。
    //! @param  [in] name 名前（"Board" など）。
    //! @return パス。
    inline std::string Tuning(const std::string& name) {
        return std::string(kRoot) + "Data/Tuning/" + name + ".json";
    }

    //! 山札（カードの一覧）の JSON のパスを返します。
    //! @param  [in] name 名前（"japan" か "world"）。
    //! @return パス。
    inline std::string Deck(const std::string& name) {
        return std::string(kRoot) + "Data/Decks/" + name + ".json";
    }

    //! 言語の文字列の表のパスを返します。
    //! @param  [in] code 言語のコード（"ja" / "en"）。
    //! @return パス。
    inline std::string TextTable(const std::string& code) {
        return std::string(kRoot) + "Data/Text/" + code + ".json";
    }

    //! 言語のカードの訳の表のパスを返します。
    //! @param  [in] code 言語のコード（"en" など）。
    //! @return パス。
    inline std::string CardTextTable(const std::string& code) {
        return std::string(kRoot) + "Data/Text/Cards/" + code + ".json";
    }

}    // namespace ChronoSort::Util::AssetPaths
