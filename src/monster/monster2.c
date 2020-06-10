/*!
 * @file monster2.c
 * @brief モンスター処理 / misc code for monsters
 * @date 2014/07/08
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "monster/monster2.h"
#include "core/player-processor.h"
#include "core/speed-table.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-object.h"
#include "floor/wild.h"
#include "io/files-util.h"
#include "io/targeting.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "mind/drs-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h" // todo 相互参照している.
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "monster/place-monster-types.h"
#include "monster/smart-learn-types.h"
#include "mspell/summon-checker.h"
#include "object/object-flavor.h"
#include "object/object-generator.h"
#include "object/warning.h"
#include "pet/pet-fall-off.h"
#include "player/eldritch-horror.h"
#include "player/player-move.h"
#include "spell/process-effect.h"
#include "spell/spells-summon.h"
#include "spell/spells-type.h"
#include "view/display-main-window.h"
#include "world/world.h"

#define HORDE_NOGOOD 0x01 /*!< (未実装フラグ)HORDE生成でGOODなモンスターの生成を禁止する？ */
#define HORDE_NOEVIL 0x02 /*!< (未実装フラグ)HORDE生成でEVILなモンスターの生成を禁止する？ */
#define MON_SCAT_MAXD 10 /*!< mon_scatter()関数によるモンスター配置で許される中心からの最大距離 */

static bool is_friendly_idx(player_type *player_ptr, MONSTER_IDX m_idx) { return m_idx > 0 && is_friendly(&player_ptr->current_floor_ptr->m_list[(m_idx)]); }

/*!
 * @brief モンスターの目標地点をセットする / Set the target of counter attack
 * @param m_ptr モンスターの参照ポインタ
 * @param y 目標y座標
 * @param x 目標x座標
 * @return なし
 */
void set_target(monster_type *m_ptr, POSITION y, POSITION x)
{
    m_ptr->target_y = y;
    m_ptr->target_x = x;
}

/*!
 * @brief モンスターの目標地点をリセットする / Reset the target of counter attack
 * @param m_ptr モンスターの参照ポインタ
 * @return なし
 */
void reset_target(monster_type *m_ptr) { set_target(m_ptr, 0, 0); }

/*!
 * @brief モンスター配列からモンスターを消去する / Delete a monster by index.
 * @param i 消去するモンスターのID
 * @return なし
 * @details
 * モンスターを削除するとそのモンスターが拾っていたアイテムも同時に削除される。 /
 * When a monster is deleted, all of its objects are deleted.
 */
void delete_monster_idx(player_type *player_ptr, MONSTER_IDX i)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[i];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    POSITION y = m_ptr->fy;
    POSITION x = m_ptr->fx;

    real_r_ptr(m_ptr)->cur_num--;
    if (r_ptr->flags2 & (RF2_MULTIPLY))
        floor_ptr->num_repro--;

    if (monster_csleep_remaining(m_ptr))
        (void)set_monster_csleep(player_ptr, i, 0);
    if (monster_fast_remaining(m_ptr))
        (void)set_monster_fast(player_ptr, i, 0);
    if (monster_slow_remaining(m_ptr))
        (void)set_monster_slow(player_ptr, i, 0);
    if (monster_stunned_remaining(m_ptr))
        (void)set_monster_stunned(player_ptr, i, 0);
    if (monster_confused_remaining(m_ptr))
        (void)set_monster_confused(player_ptr, i, 0);
    if (monster_fear_remaining(m_ptr))
        (void)set_monster_monfear(player_ptr, i, 0);
    if (monster_invulner_remaining(m_ptr))
        (void)set_monster_invulner(player_ptr, i, 0, FALSE);

    if (i == target_who)
        target_who = 0;

    if (i == player_ptr->health_who)
        health_track(player_ptr, 0);

    if (player_ptr->pet_t_m_idx == i)
        player_ptr->pet_t_m_idx = 0;
    if (player_ptr->riding_t_m_idx == i)
        player_ptr->riding_t_m_idx = 0;
    if (player_ptr->riding == i)
        player_ptr->riding = 0;

    floor_ptr->grid_array[y][x].m_idx = 0;
    OBJECT_IDX next_o_idx = 0;
    for (OBJECT_IDX this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        delete_object_idx(player_ptr, this_o_idx);
    }

    (void)WIPE(m_ptr, monster_type);
    floor_ptr->m_cnt--;
    lite_spot(player_ptr, y, x);
    if (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)) {
        player_ptr->update |= (PU_MON_LITE);
    }
}

/*!
 * @brief モンスター情報を配列内移動する / Move an object from index i1 to index i2 in the object list
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param i1 配列移動元添字
 * @param i2 配列移動先添字
 * @return なし
 */
static void compact_monsters_aux(player_type *player_ptr, MONSTER_IDX i1, MONSTER_IDX i2)
{
    if (i1 == i2)
        return;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr;
    m_ptr = &floor_ptr->m_list[i1];

    POSITION y = m_ptr->fy;
    POSITION x = m_ptr->fx;
    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    g_ptr->m_idx = i2;

    OBJECT_IDX next_o_idx = 0;
    for (OBJECT_IDX this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        o_ptr->held_m_idx = i2;
    }

    if (target_who == i1)
        target_who = i2;

    if (player_ptr->pet_t_m_idx == i1)
        player_ptr->pet_t_m_idx = i2;
    if (player_ptr->riding_t_m_idx == i1)
        player_ptr->riding_t_m_idx = i2;

    if (player_ptr->riding == i1)
        player_ptr->riding = i2;

    if (player_ptr->health_who == i1)
        health_track(player_ptr, i2);

    if (is_pet(m_ptr)) {
        for (int i = 1; i < floor_ptr->m_max; i++) {
            monster_type *m2_ptr = &floor_ptr->m_list[i];

            if (m2_ptr->parent_m_idx == i1)
                m2_ptr->parent_m_idx = i2;
        }
    }

    (void)COPY(&floor_ptr->m_list[i2], &floor_ptr->m_list[i1], monster_type);
    (void)WIPE(&floor_ptr->m_list[i1], monster_type);

    for (int i = 0; i < MAX_MTIMED; i++) {
        int mproc_idx = get_mproc_idx(floor_ptr, i1, i);
        if (mproc_idx >= 0)
            floor_ptr->mproc_list[i][mproc_idx] = i2;
    }
}

/*!
 * @brief モンスター情報配列を圧縮する / Compact and Reorder the monster list
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param size 圧縮後のモンスター件数目標
 * @return なし
 * @details
 * This function can be very dangerous, use with caution!
 *
 * When actually "compacting" monsters, we base the saving throw
 * on a combination of monster level, distance from player, and
 * current "desperation".
 *
 * After "compacting" (if needed), we "reorder" the monsters into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_monsters(player_type *player_ptr, int size)
{
    if (size)
        msg_print(_("モンスター情報を圧縮しています...", "Compacting monsters..."));

    /* Compact at least 'size' objects */
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int num = 0, cnt = 1; num < size; cnt++) {
        int cur_lev = 5 * cnt;
        int cur_dis = 5 * (20 - cnt);
        for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
            monster_type *m_ptr = &floor_ptr->m_list[i];
            monster_race *r_ptr = &r_info[m_ptr->r_idx];
            if (!monster_is_valid(m_ptr))
                continue;
            if (r_ptr->level > cur_lev)
                continue;
            if (i == player_ptr->riding)
                continue;
            if ((cur_dis > 0) && (m_ptr->cdis < cur_dis))
                continue;

            int chance = 90;
            if ((r_ptr->flags1 & (RF1_QUESTOR)) && (cnt < 1000))
                chance = 100;

            if (r_ptr->flags1 & (RF1_UNIQUE))
                chance = 100;

            if (randint0(100) < chance)
                continue;

            if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(player_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
                exe_write_diary(player_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_COMPACT, m_name);
            }

            delete_monster_idx(player_ptr, i);
            num++;
        }
    }

    /* Excise dead monsters (backwards!) */
    for (MONSTER_IDX i = floor_ptr->m_max - 1; i >= 1; i--) {
        monster_type *m_ptr = &floor_ptr->m_list[i];
        if (m_ptr->r_idx)
            continue;
        compact_monsters_aux(player_ptr, floor_ptr->m_max - 1, i);
        floor_ptr->m_max--;
    }
}

