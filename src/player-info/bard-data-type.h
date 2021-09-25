#pragma once

#include "system/angband.h"

enum realm_song_type : int;

struct bard_data_type {
    realm_song_type singing_song{}; //!< 現在歌っている歌
    realm_song_type interrputing_song{}; //!< 中断中の歌(魔力消去などで途切れた後に再開するのに使用)
    int32_t singing_duration{}; //!< 現在の歌を継続して歌っている期間
    byte singing_song_spell_idx{}; //!< 現在歌っている歌の魔法書(歌集)における要素番号
};
