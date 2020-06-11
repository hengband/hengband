/*!
 * todo 後で再分割する
 * @brief モンスター生成処理
 * @date 2020/06/10
 * @author Hourier
 */

#include "monster/monster-generator.h"
#include "core/player-processor.h" // 相互参照している、後で何とかする.
#include "core/speed-table.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h" // todo ここの関数をquest-generator.h とかに分離したい.
#include "effect/effect-characteristics.h"
#include "floor/floor.h"
#include "main/sound-definitions-table.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-move.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "monster/place-monster-types.h"
#include "monster/smart-learn-types.h"
#include "mspell/summon-checker.h"
#include "object/object-flavor.h"
#include "object/warning.h"
#include "spell/process-effect.h"
#include "spell/spells-summon.h"
#include "spell/spells-type.h"
#include "world/world.h"

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

static bool is_friendly_idx(player_type *player_ptr, MONSTER_IDX m_idx) { return m_idx > 0 && is_friendly(&player_ptr->current_floor_ptr->m_list[(m_idx)]); }

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
 * @brief モンスターを一体生成する / Attempt to place a monster of the given race at the given location.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param who 召喚を行ったモンスターID
 * @param y 生成位置y座標
 * @param x 生成位置x座標
 * @param r_idx 生成モンスター種族
 * @param mode 生成オプション
 * @return 成功したらtrue
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

    g_ptr->m_idx = m_pop(floor_ptr);
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
