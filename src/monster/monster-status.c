#include "monster/monster-status.h"
#include "autopick/autopick-pref-processor.h"
#include "cmd-io/cmd-dump.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "io/report.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h" // todo 相互依存. 後で何とかする.
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "mspell/mspell-mask-definitions.h"
#include "object-enchant/object-curse.h"
#include "player-info/avatar.h"
#include "player/player-personalities-types.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-random.h"
#include "spell/spells-summon.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief モンスターIDからPOWERFULフラグの有無を取得する /
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param m_idx モンスターID
 * @return POWERFULフラグがあればTRUE、なければFALSEを返す。
 */
bool monster_is_powerful(floor_type *floor_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    bool powerful = r_ptr->flags2 & RF2_POWERFUL ? TRUE : FALSE;
    return powerful;
}

/*!
 * @brief モンスターIDからモンスターのレベルを取得する(ただし最低1を保証する) /
 * @param m_idx モンスターID
 * @return モンスターのレベル
 */
DEPTH monster_level_idx(floor_type *floor_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    return rlev;
}

/*!
 * @brief モンスターに与えたダメージの修正処理 /
 * Modify the physical damage done to the monster.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_ptr ダメージを受けるモンスターの構造体参照ポインタ
 * @param dam ダメージ基本値
 * @param is_psy_spear 攻撃手段が光の剣ならばTRUE
 * @return 修正を行った結果のダメージ量
 * @details
 * <pre>
 * (for example when it's invulnerable or shielded)
 * ToDo: Accept a damage-type to calculate the modified damage from
 * things like fire, frost, lightning, poison, ... attacks.
 * "type" is not yet used and should be 0.
 * </pre>
 */
HIT_POINT mon_damage_mod(player_type *target_ptr, monster_type *m_ptr, HIT_POINT dam, bool is_psy_spear)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    if ((r_ptr->flagsr & RFR_RES_ALL) && dam > 0) {
        dam /= 100;
        if ((dam == 0) && one_in_(3))
            dam = 1;
    }

    if (monster_invulner_remaining(m_ptr)) {
        if (is_psy_spear) {
            if (!target_ptr->blind && is_seen(target_ptr, m_ptr)) {
                msg_print(_("バリアを切り裂いた！", "The barrier is penetrated!"));
            }
        } else if (!one_in_(PENETRATE_INVULNERABILITY)) {
            return 0;
        }
    }

    return dam;
}

/*!
 * @brief モンスターに与えたダメージを元に経験値を加算する /
 * Calculate experience point to be get
 * @param dam 与えたダメージ量
 * @param m_ptr ダメージを与えたモンスターの構造体参照ポインタ
 * @return なし
 * @details
 * <pre>
 * Even the 64 bit operation is not big enough to avoid overflaw
 * unless we carefully choose orders of multiplication and division.
 * Get the coefficient first, and multiply (potentially huge) base
 * experience point of a monster later.
 * </pre>
 */
static void get_exp_from_mon(player_type *target_ptr, HIT_POINT dam, monster_type *m_ptr)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    if (!monster_is_valid(m_ptr))
        return;
    if (is_pet(m_ptr) || target_ptr->phase_out)
        return;

    /*!
     * todo 変数宣言と代入を同時に実行するとコンパイル警告が出る
     * ここの整形は実施せず保留
     */
    s32b new_exp;
    u32b new_exp_frac;
    s32b div_h;
    u32b div_l;

    /*
     * - Ratio of monster's level to player's level effects
     * - Varying speed effects
     * - Get a fraction in proportion of damage point
     */
    new_exp = r_ptr->level * SPEED_TO_ENERGY(m_ptr->mspeed) * dam;
    new_exp_frac = 0;
    div_h = 0L;
    div_l = (target_ptr->max_plv + 2) * SPEED_TO_ENERGY(r_ptr->speed);

    /* Use (average maxhp * 2) as a denominator */
    if (!(r_ptr->flags1 & RF1_FORCE_MAXHP))
        s64b_mul(&div_h, &div_l, 0, r_ptr->hdice * (ironman_nightmare ? 2 : 1) * (r_ptr->hside + 1));
    else
        s64b_mul(&div_h, &div_l, 0, r_ptr->hdice * (ironman_nightmare ? 2 : 1) * r_ptr->hside * 2);

    /* Special penalty in the wilderness */
    if (!target_ptr->current_floor_ptr->dun_level && (!(r_ptr->flags8 & RF8_WILD_ONLY) || !(r_ptr->flags1 & RF1_UNIQUE)))
        s64b_mul(&div_h, &div_l, 0, 5);

    /* Do division first to prevent overflaw */
    s64b_div(&new_exp, &new_exp_frac, div_h, div_l);

    /* Special penalty for mutiply-monster */
    if ((r_ptr->flags2 & RF2_MULTIPLY) || (m_ptr->r_idx == MON_DAWN)) {
        int monnum_penarty = r_ptr->r_akills / 400;
        if (monnum_penarty > 8)
            monnum_penarty = 8;

        while (monnum_penarty--) {
            /* Divide by 4 */
            s64b_RSHIFT(new_exp, new_exp_frac, 2);
        }
    }

    /* Special penalty for rest_and_shoot exp scum */
    if ((m_ptr->dealt_damage > m_ptr->max_maxhp) && (m_ptr->hp >= 0)) {
        int over_damage = m_ptr->dealt_damage / m_ptr->max_maxhp;
        if (over_damage > 32)
            over_damage = 32;

        while (over_damage--) {
            /* 9/10 for once */
            s64b_mul(&new_exp, &new_exp_frac, 0, 9);
            s64b_div(&new_exp, &new_exp_frac, 0, 10);
        }
    }

    s64b_mul(&new_exp, &new_exp_frac, 0, r_ptr->mexp);
    gain_exp_64(target_ptr, new_exp, new_exp_frac);
}

