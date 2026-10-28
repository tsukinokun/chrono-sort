----------------------------------------
-- ChronoSort のワークスペース定義
--
-- エンジンは External/TsukinoEngine に submodule として取り込んでいる。
-- 共通設定は tsukino.lua のヘルパーに任せ、ここへは書き写さない
-- （NDEBUG や JPH_DEBUG_RENDERER がエンジンと食い違うのを防ぐため）
----------------------------------------
include "External/TsukinoEngine/Tools/premake/tsukino.lua"

workspace "ChronoSort"
    startproject "ChronoSort"
    location ".build"
    tsukino_workspace_defaults()

include "External/TsukinoEngine"

----------------------------------------
-- ゲーム本体
----------------------------------------
project "ChronoSort"
    location ".build/ChronoSort"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }

    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    pchheader "pch.h"
    pchsource "ChronoSort/pch.cpp"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "ChronoSort/src/**.cpp",
        "ChronoSort/include/**.hpp",
        "ChronoSort/pch.h",
        "ChronoSort/pch.cpp",
    }

    includedirs {
        "ChronoSort",
        "ChronoSort/include",
    }

    tsukino_link()
    tsukino_release_payload()
    -- exe とウィンドウのアイコン（Tools/art/make_icon.py で描く）
    tsukino_icon("ChronoSort/Assets/Icon/App.ico")

    -- Debug はワークスペースルートが作業ディレクトリなので、Assets はそのまま読める。
    -- Release は exe の隣が基準になるため、ゲーム自身の Assets をコピーする
    filter "configurations:Release"
        postbuildcommands {
            "{COPYDIR} %{wks.location}/../ChronoSort/Assets %{cfg.targetdir}/ChronoSort/Assets",
        }
    filter {}
