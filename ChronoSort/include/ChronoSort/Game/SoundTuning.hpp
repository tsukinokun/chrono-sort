//----------------------------------------------------------------------------
//! @file   SoundTuning.hpp
//! @brief  鳴らす音の合図の一覧（ChronoSort/Assets/Data/Tuning/Sounds.json）
//! @detail ゲームのコードは「play.flip」のような合図の名前で鳴らし、どのファイルをどの音量で鳴らすかはここで決めます。
//!         JSON に cues・music を書くと、その一覧で初期値をまるごと置き換えます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/IO/CerealOptional.hpp>

#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>

#include <map>
#include <string>

namespace ChronoSort::Game {

    //! 合図1つぶんの音です。
    struct SoundCue {
        std::string file;             // 効果音は Audio/Sfx/<file>.wav、BGM は Audio/Bgm/<file>.mp3
        float       volume = 1.0f;    // 音量（0〜1。設定の音量はこれとは別に掛かる）
    };

    //! 鳴らす音の合図の一覧です。
    struct SoundTuning {
        //! 効果音の合図
        std::map<std::string, SoundCue> cues = {
            {"hand.hover", {"card_slide", 0.25f}},       // 手札にカーソルが重なった
            {"hand.drag", {"card_slide", 0.6f}},         // 手札を持ち上げた
            {"hand.return", {"card_slide", 0.4f}},       // 置けない所で離して手札へ戻った
            {"hand.select", {"card_flip", 0.4f}},        // クリックで選んで掲げた
            {"slot.preview", {"tick", 0.35f}},           // 置き先の候補が変わった
            {"deal.openingFly", {"whoosh", 0.6f}},       // 場の1枚目が山札から飛んだ
            {"deal.openingLand", {"card_place", 1.0f}},  // 場の1枚目が降りた
            {"deal.card", {"deal", 0.55f}},              // 手札を1枚配った
            {"match.tiebreak", {"page", 1.0f}},          // 延長戦
            {"cpu.pick", {"card_flip", 0.4f}},           // CPU が手札を手に取った
            {"cpu.hover", {"tick", 0.3f}},               // CPU が置き先を迷った
            {"play.commit", {"whoosh", 0.5f}},           // カードを出した
            {"play.flip", {"card_flip", 1.0f}},          // 裏返した
            {"play.land", {"card_place", 1.0f}},         // 卓に着いた
            {"play.stamp", {"stamp", 0.8f}},             // 判定の印
            {"play.correct", {"correct", 1.0f}},         // 正解
            {"play.wrong", {"wrong", 1.0f}},             // 不正解
            {"play.showCorrect", {"card_slide", 0.5f}},  // 不正解のあと正しい位置を見せた
            {"play.discard", {"whoosh", 0.6f}},          // 捨て札へ
            {"match.won", {"fanfare", 1.0f}},            // 勝った
            {"match.lost", {"lose", 1.0f}},              // 負けた
            {"result.show", {"page", 1.0f}},             // 結果の紙を出した
            {"ui.button", {"button", 1.0f}},             // 画面を移るボタン
            {"ui.hover", {"card_slide", 0.3f}},          // ボタンにカーソルが重なった
            {"ui.open", {"page", 0.8f}},                 // 紙を開いた
            {"ui.close", {"page", 0.7f}},                // 紙を閉じた・めくった
            {"ui.choose", {"stamp", 0.6f}},              // 選択肢を選んだ
            {"ui.preview", {"tick", 0.8f}},              // 音量を動かしたときの試し鳴り
        };

        //! BGM の合図
        std::map<std::string, SoundCue> music = {
            {"title", {"title", 0.45f}},
            {"game", {"game", 0.45f}},
            {"result", {"result", 0.45f}},
        };
    };

    //! SoundCue を読み込みます。
    template <class Archive>
    void load(Archive& archive, SoundCue& t) {
        Tsukino::IO::LoadOptional(archive, "file", t.file);
        Tsukino::IO::LoadOptional(archive, "volume", t.volume);
    }

    //! SoundTuning を読み込みます。
    template <class Archive>
    void load(Archive& archive, SoundTuning& t) {
        Tsukino::IO::LoadOptional(archive, "cues", t.cues);
        Tsukino::IO::LoadOptional(archive, "music", t.music);
    }

}    // namespace ChronoSort::Game
