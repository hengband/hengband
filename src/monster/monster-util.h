#pragma once

#include "system/angband.h"
#include "system/enums/monrace/monrace-hook-types.h"
#include <functional>
#include <optional>

enum summon_type : int;
enum class MonraceId : short;
class PlayerType;
using monsterrace_hook_type = std::function<bool(PlayerType *, MonraceId)>;
monsterrace_hook_type get_monster_hook(PlayerType *player_ptr);
MonraceHookTerrain get_monster_hook2(PlayerType *player_ptr, POSITION y, POSITION x);
void get_mon_num_prep(PlayerType *player_ptr, const monsterrace_hook_type &hook1, MonraceHookTerrain hook2 = MonraceHookTerrain::NONE, std::optional<summon_type> summon_specific_type = std::nullopt);

class ChameleonTransformation {
public:
    /*!
     * @brief カメレオンの変身対象フィルタ
     * @param m_idx カメレオンのフロア内インデックス
     * @param terrain_id カメレオンの足元にある地形のID
     * @param is_unique ユニークであるか否か (実質、カメレオンの王であるか否か)
     * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
     */
    ChameleonTransformation(short m_idx, short terrain_id, bool is_unique, const std::optional<short> &summoner_m_idx)
        : m_idx(m_idx)
        , terrain_id(terrain_id)
        , is_unique(is_unique)
        , summoner_m_idx(summoner_m_idx)
    {
    }

    short m_idx;
    short terrain_id;
    bool is_unique;
    std::optional<short> summoner_m_idx;
};
void get_mon_num_prep_chameleon(PlayerType *player_ptr, const ChameleonTransformation &ct);
void get_mon_num_prep_bounty(PlayerType *player_ptr);
bool is_player(MONSTER_IDX m_idx);
bool is_monster(MONSTER_IDX m_idx);