/*!
 * @brief モンスターの時限ステータスを取得する
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @return m_idx モンスターの参照ID
 * @return mproc_type モンスターの時限ステータスID
 * @return 残りターン値
 */
int get_mproc_idx(floor_type *floor_ptr, MONSTER_IDX m_idx, int mproc_type)
{
    s16b *cur_mproc_list = floor_ptr->mproc_list[mproc_type];
    for (int i = floor_ptr->mproc_max[mproc_type] - 1; i >= 0; i--) {
        if (cur_mproc_list[i] == m_idx)
            return i;
    }

    return -1;
}

/*!
 * @brief モンスターの時限ステータスリストを追加する
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @return m_idx モンスターの参照ID
 * @return mproc_type 追加したいモンスターの時限ステータスID
 * @return なし
 */
void mproc_add(floor_type *floor_ptr, MONSTER_IDX m_idx, int mproc_type)
{
    if (floor_ptr->mproc_max[mproc_type] < current_world_ptr->max_m_idx) {
        floor_ptr->mproc_list[mproc_type][floor_ptr->mproc_max[mproc_type]++] = (s16b)m_idx;
    }
}

/*!
 * @brief モンスターの時限ステータスリストを初期化する / Initialize monster process
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @return なし
 */
void mproc_init(floor_type *floor_ptr)
{
    /* Reset "target_ptr->current_floor_ptr->mproc_max[]" */
    for (int i = 0; i < MAX_MTIMED; i++) {
        floor_ptr->mproc_max[i] = 0;
    }

    /* Process the monsters (backwards) */
    for (MONSTER_IDX i = floor_ptr->m_max - 1; i >= 1; i--) {
        monster_type *m_ptr;
        m_ptr = &floor_ptr->m_list[i];

        /* Ignore "dead" monsters */
        if (!monster_is_valid(m_ptr))
            continue;

        for (int cmi = 0; cmi < MAX_MTIMED; cmi++) {
            if (m_ptr->mtimed[cmi])
                mproc_add(floor_ptr, i, cmi);
        }
    }
}

static u32b csleep_noise;

/*!
 * @brief モンスターの各種状態値を時間経過により更新するサブルーチン
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param mtimed_idx 更新するモンスターの時限ステータスID
 * @return なし
 */