/*!
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * @brief プレイヤーのフロア離脱に伴う全モンスター配列の消去 / Delete/Remove all the monsters when the player leaves the level
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * This is an efficient method of simulating multiple calls to the
 * "delete_monster()" function, with no visual effects.
 */
void wipe_monsters_list(player_type *player_ptr)
{
    if (!r_info[MON_BANORLUPART].max_num) {
        if (r_info[MON_BANOR].max_num) {
            r_info[MON_BANOR].max_num = 0;
            r_info[MON_BANOR].r_pkills++;
            r_info[MON_BANOR].r_akills++;
            if (r_info[MON_BANOR].r_tkills < MAX_SHORT)
                r_info[MON_BANOR].r_tkills++;
        }

        if (r_info[MON_LUPART].max_num) {
            r_info[MON_LUPART].max_num = 0;
            r_info[MON_LUPART].r_pkills++;
            r_info[MON_LUPART].r_akills++;
            if (r_info[MON_LUPART].r_tkills < MAX_SHORT)
                r_info[MON_LUPART].r_tkills++;
        }
    }

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = floor_ptr->m_max - 1; i >= 1; i--) {
        monster_type *m_ptr = &floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;

        floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].m_idx = 0;
        (void)WIPE(m_ptr, monster_type);
    }

    /*
     * Wiping racial counters of all monsters and incrementing of racial
     * counters of monsters in party_mon[] are required to prevent multiple
     * generation of unique monster who is the minion of player.
     */
    for (int i = 1; i < max_r_idx; i++)
        r_info[i].cur_num = 0;

    floor_ptr->m_max = 1;
    floor_ptr->m_cnt = 0;
    for (int i = 0; i < MAX_MTIMED; i++)
        floor_ptr->mproc_max[i] = 0;

    floor_ptr->num_repro = 0;
    target_who = 0;
    player_ptr->pet_t_m_idx = 0;
    player_ptr->riding_t_m_idx = 0;
    health_track(player_ptr, 0);
}

/*!
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * @brief モンスター配列の空きを探す / Acquires and returns the index of a "free" monster.
 * @return 利用可能なモンスター配列の添字
 * @details
 * This routine should almost never fail, but it *can* happen.
 */
MONSTER_IDX m_pop(player_type *player_ptr)
{
    /* Normal allocation */
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->m_max < current_world_ptr->max_m_idx) {
        MONSTER_IDX i = floor_ptr->m_max;
        floor_ptr->m_max++;
        floor_ptr->m_cnt++;
        return i;
    }

    /* Recycle dead monsters */
    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        monster_type *m_ptr;
        m_ptr = &floor_ptr->m_list[i];
        if (m_ptr->r_idx)
            continue;
        floor_ptr->m_cnt++;
        return i;
    }

    if (current_world_ptr->character_dungeon)
        msg_print(_("モンスターが多すぎる！", "Too many monsters!"));
    return 0;
}

/*!
 * @var summon_specific_who
 * @brief 召喚を行ったプレイヤーあるいはモンスターのIDを示すグローバル変数 / Hack -- the index of the summoning monster
 * @todo summon_specific_who グローバル変数の除去と関数引数への代替を行う
 */
static int summon_specific_who = -1;

/*!
 * @var summon_unique_okay
 * @brief 召喚対象にユニークを含めるかを示すグローバル変数 / summoning unique enable
 * @todo summon_unique_okay グローバル変数の除去と関数引数への代替を行う
 */
static bool summon_unique_okay = FALSE;

/*!
 * @brief 生成モンスター種族を1種生成テーブルから選択する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param level 生成階
 * @return 選択されたモンスター生成種族
 */
MONRACE_IDX get_mon_num(player_type *player_ptr, DEPTH level, BIT_FLAGS option)
{
    int i, j, p;
    int r_idx;
    long value, total;
    monster_race *r_ptr;
    alloc_entry *table = alloc_race_table;

    int pls_kakuritu, pls_level, over_days;
    int delay = mysqrt(level * 10000L) + (level * 5);

    /* town level : same delay as 10F, no nasty mons till day18 */
    if (!level)
        delay = 360;

    if (level > MAX_DEPTH - 1)
        level = MAX_DEPTH - 1;

    /* +1 per day after the base date */
    /* base dates : day5(1F), day18(10F,0F), day34(30F), day53(60F), day69(90F) */
    over_days = MAX(0, current_world_ptr->dungeon_turn / (TURNS_PER_TICK * 10000L) - delay / 20);

    /* starts from 1/25, reaches 1/3 after 44days from a level dependent base date */
    pls_kakuritu = MAX(NASTY_MON_MAX, NASTY_MON_BASE - over_days / 2);
    /* starts from 0, reaches +25lv after 75days from a level dependent base date */
    pls_level = MIN(NASTY_MON_PLUS_MAX, over_days / 3);

    if (d_info[player_ptr->dungeon_idx].flags1 & DF1_MAZE) {
        pls_kakuritu = MIN(pls_kakuritu / 2, pls_kakuritu - 10);
        if (pls_kakuritu < 2)
            pls_kakuritu = 2;
        pls_level += 2;
        level += 3;
    }

    /* Boost the level */
    if (!player_ptr->phase_out && !(d_info[player_ptr->dungeon_idx].flags1 & DF1_BEGINNER)) {
        /* Nightmare mode allows more out-of depth monsters */
        if (ironman_nightmare && !randint0(pls_kakuritu)) {
            /* What a bizarre calculation */
            level = 1 + (level * MAX_DEPTH / randint1(MAX_DEPTH));
        } else {
            /* Occasional "nasty" monster */
            if (!randint0(pls_kakuritu)) {
                /* Pick a level bonus */
                level += pls_level;
            }
        }
    }

    total = 0L;

    /* Process probabilities */
    for (i = 0; i < alloc_race_size; i++) {
        if (table[i].level > level)
            break;
        table[i].prob3 = 0;
        r_idx = table[i].index;
        r_ptr = &r_info[r_idx];
        if (!(option & GMN_ARENA) && !chameleon_change_m_idx) {
            if (((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flags7 & (RF7_NAZGUL))) && (r_ptr->cur_num >= r_ptr->max_num)) {
                continue;
            }

            if ((r_ptr->flags7 & (RF7_UNIQUE2)) && (r_ptr->cur_num >= 1)) {
                continue;
            }

            if (r_idx == MON_BANORLUPART) {
                if (r_info[MON_BANOR].cur_num > 0)
                    continue;
                if (r_info[MON_LUPART].cur_num > 0)
                    continue;
            }
        }

        table[i].prob3 = table[i].prob2;
        total += table[i].prob3;
    }

    if (total <= 0)
        return 0;

    value = randint0(total);
    int found_count = 0;
    for (i = 0; i < alloc_race_size; i++) {
        if (value < table[i].prob3)
            break;
        value = value - table[i].prob3;
        found_count++;
    }

    p = randint0(100);

    /* Try for a "harder" monster once (50%) or twice (10%) */
    if (p < 60) {
        j = found_count;
        value = randint0(total);
        for (found_count = 0; found_count < alloc_race_size; found_count++) {
            if (value < table[found_count].prob3)
                break;

            value = value - table[found_count].prob3;
        }

        if (table[found_count].level < table[j].level)
            found_count = j;
    }

    /* Try for a "harder" monster twice (10%) */
    if (p < 10) {
        j = found_count;
        value = randint0(total);
        for (found_count = 0; found_count < alloc_race_size; found_count++) {
            if (value < table[found_count].prob3)
                break;

            value = value - table[found_count].prob3;
        }

        if (table[found_count].level < table[j].level)
            found_count = j;
    }

    return (table[found_count].index);
}

/*!
 * @brief モンスターIDを取り、モンスター名をm_nameに代入する /
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param m_name モンスター名を入力する配列
 */
void monster_name(player_type *player_ptr, MONSTER_IDX m_idx, char *m_name)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_desc(player_ptr, m_name, m_ptr, 0x00);
}

