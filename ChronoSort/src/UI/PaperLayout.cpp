//----------------------------------------------------------------------------
//! @file   PaperLayout.cpp
//! @brief  画面に常に出しておく紙片の置き方の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/PaperLayout.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>

#include <fstream>

namespace ChronoSort::UI {

    namespace {

        //----------------------------------------------------------------------------
        //! 書かれていれば読み、無ければ既定値のままにします。
        //! @param  [in,out] archive アーカイブ。
        //! @param  [in]     name    項目名。
        //! @param  [out]    value   読み込み先。
        //----------------------------------------------------------------------------
        template <class Archive, class T>
        void Optional(Archive& archive, const char* name, T& value) {
            try {
                archive(cereal::make_nvp(name, value));
            } catch(const cereal::Exception&) {
            }
        }

    }    // namespace

    //----------------------------------------------------------------------------
    //! PaperSpec を JSON から読み込みます（書いていない項目は既定値のまま）。
    //----------------------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, PaperSpec& spec) {
        Optional(archive, "texture", spec.texture);
        Optional(archive, "center", spec.center);
        Optional(archive, "size", spec.size);
        Optional(archive, "angle", spec.angleDegrees);
        Optional(archive, "sway", spec.swayDegrees);
        Optional(archive, "swaySeconds", spec.swaySeconds);
        Optional(archive, "bob", spec.bobPixels);
        Optional(archive, "sortOrder", spec.sortOrder);
    }

    //----------------------------------------------------------------------------
    //! 置き方の JSON を読み込みます。
    //----------------------------------------------------------------------------
    PaperLayout::PaperLayout(const std::string& path)
        : m_path(path) {
        std::ifstream is(path);
        if(!is) {
            Tsukino::Core::Log::Warn("PaperLayout: file not found: " + path);
            return;
        }
        try {
            cereal::JSONInputArchive archive(is);
            archive(cereal::make_nvp("papers", m_papers));
        } catch(const std::exception& e) {
            Tsukino::Core::Log::Warn("PaperLayout: failed to read " + path + ": " + e.what());
            m_papers.clear();
        }
    }

    //----------------------------------------------------------------------------
    //! 名前の紙片の作り方を返します。
    //----------------------------------------------------------------------------
    PaperSpec PaperLayout::Spec(const std::string& name) const {
        if(const auto it = m_papers.find(name); it != m_papers.end()) {
            return it->second;
        }
        Tsukino::Core::Log::Warn("PaperLayout: '" + name + "' is not in " + m_path);
        return PaperSpec{};
    }

}    // namespace ChronoSort::UI
