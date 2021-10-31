/*!
 * @brief モンスターをフロアに1体配置する処理
 * @date 2020/06/13
 * @author Hourier
 */

#include "monster-floor/one-monster-placer.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-save-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "grid/grid.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-floor/monster-move.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "object/warning.h"
#include "player/player-status.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

static bool is_friendly_idx(player_type *player_ptr, MONSTER_IDX m_idx)
{
    return m_idx > 0 && is_friendly(&player_ptr->current_floor_ptr->m_list[(m_idx)]);
}

/*!
 * @brief たぬきの変身対象となるモンスターかどうか判定する / Hook for Tanuki
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_tanuki(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    bool unselectable = any_bits(r_ptr->flags1, RF1_UNIQUE);
    unselectable |= any_bits(r_ptr->flags2, RF2_MULTIPLY);
    unselectable |= any_bits(r_ptr->flags7, RF7_FRIENDLY | RF7_CHAMELEON);
    unselectable |= any_bits(r_ptr->flags7, RF7_AQUATIC);
    if (unselectable)
        return false;

    for (int i = 0; i < 4; i++)
        if (r_ptr->blow[i].method == RBM_EXPLODE)
            return false;

    return (*(get_monster_hook(player_ptr)))(player_ptr, r_idx);
}

/*!
 * @param player_ptr プレイヤーへの参照ポインタ
 * @brief モンスターの表層IDを設定する / Set initial racial appearance of a monster
 * @param r_idx モンスター種族ID
 * @return モンスター種族の表層ID
 */
static MONRACE_IDX initial_r_appearance(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS generate_mode)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (is_chargeman(player_ptr) && any_bits(generate_mode, PM_JURAL) && none_bits(generate_mode, PM_MULTIPLY | PM_KAGE))
        return MON_ALIEN_JURAL;

    if (none_bits(r_info[r_idx].flags7, RF7_TANUKI))
        return r_idx;

    get_mon_num_prep(player_ptr, monster_hook_tanuki, nullptr);
    int attempts = 1000;
    DEPTH min = std::min(floor_ptr->base_level - 5, 50);
    while (--attempts) {
        MONRACE_IDX ap_r_idx = get_mon_num(player_ptr, 0, floor_ptr->base_level + 10, 0);
        if (r_info[ap_r_idx].level >= min)
            return ap_r_idx;
    }

    return r_idx;
}

/*!
 * @brief ユニークが生成可能か評価する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx 生成モンスター種族
 * @return ユニークの生成が不可能な条件ならFALSE、それ以外はTRUE
 */
static bool check_unique_placeable(player_type *player_ptr, MONRACE_IDX r_idx)
{
    if (player_ptr->phase_out)
        return true;

    monster_race *r_ptr = &r_info[r_idx];
    if ((any_bits(r_ptr->flags1, RF1_UNIQUE) || any_bits(r_ptr->flags7, RF7_NAZGUL)) && (r_ptr->cur_num >= r_ptr->max_num)) {
        return false;
    }

    if (any_bits(r_ptr->flags7, RF7_UNIQUE2) && (r_ptr->cur_num >= 1)) {
        return false;
    }

    if (r_idx == MON_BANORLUPART) {
        if (r_info[MON_BANOR].cur_num > 0)
            return false;
        if (r_info[MON_LUPART].cur_num > 0)
            return false;
    }

    if (any_bits(r_ptr->flags1, RF1_FORCE_DEPTH) && (player_ptr->current_floor_ptr->dun_level < r_ptr->level)
        && (!ironman_nightmare || any_bits(r_ptr->flags1, RF1_QUESTOR))) {
        return false;
    }

    return true;
}

/*!
 * @brief クエスト内に生成可能か評価する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx 生成モンスター種族
 * @return 生成が可能ならTRUE、不可能ならFALSE
 */
static bool check_quest_placeable(player_type *player_ptr, MONRACE_IDX r_idx)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (quest_number(player_ptr, floor_ptr->dun_level) == 0)
        return true;

    int hoge = quest_number(player_ptr, floor_ptr->dun_level);
    if ((quest[hoge].type != QuestKindType::KILL_LEVEL) && (quest[hoge].type != QuestKindType::RANDOM))
        return true;

    if (r_idx != quest[hoge].r_idx)
        return true;

    int number_mon = 0;
    for (int i2 = 0; i2 < floor_ptr->width; ++i2)
        for (int j2 = 0; j2 < floor_ptr->height; j2++)
            if ((floor_ptr->grid_array[j2][i2].m_idx > 0) && (floor_ptr->m_list[floor_ptr->grid_array[j2][i2].m_idx].r_idx == quest[hoge].r_idx))
                number_mon++;

    if (number_mon + quest[hoge].cur_num >= quest[hoge].max_num)
        return false;

    return true;
}

