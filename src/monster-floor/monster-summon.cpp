#include "monster-floor/monster-summon.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "mspell/summon-checker.h"
#include "spell/summon-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @var summon_specific_who
 * @brief 召喚を行ったプレイヤーあるいはモンスターのIDを示すグローバル変数 / Hack -- the index of the summoning monster
 * @todo summon_specific_who グローバル変数の除去と関数引数への代替を行う
 */
int summon_specific_who = -1;

/*!
 * @var summon_unique_okay
 * @brief 召喚対象にユニークを含めるかを示すグローバル変数 / summoning unique enable
 * @todo summon_unique_okay グローバル変数の除去と関数引数への代替を行う
 */
bool summon_unique_okay = false;

/*!
 * @brief モンスターが召喚の基本条件に合っているかをチェックする / Hack -- help decide if a monster race is "okay" to summon
 * @param r_idx チェックするモンスター種族ID
 * @return 召喚対象にできるならばTRUE
 */
static bool summon_specific_okay(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!mon_hook_dungeon(player_ptr, r_idx))
        return false;

    if (summon_specific_who > 0) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[summon_specific_who];
        if (monster_has_hostile_align(player_ptr, m_ptr, 0, 0, r_ptr))
            return false;
    } else if (summon_specific_who < 0) {
        if (monster_has_hostile_align(player_ptr, nullptr, 10, -10, r_ptr) && !one_in_(ABS(player_ptr->alignment) / 2 + 1))
            return false;
    }

    if (!summon_unique_okay && ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)))
        return false;

    if (!summon_specific_type)
        return true;

    if ((summon_specific_who < 0) && ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL))
        && monster_has_hostile_align(player_ptr, nullptr, 10, -10, r_ptr))
        return false;

    if ((r_ptr->flags7 & RF7_CHAMELEON) && d_info[player_ptr->dungeon_idx].flags.has(DF::CHAMELEON))
        return true;

    if (summon_specific_who > 0) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[summon_specific_who];
        return check_summon_specific(player_ptr, m_ptr->r_idx, r_idx);
    } else {
        return check_summon_specific(player_ptr, 0, r_idx);
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
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 階層レベル
 * @details
 * ダンジョン及びクエストはdun_level>0となる。
 * 荒野はdun_level==0なので、その場合荒野レベルを返す。
 */
DEPTH get_dungeon_or_wilderness_level(player_type *player_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->dun_level > 0)
        return floor_ptr->dun_level;

    return wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].level;
}

/*!
 * @brief モンスターを召喚により配置する / Place a monster (of the specified "type") near the given location. Return TRUE if a monster was actually summoned.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param who 召喚主のモンスター情報ID
 * @param y1 目標地点y座標
 * @param x1 目標地点x座標
 * @param lev 相当生成階
 * @param type 召喚種別
 * @param mode 生成オプション
 * @return 召喚できたらtrueを返す
 */
bool summon_specific(player_type *player_ptr, MONSTER_IDX who, POSITION y1, POSITION x1, DEPTH lev, summon_type type, BIT_FLAGS mode)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->inside_arena)
        return false;

    POSITION x, y;
    if (!mon_scatter(player_ptr, 0, &y, &x, y1, x1, 2))
        return false;

    summon_specific_who = who;
    summon_specific_type = type;
    summon_unique_okay = (mode & PM_ALLOW_UNIQUE) != 0;
    get_mon_num_prep(player_ptr, summon_specific_okay, get_monster_hook2(player_ptr, y, x));

    DEPTH dlev = get_dungeon_or_wilderness_level(player_ptr);
    MONRACE_IDX r_idx = get_mon_num(player_ptr, 0, (dlev + lev) / 2 + 5, 0);
    if (!r_idx) {
        summon_specific_type = SUMMON_NONE;
        return false;
    }

    if (is_dead_summoning(type))
        mode |= PM_NO_KAGE;

    if (!place_monster_aux(player_ptr, who, y, x, r_idx, mode)) {
        summon_specific_type = SUMMON_NONE;
        return false;
    }

    summon_specific_type = SUMMON_NONE;

    bool notice = false;
    if (who <= 0) {
        notice = true;
    } else {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[who];
        if (is_pet(m_ptr)) {
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

    return true;
}

/*!
 * @brief 特定モンスター種族を召喚により生成する / A "dangerous" function, creates a pet of the specified type
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param who 召喚主のモンスター情報ID
 * @param oy 目標地点y座標
 * @param ox 目標地点x座標
 * @param r_idx 生成するモンスター種族ID
 * @param mode 生成オプション
 * @return 召喚できたらtrueを返す
 */
bool summon_named_creature(player_type *player_ptr, MONSTER_IDX who, POSITION oy, POSITION ox, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
    if ((r_idx <= 0) || (r_idx >= max_r_idx)) {
        return false;
    }
    
    POSITION x, y;
    if (player_ptr->current_floor_ptr->inside_arena || !mon_scatter(player_ptr, r_idx, &y, &x, oy, ox, 2)) {
        return false;
    }
    
    return place_monster_aux(player_ptr, who, y, x, r_idx, (mode | PM_NO_KAGE));
}
