#pragma once

#include "system/angband.h"
#include "system/enums/monrace/monrace-hook-types.h"
#include "util/point-2d.h"
#include <functional>
#include <optional>

enum summon_type : int;
enum class MonraceId : short;
class PlayerType;
MonraceHook get_monster_hook(const Pos2D &pos_wilderness, bool is_underground);
MonraceHookTerrain get_monster_hook2(PlayerType *player_ptr, POSITION y, POSITION x);
void get_mon_num_prep_enum(PlayerType *player_ptr, MonraceHook hook1 = MonraceHook::NONE, MonraceHookTerrain hook2 = MonraceHookTerrain::NONE);
void get_mon_num_prep_escort(PlayerType *player_ptr, MonraceId escorted_monrace_id, short m_idx, MonraceHookTerrain hook2);

class SummonCondition {
public:
    SummonCondition(summon_type type, BIT_FLAGS mode, const std::optional<short> &summoner_m_idx, MonraceHookTerrain hook)
        : type(type)
        , mode(mode)
        , summoner_m_idx(summoner_m_idx)
        , hook(hook)
    {
    }

    summon_type type;
    BIT_FLAGS mode;
    std::optional<short> summoner_m_idx;
    MonraceHookTerrain hook;
};

void get_mon_num_prep_summon(PlayerType *player_ptr, const SummonCondition &condition);

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