/*!
 * @brief 守りのルーン上にモンスターの配置を試みる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx 生成モンスター種族
 * @param y 生成位置y座標
 * @param x 生成位置x座標
 * @return 生成が可能ならTRUE、不可能ならFALSE
 */
static bool check_procection_rune(player_type *player_ptr, MONRACE_IDX r_idx, POSITION y, POSITION x)
{
    grid_type *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
    if (!g_ptr->is_rune_protection())
        return true;

    monster_race *r_ptr = &r_info[r_idx];
    if (randint1(BREAK_RUNE_PROTECTION) >= (r_ptr->level + 20))
        return false;

    if (any_bits(g_ptr->info, CAVE_MARK))
        msg_print(_("守りのルーンが壊れた！", "The rune of protection is broken!"));

    reset_bits(g_ptr->info, CAVE_MARK);
    reset_bits(g_ptr->info, CAVE_OBJECT);
    g_ptr->mimic = 0;
    note_spot(player_ptr, y, x);
    return true;
}

static void warn_unique_generation(player_type *player_ptr, MONRACE_IDX r_idx)
{
    if (!player_ptr->warning || !w_ptr->character_dungeon)
        return;

    monster_race *r_ptr = &r_info[r_idx];
    if (none_bits(r_ptr->flags1, RF1_UNIQUE))
        return;

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
    if (o_ptr != nullptr) {
        describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        msg_format(_("%sは%s光った。", "%s glows %s."), o_name, color);
    } else {
        msg_format(_("%s光る物が頭に浮かんだ。", "A %s image forms in your mind."), color);
    }
}

/*!
 * @brief モンスターを一体生成する / Attempt to place a monster of the given race at the given location.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param who 召喚を行ったモンスターID
 * @param y 生成位置y座標
 * @param x 生成位置x座標
 * @param r_idx 生成モンスター種族
 * @param mode 生成オプション
 * @return 成功したらtrue
 */