/*!
 * todo ここにplayer_typeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @param player_ptr プレーヤーへの参照ポインタ
 * @brief カメレオンの王の変身対象となるモンスターかどうか判定する / Hack -- the index of the summoning monster
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 */
static bool monster_hook_chameleon_lord(MONRACE_IDX r_idx)
{
    floor_type *floor_ptr = p_ptr->current_floor_ptr;
    monster_race *r_ptr = &r_info[r_idx];
    monster_type *m_ptr = &floor_ptr->m_list[chameleon_change_m_idx];
    monster_race *old_r_ptr = &r_info[m_ptr->r_idx];

    if (!(r_ptr->flags1 & (RF1_UNIQUE)))
        return FALSE;
    if (r_ptr->flags7 & (RF7_FRIENDLY | RF7_CHAMELEON))
        return FALSE;

    if (ABS(r_ptr->level - r_info[MON_CHAMELEON_K].level) > 5)
        return FALSE;

    if ((r_ptr->blow[0].method == RBM_EXPLODE) || (r_ptr->blow[1].method == RBM_EXPLODE) || (r_ptr->blow[2].method == RBM_EXPLODE)
        || (r_ptr->blow[3].method == RBM_EXPLODE))
        return FALSE;

    if (!monster_can_cross_terrain(p_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].feat, r_ptr, 0))
        return FALSE;

    if (!(old_r_ptr->flags7 & RF7_CHAMELEON)) {
        if (monster_has_hostile_align(p_ptr, m_ptr, 0, 0, r_ptr))
            return FALSE;
    } else if (summon_specific_who > 0) {
        if (monster_has_hostile_align(p_ptr, &floor_ptr->m_list[summon_specific_who], 0, 0, r_ptr))
            return FALSE;
    }

    return TRUE;
}

/*!
 * todo ここにplayer_typeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @brief カメレオンの変身対象となるモンスターかどうか判定する / Hack -- the index of the summoning monster
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_chameleon(MONRACE_IDX r_idx)
{
    floor_type *floor_ptr = p_ptr->current_floor_ptr;
    monster_race *r_ptr = &r_info[r_idx];
    monster_type *m_ptr = &floor_ptr->m_list[chameleon_change_m_idx];
    monster_race *old_r_ptr = &r_info[m_ptr->r_idx];

    if (r_ptr->flags1 & (RF1_UNIQUE))
        return FALSE;
    if (r_ptr->flags2 & RF2_MULTIPLY)
        return FALSE;
    if (r_ptr->flags7 & (RF7_FRIENDLY | RF7_CHAMELEON))
        return FALSE;

    if ((r_ptr->blow[0].method == RBM_EXPLODE) || (r_ptr->blow[1].method == RBM_EXPLODE) || (r_ptr->blow[2].method == RBM_EXPLODE)
        || (r_ptr->blow[3].method == RBM_EXPLODE))
        return FALSE;

    if (!monster_can_cross_terrain(p_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].feat, r_ptr, 0))
        return FALSE;

    if (!(old_r_ptr->flags7 & RF7_CHAMELEON)) {
        if ((old_r_ptr->flags3 & RF3_GOOD) && !(r_ptr->flags3 & RF3_GOOD))
            return FALSE;
        if ((old_r_ptr->flags3 & RF3_EVIL) && !(r_ptr->flags3 & RF3_EVIL))
            return FALSE;
        if (!(old_r_ptr->flags3 & (RF3_GOOD | RF3_EVIL)) && (r_ptr->flags3 & (RF3_GOOD | RF3_EVIL)))
            return FALSE;
    } else if (summon_specific_who > 0) {
        if (monster_has_hostile_align(p_ptr, &floor_ptr->m_list[summon_specific_who], 0, 0, r_ptr))
            return FALSE;
    }

    return (*(get_monster_hook(p_ptr)))(r_idx);
}

/*!
 * @brief モンスターの変身処理
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx 変身処理を受けるモンスター情報のID
 * @param born 生成時の初変身先指定ならばtrue
 * @param r_idx 旧モンスター種族のID
 * @return なし
 */
void choose_new_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool born, MONRACE_IDX r_idx)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr;

    bool old_unique = FALSE;
    if (r_info[m_ptr->r_idx].flags1 & RF1_UNIQUE)
        old_unique = TRUE;
    if (old_unique && (r_idx == MON_CHAMELEON))
        r_idx = MON_CHAMELEON_K;
    r_ptr = &r_info[r_idx];

    char old_m_name[MAX_NLEN];
    monster_desc(player_ptr, old_m_name, m_ptr, 0);

    if (!r_idx) {
        DEPTH level;

        chameleon_change_m_idx = m_idx;
        if (old_unique)
            get_mon_num_prep(player_ptr, monster_hook_chameleon_lord, NULL);
        else
            get_mon_num_prep(player_ptr, monster_hook_chameleon, NULL);

        if (old_unique)
            level = r_info[MON_CHAMELEON_K].level;
        else if (!floor_ptr->dun_level)
            level = wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].level;
        else
            level = floor_ptr->dun_level;

        if (d_info[player_ptr->dungeon_idx].flags1 & DF1_CHAMELEON)
            level += 2 + randint1(3);

        r_idx = get_mon_num(player_ptr, level, 0);
        r_ptr = &r_info[r_idx];

        chameleon_change_m_idx = 0;
        if (!r_idx)
            return;
    }

    m_ptr->r_idx = r_idx;
    m_ptr->ap_r_idx = r_idx;
    update_monster(player_ptr, m_idx, FALSE);
    lite_spot(player_ptr, m_ptr->fy, m_ptr->fx);

    int old_r_idx = m_ptr->r_idx;
    if ((r_info[old_r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)) || (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)))
        player_ptr->update |= (PU_MON_LITE);

    if (born) {
        if (r_ptr->flags3 & (RF3_EVIL | RF3_GOOD)) {
            m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
            if (r_ptr->flags3 & RF3_EVIL)
                m_ptr->sub_align |= SUB_ALIGN_EVIL;
            if (r_ptr->flags3 & RF3_GOOD)
                m_ptr->sub_align |= SUB_ALIGN_GOOD;
        }

        return;
    }

    if (m_idx == player_ptr->riding) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(player_ptr, m_name, m_ptr, 0);
        msg_format(_("突然%sが変身した。", "Suddenly, %s transforms!"), old_m_name);
        if (!(r_ptr->flags7 & RF7_RIDING))
            if (process_fall_off_horse(player_ptr, 0, TRUE))
                msg_format(_("地面に落とされた。", "You have fallen from %s."), m_name);
    }

    m_ptr->mspeed = get_mspeed(player_ptr, r_ptr);

    int oldmaxhp = m_ptr->max_maxhp;
    if (r_ptr->flags1 & RF1_FORCE_MAXHP) {
        m_ptr->max_maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
    } else {
        m_ptr->max_maxhp = damroll(r_ptr->hdice, r_ptr->hside);
    }

    if (ironman_nightmare) {
        u32b hp = m_ptr->max_maxhp * 2L;
        m_ptr->max_maxhp = (HIT_POINT)MIN(30000, hp);
    }

    m_ptr->maxhp = (long)(m_ptr->maxhp * m_ptr->max_maxhp) / oldmaxhp;
    if (m_ptr->maxhp < 1)
        m_ptr->maxhp = 1;
    m_ptr->hp = (long)(m_ptr->hp * m_ptr->max_maxhp) / oldmaxhp;
    m_ptr->dealt_damage = 0;
}

/*!
 * todo ここにplayer_typeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @brief たぬきの変身対象となるモンスターかどうか判定する / Hook for Tanuki
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_tanuki(MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];

    if (r_ptr->flags1 & (RF1_UNIQUE))
        return FALSE;
    if (r_ptr->flags2 & RF2_MULTIPLY)
        return FALSE;
    if (r_ptr->flags7 & (RF7_FRIENDLY | RF7_CHAMELEON))
        return FALSE;
    if (r_ptr->flags7 & RF7_AQUATIC)
        return FALSE;

    if ((r_ptr->blow[0].method == RBM_EXPLODE) || (r_ptr->blow[1].method == RBM_EXPLODE) || (r_ptr->blow[2].method == RBM_EXPLODE)
        || (r_ptr->blow[3].method == RBM_EXPLODE))
        return FALSE;

    return (*(get_monster_hook(p_ptr)))(r_idx);
}

/*!
 * @param player_ptr プレーヤーへの参照ポインタ
 * @brief モンスターの表層IDを設定する / Set initial racial appearance of a monster
 * @param r_idx モンスター種族ID
 * @return モンスター種族の表層ID
 */
