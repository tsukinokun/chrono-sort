//----------------------------------------------------------------------------
//! @file   AssetManifest.cpp
//! @brief  起動時に読み込んでおくアセットの一覧の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/AssetManifest.hpp>

#include <ChronoSort/Rules/Deck.hpp>

#include <filesystem>

namespace ChronoSort::Game {

    namespace {

        //----------------------------------------------------------------------------
        //! フォルダ内の、指定した拡張子のファイルを一覧に足します。
        //! @param  [in]     directory フォルダ（ワークスペースルートからの相対）。
        //! @param  [in]     extension 拡張子（例：".png"）。
        //! @param  [in,out] paths     追加先。
        //----------------------------------------------------------------------------
        void AddFiles(const std::string& directory, const std::string& extension, std::vector<std::string>& paths) {
            std::error_code error;
            for(const auto& entry : std::filesystem::directory_iterator(directory, error)) {
                if(entry.is_regular_file() && entry.path().extension() == extension) {
                    paths.push_back(directory + "/" + entry.path().filename().string());
                }
            }
        }

    }    // namespace

    //----------------------------------------------------------------------------
    //! 起動時に読み込んでおくアセットのパスを集めます。
    //----------------------------------------------------------------------------
    std::vector<std::string> CollectStartupAssets() {
        std::vector<std::string> paths = {
            "ChronoSort/Assets/Meshes/card.obj",
            "ChronoSort/Assets/Meshes/table.obj",
            "ChronoSort/Assets/Textures/Table/table.tmat",
            "ChronoSort/Assets/Textures/Board/cover.tmat",
            "ChronoSort/Assets/Textures/Board/marker.tmat",
            "ChronoSort/Assets/Textures/Board/marker_warning.tmat",
        };

        // 音は初回の取り込みで外部ツールを起動するので、操作中ではなくここで済ませる
        AddFiles("ChronoSort/Assets/Audio/Sfx", ".wav", paths);
        AddFiles("ChronoSort/Assets/Audio/Bgm", ".mp3", paths);
        AddFiles("ChronoSort/Assets/Textures/UI", ".png", paths);
        AddFiles("ChronoSort/Assets/Textures/Fx", ".png", paths);

        // カード面（.tmat を読むと、参照しているテクスチャも一緒に読まれる）
        for(const Rules::CardData& card : Rules::LoadAllCards()) {
            const std::string prefix = "ChronoSort/Assets/Textures/Cards/" + card.id;
            paths.push_back(prefix + "_front.tmat");
            paths.push_back(prefix + "_back.tmat");
        }
        return paths;
    }

}    // namespace ChronoSort::Game
