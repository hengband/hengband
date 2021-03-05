﻿/*!
 * todo 後で再分割する
 * @brief モンスター生成処理
 * @date 2020/06/10
 * @author Hourier
 */

#include "monster-floor/monster-generator.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "game-option/cheat-options.h"
#include "grid/grid.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "mspell/summon-checker.h"
#include "spell/summon-types.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

#define MON_SCAT_MAXD 10 /*!< mon_scatter()関数によるモンスター配置で許される中心からの最大距離 */

/*!
 * @var place_monster_idx
 * @brief 護衛対象となるモンスター種族IDを渡すグローバル変数 / Hack -- help pick an escort type
 * @todo 関数ポインタの都合を配慮しながら、グローバル変数place_monster_idxを除去し、関数引数化する
 */
static MONSTER_IDX place_monster_idx = 0;

/*!
 * @var place_monster_m_idx
 * @brief 護衛対象となるモンスターIDを渡すグローバル変数 / Hack -- help pick an escort type
 * @todo 関数ポインタの都合を配慮しながら、グローバル変数place_monster_m_idxを除去し、関数引数化する
 */
static MONSTER_IDX place_monster_m_idx = 0;

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
bool mon_scatter(player_type *player_ptr, MONRACE_IDX r_idx, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION max_dist)
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

/*!
 * @brief モンスターを目標地点に集団生成する / Attempt to place a "group" of monsters around the given location
 * @param who 召喚主のモンスター情報ID
 * @param y 中心生成位置y座標
 * @param x 中心生成位置x座標
 * @param r_idx 生成モンスター種族
 * @param mode 生成オプション
 * @return 成功したらtrue
 */
static bool place_monster_group(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
    monster_race *r_ptr = &r_info[r_idx];
    int total = randint1(10);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    int extra = 0;
    if (r_ptr->level > floor_ptr->dun_level) {
        extra = r_ptr->level - floor_ptr->dun_level;
        extra = 0 - randint1(extra);
    } else if (r_ptr->level < floor_ptr->dun_level) {
        extra = floor_ptr->dun_level - r_ptr->level;
        extra = randint1(extra);
    }

    if (extra > 9)
        extra = 9;

    total += extra;

    if (total < 1)
        total = 1;
    if (total > GROUP_MAX)
        total = GROUP_MAX;

    int hack_n = 1;
    POSITION hack_x[GROUP_MAX];
    hack_x[0] = x;
    POSITION hack_y[GROUP_MAX];
    hack_y[0] = y;

    for (int n = 0; (n < hack_n) && (hack_n < total); n++) {
        POSITION hx = hack_x[n];
        POSITION hy = hack_y[n];
        for (int i = 0; (i < 8) && (hack_n < total); i++) {
            POSITION mx, my;
            scatter(player_ptr, &my, &mx, hy, hx, 4, PROJECT_NONE);
            if (!is_cave_empty_bold2(player_ptr, my, mx))
                continue;

            if (place_monster_one(player_ptr, who, my, mx, r_idx, mode)) {
                hack_y[hack_n] = my;
                hack_x[hack_n] = mx;
                hack_n++;
            }
        }
    }

    return TRUE;
}

/*!
 * @brief モンスター種族が召喚主の護衛となれるかどうかをチェックする / Hack -- help pick an escort type
 * @param r_idx チェックするモンスター種族のID
 * @return 護衛にできるならばtrue
 */
static bool place_monster_can_escort(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[place_monster_idx];
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[place_monster_m_idx];
    monster_race *z_ptr = &r_info[r_idx];

    if (mon_hook_dungeon(player_ptr, place_monster_idx) != mon_hook_dungeon(player_ptr, r_idx))
        return FALSE;

    if (z_ptr->d_char != r_ptr->d_char)
        return FALSE;

    if (z_ptr->level > r_ptr->level)
        return FALSE;

    if (z_ptr->flags1 & RF1_UNIQUE)
        return FALSE;

    if (place_monster_idx == r_idx)
        return FALSE;

    if (monster_has_hostile_align(player_ptr, m_ptr, 0, 0, z_ptr))
        return FALSE;

    if (r_ptr->flags7 & RF7_FRIENDLY) {
        if (monster_has_hostile_align(player_ptr, NULL, 1, -1, z_ptr))
            return FALSE;
    }

    if ((r_ptr->flags7 & RF7_CHAMELEON) && !(z_ptr->flags7 & RF7_CHAMELEON))
        return FALSE;

    return TRUE;
}