static MONRACE_IDX initial_r_appearance(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS generate_mode)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (player_ptr->pseikaku == PERSONALITY_CHARGEMAN && (generate_mode & PM_JURAL) && !(generate_mode & (PM_MULTIPLY | PM_KAGE))) {
        return MON_ALIEN_JURAL;
    }

    if (!(r_info[r_idx].flags7 & RF7_TANUKI))
        return r_idx;

    get_mon_num_prep(player_ptr, monster_hook_tanuki, NULL);

    int attempts = 1000;
    DEPTH min = MIN(floor_ptr->base_level - 5, 50);
    while (--attempts) {
        MONRACE_IDX ap_r_idx = get_mon_num(player_ptr, floor_ptr->base_level + 10, 0);
        if (r_info[ap_r_idx].level >= min)
            return ap_r_idx;
    }

    return r_idx;
}

/*!
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * @brief モンスターの個体加速を設定する / Get initial monster speed
 * @param r_ptr モンスター種族の参照ポインタ
 * @return 加速値
 */
SPEED get_mspeed(player_type *player_ptr, monster_race *r_ptr)
{
    SPEED mspeed = r_ptr->speed;
    if (!(r_ptr->flags1 & RF1_UNIQUE) && !player_ptr->current_floor_ptr->inside_arena) {
        /* Allow some small variation per monster */
        int i = SPEED_TO_ENERGY(r_ptr->speed) / (one_in_(4) ? 3 : 10);
        if (i)
            mspeed += rand_spread(0, i);
    }

    if (mspeed > 199)
        mspeed = 199;

    return mspeed;
}

/*!
 * @brief モンスターを一体生成する / Attempt to place a monster of the given race at the given location.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param who 召喚を行ったモンスターID
 * @param y 生成位置y座標
 * @param x 生成位置x座標
 * @param r_idx 生成モンスター種族
 * @param mode 生成オプション
 * @return 成功したらtrue
 * @details
 * To give the player a sporting chance, any monster that appears in
 * line-of-sight and is extremely dangerous can be marked as
 * "FORCE_SLEEP", which will cause them to be placed with low energy,
 * which often (but not always) lets the player move before they do.
 *
 * This routine refuses to place out-of-depth "FORCE_DEPTH" monsters.
 *
 * Use special "here" and "dead" flags for unique monsters,
 * remove old "cur_num" and "max_num" fields.
 *
 * Actually, do something similar for artifacts, to simplify
 * the "preserve" mode, and to make the "what artifacts" flag more useful.
 *
 * This is the only function which may place a monster in the dungeon,
 * except for the savefile loading code.
 */
