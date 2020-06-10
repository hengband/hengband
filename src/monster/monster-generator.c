#include "monster/monster-generator.h"
#include "dungeon/dungeon.h"
#include "floor/floor.h"
#include "main/sound-definitions-table.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "monster/monster2.h" // todo get_mon_num() が依存している。後で消す.
#include "monster/place-monster-types.h"
#include "monster/smart-learn-types.h"
#include "mspell/summon-checker.h"
#include "spell/spells-summon.h"

#define MON_SCAT_MAXD 10 /*!< mon_scatter()関数によるモンスター配置で許される中心からの最大距離 */

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
bool summon_unique_okay = FALSE;

/*!
 * @brief モンスター1体を目標地点に可能な限り近い位置に生成する / improved version of scatter() for place monster
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param r_idx 生成モンスター種族
 * @param yp 結果生成位置y座標
 * @param xp 結果生成位置x座標
 * @param y 中心生成位置y座標
 * @param x 中心生成位置x座標
 * @param max_dist 生成位置の最大半径
 * @return 成功したらtrue
 *
 */
static bool mon_scatter(player_type *player_ptr, MONRACE_IDX r_idx, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION max_dist)
{
    POSITION place_x[MON_SCAT_MAXD];
    POSITION place_y[MON_SCAT_MAXD];
    int num[MON_SCAT_MAXD];

    if (max_dist >= MON_SCAT_MAXD)
        return FALSE;

    int i;
    for (i = 0; i < MON_SCAT_MAXD; i++)
        num[i] = 0;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION nx = x - max_dist; nx <= x + max_dist; nx++) {
        for (POSITION ny = y - max_dist; ny <= y + max_dist; ny++) {
            if (!in_bounds(floor_ptr, ny, nx))
                continue;
            if (!projectable(player_ptr, y, x, ny, nx))
                continue;
            if (r_idx > 0) {
                monster_race *r_ptr = &r_info[r_idx];
                if (!monster_can_enter(player_ptr, ny, nx, r_ptr, 0))
                    continue;
            } else {
                if (!is_cave_empty_bold2(player_ptr, ny, nx))
                    continue;
                if (pattern_tile(floor_ptr, ny, nx))
                    continue;
            }

            i = distance(y, x, ny, nx);
            if (i > max_dist)
                continue;

            num[i]++;
            if (one_in_(num[i])) {
                place_x[i] = nx;
                place_y[i] = ny;
            }
        }
    }

    i = 0;
    while (i < MON_SCAT_MAXD && 0 == num[i])
        i++;
    if (i >= MON_SCAT_MAXD)
        return FALSE;

    *xp = place_x[i];
    *yp = place_y[i];

    return TRUE;
}

/*!
 * todo ここにplayer_typeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @brief モンスターが召喚の基本条件に合っているかをチェックする / Hack -- help decide if a monster race is "okay" to summon
 * @param r_idx チェックするモンスター種族ID
 * @return 召喚対象にできるならばTRUE
 */
static bool summon_specific_okay(MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[summon_specific_who];
    if (!mon_hook_dungeon(r_idx))
        return FALSE;

    if (summon_specific_who > 0) {
        if (monster_has_hostile_align(p_ptr, m_ptr, 0, 0, r_ptr))
            return FALSE;
    } else if (summon_specific_who < 0) {
        if (monster_has_hostile_align(p_ptr, NULL, 10, -10, r_ptr)) {
            if (!one_in_(ABS(p_ptr->align) / 2 + 1))
                return FALSE;
        }
    }

    if (!summon_unique_okay && ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)))
        return FALSE;

    if (!summon_specific_type)
        return TRUE;

    if ((summon_specific_who < 0) && ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)) && monster_has_hostile_align(p_ptr, NULL, 10, -10, r_ptr))
        return FALSE;

    if ((r_ptr->flags7 & RF7_CHAMELEON) && (d_info[p_ptr->dungeon_idx].flags1 & DF1_CHAMELEON))
        return TRUE;

    return (check_summon_specific(p_ptr, m_ptr->r_idx, r_idx));
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
bool summon_specific(player_type *player_ptr, MONSTER_IDX who, POSITION y1, POSITION x1, DEPTH lev, int type, BIT_FLAGS mode)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->inside_arena)
        return FALSE;

    POSITION x, y;
    if (!mon_scatter(player_ptr, 0, &y, &x, y1, x1, 2))
        return FALSE;

    summon_specific_who = who;
    summon_specific_type = type;
    summon_unique_okay = (mode & PM_ALLOW_UNIQUE) ? TRUE : FALSE;
    get_mon_num_prep(player_ptr, summon_specific_okay, get_monster_hook2(player_ptr, y, x));

    MONRACE_IDX r_idx = get_mon_num(player_ptr, (floor_ptr->dun_level + lev) / 2 + 5, 0);
    if (!r_idx) {
        summon_specific_type = 0;
        return FALSE;
    }

    if ((type == SUMMON_BLUE_HORROR) || (type == SUMMON_DAWN))
        mode |= PM_NO_KAGE;

    if (!place_monster_aux(player_ptr, who, y, x, r_idx, mode)) {
        summon_specific_type = 0;
        return FALSE;
    }

    summon_specific_type = 0;
    sound(SOUND_SUMMON);
    return TRUE;
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
    if (r_idx >= max_r_idx)
        return FALSE;

    POSITION x, y;
    if (player_ptr->current_floor_ptr->inside_arena)
        return FALSE;

    if (!mon_scatter(player_ptr, r_idx, &y, &x, oy, ox, 2))
        return FALSE;

    return place_monster_aux(player_ptr, who, y, x, r_idx, (mode | PM_NO_KAGE));
}

/*!
 * @brief モンスターを増殖生成する / Let the given monster attempt to reproduce.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx 増殖するモンスター情報ID
 * @param clone クローン・モンスター処理ならばtrue
 * @param mode 生成オプション
 * @return 生成できたらtrueを返す
 * @details
 * Note that "reproduction" REQUIRES empty space.
 */
bool multiply_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    POSITION y, x;
    if (!mon_scatter(player_ptr, m_ptr->r_idx, &y, &x, m_ptr->fy, m_ptr->fx, 1))
        return FALSE;

    if (m_ptr->mflag2 & MFLAG2_NOPET)
        mode |= PM_NO_PET;

    if (!place_monster_aux(player_ptr, m_idx, y, x, m_ptr->r_idx, (mode | PM_NO_KAGE | PM_MULTIPLY)))
        return FALSE;

    if (clone || (m_ptr->smart & SM_CLONED)) {
        floor_ptr->m_list[hack_m_idx_ii].smart |= SM_CLONED;
        floor_ptr->m_list[hack_m_idx_ii].mflag2 |= MFLAG2_NOPET;
    }

    return TRUE;
}