/*!
 * @brief 一般的なモンスター生成処理のサブルーチン / Attempt to place a monster of the given race at the given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param who 召喚主のモンスター情報ID
 * @param y 生成地点y座標
 * @param x 生成地点x座標
 * @param r_idx 生成するモンスターの種族ID
 * @param mode 生成オプション
 * @return 生成に成功したらtrue
 */
bool place_monster_aux(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
    monster_race *r_ptr = &r_info[r_idx];

    if (!(mode & PM_NO_KAGE) && one_in_(333))
        mode |= PM_KAGE;

    if (!place_monster_one(player_ptr, who, y, x, r_idx, mode))
        return FALSE;
    if (!(mode & PM_ALLOW_GROUP))
        return TRUE;

    place_monster_m_idx = hack_m_idx_ii;

    /* Reinforcement */
    for (int i = 0; i < 6; i++) {
        if (!r_ptr->reinforce_id[i])
            break;
        int n = damroll(r_ptr->reinforce_dd[i], r_ptr->reinforce_ds[i]);
        for (int j = 0; j < n; j++) {
            POSITION nx, ny, d = 7;
            scatter(player_ptr, &ny, &nx, y, x, d, PROJECT_NONE);
            (void)place_monster_one(player_ptr, place_monster_m_idx, ny, nx, r_ptr->reinforce_id[i], mode);
        }
    }

    if (r_ptr->flags1 & (RF1_FRIENDS)) {
        (void)place_monster_group(player_ptr, who, y, x, r_idx, mode);
    }

    if (!(r_ptr->flags1 & (RF1_ESCORT)))
        return TRUE;

    place_monster_idx = r_idx;
    for (int i = 0; i < 32; i++) {
        POSITION nx, ny, d = 3;
        MONRACE_IDX z;
        scatter(player_ptr, &ny, &nx, y, x, d, PROJECT_NONE);
        if (!is_cave_empty_bold2(player_ptr, ny, nx))
            continue;

        get_mon_num_prep(player_ptr, place_monster_can_escort, get_monster_hook2(player_ptr, ny, nx));
        z = get_mon_num(player_ptr, 0, r_ptr->level, 0);
        if (!z)
            break;

        (void)place_monster_one(player_ptr, place_monster_m_idx, ny, nx, z, mode);
        if ((r_info[z].flags1 & RF1_FRIENDS) || (r_ptr->flags1 & RF1_ESCORTS)) {
            (void)place_monster_group(player_ptr, place_monster_m_idx, ny, nx, z, mode);
        }
    }

    return TRUE;
}

/*!
 * @brief 一般的なモンスター生成処理のメインルーチン / Attempt to place a monster of the given race at the given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 生成地点y座標
 * @param x 生成地点x座標
 * @param mode 生成オプション
 * @return 生成に成功したらtrue
 */
bool place_monster(player_type *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode)
{
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), get_monster_hook2(player_ptr, y, x));
    MONRACE_IDX r_idx = get_mon_num(player_ptr, 0, player_ptr->current_floor_ptr->monster_level, 0);
    if (r_idx == 0)
        return FALSE;

    if ((one_in_(5) || (player_ptr->current_floor_ptr->dun_level == 0)) && !(r_info[r_idx].flags1 & RF1_UNIQUE)
        && angband_strchr("hkoptuyAHLOPTUVY", r_info[r_idx].d_char)) {
        mode |= PM_JURAL;
    }

    return place_monster_aux(player_ptr, 0, y, x, r_idx, mode);
}

/*!
 * @brief 指定地点に1種類のモンスター種族による群れを生成する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 生成地点y座標
 * @param x 生成地点x座標
 * @return 生成に成功したらtrue
 */
