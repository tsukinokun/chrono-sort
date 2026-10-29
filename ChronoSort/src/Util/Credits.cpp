//----------------------------------------------------------------------------
//! @file   Credits.cpp
//! @brief  出典の一覧の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Util/Credits.hpp>

#include <ChronoSort/Util/AssetPaths.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <exception>
#include <fstream>

namespace ChronoSort::Util {

    //----------------------------------------------------------------------------
    //! CreditEntry を読み書きします。
    //----------------------------------------------------------------------------
    template <class Archive>
    void serialize(Archive& archive, CreditEntry& entry) {
        archive(cereal::make_nvp("id", entry.id), cereal::make_nvp("title", entry.title), cereal::make_nvp("file", entry.file),
                cereal::make_nvp("page", entry.page), cereal::make_nvp("artist", entry.artist), cereal::make_nvp("license", entry.license),
                cereal::make_nvp("licenseUrl", entry.licenseUrl));
    }

    //----------------------------------------------------------------------------
    //! カードの絵の出典を、カードの id で探します。
    //----------------------------------------------------------------------------
    const CreditEntry* Credits::FindImage(const std::string& id) const {
        const auto it = std::find_if(images.begin(), images.end(), [&](const CreditEntry& entry) { return entry.id == id; });
        return it != images.end() ? &*it : nullptr;
    }

    //----------------------------------------------------------------------------
    //! Credits.json を読みます。
    //----------------------------------------------------------------------------
    Credits LoadCredits() {
        Credits       credits;
        std::ifstream is(AssetPaths::kCredits);
        if(!is) {
            Tsukino::Core::Log::Warn(std::string("Credits.json not found: ") + AssetPaths::kCredits);
            return credits;
        }
        try {
            cereal::JSONInputArchive archive(is);
            archive(cereal::make_nvp("images", credits.images), cereal::make_nvp("audio", credits.audio));
        } catch(const std::exception& e) {
            Tsukino::Core::Log::Warn(std::string("Failed to read Credits.json: ") + e.what());
        }
        // 集めたときに作者名が2度続けて入ったもの（"Unknown authorUnknown author" など）は1つにする
        for(CreditEntry& entry : credits.images) {
            const size_t half = entry.artist.size() / 2;
            if(half > 0 && entry.artist.size() % 2 == 0 && entry.artist.compare(0, half, entry.artist, half, half) == 0) {
                entry.artist.resize(half);
            }
        }
        return credits;
    }

}    // namespace ChronoSort::Util