bool place_monster_one(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    monster_race *r_ptr = &r_info[r_idx];
    concptr name = r_ptr->name.c_str();

    if (player_ptr->wild_mode || !in_bounds(floor_ptr, y, x) || (r_idx == 0) || r_ptr->name.empty())
        return false;

    if (none_bits(mode, PM_IGNORE_TERRAIN) && (pattern_tile(floor_ptr, y, x) || !monster_can_enter(player_ptr, y, x, r_ptr, 0)))
        return false;

    if (!check_unique_placeable(player_ptr, r_idx) || !check_quest_placeable(player_ptr, r_idx) || !check_procection_rune(player_ptr, r_idx, y, x))
        return false;

    msg_format_wizard(player_ptr, CHEAT_MONSTER, _("%s(Lv%d)を生成しました。", "%s(Lv%d) was generated."), name, r_ptr->level);
    if (any_bits(r_ptr->flags1, RF1_UNIQUE) || any_bits(r_ptr->flags7, RF7_NAZGUL) || (r_ptr->level < 10))
        reset_bits(mode, PM_KAGE);

    g_ptr->m_idx = m_pop(floor_ptr);
    hack_m_idx_ii = g_ptr->m_idx;
    if (!g_ptr->m_idx)
        return false;

    monster_type *m_ptr;
    m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    m_ptr->r_idx = r_idx;
    m_ptr->ap_r_idx = initial_r_appearance(player_ptr, r_idx, mode);

    m_ptr->mflag.clear();
    m_ptr->mflag2.clear();
    if (any_bits(mode, PM_MULTIPLY) && (who > 0) && !is_original_ap(&floor_ptr->m_list[who])) {
        m_ptr->ap_r_idx = floor_ptr->m_list[who].ap_r_idx;
        if (floor_ptr->m_list[who].mflag2.has(MFLAG2::KAGE))
            m_ptr->mflag2.set(MFLAG2::KAGE);
    }

    if ((who > 0) && none_bits(r_ptr->flags3, RF3_EVIL | RF3_GOOD))
        m_ptr->sub_align = floor_ptr->m_list[who].sub_align;
    else {
        m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
        if (any_bits(r_ptr->flags3, RF3_EVIL))
            set_bits(m_ptr->sub_align, SUB_ALIGN_EVIL);
        if (any_bits(r_ptr->flags3, RF3_GOOD))
            set_bits(m_ptr->sub_align, SUB_ALIGN_GOOD);
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
        set_bits(mode, PM_FORCE_PET);
        m_ptr->parent_m_idx = who;
    } else {
        m_ptr->parent_m_idx = 0;
    }

    if (any_bits(r_ptr->flags7, RF7_CHAMELEON)) {
        choose_new_monster(player_ptr, g_ptr->m_idx, true, 0);
        r_ptr = &r_info[m_ptr->r_idx];
        m_ptr->mflag2.set(MFLAG2::CHAMELEON);
        if (any_bits(r_ptr->flags1, RF1_UNIQUE) && (who <= 0))
            m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
    } else if (any_bits(mode, PM_KAGE) && none_bits(mode, PM_FORCE_PET)) {
        m_ptr->ap_r_idx = MON_KAGE;
        m_ptr->mflag2.set(MFLAG2::KAGE);
    }

    if (any_bits(mode, PM_NO_PET))
        m_ptr->mflag2.set(MFLAG2::NOPET);

    m_ptr->ml = false;
    if (any_bits(mode, PM_FORCE_PET)) {
        set_pet(player_ptr, m_ptr);
    } else if (((who == 0) && any_bits(r_ptr->flags7, RF7_FRIENDLY)) || is_friendly_idx(player_ptr, who) || any_bits(mode, PM_FORCE_FRIENDLY)) {
        if (!monster_has_hostile_align(player_ptr, nullptr, 0, -1, r_ptr) && !player_ptr->current_floor_ptr->inside_arena)
            set_friendly(m_ptr);
    }

    m_ptr->mtimed[MTIMED_CSLEEP] = 0;
    if (any_bits(mode, PM_ALLOW_SLEEP) && r_ptr->sleep && !ironman_nightmare) {
        int val = r_ptr->sleep;
        (void)set_monster_csleep(player_ptr, g_ptr->m_idx, (val * 2) + randint1(val * 10));
    }

    if (any_bits(r_ptr->flags1, RF1_FORCE_MAXHP)) {
        m_ptr->max_maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
    } else {
        m_ptr->max_maxhp = damroll(r_ptr->hdice, r_ptr->hside);
    }

    if (ironman_nightmare) {
        auto hp = m_ptr->max_maxhp * 2;
        m_ptr->max_maxhp = std::min(MONSTER_MAXHP, hp);
    }

    m_ptr->maxhp = m_ptr->max_maxhp;
    if (m_ptr->r_idx == MON_WOUNDED_BEAR)
        m_ptr->hp = m_ptr->maxhp / 2;
    else
        m_ptr->hp = m_ptr->maxhp;

    m_ptr->dealt_damage = 0;

    m_ptr->mspeed = get_mspeed(floor_ptr, r_ptr);

    if (any_bits(mode, PM_HASTE))
        (void)set_monster_fast(player_ptr, g_ptr->m_idx, 100);

    if (!ironman_nightmare) {
        m_ptr->energy_need = ENERGY_NEED() - (int16_t)randint0(100);
    } else {
        m_ptr->energy_need = ENERGY_NEED() - (int16_t)randint0(100) * 2;
    }

    if (any_bits(r_ptr->flags1, RF1_PREVENT_SUDDEN_MAGIC) && !ironman_nightmare) {
        m_ptr->mflag.set(MFLAG::PREVENT_MAGIC);
    }

    if (g_ptr->m_idx < hack_m_idx) {
        m_ptr->mflag.set(MFLAG::BORN);
    }

    if (any_bits(r_ptr->flags7, RF7_SELF_LD_MASK))
        set_bits(player_ptr->update, PU_MON_LITE);
    else if (any_bits(r_ptr->flags7, RF7_HAS_LD_MASK) && !monster_csleep_remaining(m_ptr))
        set_bits(player_ptr->update, PU_MON_LITE);
    update_monster(player_ptr, g_ptr->m_idx, true);

    real_r_ptr(m_ptr)->cur_num++;

    /*
     * Memorize location of the unique monster in saved floors.
     * A unique monster move from old saved floor.
     */
    if (w_ptr->character_dungeon && (any_bits(r_ptr->flags1, RF1_UNIQUE) || any_bits(r_ptr->flags7, RF7_NAZGUL)))
        real_r_ptr(m_ptr)->floor_id = player_ptr->floor_id;

    if (any_bits(r_ptr->flags2, RF2_MULTIPLY))
        floor_ptr->num_repro++;

    warn_unique_generation(player_ptr, r_idx);
    if (!g_ptr->is_rune_explosion())
        return true;

    if (randint1(BREAK_RUNE_EXPLOSION) > r_ptr->level) {
        if (any_bits(g_ptr->info, CAVE_MARK)) {
            msg_print(_("ルーンが爆発した！", "The rune explodes!"));
            project(player_ptr, 0, 2, y, x, 2 * (player_ptr->lev + damroll(7, 7)), GF_MANA,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI));
        }
    } else {
        msg_print(_("爆発のルーンは解除された。", "An explosive rune was disarmed."));
    }

    reset_bits(g_ptr->info, CAVE_MARK);
    reset_bits(g_ptr->info, CAVE_OBJECT);
    g_ptr->mimic = 0;

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);

    return true;
}
