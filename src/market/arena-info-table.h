#pragma once

#include "system/baseitem-info.h"
#include <vector>

/*!
 * @brief 闘技場のモンスターエントリー構造体
 */
enum class MonsterRaceId : int16_t;
class ArenaMonsterEntry {
public:
    ArenaMonsterEntry(MonsterRaceId r_idx, const BaseitemKey &key)
        : r_idx(r_idx)
        , key(key)
    {
    }

    MonsterRaceId r_idx; /*!< 闘技場のモンスター種族ID(0ならば表彰式) / Monster (0 means victory prizing) */
    BaseitemKey key;
};

extern const std::vector<ArenaMonsterEntry> arena_info;

/*
 * @brief 表彰式までの最大数.
 * @todo arena_info を配列からvectorに変えたので、後でarena_info.size() に差し替えて廃止予定.
 */
extern const int MAX_ARENA_MONS;