static bool place_monster_one(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    monster_type *m_ptr;
    monster_race *r_ptr = &r_info[r_idx];
    concptr name = (r_name + r_ptr->name);

    if (player_ptr->wild_mode)
        return FALSE;
    if (!in_bounds(floor_ptr, y, x))
        return FALSE;
    if (!r_idx)
        return FALSE;
    if (!r_ptr->name)
        return FALSE;

    if (!(mode & PM_IGNORE_TERRAIN)) {
        if (pattern_tile(floor_ptr, y, x))
            return FALSE;
        if (!monster_can_enter(player_ptr, y, x, r_ptr, 0))
            return FALSE;
    }

    if (!player_ptr->phase_out) {
        if (((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flags7 & (RF7_NAZGUL))) && (r_ptr->cur_num >= r_ptr->max_num)) {
            return FALSE;
        }

        if ((r_ptr->flags7 & (RF7_UNIQUE2)) && (r_ptr->cur_num >= 1)) {
            return FALSE;
        }

        if (r_idx == MON_BANORLUPART) {
            if (r_info[MON_BANOR].cur_num > 0)
                return FALSE;
            if (r_info[MON_LUPART].cur_num > 0)
                return FALSE;
        }

        if ((r_ptr->flags1 & (RF1_FORCE_DEPTH)) && (floor_ptr->dun_level < r_ptr->level) && (!ironman_nightmare || (r_ptr->flags1 & (RF1_QUESTOR)))) {
            return FALSE;
        }
    }

    if (quest_number(player_ptr, floor_ptr->dun_level)) {
        int hoge = quest_number(player_ptr, floor_ptr->dun_level);
        if ((quest[hoge].type == QUEST_TYPE_KILL_LEVEL) || (quest[hoge].type == QUEST_TYPE_RANDOM)) {
            if (r_idx == quest[hoge].r_idx) {
                int number_mon, i2, j2;
                number_mon = 0;

                for (i2 = 0; i2 < floor_ptr->width; ++i2)
                    for (j2 = 0; j2 < floor_ptr->height; j2++)
                        if (floor_ptr->grid_array[j2][i2].m_idx > 0)
                            if (floor_ptr->m_list[floor_ptr->grid_array[j2][i2].m_idx].r_idx == quest[hoge].r_idx)
                                number_mon++;
                if (number_mon + quest[hoge].cur_num >= quest[hoge].max_num)
                    return FALSE;
            }
        }
    }

    if (is_glyph_grid(g_ptr)) {
        if (randint1(BREAK_GLYPH) < (r_ptr->level + 20)) {
            if (g_ptr->info & CAVE_MARK) {
                msg_print(_("守りのルーンが壊れた！", "The rune of protection is broken!"));
            }

            g_ptr->info &= ~(CAVE_MARK);
            g_ptr->info &= ~(CAVE_OBJECT);
            g_ptr->mimic = 0;

            note_spot(player_ptr, y, x);
        } else
            return FALSE;
    }

    msg_format_wizard(CHEAT_MONSTER, _("%s(Lv%d)を生成しました。", "%s(Lv%d) was generated."), name, r_ptr->level);
    if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL) || (r_ptr->level < 10))
        mode &= ~PM_KAGE;

    g_ptr->m_idx = m_pop(player_ptr);
    hack_m_idx_ii = g_ptr->m_idx;
    if (!g_ptr->m_idx)
        return FALSE;

    m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    m_ptr->r_idx = r_idx;
    m_ptr->ap_r_idx = initial_r_appearance(player_ptr, r_idx, mode);

    m_ptr->mflag = 0;
    m_ptr->mflag2 = 0;
    if ((mode & PM_MULTIPLY) && (who > 0) && !is_original_ap(&floor_ptr->m_list[who])) {
        m_ptr->ap_r_idx = floor_ptr->m_list[who].ap_r_idx;
        if (floor_ptr->m_list[who].mflag2 & MFLAG2_KAGE)
            m_ptr->mflag2 |= MFLAG2_KAGE;
    }

    if ((who > 0) && !(r_ptr->flags3 & (RF3_EVIL | RF3_GOOD)))
        m_ptr->sub_align = floor_ptr->m_list[who].sub_align;
    else {
        m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
        if (r_ptr->flags3 & RF3_EVIL)
            m_ptr->sub_align |= SUB_ALIGN_EVIL;
        if (r_ptr->flags3 & RF3_GOOD)
            m_ptr->sub_align |= SUB_ALIGN_GOOD;
    }

    m_ptr->fy = y;
    m_ptr->fx = x;
    m_ptr->current_floor_ptr = floor_ptr;

    for (int cmi = 0; cmi < MAX_MTIMED; cmi++)
        m_ptr->mtimed[cmi] = 0;

    m_ptr->cdis = 0;
    reset_target(m_ptr);
    m_ptr->nickname = 0;
    m_ptr->exp = 0;

    if (who > 0 && is_pet(&floor_ptr->m_list[who])) {
        mode |= PM_FORCE_PET;
        m_ptr->parent_m_idx = who;
    } else {
        m_ptr->parent_m_idx = 0;
    }

    if (r_ptr->flags7 & RF7_CHAMELEON) {
        choose_new_monster(player_ptr, g_ptr->m_idx, TRUE, 0);
        r_ptr = &r_info[m_ptr->r_idx];
        m_ptr->mflag2 |= MFLAG2_CHAMELEON;
        if ((r_ptr->flags1 & RF1_UNIQUE) && (who <= 0))
            m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
    } else if ((mode & PM_KAGE) && !(mode & PM_FORCE_PET)) {
        m_ptr->ap_r_idx = MON_KAGE;
        m_ptr->mflag2 |= MFLAG2_KAGE;
    }

    if (mode & PM_NO_PET)
        m_ptr->mflag2 |= MFLAG2_NOPET;

    m_ptr->ml = FALSE;
    if (mode & PM_FORCE_PET) {
        set_pet(player_ptr, m_ptr);
    } else if ((r_ptr->flags7 & RF7_FRIENDLY) || (mode & PM_FORCE_FRIENDLY) || is_friendly_idx(player_ptr, who)) {
        if (!monster_has_hostile_align(player_ptr, NULL, 0, -1, r_ptr))
            set_friendly(m_ptr);
    }

    m_ptr->mtimed[MTIMED_CSLEEP] = 0;
    if ((mode & PM_ALLOW_SLEEP) && r_ptr->sleep && !ironman_nightmare) {
        int val = r_ptr->sleep;
        (void)set_monster_csleep(player_ptr, g_ptr->m_idx, (val * 2) + randint1(val * 10));
    }

    if (r_ptr->flags1 & RF1_FORCE_MAXHP) {
        m_ptr->max_maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
    } else {
        m_ptr->max_maxhp = damroll(r_ptr->hdice, r_ptr->hside);
    }

    if (ironman_nightmare) {
        u32b hp = m_ptr->max_maxhp * 2L;

        m_ptr->max_maxhp = (HIT_POINT)MIN(30000, hp);
    }

    m_ptr->maxhp = m_ptr->max_maxhp;
    if (m_ptr->r_idx == MON_WOUNDED_BEAR)
        m_ptr->hp = m_ptr->maxhp / 2;
    else
        m_ptr->hp = m_ptr->maxhp;

    m_ptr->dealt_damage = 0;

    m_ptr->mspeed = get_mspeed(player_ptr, r_ptr);

    if (mode & PM_HASTE)
        (void)set_monster_fast(player_ptr, g_ptr->m_idx, 100);

    if (!ironman_nightmare) {
        m_ptr->energy_need = ENERGY_NEED() - (s16b)randint0(100);
    } else {
        m_ptr->energy_need = ENERGY_NEED() - (s16b)randint0(100) * 2;
    }

    if ((r_ptr->flags1 & RF1_FORCE_SLEEP) && !ironman_nightmare) {
        m_ptr->mflag |= (MFLAG_NICE);
        repair_monsters = TRUE;
    }

    if (g_ptr->m_idx < hack_m_idx) {
        m_ptr->mflag |= (MFLAG_BORN);
    }

    if (r_ptr->flags7 & RF7_SELF_LD_MASK)
        player_ptr->update |= (PU_MON_LITE);
    else if ((r_ptr->flags7 & RF7_HAS_LD_MASK) && !monster_csleep_remaining(m_ptr))
        player_ptr->update |= (PU_MON_LITE);
    update_monster(player_ptr, g_ptr->m_idx, TRUE);

    real_r_ptr(m_ptr)->cur_num++;

    /*
     * Memorize location of the unique monster in saved floors.
     * A unique monster move from old saved floor.
     */
    if (current_world_ptr->character_dungeon && ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)))
        real_r_ptr(m_ptr)->floor_id = player_ptr->floor_id;

    if (r_ptr->flags2 & RF2_MULTIPLY)
        floor_ptr->num_repro++;

    if (player_ptr->warning && current_world_ptr->character_dungeon) {
        if (r_ptr->flags1 & RF1_UNIQUE) {
            concptr color;
            object_type *o_ptr;
            GAME_TEXT o_name[MAX_NLEN];

            if (r_ptr->level > player_ptr->lev + 30)
                color = _("黒く", "black");
            else if (r_ptr->level > player_ptr->lev + 15)
                color = _("紫色に", "purple");
            else if (r_ptr->level > player_ptr->lev + 5)
                color = _("ルビー色に", "deep red");
            else if (r_ptr->level > player_ptr->lev - 5)
                color = _("赤く", "red");
            else if (r_ptr->level > player_ptr->lev - 15)
                color = _("ピンク色に", "pink");
            else
                color = _("白く", "white");

            o_ptr = choose_warning_item(player_ptr);
            if (o_ptr) {
                object_desc(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
                msg_format(_("%sは%s光った。", "%s glows %s."), o_name, color);
            } else {
                msg_format(_("%s光る物が頭に浮かんだ。", "An %s image forms in your mind."), color);
            }
        }
    }

    if (!is_explosive_rune_grid(g_ptr))
        return TRUE;

    if (randint1(BREAK_MINOR_GLYPH) > r_ptr->level) {
        if (g_ptr->info & CAVE_MARK) {
            msg_print(_("ルーンが爆発した！", "The rune explodes!"));
            project(player_ptr, 0, 2, y, x, 2 * (player_ptr->lev + damroll(7, 7)), GF_MANA,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
        }
    } else {
        msg_print(_("爆発のルーンは解除された。", "An explosive rune was disarmed."));
    }

    g_ptr->info &= ~(CAVE_MARK);
    g_ptr->info &= ~(CAVE_OBJECT);
    g_ptr->mimic = 0;

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);

    return TRUE;
}

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
            scatter(player_ptr, &my, &mx, hy, hx, 4, 0);
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
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * todo ここにplayer_typeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @brief モンスター種族が召喚主の護衛となれるかどうかをチェックする / Hack -- help pick an escort type
 * @param r_idx チェックするモンスター種族のID
 * @return 護衛にできるならばtrue
 */
static bool place_monster_can_escort(MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[place_monster_idx];
    monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[place_monster_m_idx];
    monster_race *z_ptr = &r_info[r_idx];

    if (mon_hook_dungeon(place_monster_idx) != mon_hook_dungeon(r_idx))
        return FALSE;
    if (z_ptr->d_char != r_ptr->d_char)
        return FALSE;
    if (z_ptr->level > r_ptr->level)
        return FALSE;
    if (z_ptr->flags1 & RF1_UNIQUE)
        return FALSE;
    if (place_monster_idx == r_idx)
        return FALSE;
    if (monster_has_hostile_align(p_ptr, m_ptr, 0, 0, z_ptr))
        return FALSE;

    if (r_ptr->flags7 & RF7_FRIENDLY) {
        if (monster_has_hostile_align(p_ptr, NULL, 1, -1, z_ptr))
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
 * @details
 * Note that certain monsters are now marked as requiring "friends".
 * These monsters, if successfully placed, and if the "grp" parameter
 * is TRUE, will be surrounded by a "group" of identical monsters.
 *
 * Note that certain monsters are now marked as requiring an "escort",
 * which is a collection of monsters with similar "race" but lower level.
 *
 * Some monsters induce a fake "group" flag on their escorts.
 *
 * Note the "bizarre" use of non-recursion to prevent annoying output
 * when running a code profiler.
 *
 * Note the use of the new "monster allocation table" code to restrict
 * the "get_mon_num()" function to "legal" escort types.
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
            scatter(player_ptr, &ny, &nx, y, x, d, 0);
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
        scatter(player_ptr, &ny, &nx, y, x, d, 0);
        if (!is_cave_empty_bold2(player_ptr, ny, nx))
            continue;

        get_mon_num_prep(player_ptr, place_monster_can_escort, get_monster_hook2(player_ptr, ny, nx));
        z = get_mon_num(player_ptr, r_ptr->level, 0);
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
    MONRACE_IDX r_idx;
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), get_monster_hook2(player_ptr, y, x));
    r_idx = get_mon_num(player_ptr, player_ptr->current_floor_ptr->monster_level, 0);
    if (!r_idx)
        return FALSE;

    if ((one_in_(5) || (player_ptr->current_floor_ptr->base_level == 0)) && !(r_info[r_idx].flags1 & RF1_UNIQUE)
        && my_strchr("hkoptuyAHLOPTUVY", r_info[r_idx].d_char)) {
        mode |= PM_JURAL;
    }

    if (place_monster_aux(player_ptr, 0, y, x, r_idx, mode))
        return TRUE;

    return FALSE;
}

/*!
 * @brief 指定地点に1種類のモンスター種族による群れを生成する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 生成地点y座標
 * @param x 生成地点x座標
 * @return 生成に成功したらtrue
 */
