#include "monster-floor/monster-summon.h"
#include "floor/geometry.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "spell/summon-types.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief モンスター死亡時に召喚されうるモンスターかどうかの判定
 * @param type モンスター種族ID
 * @return 召喚されうるならばTRUE (あやしい影として生成されない)
 */
static bool is_dead_summoning(summon_type type)
{
    bool summoning = type == SUMMON_BLUE_HORROR;
    summoning |= type == SUMMON_DAWN;
    summoning |= type == SUMMON_TOTEM_MOAI;
    return summoning;
}

/*!
 * @brief モンスターを召喚により配置する / Place a monster (of the specified "type") near the given location. Return TRUE if a monster was actually summoned.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y1 目標地点y座標
 * @param x1 目標地点x座標
 * @param lev 相当生成階
 * @param type 召喚種別
 * @param mode 生成オプション
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 * @return 召喚に成功したらモンスターID、失敗したらstd::nullopt
 */
std::optional<MONSTER_IDX> summon_specific(PlayerType *player_ptr, POSITION y1, POSITION x1, DEPTH lev, summon_type type, BIT_FLAGS mode, std::optional<MONSTER_IDX> summoner_m_idx)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (floor.inside_arena) {
        return std::nullopt;
    }

    const auto pos = mon_scatter(player_ptr, MonraceList::empty_id(), { y1, x1 }, 2);
    if (!pos) {
        return std::nullopt;
    }

    const auto hook = floor.get_monrace_hook_terrain_at(*pos);
    SummonCondition condition(type, mode, summoner_m_idx, hook);
    get_mon_num_prep_summon(player_ptr, condition);

    const auto dlev = floor.is_underground() ? floor.get_level() : WildernessGrids::get_instance().get_player_grid().get_level();
    const auto r_idx = get_mon_num(player_ptr, 0, (dlev + lev) / 2 + 5, mode);
    if (!MonraceList::is_valid(r_idx)) {
        return std::nullopt;
    }

    if (is_dead_summoning(type)) {
        mode |= PM_NO_KAGE;
    }

    auto summoned_m_idx = place_specific_monster(player_ptr, pos->y, pos->x, r_idx, mode, summoner_m_idx);
    if (!summoned_m_idx) {
        return std::nullopt;
    }

    bool notice = false;
    if (!summoner_m_idx) {
        notice = true;
    } else {
        const auto &monster = player_ptr->current_floor_ptr->m_list[*summoner_m_idx];
        if (monster.is_pet()) {
            notice = true;
        } else if (is_seen(player_ptr, monster)) {
            notice = true;
        } else if (player_can_see_bold(player_ptr, pos->y, pos->x)) {
            notice = true;
        }
    }

    if (notice) {
        sound(SoundKind::SUMMON);
    }

    return summoned_m_idx;
}

/*!
 * @brief 特定モンスター種族を召喚により生成する / A "dangerous" function, creates a pet of the specified type
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param src_idx 召喚主のモンスター情報ID
 * @param oy 目標地点y座標
 * @param ox 目標地点x座標
 * @param r_idx 生成するモンスター種族ID
 * @param mode 生成オプション
 * @return 召喚に成功したらモンスターID、失敗したらstd::nullopt
 */
std::optional<MONSTER_IDX> summon_named_creature(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION oy, POSITION ox, MonraceId r_idx, BIT_FLAGS mode)
{
    if (!MonraceList::is_valid(r_idx) || (r_idx >= static_cast<MonraceId>(MonraceList::get_instance().size()))) {
        return false;
    }

    if (player_ptr->current_floor_ptr->inside_arena) {
        return false;
    }

    const auto pos = mon_scatter(player_ptr, r_idx, { oy, ox }, 2);
    if (!pos) {
        return false;
    }

    const auto summon_who = is_monster(src_idx) ? std::make_optional(src_idx) : std::nullopt;
    return place_specific_monster(player_ptr, pos->y, pos->x, r_idx, (mode | PM_NO_KAGE), summon_who);
}
