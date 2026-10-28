//----------------------------------------------------------------------------
//! @file   PaperLayout.cpp
//! @brief  画面に常に出しておく紙片の置き方の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/PaperLayout.hpp>

#include <Tsukino/Core/IO/JsonFile.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>

namespace ChronoSort::UI {

    //----------------------------------------------------------------------------
    //! PaperSpec を JSON から読み込みます（書いていない項目は既定値のまま）。
    //----------------------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, PaperSpec& spec) {
        using Tsukino::IO::LoadOptional;
        LoadOptional(archive, "texture", spec.texture);
        LoadOptional(archive, "center", spec.center);
        LoadOptional(archive, "size", spec.size);
        LoadOptional(archive, "angle", spec.angleDegrees);
        LoadOptional(archive, "sway", spec.swayDegrees);
        LoadOptional(archive, "swaySeconds", spec.swaySeconds);
        LoadOptional(archive, "bob", spec.bobPixels);
        LoadOptional(archive, "sortOrder", spec.sortOrder);
    }

    //----------------------------------------------------------------------------
    //! 置き方の JSON を読み込みます。
    //----------------------------------------------------------------------------
    PaperLayout::PaperLayout(const std::string& path)
        : m_path(path) {
        // 読めなければ警告を出して空のまま（Spec は既定値を返す）
        Tsukino::IO::TryLoadJson(path, "papers", m_papers);
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