bool alloc_horde(player_type *player_ptr, POSITION y, POSITION x)
{
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), get_monster_hook2(player_ptr, y, x));

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    MONRACE_IDX r_idx = 0;
    int attempts = 1000;
    monster_race *r_ptr = NULL;
    while (--attempts) {
        r_idx = get_mon_num(player_ptr, floor_ptr->monster_level, 0);
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
        scatter(player_ptr, &cy, &cx, y, x, 5, 0);
        (void)summon_specific(player_ptr, m_idx, cy, cx, floor_ptr->dun_level + 5, SUMMON_KIN, PM_ALLOW_GROUP);
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
bool alloc_monster(player_type *player_ptr, POSITION dis, BIT_FLAGS mode)
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
        if (alloc_horde(player_ptr, y, x)) {
            return TRUE;
        }
    } else {
        if (place_monster(player_ptr, y, x, (mode | PM_ALLOW_GROUP)))
            return TRUE;
    }

    return FALSE;
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
 * @details
 *
 * We will attempt to place the monster up to 10 times before giving up.
 *
 * Note: SUMMON_UNIQUE and SUMMON_AMBERITES will summon Unique's
 * Note: SUMMON_HI_UNDEAD and SUMMON_HI_DRAGON may summon Unique's
 * Note: None of the other summon codes will ever summon Unique's.
 *
 * This function has been changed.  We now take the "monster level"
 * of the summoning monster as a parameter, and use that, along with
 * the current dungeon level, to help determine the level of the
 * desired monster.  Note that this is an upper bound, and also
 * tends to "prefer" monsters of that level.  Currently, we use
 * the average of the dungeon and monster levels, and then add
 * five to allow slight increases in monster power.
 *
 * Note that we use the new "monster allocation table" creation code
 * to restrict the "get_mon_num()" function to the set of "legal"
 * monsters, making this function much faster and more reliable.
 *
 * Note that this function may not succeed, though this is very rare.
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

/*!
 * @brief ダメージを受けたモンスターの様子を記述する / Dump a message describing a monster's reaction to damage
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター情報ID
 * @param dam 与えたダメージ
 * @return なし
 * @details
 * Technically should attempt to treat "Beholder"'s as jelly's
 */