bool alloc_horde(player_type *player_ptr, POSITION y, POSITION x, summon_specific_pf summon_specific)
{
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), get_monster_hook2(player_ptr, y, x));

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    MONRACE_IDX r_idx = 0;
    int attempts = 1000;
    monster_race *r_ptr = NULL;
    while (--attempts) {
        r_idx = get_mon_num(player_ptr, 0, floor_ptr->monster_level, 0);
        if (!r_idx)
            return FALSE;

        r_ptr = &r_info[r_idx];
        if (r_ptr->flags1 & RF1_UNIQUE)
            continue;

        if (r_idx == MON_HAGURE)
            continue;
        break;
    }

    if (attempts < 1)
        return FALSE;

    attempts = 1000;

    while (--attempts) {
        if (place_monster_aux(player_ptr, 0, y, x, r_idx, 0L))
            break;
    }

    if (attempts < 1)
        return FALSE;

    MONSTER_IDX m_idx = floor_ptr->grid_array[y][x].m_idx;
    if (floor_ptr->m_list[m_idx].mflag2 & MFLAG2_CHAMELEON)
        r_ptr = &r_info[floor_ptr->m_list[m_idx].r_idx];

    POSITION cy = y;
    POSITION cx = x;
    for (attempts = randint1(10) + 5; attempts; attempts--) {
        scatter(player_ptr, &cy, &cx, y, x, 5, PROJECT_NONE);
        (void)(*summon_specific)(player_ptr, m_idx, cy, cx, floor_ptr->dun_level + 5, SUMMON_KIN, PM_ALLOW_GROUP);
        y = cy;
        x = cx;
    }

    if (cheat_hear)
        msg_format(_("モンスターの大群(%c)", "Monster horde (%c)."), r_ptr->d_char);
    return TRUE;
}

/*!
 * @brief ダンジョンの主生成を試みる / Put the Guardian
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param def_val 現在の主の生成状態
 * @return 生成に成功したらtrue
 */
bool alloc_guardian(player_type *player_ptr, bool def_val)
{
    MONRACE_IDX guardian = d_info[player_ptr->dungeon_idx].final_guardian;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    bool is_guardian_applicable = guardian > 0;
    is_guardian_applicable &= d_info[player_ptr->dungeon_idx].maxdepth == floor_ptr->dun_level;
    is_guardian_applicable &= r_info[guardian].cur_num < r_info[guardian].max_num;
    if (!is_guardian_applicable)
        return def_val;

    int try_count = 4000;
    while (try_count) {
        POSITION oy = randint1(floor_ptr->height - 4) + 2;
        POSITION ox = randint1(floor_ptr->width - 4) + 2;
        if (!is_cave_empty_bold2(player_ptr, oy, ox)) {
            try_count++;
            continue;
        }

        if (!monster_can_cross_terrain(player_ptr, floor_ptr->grid_array[oy][ox].feat, &r_info[guardian], 0)) {
            try_count++;
            continue;
        }

        if (place_monster_aux(player_ptr, 0, oy, ox, guardian, (PM_ALLOW_GROUP | PM_NO_KAGE | PM_NO_PET)))
            return TRUE;

        try_count--;
    }

    return FALSE;
}

/*!
 * @brief ダンジョンの初期配置モンスターを生成1回生成する / Attempt to allocate a random monster in the dungeon.
 * @param dis プレイヤーから離れるべき最低距離
 * @param mode 生成オプション
 * @return 生成に成功したらtrue
 * @details
 * Place the monster at least "dis" distance from the player.
 * Use "slp" to choose the initial "sleep" status
 * Use "floor_ptr->monster_level" for the monster level
 */
bool alloc_monster(player_type *player_ptr, POSITION dis, BIT_FLAGS mode, summon_specific_pf summon_specific)
{
    if (alloc_guardian(player_ptr, FALSE))
        return TRUE;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    POSITION y = 0, x = 0;
    int attempts_left = 10000;
    while (attempts_left--) {
        y = randint0(floor_ptr->height);
        x = randint0(floor_ptr->width);

        if (floor_ptr->dun_level) {
            if (!is_cave_empty_bold2(player_ptr, y, x))
                continue;
        } else {
            if (!is_cave_empty_bold(player_ptr, y, x))
                continue;
        }

        if (distance(y, x, player_ptr->y, player_ptr->x) > dis)
            break;
    }

    if (!attempts_left) {
        if (cheat_xtra || cheat_hear) {
            msg_print(_("警告！新たなモンスターを配置できません。小さい階ですか？", "Warning! Could not allocate a new monster. Small level?"));
        }

        return FALSE;
    }

    if (randint1(5000) <= floor_ptr->dun_level) {
        if (alloc_horde(player_ptr, y, x, summon_specific)) {
            return TRUE;
        }
    } else {
        if (place_monster(player_ptr, y, x, (mode | PM_ALLOW_GROUP)))
            return TRUE;
    }

    return FALSE;
}