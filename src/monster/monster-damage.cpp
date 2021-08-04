/*
 * @brief モンスターがダメージを受けた時の処理と経験値の加算処理
 * @date 2021/08/04
 * @author Hourier
 */

#include "monster/monster-damage.h"
#include "core/player-redraw-types.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "io/files-util.h"
#include "io/report.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object-enchant/object-curse.h"
#include "player-info/avatar.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-random.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief モンスターに与えたダメージを元に経験値を加算する /
 * Calculate experience point to be get
 * @param dam 与えたダメージ量
 * @param m_ptr ダメージを与えたモンスターの構造体参照ポインタ
 * @details
 * <pre>
 * Even the 64 bit operation is not big enough to avoid overflaw
 * unless we carefully choose orders of ENERGY_MULTIPLICATION and ENERGY_DIVISION.
 * Get the coefficient first, and multiply (potentially huge) base
 * experience point of a monster later.
 * </pre>
 */
static void get_exp_from_mon(player_type *target_ptr, HIT_POINT dam, monster_type *m_ptr)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (!monster_is_valid(m_ptr) || is_pet(m_ptr) || target_ptr->phase_out) {
        return;
    }

    /*
     * - Ratio of monster's level to player's level effects
     * - Varying speed effects
     * - Get a fraction in proportion of damage point
     */
    auto new_exp = r_ptr->level * SPEED_TO_ENERGY(m_ptr->mspeed) * dam;
    auto new_exp_frac = 0U;
    auto div_h = 0;
    auto div_l = (uint)((target_ptr->max_plv + 2) * SPEED_TO_ENERGY(r_ptr->speed));

    /* Use (average maxhp * 2) as a denominator */
    auto compensation = any_bits(r_ptr->flags1, RF1_FORCE_MAXHP) ? r_ptr->hside * 2 : r_ptr->hside + 1;
    s64b_mul(&div_h, &div_l, 0, r_ptr->hdice * (ironman_nightmare ? 2 : 1) * compensation);

    /* Special penalty in the wilderness */
    if (!target_ptr->current_floor_ptr->dun_level && (none_bits(r_ptr->flags8, RF8_WILD_ONLY) || none_bits(r_ptr->flags1, RF1_UNIQUE))) {
        s64b_mul(&div_h, &div_l, 0, 5);
    }

    /* Do ENERGY_DIVISION first to prevent overflaw */
    s64b_div(&new_exp, &new_exp_frac, div_h, div_l);

    /* Special penalty for mutiply-monster */
    if (any_bits(r_ptr->flags2, RF2_MULTIPLY) || (m_ptr->r_idx == MON_DAWN)) {
        int monnum_penarty = r_ptr->r_akills / 400;
        if (monnum_penarty > 8) {
            monnum_penarty = 8;
        }

        while (monnum_penarty--) {
            /* Divide by 4 */
            s64b_rshift(&new_exp, &new_exp_frac, 2);
        }
    }

    /* Special penalty for rest_and_shoot exp scum */
    if ((m_ptr->dealt_damage > m_ptr->max_maxhp) && (m_ptr->hp >= 0)) {
        int over_damage = m_ptr->dealt_damage / m_ptr->max_maxhp;
        if (over_damage > 32) {
            over_damage = 32;
        }

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
 * @brief モンスターのHPをダメージに応じて減算する /
 * Decreases monsters hit points, handling monster death.
 * @param dam 与えたダメージ量
 * @param m_idx ダメージを与えたモンスターのID
 * @param fear ダメージによってモンスターが恐慌状態に陥ったならばTRUEを返す
 * @param note モンスターが倒された際の特別なメッセージ述語
 * @return モンスターが生きていればfalse、死んだらtrue
 */
bool mon_take_hit(player_type *target_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *fear, concptr note)
{
    auto *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];

    /* Innocent until proven otherwise */
    auto innocent = true;
    auto thief = false;
    int i;
    HIT_POINT expdam;

    monster_type exp_mon;
    (void)COPY(&exp_mon, m_ptr, monster_type);

    expdam = (m_ptr->hp > dam) ? dam : m_ptr->hp;

    get_exp_from_mon(target_ptr, expdam, &exp_mon);

    /* Genocided by chaos patron */
    if (!monster_is_valid(m_ptr)) {
        m_idx = 0;
    }

    /* Redraw (later) if needed */
    if (target_ptr->health_who == m_idx) {
        target_ptr->redraw |= PR_HEALTH;
    }

    if (target_ptr->riding == m_idx) {
        target_ptr->redraw |= PR_UHEALTH;
    }

    (void)set_monster_csleep(target_ptr, m_idx, 0);

    /* Hack - Cancel any special player stealth magics. -LM- */
    if (target_ptr->special_defense & NINJA_S_STEALTH) {
        set_superstealth(target_ptr, false);
    }

    /* Genocided by chaos patron */
    if (m_idx == 0) {
        return true;
    }

    m_ptr->hp -= dam;
    m_ptr->dealt_damage += dam;

    if (m_ptr->dealt_damage > m_ptr->max_maxhp * 100) {
        m_ptr->dealt_damage = m_ptr->max_maxhp * 100;
    }

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

        if (m_ptr->mflag2.has(MFLAG2::CHAMELEON)) {
            /* You might have unmasked Chameleon first time */
            r_ptr = real_r_ptr(m_ptr);
            if (r_ptr->r_sights < MAX_SHORT)
                r_ptr->r_sights++;
        }

        if (m_ptr->mflag2.has_not(MFLAG2::CLONED)) {
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
            if (m_ptr->mflag2.has(MFLAG2::KAGE) && (r_info[MON_KAGE].r_pkills < MAX_SHORT))
                r_info[MON_KAGE].r_pkills++;
            else if (r_ptr->r_pkills < MAX_SHORT)
                r_ptr->r_pkills++;

            /* Count kills in all lives */
            if (m_ptr->mflag2.has(MFLAG2::KAGE) && (r_info[MON_KAGE].r_tkills < MAX_SHORT))
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
            bool stop_ty = false;
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
                screen_dump = make_screen_dump(target_ptr);
            }
#endif
        }

        if (d_info[target_ptr->dungeon_idx].flags.has_not(DF::BEGINNER)) {
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
                innocent = false; /* Murderer! */

            if ((r_ptr->blow[i].effect == RBE_EAT_ITEM) || (r_ptr->blow[i].effect == RBE_EAT_GOLD))

                thief = true; /* Thief! */
        }

        /* The new law says it is illegal to live in the dungeon */
        if (r_ptr->level != 0)
            innocent = false;

        if (thief) {
            if (r_ptr->flags1 & RF1_UNIQUE)
                chg_virtue(target_ptr, V_JUSTICE, 3);
            else if (1 + ((r_ptr->level) / 10 + (2 * target_ptr->current_floor_ptr->dun_level)) >= randint1(100))
                chg_virtue(target_ptr, V_JUSTICE, 1);
        } else if (innocent) {
            chg_virtue(target_ptr, V_JUSTICE, -1);
        }

        auto magic_ability_flags = r_ptr->ability_flags;
        magic_ability_flags.reset(RF_ABILITY_NOMAGIC_MASK);
        if ((r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags3 & RF3_EVIL) && magic_ability_flags.none()) {
            if (one_in_(4))
                chg_virtue(target_ptr, V_NATURE, -1);
        }

        if ((r_ptr->flags1 & RF1_UNIQUE) && record_destroy_uniq) {
            char note_buf[160];
            sprintf(note_buf, "%s%s", r_ptr->name.c_str(), m_ptr->mflag2.has(MFLAG2::CLONED) ? _("(クローン)", "(Clone)") : "");
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
            bool explode = false;

            for (i = 0; i < 4; i++) {
                if (r_ptr->blow[i].method == RBM_EXPLODE)
                    explode = true;
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
        if ((r_ptr->flags1 & RF1_UNIQUE) && m_ptr->mflag2.has_not(MFLAG2::CLONED) && !vanilla_town) {
            for (i = 0; i < MAX_BOUNTY; i++) {
                if ((current_world_ptr->bounty_r_idx[i] == m_ptr->r_idx) && m_ptr->mflag2.has_not(MFLAG2::CHAMELEON)) {
                    msg_format(_("%sの首には賞金がかかっている。", "There is a price on %s's head."), m_name);
                    break;
                }
            }
        }

        /* Generate treasure */
        monster_death(target_ptr, m_idx, true);

        // @todo デッドアタック扱いにしてここから削除したい.
        bool is_special_summon = m_ptr->r_idx == MON_IKETA;
        is_special_summon |= m_ptr->r_idx == MON_DOPPIO;
        if (is_special_summon && !(target_ptr->current_floor_ptr->inside_arena || target_ptr->phase_out)) {
            POSITION dummy_y = m_ptr->fy;
            POSITION dummy_x = m_ptr->fx;
            BIT_FLAGS mode = 0L;
            if (is_pet(m_ptr))
                mode |= PM_FORCE_PET;

            MONRACE_IDX new_unique_idx;
            concptr mes;
            switch (m_ptr->r_idx) {
            case MON_IKETA:
                new_unique_idx = MON_BIKETAL;
                mes = _("「ハァッハッハッハ！！私がバイケタルだ！！」", "Uwa-hahaha!  *I* am Biketal!");
                break;
            case MON_DOPPIO:
                new_unique_idx = MON_DIAVOLO;
                mes = _("「これは『試練』だ　過去に打ち勝てという『試練』とオレは受けとった」",
                    "This is a 'trial'. I took it as a 'trial' to overcome in the past.");
                break;
            default: // バグでなければ入らない.
                new_unique_idx = 0;
                mes = "";
                break;
            }

            delete_monster_idx(target_ptr, m_idx);
            if (summon_named_creature(target_ptr, 0, dummy_y, dummy_x, new_unique_idx, mode))
                msg_print(mes);
        } else {
            delete_monster_idx(target_ptr, m_idx);
        }

        get_exp_from_mon(target_ptr, (long)exp_mon.max_maxhp * 2, &exp_mon);

        /* Not afraid */
        (*fear) = false;

        /* Monster is dead */
        return true;
    }
    /* Mega-Hack -- Pain cancels fear */
    if (monster_fear_remaining(m_ptr) && (dam > 0)) {
        /* Cure fear */
        if (set_monster_monfear(target_ptr, m_idx, monster_fear_remaining(m_ptr) - randint1(dam))) {
            /* No more fear */
            *fear = false;
        }
    }

    /* Sometimes a monster gets scared by damage */
    if (!monster_fear_remaining(m_ptr) && none_bits(r_ptr->flags3, RF3_NO_FEAR)) {
        /* Percentage of fully healthy */
        int percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

        /*
         * Run (sometimes) if at 10% or less of max hit points,
         * or (usually) when hit for half its current hit points
         */
        if ((randint1(10) >= percentage) || ((dam >= m_ptr->hp) && (randint0(100) < 80))) {
            /* Hack -- note fear */
            *fear = true;

            /* Hack -- Add some timed fear */
            (void)set_monster_monfear(target_ptr, m_idx, (randint1(10) + (((dam >= m_ptr->hp) && (percentage > 7)) ? 20 : ((11 - percentage) * 5))));
        }
    }

    /* Not dead yet */
    return false;
}