static void process_monsters_mtimed_aux(player_type *target_ptr, MONSTER_IDX m_idx, int mtimed_idx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];

    switch (mtimed_idx) {
    case MTIMED_CSLEEP: {
        monster_race *r_ptr = &r_info[m_ptr->r_idx];

        /* Assume does not wake up */
        bool test = FALSE;

        /* Hack -- Require proximity */
        if (m_ptr->cdis < AAF_LIMIT) {
            /* Handle "sensing radius" */
            if (m_ptr->cdis <= (is_pet(m_ptr) ? ((r_ptr->aaf > MAX_SIGHT) ? MAX_SIGHT : r_ptr->aaf) : r_ptr->aaf)) {
                /* We may wake up */
                test = TRUE;
            }

            /* Handle "sight" and "aggravation" */
            else if ((m_ptr->cdis <= MAX_SIGHT) && (player_has_los_bold(target_ptr, m_ptr->fy, m_ptr->fx))) {
                /* We may wake up */
                test = TRUE;
            }
        }

        if (!test)
            break;

        u32b notice = randint0(1024);

        /* Nightmare monsters are more alert */
        if (ironman_nightmare)
            notice /= 2;

        /* Hack -- See if monster "notices" player */
        if ((notice * notice * notice) > csleep_noise)
            break;

        /* Hack -- amount of "waking" */
        /* Wake up faster near the player */
        int d = (m_ptr->cdis < AAF_LIMIT / 2) ? (AAF_LIMIT / m_ptr->cdis) : 1;

        /* Hack -- amount of "waking" is affected by speed of player */
        d = (d * SPEED_TO_ENERGY(target_ptr->pspeed)) / 10;
        if (d < 0)
            d = 1;

        /* Monster wakes up "a little bit" */

        /* Still asleep */
        if (!set_monster_csleep(target_ptr, m_idx, monster_csleep_remaining(m_ptr) - d)) {
            /* Notice the "not waking up" */
            if (is_original_ap_and_seen(target_ptr, m_ptr)) {
                /* Hack -- Count the ignores */
                if (r_ptr->r_ignore < MAX_UCHAR)
                    r_ptr->r_ignore++;
            }

            break;
        }

        /* Notice the "waking up" */
        if (m_ptr->ml) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(target_ptr, m_name, m_ptr, 0);
            msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
        }

        if (is_original_ap_and_seen(target_ptr, m_ptr)) {
            /* Hack -- Count the wakings */
            if (r_ptr->r_wake < MAX_UCHAR)
                r_ptr->r_wake++;
        }

        break;
    }

    case MTIMED_FAST:
        /* Reduce by one, note if expires */
        if (set_monster_fast(target_ptr, m_idx, monster_fast_remaining(m_ptr) - 1)) {
            if (is_seen(target_ptr, m_ptr)) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(target_ptr, m_name, m_ptr, 0);
                msg_format(_("%^sはもう加速されていない。", "%^s is no longer fast."), m_name);
            }
        }

        break;

    case MTIMED_SLOW:
        /* Reduce by one, note if expires */
        if (set_monster_slow(target_ptr, m_idx, monster_slow_remaining(m_ptr) - 1)) {
            if (is_seen(target_ptr, m_ptr)) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(target_ptr, m_name, m_ptr, 0);
                msg_format(_("%^sはもう減速されていない。", "%^s is no longer slow."), m_name);
            }
        }

        break;

    case MTIMED_STUNNED: {
        int rlev = r_info[m_ptr->r_idx].level;

        /* Recover from stun */
        if (set_monster_stunned(target_ptr, m_idx, (randint0(10000) <= rlev * rlev) ? 0 : (monster_stunned_remaining(m_ptr) - 1))) {
            /* Message if visible */
            if (is_seen(target_ptr, m_ptr)) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(target_ptr, m_name, m_ptr, 0);
                msg_format(_("%^sは朦朧状態から立ち直った。", "%^s is no longer stunned."), m_name);
            }
        }

        break;
    }

    case MTIMED_CONFUSED: {
        /* Reduce the confusion */
        if (!set_monster_confused(target_ptr, m_idx, monster_confused_remaining(m_ptr) - randint1(r_info[m_ptr->r_idx].level / 20 + 1)))
            break;
        /* Message if visible */
        if (is_seen(target_ptr, m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(target_ptr, m_name, m_ptr, 0);
            msg_format(_("%^sは混乱から立ち直った。", "%^s is no longer confused."), m_name);
        }

        break;
    }

    case MTIMED_MONFEAR: {
        /* Reduce the fear */
        if (!set_monster_monfear(target_ptr, m_idx, monster_fear_remaining(m_ptr) - randint1(r_info[m_ptr->r_idx].level / 20 + 1)))
            break;

        /* Visual note */
        if (is_seen(target_ptr, m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
#ifdef JP
#else
            char m_poss[80];

            /* Acquire the monster possessive */
            monster_desc(target_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif
            monster_desc(target_ptr, m_name, m_ptr, 0);
#ifdef JP
            msg_format("%^sは勇気を取り戻した。", m_name);
#else
            msg_format("%^s recovers %s courage.", m_name, m_poss);
#endif
        }

        break;
    }

    case MTIMED_INVULNER: {
        /* Reduce by one, note if expires */
        if (!set_monster_invulner(target_ptr, m_idx, monster_invulner_remaining(m_ptr) - 1, TRUE))
            break;

        if (is_seen(target_ptr, m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(target_ptr, m_name, m_ptr, 0);
            msg_format(_("%^sはもう無敵でない。", "%^s is no longer invulnerable."), m_name);
        }

        break;
    }
    }
}

/*!
 * @brief 全モンスターの各種状態値を時間経過により更新するメインルーチン
 * @param mtimed_idx 更新するモンスターの時限ステータスID
 * @param target_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * Process the counters of monsters (once per 10 game turns)\n
 * These functions are to process monsters' counters same as player's.
 */
void process_monsters_mtimed(player_type *target_ptr, int mtimed_idx)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    s16b *cur_mproc_list = floor_ptr->mproc_list[mtimed_idx];

    /* Hack -- calculate the "player noise" */
    if (mtimed_idx == MTIMED_CSLEEP)
        csleep_noise = (1L << (30 - target_ptr->skill_stl));

    /* Process the monsters (backwards) */
    for (int i = floor_ptr->mproc_max[mtimed_idx] - 1; i >= 0; i--) {
        process_monsters_mtimed_aux(target_ptr, cur_mproc_list[i], mtimed_idx);
    }
}

/*!
 * @brief モンスターへの魔力消去処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 魔力消去を受けるモンスターの参照ID
 * @return なし
 */
void dispel_monster_status(player_type *target_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    GAME_TEXT m_name[MAX_NLEN];

    monster_desc(target_ptr, m_name, m_ptr, 0);
    if (set_monster_invulner(target_ptr, m_idx, 0, TRUE)) {
        if (m_ptr->ml)
            msg_format(_("%sはもう無敵ではない。", "%^s is no longer invulnerable."), m_name);
    }

    if (set_monster_fast(target_ptr, m_idx, 0)) {
        if (m_ptr->ml)
            msg_format(_("%sはもう加速されていない。", "%^s is no longer fast."), m_name);
    }

    if (set_monster_slow(target_ptr, m_idx, 0)) {
        if (m_ptr->ml)
            msg_format(_("%sはもう減速されていない。", "%^s is no longer slow."), m_name);
    }
}

/*!
 * @brief モンスターの経験値取得処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 経験値を得るモンスターの参照ID
 * @param s_idx 撃破されたモンスター種族の参照ID
 * @return なし
 */
void monster_gain_exp(player_type *target_ptr, MONSTER_IDX m_idx, MONRACE_IDX s_idx)
{
    monster_type *m_ptr;
    monster_race *r_ptr;
    monster_race *s_ptr;
    if (m_idx <= 0 || s_idx <= 0)
        return;

    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    m_ptr = &floor_ptr->m_list[m_idx];

    if (!monster_is_valid(m_ptr))
        return;

    r_ptr = &r_info[m_ptr->r_idx];
    s_ptr = &r_info[s_idx];

    if (target_ptr->phase_out)
        return;

    if (!r_ptr->next_exp)
        return;

    int new_exp = s_ptr->mexp * s_ptr->level / (r_ptr->level + 2);
    if (m_idx == target_ptr->riding)
        new_exp = (new_exp + 1) / 2;
    if (!floor_ptr->dun_level)
        new_exp /= 5;
    m_ptr->exp += new_exp;
    if (m_ptr->mflag2 & MFLAG2_CHAMELEON)
        return;

    if (m_ptr->exp < r_ptr->next_exp) {
        if (m_idx == target_ptr->riding)
            target_ptr->update |= PU_BONUS;
        return;
    }

    GAME_TEXT m_name[MAX_NLEN];
    int old_hp = m_ptr->hp;
    int old_maxhp = m_ptr->max_maxhp;
    int old_r_idx = m_ptr->r_idx;
    byte old_sub_align = m_ptr->sub_align;

    /* Hack -- Reduce the racial counter of previous monster */
    real_r_ptr(m_ptr)->cur_num--;

    monster_desc(target_ptr, m_name, m_ptr, 0);
    m_ptr->r_idx = r_ptr->next_r_idx;

    /* Count the monsters on the level */
    real_r_ptr(m_ptr)->cur_num++;

    m_ptr->ap_r_idx = m_ptr->r_idx;
    r_ptr = &r_info[m_ptr->r_idx];

    if (r_ptr->flags1 & RF1_FORCE_MAXHP) {
        m_ptr->max_maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
    } else {
        m_ptr->max_maxhp = damroll(r_ptr->hdice, r_ptr->hside);
    }
    if (ironman_nightmare) {
        HIT_POINT hp = m_ptr->max_maxhp * 2L;
        m_ptr->max_maxhp = MIN(30000, hp);
    }

    m_ptr->maxhp = m_ptr->max_maxhp;
    m_ptr->hp = old_hp * m_ptr->maxhp / old_maxhp;

    /* dealt damage is 0 at initial*/
    m_ptr->dealt_damage = 0;

    /* Extract the monster base speed */
    m_ptr->mspeed = get_mspeed(floor_ptr, r_ptr);

    /* Sub-alignment of a monster */
    if (!is_pet(m_ptr) && !(r_ptr->flags3 & (RF3_EVIL | RF3_GOOD)))
        m_ptr->sub_align = old_sub_align;
    else {
        m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
        if (r_ptr->flags3 & RF3_EVIL)
            m_ptr->sub_align |= SUB_ALIGN_EVIL;
        if (r_ptr->flags3 & RF3_GOOD)
            m_ptr->sub_align |= SUB_ALIGN_GOOD;
    }

    m_ptr->exp = 0;

    if (is_pet(m_ptr) || m_ptr->ml) {
        if (!ignore_unview || player_can_see_bold(target_ptr, m_ptr->fy, m_ptr->fx)) {
            if (target_ptr->image) {
                monster_race *hallu_race;

                do {
                    hallu_race = &r_info[randint1(max_r_idx - 1)];
                } while (!hallu_race->name || (hallu_race->flags1 & RF1_UNIQUE));
                msg_format(_("%sは%sに進化した。", "%^s evolved into %s."), m_name, r_name + hallu_race->name);
            } else {
                msg_format(_("%sは%sに進化した。", "%^s evolved into %s."), m_name, r_name + r_ptr->name);
            }
        }

        if (!target_ptr->image)
            r_info[old_r_idx].r_xtra1 |= MR1_EVOLUTION;

        /* Now you feel very close to this pet. */
        m_ptr->parent_m_idx = 0;
    }

    update_monster(target_ptr, m_idx, FALSE);
    lite_spot(target_ptr, m_ptr->fy, m_ptr->fx);

    if (m_idx == target_ptr->riding)
        target_ptr->update |= PU_BONUS;
}

/*!
 * @brief モンスターのHPをダメージに応じて減算する /
 * Decreases monsters hit points, handling monster death.
 * @param dam 与えたダメージ量
 * @param m_idx ダメージを与えたモンスターのID
 * @param fear ダメージによってモンスターが恐慌状態に陥ったならばTRUEを返す
 * @param note モンスターが倒された際の特別なメッセージ述語
 * @return なし
 * @details
 * <pre>
 * We return TRUE if the monster has been killed (and deleted).
 * We announce monster death (using an optional "death message"
 * if given, and a otherwise a generic killed/destroyed message).
 * Only "physical attacks" can induce the "You have slain" message.
 * Missile and Spell attacks will induce the "dies" message, or
 * various "specialized" messages.  Note that "You have destroyed"
 * and "is destroyed" are synonyms for "You have slain" and "dies".
 * Hack -- unseen monsters yield "You have killed it." message.
 * Added fear (DGK) and check whether to print fear messages -CWS
 * Made name, sex, and capitalization generic -BEN-
 * As always, the "ghost" processing is a total hack.
 * Hack -- we "delay" fear messages by passing around a "fear" flag.
 * Consider decreasing monster experience over time, say,
 * by using "(m_exp * m_lev * (m_lev)) / (p_lev * (m_lev + n_killed))"
 * instead of simply "(m_exp * m_lev) / (p_lev)", to make the first
 * monster worth more than subsequent monsters.  This would also need
 * to induce changes in the monster recall code.
 * </pre>
 */
bool mon_take_hit(player_type *target_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *fear, concptr note)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_type exp_mon;

    /* Innocent until proven otherwise */
    bool innocent = TRUE, thief = FALSE;
    int i;
    HIT_POINT expdam;

    (void)COPY(&exp_mon, m_ptr, monster_type);

    expdam = (m_ptr->hp > dam) ? dam : m_ptr->hp;

    get_exp_from_mon(target_ptr, expdam, &exp_mon);

    /* Genocided by chaos patron */
    if (!monster_is_valid(m_ptr))
        m_idx = 0;

    /* Redraw (later) if needed */
    if (target_ptr->health_who == m_idx)
        target_ptr->redraw |= (PR_HEALTH);
    if (target_ptr->riding == m_idx)
        target_ptr->redraw |= (PR_UHEALTH);

    (void)set_monster_csleep(target_ptr, m_idx, 0);

    /* Hack - Cancel any special player stealth magics. -LM- */
    if (target_ptr->special_defense & NINJA_S_STEALTH) {
        set_superstealth(target_ptr, FALSE);
    }

    /* Genocided by chaos patron */
    if (!m_idx)
        return TRUE;

    m_ptr->hp -= dam;
    m_ptr->dealt_damage += dam;

    if (m_ptr->dealt_damage > m_ptr->max_maxhp * 100)
        m_ptr->dealt_damage = m_ptr->max_maxhp * 100;

    if (current_world_ptr->wizard) {
        msg_format(_("合計%d/%dのダメージを与えた。", "You do %d (out of %d) damage."), m_ptr->dealt_damage, m_ptr->maxhp);
    }

    /* It is dead now */
    if (m_ptr->hp < 0) {
        GAME_TEXT m_name[MAX_NLEN];

        if (r_info[m_ptr->r_idx].flags7 & RF7_TANUKI) {
            /* You might have unmasked Tanuki first time */
            r_ptr = &r_info[m_ptr->r_idx];
            m_ptr->ap_r_idx = m_ptr->r_idx;
            if (r_ptr->r_sights < MAX_SHORT)
                r_ptr->r_sights++;
        }

        if (m_ptr->mflag2 & MFLAG2_CHAMELEON) {
            /* You might have unmasked Chameleon first time */
            r_ptr = real_r_ptr(m_ptr);
            if (r_ptr->r_sights < MAX_SHORT)
                r_ptr->r_sights++;
        }

        if (!(m_ptr->smart & SM_CLONED)) {
            /* When the player kills a Unique, it stays dead */
            if (r_ptr->flags1 & RF1_UNIQUE) {
                r_ptr->max_num = 0;

                /* Mega-Hack -- Banor & Lupart */
                if ((m_ptr->r_idx == MON_BANOR) || (m_ptr->r_idx == MON_LUPART)) {
                    r_info[MON_BANORLUPART].max_num = 0;
                    r_info[MON_BANORLUPART].r_pkills++;
                    r_info[MON_BANORLUPART].r_akills++;
                    if (r_info[MON_BANORLUPART].r_tkills < MAX_SHORT)
                        r_info[MON_BANORLUPART].r_tkills++;
                } else if (m_ptr->r_idx == MON_BANORLUPART) {
                    r_info[MON_BANOR].max_num = 0;
                    r_info[MON_BANOR].r_pkills++;
                    r_info[MON_BANOR].r_akills++;
                    if (r_info[MON_BANOR].r_tkills < MAX_SHORT)
                        r_info[MON_BANOR].r_tkills++;
                    r_info[MON_LUPART].max_num = 0;
                    r_info[MON_LUPART].r_pkills++;
                    r_info[MON_LUPART].r_akills++;
                    if (r_info[MON_LUPART].r_tkills < MAX_SHORT)
                        r_info[MON_LUPART].r_tkills++;
                }
            }

            /* When the player kills a Nazgul, it stays dead */
            else if (r_ptr->flags7 & RF7_NAZGUL)
                r_ptr->max_num--;
        }

        /* Count all monsters killed */
        if (r_ptr->r_akills < MAX_SHORT)
            r_ptr->r_akills++;

        /* Recall even invisible uniques or winners */
        if ((m_ptr->ml && !target_ptr->image) || (r_ptr->flags1 & RF1_UNIQUE)) {
            /* Count kills this life */
            if ((m_ptr->mflag2 & MFLAG2_KAGE) && (r_info[MON_KAGE].r_pkills < MAX_SHORT))
                r_info[MON_KAGE].r_pkills++;
            else if (r_ptr->r_pkills < MAX_SHORT)
                r_ptr->r_pkills++;

            /* Count kills in all lives */
            if ((m_ptr->mflag2 & MFLAG2_KAGE) && (r_info[MON_KAGE].r_tkills < MAX_SHORT))
                r_info[MON_KAGE].r_tkills++;
            else if (r_ptr->r_tkills < MAX_SHORT)
                r_ptr->r_tkills++;

            /* Hack -- Auto-recall */
            monster_race_track(target_ptr, m_ptr->ap_r_idx);
        }

        monster_desc(target_ptr, m_name, m_ptr, MD_TRUE_NAME);

        /* Don't kill Amberites */
        if ((r_ptr->flags3 & RF3_AMBERITE) && one_in_(2)) {
            int curses = 1 + randint1(3);
            bool stop_ty = FALSE;
            int count = 0;

            msg_format(_("%^sは恐ろしい血の呪いをあなたにかけた！", "%^s puts a terrible blood curse on you!"), m_name);
            curse_equipment(target_ptr, 100, 50);

            do {
                stop_ty = activate_ty_curse(target_ptr, stop_ty, &count);
            } while (--curses);
        }

        if (r_ptr->flags2 & RF2_CAN_SPEAK) {
            char line_got[1024];
            if (!get_rnd_line(_("mondeath_j.txt", "mondeath.txt"), m_ptr->r_idx, line_got)) {
                msg_format("%^s %s", m_name, line_got);
            }

#ifdef WORLD_SCORE
            if (m_ptr->r_idx == MON_SERPENT) {
                screen_dump = make_screen_dump(target_ptr, process_autopick_file_command);
            }
#endif
        }

        if (!(d_info[target_ptr->dungeon_idx].flags1 & DF1_BEGINNER)) {
            if (!target_ptr->current_floor_ptr->dun_level && !target_ptr->ambush_flag && !target_ptr->current_floor_ptr->inside_arena) {
                chg_virtue(target_ptr, V_VALOUR, -1);
            } else if (r_ptr->level > target_ptr->current_floor_ptr->dun_level) {
                if (randint1(10) <= (r_ptr->level - target_ptr->current_floor_ptr->dun_level))
                    chg_virtue(target_ptr, V_VALOUR, 1);
            }
            if (r_ptr->level > 60) {
                chg_virtue(target_ptr, V_VALOUR, 1);
            }
            if (r_ptr->level >= 2 * (target_ptr->lev + 1))
                chg_virtue(target_ptr, V_VALOUR, 2);
        }

        if (r_ptr->flags1 & RF1_UNIQUE) {
            if (r_ptr->flags3 & (RF3_EVIL | RF3_GOOD))
                chg_virtue(target_ptr, V_HARMONY, 2);

            if (r_ptr->flags3 & RF3_GOOD) {
                chg_virtue(target_ptr, V_UNLIFE, 2);
                chg_virtue(target_ptr, V_VITALITY, -2);
            }

            if (one_in_(3))
                chg_virtue(target_ptr, V_INDIVIDUALISM, -1);
        }

        if (m_ptr->r_idx == MON_BEGGAR || m_ptr->r_idx == MON_LEPER) {
            chg_virtue(target_ptr, V_COMPASSION, -1);
        }

        if ((r_ptr->flags3 & RF3_GOOD) && ((r_ptr->level) / 10 + (3 * target_ptr->current_floor_ptr->dun_level) >= randint1(100)))
            chg_virtue(target_ptr, V_UNLIFE, 1);

        if (r_ptr->d_char == 'A') {
            if (r_ptr->flags1 & RF1_UNIQUE)
                chg_virtue(target_ptr, V_FAITH, -2);
            else if ((r_ptr->level) / 10 + (3 * target_ptr->current_floor_ptr->dun_level) >= randint1(100)) {
                if (r_ptr->flags3 & RF3_GOOD)
                    chg_virtue(target_ptr, V_FAITH, -1);
                else
                    chg_virtue(target_ptr, V_FAITH, 1);
            }
        } else if (r_ptr->flags3 & RF3_DEMON) {
            if (r_ptr->flags1 & RF1_UNIQUE)
                chg_virtue(target_ptr, V_FAITH, 2);
            else if ((r_ptr->level) / 10 + (3 * target_ptr->current_floor_ptr->dun_level) >= randint1(100))
                chg_virtue(target_ptr, V_FAITH, 1);
        }

        if ((r_ptr->flags3 & RF3_UNDEAD) && (r_ptr->flags1 & RF1_UNIQUE))
            chg_virtue(target_ptr, V_VITALITY, 2);

        if (r_ptr->r_deaths) {
            if (r_ptr->flags1 & RF1_UNIQUE) {
                chg_virtue(target_ptr, V_HONOUR, 10);
            } else if ((r_ptr->level) / 10 + (2 * target_ptr->current_floor_ptr->dun_level) >= randint1(100)) {
                chg_virtue(target_ptr, V_HONOUR, 1);
            }
        }
        if ((r_ptr->flags2 & RF2_MULTIPLY) && (r_ptr->r_akills > 1000) && one_in_(10)) {
            chg_virtue(target_ptr, V_VALOUR, -1);
        }

        for (i = 0; i < 4; i++) {
            if (r_ptr->blow[i].d_dice != 0)
                innocent = FALSE; /* Murderer! */

            if ((r_ptr->blow[i].effect == RBE_EAT_ITEM) || (r_ptr->blow[i].effect == RBE_EAT_GOLD))

                thief = TRUE; /* Thief! */
        }

        /* The new law says it is illegal to live in the dungeon */
        if (r_ptr->level != 0)
            innocent = FALSE;

        if (thief) {
            if (r_ptr->flags1 & RF1_UNIQUE)
                chg_virtue(target_ptr, V_JUSTICE, 3);
            else if (1 + ((r_ptr->level) / 10 + (2 * target_ptr->current_floor_ptr->dun_level)) >= randint1(100))
                chg_virtue(target_ptr, V_JUSTICE, 1);
        } else if (innocent) {
            chg_virtue(target_ptr, V_JUSTICE, -1);
        }

        if ((r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags3 & RF3_EVIL) && !(r_ptr->flags4 & ~(RF4_NOMAGIC_MASK))
            && !(r_ptr->a_ability_flags1 & ~(RF5_NOMAGIC_MASK)) && !(r_ptr->a_ability_flags2 & ~(RF6_NOMAGIC_MASK))) {
            if (one_in_(4))
                chg_virtue(target_ptr, V_NATURE, -1);
        }

        if ((r_ptr->flags1 & RF1_UNIQUE) && record_destroy_uniq) {
            char note_buf[160];
            sprintf(note_buf, "%s%s", r_name + r_ptr->name, (m_ptr->smart & SM_CLONED) ? _("(クローン)", "(Clone)") : "");
            exe_write_diary(target_ptr, DIARY_UNIQUE, 0, note_buf);
        }

        /* Make a sound */
        sound(SOUND_KILL);

        /* Death by Missile/Spell attack */
        if (note) {
            msg_format("%^s%s", m_name, note);
        }

        /* Death by physical attack -- invisible monster */
        else if (!m_ptr->ml) {
#ifdef JP
            if (is_echizen(target_ptr))
                msg_format("せっかくだから%sを殺した。", m_name);
            else
                msg_format("%sを殺した。", m_name);
#else
            msg_format("You have killed %s.", m_name);
#endif

        }

        /* Death by Physical attack -- non-living monster */
        else if (!monster_living(m_ptr->r_idx)) {
            bool explode = FALSE;

            for (i = 0; i < 4; i++) {
                if (r_ptr->blow[i].method == RBM_EXPLODE)
                    explode = TRUE;
            }

            /* Special note at death */
            if (explode)
                msg_format(_("%sは爆発して粉々になった。", "%^s explodes into tiny shreds."), m_name);
            else {
#ifdef JP
                if (is_echizen(target_ptr))
                    msg_format("せっかくだから%sを倒した。", m_name);
                else
                    msg_format("%sを倒した。", m_name);
#else
                msg_format("You have destroyed %s.", m_name);
#endif
            }
        }

        /* Death by Physical attack -- living monster */
        else {
#ifdef JP
            if (is_echizen(target_ptr))
                msg_format("せっかくだから%sを葬り去った。", m_name);
            else
                msg_format("%sを葬り去った。", m_name);
#else
            msg_format("You have slain %s.", m_name);
#endif
        }
        if ((r_ptr->flags1 & RF1_UNIQUE) && !(m_ptr->smart & SM_CLONED) && !vanilla_town) {
            for (i = 0; i < MAX_BOUNTY; i++) {
                if ((current_world_ptr->bounty_r_idx[i] == m_ptr->r_idx) && !(m_ptr->mflag2 & MFLAG2_CHAMELEON)) {
                    msg_format(_("%sの首には賞金がかかっている。", "There is a price on %s's head."), m_name);
                    break;
                }
            }
        }

        /* Generate treasure */
        monster_death(target_ptr, m_idx, TRUE);

        /* Mega hack : replace IKETA to BIKETAL */
        if ((m_ptr->r_idx == MON_IKETA) && !(target_ptr->current_floor_ptr->inside_arena || target_ptr->phase_out)) {
            POSITION dummy_y = m_ptr->fy;
            POSITION dummy_x = m_ptr->fx;
            BIT_FLAGS mode = 0L;
            if (is_pet(m_ptr))
                mode |= PM_FORCE_PET;
            delete_monster_idx(target_ptr, m_idx);
            if (summon_named_creature(target_ptr, 0, dummy_y, dummy_x, MON_BIKETAL, mode)) {
                msg_print(_("「ハァッハッハッハ！！私がバイケタルだ！！」", "Uwa-hahaha!  *I* am Biketal!"));
            }
        } else {
            delete_monster_idx(target_ptr, m_idx);
        }

        get_exp_from_mon(target_ptr, (long)exp_mon.max_maxhp * 2, &exp_mon);

        /* Not afraid */
        (*fear) = FALSE;

        /* Monster is dead */
        return TRUE;
    }

    /* Mega-Hack -- Pain cancels fear */
    if (monster_fear_remaining(m_ptr) && (dam > 0)) {
        /* Cure fear */
        if (set_monster_monfear(target_ptr, m_idx, monster_fear_remaining(m_ptr) - randint1(dam))) {
            /* No more fear */
            (*fear) = FALSE;
        }
    }

    /* Sometimes a monster gets scared by damage */
    if (!monster_fear_remaining(m_ptr) && !(r_ptr->flags3 & (RF3_NO_FEAR))) {
        /* Percentage of fully healthy */
        int percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

        /*
         * Run (sometimes) if at 10% or less of max hit points,
         * or (usually) when hit for half its current hit points
         */
        if ((randint1(10) >= percentage) || ((dam >= m_ptr->hp) && (randint0(100) < 80))) {
            /* Hack -- note fear */
            (*fear) = TRUE;

            /* Hack -- Add some timed fear */
            (void)set_monster_monfear(target_ptr, m_idx, (randint1(10) + (((dam >= m_ptr->hp) && (percentage > 7)) ? 20 : ((11 - percentage) * 5))));
        }
    }

    /* Not dead yet */
    return FALSE;
}

bool monster_is_valid(monster_type *m_ptr) { return (m_ptr->r_idx != 0); }

TIME_EFFECT monster_csleep_remaining(monster_type *m_ptr) { return m_ptr->mtimed[MTIMED_CSLEEP]; }

TIME_EFFECT monster_fast_remaining(monster_type *m_ptr) { return m_ptr->mtimed[MTIMED_FAST]; }

TIME_EFFECT monster_slow_remaining(monster_type *m_ptr) { return m_ptr->mtimed[MTIMED_SLOW]; }

TIME_EFFECT monster_stunned_remaining(monster_type *m_ptr) { return m_ptr->mtimed[MTIMED_STUNNED]; }

TIME_EFFECT monster_confused_remaining(monster_type *m_ptr) { return m_ptr->mtimed[MTIMED_CONFUSED]; }

TIME_EFFECT monster_fear_remaining(monster_type *m_ptr) { return m_ptr->mtimed[MTIMED_MONFEAR]; }

TIME_EFFECT monster_invulner_remaining(monster_type *m_ptr) { return m_ptr->mtimed[MTIMED_INVULNER]; }