void message_pain(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    GAME_TEXT m_name[MAX_NLEN];

    monster_desc(player_ptr, m_name, m_ptr, 0);

    if (dam == 0) {
        msg_format(_("%^sはダメージを受けていない。", "%^s is unharmed."), m_name);
        return;
    }

    HIT_POINT newhp = m_ptr->hp;
    HIT_POINT oldhp = newhp + dam;
    HIT_POINT tmp = (newhp * 100L) / oldhp;
    PERCENTAGE percentage = tmp;

    if (my_strchr(",ejmvwQ", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%^sはほとんど気にとめていない。", m_name);
        else if (percentage > 75)
            msg_format("%^sはしり込みした。", m_name);
        else if (percentage > 50)
            msg_format("%^sは縮こまった。", m_name);
        else if (percentage > 35)
            msg_format("%^sは痛みに震えた。", m_name);
        else if (percentage > 20)
            msg_format("%^sは身もだえした。", m_name);
        else if (percentage > 10)
            msg_format("%^sは苦痛で身もだえした。", m_name);
        else
            msg_format("%^sはぐにゃぐにゃと痙攣した。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s barely notices.", m_name);
        else if (percentage > 75)
            msg_format("%^s flinches.", m_name);
        else if (percentage > 50)
            msg_format("%^s squelches.", m_name);
        else if (percentage > 35)
            msg_format("%^s quivers in pain.", m_name);
        else if (percentage > 20)
            msg_format("%^s writhes about.", m_name);
        else if (percentage > 10)
            msg_format("%^s writhes in agony.", m_name);
        else
            msg_format("%^s jerks limply.", m_name);
        return;
#endif
    }

    if (my_strchr("l", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%^sはほとんど気にとめていない。", m_name);
        else if (percentage > 75)
            msg_format("%^sはしり込みした。", m_name);
        else if (percentage > 50)
            msg_format("%^sは躊躇した。", m_name);
        else if (percentage > 35)
            msg_format("%^sは痛みに震えた。", m_name);
        else if (percentage > 20)
            msg_format("%^sは身もだえした。", m_name);
        else if (percentage > 10)
            msg_format("%^sは苦痛で身もだえした。", m_name);
        else
            msg_format("%^sはぐにゃぐにゃと痙攣した。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s barely notices.", m_name);
        else if (percentage > 75)
            msg_format("%^s flinches.", m_name);
        else if (percentage > 50)
            msg_format("%^s hesitates.", m_name);
        else if (percentage > 35)
            msg_format("%^s quivers in pain.", m_name);
        else if (percentage > 20)
            msg_format("%^s writhes about.", m_name);
        else if (percentage > 10)
            msg_format("%^s writhes in agony.", m_name);
        else
            msg_format("%^s jerks limply.", m_name);
        return;
#endif
    }

    if (my_strchr("g#+<>", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%sは攻撃を気にとめていない。", m_name);
        else if (percentage > 75)
            msg_format("%sは攻撃に肩をすくめた。", m_name);
        else if (percentage > 50)
            msg_format("%^sは雷鳴のように吠えた。", m_name);
        else if (percentage > 35)
            msg_format("%^sは苦しげに吠えた。", m_name);
        else if (percentage > 20)
            msg_format("%^sはうめいた。", m_name);
        else if (percentage > 10)
            msg_format("%^sは躊躇した。", m_name);
        else
            msg_format("%^sはくしゃくしゃになった。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s ignores the attack.", m_name);
        else if (percentage > 75)
            msg_format("%^s shrugs off the attack.", m_name);
        else if (percentage > 50)
            msg_format("%^s roars thunderously.", m_name);
        else if (percentage > 35)
            msg_format("%^s rumbles.", m_name);
        else if (percentage > 20)
            msg_format("%^s grunts.", m_name);
        else if (percentage > 10)
            msg_format("%^s hesitates.", m_name);
        else
            msg_format("%^s crumples.", m_name);
        return;
#endif
    }

    if (my_strchr("JMR", r_ptr->d_char) || !isalpha(r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%^sはほとんど気にとめていない。", m_name);
        else if (percentage > 75)
            msg_format("%^sはシーッと鳴いた。", m_name);
        else if (percentage > 50)
            msg_format("%^sは怒って頭を上げた。", m_name);
        else if (percentage > 35)
            msg_format("%^sは猛然と威嚇した。", m_name);
        else if (percentage > 20)
            msg_format("%^sは身もだえした。", m_name);
        else if (percentage > 10)
            msg_format("%^sは苦痛で身もだえした。", m_name);
        else
            msg_format("%^sはぐにゃぐにゃと痙攣した。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s barely notices.", m_name);
        else if (percentage > 75)
            msg_format("%^s hisses.", m_name);
        else if (percentage > 50)
            msg_format("%^s rears up in anger.", m_name);
        else if (percentage > 35)
            msg_format("%^s hisses furiously.", m_name);
        else if (percentage > 20)
            msg_format("%^s writhes about.", m_name);
        else if (percentage > 10)
            msg_format("%^s writhes in agony.", m_name);
        else
            msg_format("%^s jerks limply.", m_name);
        return;
#endif
    }

    if (my_strchr("f", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%sは攻撃に肩をすくめた。", m_name);
        else if (percentage > 75)
            msg_format("%^sは吠えた。", m_name);
        else if (percentage > 50)
            msg_format("%^sは怒って吠えた。", m_name);
        else if (percentage > 35)
            msg_format("%^sは痛みでシーッと鳴いた。", m_name);
        else if (percentage > 20)
            msg_format("%^sは痛みで弱々しく鳴いた。", m_name);
        else if (percentage > 10)
            msg_format("%^sは苦痛にうめいた。", m_name);
        else
            msg_format("%sは哀れな鳴き声を出した。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s shrugs off the attack.", m_name);
        else if (percentage > 75)
            msg_format("%^s roars.", m_name);
        else if (percentage > 50)
            msg_format("%^s growls angrily.", m_name);
        else if (percentage > 35)
            msg_format("%^s hisses with pain.", m_name);
        else if (percentage > 20)
            msg_format("%^s mewls in pain.", m_name);
        else if (percentage > 10)
            msg_format("%^s hisses in agony.", m_name);
        else
            msg_format("%^s mewls pitifully.", m_name);
        return;
#endif
    }

    if (my_strchr("acFIKS", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%sは攻撃を気にとめていない。", m_name);
        else if (percentage > 75)
            msg_format("%^sはキーキー鳴いた。", m_name);
        else if (percentage > 50)
            msg_format("%^sはヨロヨロ逃げ回った。", m_name);
        else if (percentage > 35)
            msg_format("%^sはうるさく鳴いた。", m_name);
        else if (percentage > 20)
            msg_format("%^sは痛みに痙攣した。", m_name);
        else if (percentage > 10)
            msg_format("%^sは苦痛で痙攣した。", m_name);
        else
            msg_format("%^sはピクピクひきつった。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s ignores the attack.", m_name);
        else if (percentage > 75)
            msg_format("%^s chitters.", m_name);
        else if (percentage > 50)
            msg_format("%^s scuttles about.", m_name);
        else if (percentage > 35)
            msg_format("%^s twitters.", m_name);
        else if (percentage > 20)
            msg_format("%^s jerks in pain.", m_name);
        else if (percentage > 10)
            msg_format("%^s jerks in agony.", m_name);
        else
            msg_format("%^s twitches.", m_name);
        return;
#endif
    }

    if (my_strchr("B", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%^sはさえずった。", m_name);
        else if (percentage > 75)
            msg_format("%^sはピーピー鳴いた。", m_name);
        else if (percentage > 50)
            msg_format("%^sはギャーギャー鳴いた。", m_name);
        else if (percentage > 35)
            msg_format("%^sはギャーギャー鳴きわめいた。", m_name);
        else if (percentage > 20)
            msg_format("%^sは苦しんだ。", m_name);
        else if (percentage > 10)
            msg_format("%^sはのたうち回った。", m_name);
        else
            msg_format("%^sはキーキーと鳴き叫んだ。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s chirps.", m_name);
        else if (percentage > 75)
            msg_format("%^s twitters.", m_name);
        else if (percentage > 50)
            msg_format("%^s squawks.", m_name);
        else if (percentage > 35)
            msg_format("%^s chatters.", m_name);
        else if (percentage > 20)
            msg_format("%^s jeers.", m_name);
        else if (percentage > 10)
            msg_format("%^s flutters about.", m_name);
        else
            msg_format("%^s squeaks.", m_name);
        return;
#endif
    }

    if (my_strchr("duDLUW", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%sは攻撃を気にとめていない。", m_name);
        else if (percentage > 75)
            msg_format("%^sはしり込みした。", m_name);
        else if (percentage > 50)
            msg_format("%^sは痛みでシーッと鳴いた。", m_name);
        else if (percentage > 35)
            msg_format("%^sは痛みでうなった。", m_name);
        else if (percentage > 20)
            msg_format("%^sは痛みに吠えた。", m_name);
        else if (percentage > 10)
            msg_format("%^sは苦しげに叫んだ。", m_name);
        else
            msg_format("%^sは弱々しくうなった。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s ignores the attack.", m_name);
        else if (percentage > 75)
            msg_format("%^s flinches.", m_name);
        else if (percentage > 50)
            msg_format("%^s hisses in pain.", m_name);
        else if (percentage > 35)
            msg_format("%^s snarls with pain.", m_name);
        else if (percentage > 20)
            msg_format("%^s roars with pain.", m_name);
        else if (percentage > 10)
            msg_format("%^s gasps.", m_name);
        else
            msg_format("%^s snarls feebly.", m_name);
        return;
#endif
    }

    if (my_strchr("s", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%sは攻撃を気にとめていない。", m_name);
        else if (percentage > 75)
            msg_format("%sは攻撃に肩をすくめた。", m_name);
        else if (percentage > 50)
            msg_format("%^sはカタカタと笑った。", m_name);
        else if (percentage > 35)
            msg_format("%^sはよろめいた。", m_name);
        else if (percentage > 20)
            msg_format("%^sはカタカタ言った。", m_name);
        else if (percentage > 10)
            msg_format("%^sはよろめいた。", m_name);
        else
            msg_format("%^sはガタガタ言った。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s ignores the attack.", m_name);
        else if (percentage > 75)
            msg_format("%^s shrugs off the attack.", m_name);
        else if (percentage > 50)
            msg_format("%^s rattles.", m_name);
        else if (percentage > 35)
            msg_format("%^s stumbles.", m_name);
        else if (percentage > 20)
            msg_format("%^s rattles.", m_name);
        else if (percentage > 10)
            msg_format("%^s staggers.", m_name);
        else
            msg_format("%^s clatters.", m_name);
        return;
#endif
    }

    if (my_strchr("z", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%sは攻撃を気にとめていない。", m_name);
        else if (percentage > 75)
            msg_format("%sは攻撃に肩をすくめた。", m_name);
        else if (percentage > 50)
            msg_format("%^sはうめいた。", m_name);
        else if (percentage > 35)
            msg_format("%sは苦しげにうめいた。", m_name);
        else if (percentage > 20)
            msg_format("%^sは躊躇した。", m_name);
        else if (percentage > 10)
            msg_format("%^sはうなった。", m_name);
        else
            msg_format("%^sはよろめいた。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s ignores the attack.", m_name);
        else if (percentage > 75)
            msg_format("%^s shrugs off the attack.", m_name);
        else if (percentage > 50)
            msg_format("%^s groans.", m_name);
        else if (percentage > 35)
            msg_format("%^s moans.", m_name);
        else if (percentage > 20)
            msg_format("%^s hesitates.", m_name);
        else if (percentage > 10)
            msg_format("%^s grunts.", m_name);
        else
            msg_format("%^s staggers.", m_name);
        return;
#endif
    }

    if (my_strchr("G", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%sは攻撃を気にとめていない。", m_name);
        else if (percentage > 75)
            msg_format("%sは攻撃に肩をすくめた。", m_name);
        else if (percentage > 50)
            msg_format("%sはうめいた。", m_name);
        else if (percentage > 35)
            msg_format("%^sは泣きわめいた。", m_name);
        else if (percentage > 20)
            msg_format("%^sは吠えた。", m_name);
        else if (percentage > 10)
            msg_format("%sは弱々しくうめいた。", m_name);
        else
            msg_format("%^sはかすかにうめいた。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s ignores the attack.", m_name);
        else if (percentage > 75)
            msg_format("%^s shrugs off the attack.", m_name);
        else if (percentage > 50)
            msg_format("%^s moans.", m_name);
        else if (percentage > 35)
            msg_format("%^s wails.", m_name);
        else if (percentage > 20)
            msg_format("%^s howls.", m_name);
        else if (percentage > 10)
            msg_format("%^s moans softly.", m_name);
        else
            msg_format("%^s sighs.", m_name);
        return;
#endif
    }

    if (my_strchr("CZ", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%^sは攻撃に肩をすくめた。", m_name);
        else if (percentage > 75)
            msg_format("%^sは痛みでうなった。", m_name);
        else if (percentage > 50)
            msg_format("%^sは痛みでキャンキャン吠えた。", m_name);
        else if (percentage > 35)
            msg_format("%^sは痛みで鳴きわめいた。", m_name);
        else if (percentage > 20)
            msg_format("%^sは苦痛のあまり鳴きわめいた。", m_name);
        else if (percentage > 10)
            msg_format("%^sは苦痛でもだえ苦しんだ。", m_name);
        else
            msg_format("%^sは弱々しく吠えた。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s shrugs off the attack.", m_name);
        else if (percentage > 75)
            msg_format("%^s snarls with pain.", m_name);
        else if (percentage > 50)
            msg_format("%^s yelps in pain.", m_name);
        else if (percentage > 35)
            msg_format("%^s howls in pain.", m_name);
        else if (percentage > 20)
            msg_format("%^s howls in agony.", m_name);
        else if (percentage > 10)
            msg_format("%^s writhes in agony.", m_name);
        else
            msg_format("%^s yelps feebly.", m_name);
        return;
#endif
    }

    if (my_strchr("Xbilqrt", r_ptr->d_char)) {
#ifdef JP
        if (percentage > 95)
            msg_format("%^sは攻撃を気にとめていない。", m_name);
        else if (percentage > 75)
            msg_format("%^sは痛みでうなった。", m_name);
        else if (percentage > 50)
            msg_format("%^sは痛みで叫んだ。", m_name);
        else if (percentage > 35)
            msg_format("%^sは痛みで絶叫した。", m_name);
        else if (percentage > 20)
            msg_format("%^sは苦痛のあまり絶叫した。", m_name);
        else if (percentage > 10)
            msg_format("%^sは苦痛でもだえ苦しんだ。", m_name);
        else
            msg_format("%^sは弱々しく叫んだ。", m_name);
        return;
#else
        if (percentage > 95)
            msg_format("%^s ignores the attack.", m_name);
        else if (percentage > 75)
            msg_format("%^s grunts with pain.", m_name);
        else if (percentage > 50)
            msg_format("%^s squeals in pain.", m_name);
        else if (percentage > 35)
            msg_format("%^s shrieks in pain.", m_name);
        else if (percentage > 20)
            msg_format("%^s shrieks in agony.", m_name);
        else if (percentage > 10)
            msg_format("%^s writhes in agony.", m_name);
        else
            msg_format("%^s cries out feebly.", m_name);
        return;
#endif
    }

#ifdef JP
    if (percentage > 95)
        msg_format("%^sは攻撃に肩をすくめた。", m_name);
    else if (percentage > 75)
        msg_format("%^sは痛みでうなった。", m_name);
    else if (percentage > 50)
        msg_format("%^sは痛みで叫んだ。", m_name);
    else if (percentage > 35)
        msg_format("%^sは痛みで絶叫した。", m_name);
    else if (percentage > 20)
        msg_format("%^sは苦痛のあまり絶叫した。", m_name);
    else if (percentage > 10)
        msg_format("%^sは苦痛でもだえ苦しんだ。", m_name);
    else
        msg_format("%^sは弱々しく叫んだ。", m_name);
#else
    if (percentage > 95)
        msg_format("%^s shrugs off the attack.", m_name);
    else if (percentage > 75)
        msg_format("%^s grunts with pain.", m_name);
    else if (percentage > 50)
        msg_format("%^s cries out in pain.", m_name);
    else if (percentage > 35)
        msg_format("%^s screams in pain.", m_name);
    else if (percentage > 20)
        msg_format("%^s screams in agony.", m_name);
    else if (percentage > 10)
        msg_format("%^s writhes in agony.", m_name);
    else
        msg_format("%^s cries out feebly.", m_name);
#endif
}

/*!
 * @brief SMART(適格に攻撃を行う)モンスターの学習状況を更新する / Learn about an "observed" resistance.
 * @param m_idx 更新を行う「モンスター情報ID
 * @param what 学習対象ID
 * @return なし
 */
void update_smart_learn(player_type *player_ptr, MONSTER_IDX m_idx, int what)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    if (!smart_learn)
        return;
    if (r_ptr->flags2 & (RF2_STUPID))
        return;
    if (!(r_ptr->flags2 & (RF2_SMART)) && (randint0(100) < 50))
        return;

    switch (what) {
    case DRS_ACID:
        if (player_ptr->resist_acid)
            m_ptr->smart |= (SM_RES_ACID);
        if (is_oppose_acid(player_ptr))
            m_ptr->smart |= (SM_OPP_ACID);
        if (player_ptr->immune_acid)
            m_ptr->smart |= (SM_IMM_ACID);
        break;

    case DRS_ELEC:
        if (player_ptr->resist_elec)
            m_ptr->smart |= (SM_RES_ELEC);
        if (is_oppose_elec(player_ptr))
            m_ptr->smart |= (SM_OPP_ELEC);
        if (player_ptr->immune_elec)
            m_ptr->smart |= (SM_IMM_ELEC);
        break;

    case DRS_FIRE:
        if (player_ptr->resist_fire)
            m_ptr->smart |= (SM_RES_FIRE);
        if (is_oppose_fire(player_ptr))
            m_ptr->smart |= (SM_OPP_FIRE);
        if (player_ptr->immune_fire)
            m_ptr->smart |= (SM_IMM_FIRE);
        break;

    case DRS_COLD:
        if (player_ptr->resist_cold)
            m_ptr->smart |= (SM_RES_COLD);
        if (is_oppose_cold(player_ptr))
            m_ptr->smart |= (SM_OPP_COLD);
        if (player_ptr->immune_cold)
            m_ptr->smart |= (SM_IMM_COLD);
        break;

    case DRS_POIS:
        if (player_ptr->resist_pois)
            m_ptr->smart |= (SM_RES_POIS);
        if (is_oppose_pois(player_ptr))
            m_ptr->smart |= (SM_OPP_POIS);
        break;

    case DRS_NETH:
        if (player_ptr->resist_neth)
            m_ptr->smart |= (SM_RES_NETH);
        break;

    case DRS_LITE:
        if (player_ptr->resist_lite)
            m_ptr->smart |= (SM_RES_LITE);
        break;

    case DRS_DARK:
        if (player_ptr->resist_dark)
            m_ptr->smart |= (SM_RES_DARK);
        break;

    case DRS_FEAR:
        if (player_ptr->resist_fear)
            m_ptr->smart |= (SM_RES_FEAR);
        break;

    case DRS_CONF:
        if (player_ptr->resist_conf)
            m_ptr->smart |= (SM_RES_CONF);
        break;

    case DRS_CHAOS:
        if (player_ptr->resist_chaos)
            m_ptr->smart |= (SM_RES_CHAOS);
        break;

    case DRS_DISEN:
        if (player_ptr->resist_disen)
            m_ptr->smart |= (SM_RES_DISEN);
        break;

    case DRS_BLIND:
        if (player_ptr->resist_blind)
            m_ptr->smart |= (SM_RES_BLIND);
        break;

    case DRS_NEXUS:
        if (player_ptr->resist_nexus)
            m_ptr->smart |= (SM_RES_NEXUS);
        break;

    case DRS_SOUND:
        if (player_ptr->resist_sound)
            m_ptr->smart |= (SM_RES_SOUND);
        break;

    case DRS_SHARD:
        if (player_ptr->resist_shard)
            m_ptr->smart |= (SM_RES_SHARD);
        break;

    case DRS_FREE:
        if (player_ptr->free_act)
            m_ptr->smart |= (SM_IMM_FREE);
        break;

    case DRS_MANA:
        if (!player_ptr->msp)
            m_ptr->smart |= (SM_IMM_MANA);
        break;

    case DRS_REFLECT:
        if (player_ptr->reflect)
            m_ptr->smart |= (SM_IMM_REFLECT);
        break;
    }
}

/*!
 * @brief モンスターが盗みや拾いで確保していたアイテムを全てドロップさせる / Drop all items carried by a monster
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_ptr モンスター参照ポインタ
 * @return なし
 */
void monster_drop_carried_objects(player_type *player_ptr, monster_type *m_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    OBJECT_IDX next_o_idx = 0;
    for (OBJECT_IDX this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type forge;
        object_type *o_ptr;
        object_type *q_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        q_ptr = &forge;

        object_copy(q_ptr, o_ptr);
        q_ptr->held_m_idx = 0;
        delete_object_idx(player_ptr, this_o_idx);
        (void)drop_near(player_ptr, q_ptr, -1, m_ptr->fy, m_ptr->fx);
    }

    m_ptr->hold_o_idx = 0;
}

/*!
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * @brief 指定したモンスターに隣接しているモンスターの数を返す。
 * / Count number of adjacent monsters
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx 隣接数を調べたいモンスターのID
 * @return 隣接しているモンスターの数
 */
int get_monster_crowd_number(player_type *player_ptr, MONSTER_IDX m_idx)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    POSITION my = m_ptr->fy;
    POSITION mx = m_ptr->fx;
    int count = 0;
    for (int i = 0; i < 7; i++) {
        int ay = my + ddy_ddd[i];
        int ax = mx + ddx_ddd[i];

        if (!in_bounds(floor_ptr, ay, ax))
            continue;
        if (floor_ptr->grid_array[ay][ax].m_idx > 0)
            count++;
    }

    return count;
}
