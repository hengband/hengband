#include "monster-floor/monster-summon.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "mspell/summon-checker.h"
#include "spell/summon-types.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"

/*!
 * @brief モンスターが召喚の基本条件に合っているかをチェックする / Hack -- help decide if a monster race is "okay" to summon
 * @param r_idx チェックするモンスター種族ID
 * @param type 召喚種別
 * @param mode 生成オプション
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 * @return 召喚対象にできるならばTRUE
 */
static bool summon_specific_okay(PlayerType *player_ptr, MonsterRaceId r_idx, summon_type type, BIT_FLAGS mode, std::optional<MONSTER_IDX> summoner_m_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!mon_hook_dungeon(player_ptr, r_idx)) {
        return false;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    if (summoner_m_idx) {
        auto *m_ptr = &floor.m_list[*summoner_m_idx];
        if (monster_has_hostile_align(player_ptr, m_ptr, 0, 0, r_ptr)) {
            return false;
        }
    } else if (any_bits(mode, PM_FORCE_PET)) {
        if (monster_has_hostile_align(player_ptr, nullptr, 10, -10, r_ptr) && !one_in_(std::abs(player_ptr->alignment) / 2 + 1)) {
            return false;
        }
    }

    if (none_bits(mode, PM_ALLOW_UNIQUE) && (r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || (r_ptr->population_flags.has(MonsterPopulationType::NAZGUL)))) {
        return false;
    }

    if (type == SUMMON_NONE) {
        return true;
    }

    const auto is_like_unique = r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || (r_ptr->population_flags.has(MonsterPopulationType::NAZGUL));
    if (any_bits(mode, PM_FORCE_PET) && is_like_unique && monster_has_hostile_align(player_ptr, nullptr, 10, -10, r_ptr)) {
        return false;
    }

    if (r_ptr->misc_flags.has(MonsterMiscType::CHAMELEON) && floor.get_dungeon_definition().flags.has(DungeonFeatureType::CHAMELEON)) {
        return true;
    }

    if (summoner_m_idx) {
        auto *m_ptr = &floor.m_list[*summoner_m_idx];
        return check_summon_specific(player_ptr, m_ptr->r_idx, r_idx, type);
    } else {
        return check_summon_specific(player_ptr, MonsterRaceId::PLAYER, r_idx, type);
    }
}

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
 * @brief 荒野のレベルを含めた階層レベルを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 階層レベル
 * @details
 * ダンジョン及びクエストはdun_level>0となる。
 * 荒野はdun_level==0なので、その場合荒野レベルを返す。
 */
DEPTH get_dungeon_or_wilderness_level(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->dun_level > 0) {
        return floor_ptr->dun_level;
    }

    return wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].level;
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
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->inside_arena) {
        return std::nullopt;
    }

    POSITION x, y;
    if (!mon_scatter(player_ptr, MonraceList::empty_id(), &y, &x, y1, x1, 2)) {
        return std::nullopt;
    }

    auto summon_specific_hook = [type, mode, summoner_m_idx](PlayerType *player_ptr, MonsterRaceId r_idx) {
        return summon_specific_okay(player_ptr, r_idx, type, mode, summoner_m_idx);
    };
    get_mon_num_prep(player_ptr, std::move(summon_specific_hook), get_monster_hook2(player_ptr, y, x), type);

    DEPTH dlev = get_dungeon_or_wilderness_level(player_ptr);
    const auto r_idx = get_mon_num(player_ptr, 0, (dlev + lev) / 2 + 5, mode);
    if (!MonraceList::is_valid(r_idx)) {
        return std::nullopt;
    }

    if (is_dead_summoning(type)) {
        mode |= PM_NO_KAGE;
    }

    auto summoned_m_idx = place_specific_monster(player_ptr, summoner_m_idx.value_or(0), y, x, r_idx, mode, summoner_m_idx);
    if (!summoned_m_idx) {
        return std::nullopt;
    }

    bool notice = false;
    if (!summoner_m_idx) {
        notice = true;
    } else {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[*summoner_m_idx];
        if (m_ptr->is_pet()) {
            notice = true;
        } else if (is_seen(player_ptr, m_ptr)) {
            notice = true;
        } else if (player_can_see_bold(player_ptr, y, x)) {
            notice = true;
        }
    }

    if (notice) {
        sound(SOUND_SUMMON);
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
std::optional<MONSTER_IDX> summon_named_creature(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION oy, POSITION ox, MonsterRaceId r_idx, BIT_FLAGS mode)
{
    if (!MonraceList::is_valid(r_idx) || (r_idx >= static_cast<MonsterRaceId>(monraces_info.size()))) {
        return false;
    }

    POSITION x, y;
    if (player_ptr->current_floor_ptr->inside_arena || !mon_scatter(player_ptr, r_idx, &y, &x, oy, ox, 2)) {
        return false;
    }

    return place_specific_monster(player_ptr, src_idx, y, x, r_idx, (mode | PM_NO_KAGE));
}
