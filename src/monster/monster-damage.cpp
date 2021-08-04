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
#include <algorithm>

/*
 * @brief コンストラクタ
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx ダメージを与えたモンスターのID
 * @param dam 与えたダメージ量
 * @param fear ダメージによってモンスターが恐慌状態に陥ったならばtrue
 * @param note モンスターが倒された際の特別なメッセージ述語
 */
MonsterDamageProcessor::MonsterDamageProcessor(player_type *target_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *fear)
    : target_ptr(target_ptr)
    , m_idx(m_idx)
    , dam(dam)
    , fear(fear)
{
}

/*!
 * @brief モンスターのHPをダメージに応じて減算する /
 * @return モンスターが生きていればfalse、死んだらtrue
 */
bool MonsterDamageProcessor::mon_take_hit(concptr note)
{
    auto *m_ptr = &this->target_ptr->current_floor_ptr->m_list[this->m_idx];
    auto innocent = true;
    auto thief = false;

    monster_type exp_mon;
    (void)COPY(&exp_mon, m_ptr, monster_type);

    auto exp_dam = (m_ptr->hp > this->dam) ? this->dam : m_ptr->hp;

    this->get_exp_from_mon(&exp_mon, exp_dam);
    if (this->genocide_chaos_patron(m_ptr)) {
        return true;
    }

    m_ptr->hp -= this->dam;
    m_ptr->dealt_damage += this->dam;
    if (m_ptr->dealt_damage > m_ptr->max_maxhp * 100) {
        m_ptr->dealt_damage = m_ptr->max_maxhp * 100;
    }

    if (current_world_ptr->wizard) {
        msg_format(_("合計%d/%dのダメージを与えた。", "You do %d (out of %d) damage."), m_ptr->dealt_damage, m_ptr->maxhp);
    }

    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (m_ptr->hp < 0) {
        this->death_special_flag_monster(m_ptr);

        /* Count all monsters killed */
        if (r_ptr->r_akills < MAX_SHORT) {
            r_ptr->r_akills++;
        }

        /* Recall even invisible uniques or winners */
        if ((m_ptr->ml && !this->target_ptr->image) || (r_ptr->flags1 & RF1_UNIQUE)) {
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
            monster_race_track(this->target_ptr, m_ptr->ap_r_idx);
        }

        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(this->target_ptr, m_name, m_ptr, MD_TRUE_NAME);

        /* Don't kill Amberites */
        if ((r_ptr->flags3 & RF3_AMBERITE) && one_in_(2)) {
            int curses = 1 + randint1(3);
            bool stop_ty = false;
            int count = 0;

            msg_format(_("%^sは恐ろしい血の呪いをあなたにかけた！", "%^s puts a terrible blood curse on you!"), m_name);
            curse_equipment(this->target_ptr, 100, 50);

            do {
                stop_ty = activate_ty_curse(this->target_ptr, stop_ty, &count);
            } while (--curses);
        }

        if (r_ptr->flags2 & RF2_CAN_SPEAK) {
            char line_got[1024];
            if (!get_rnd_line(_("mondeath_j.txt", "mondeath.txt"), m_ptr->r_idx, line_got)) {
                msg_format("%^s %s", m_name, line_got);
            }

#ifdef WORLD_SCORE
            if (m_ptr->r_idx == MON_SERPENT) {
                screen_dump = make_screen_dump(this->target_ptr);
            }
#endif
        }

        if (d_info[this->target_ptr->dungeon_idx].flags.has_not(DF::BEGINNER)) {
            if (!this->target_ptr->current_floor_ptr->dun_level && !this->target_ptr->ambush_flag && !this->target_ptr->current_floor_ptr->inside_arena) {
                chg_virtue(this->target_ptr, V_VALOUR, -1);
            } else if (r_ptr->level > this->target_ptr->current_floor_ptr->dun_level) {
                if (randint1(10) <= (r_ptr->level - this->target_ptr->current_floor_ptr->dun_level))
                    chg_virtue(this->target_ptr, V_VALOUR, 1);
            }
            if (r_ptr->level > 60) {
                chg_virtue(this->target_ptr, V_VALOUR, 1);
            }
            if (r_ptr->level >= 2 * (this->target_ptr->lev + 1))
                chg_virtue(this->target_ptr, V_VALOUR, 2);
        }

        if (r_ptr->flags1 & RF1_UNIQUE) {
            if (r_ptr->flags3 & (RF3_EVIL | RF3_GOOD))
                chg_virtue(this->target_ptr, V_HARMONY, 2);

            if (r_ptr->flags3 & RF3_GOOD) {
                chg_virtue(this->target_ptr, V_UNLIFE, 2);
                chg_virtue(this->target_ptr, V_VITALITY, -2);
            }

            if (one_in_(3))
                chg_virtue(this->target_ptr, V_INDIVIDUALISM, -1);
        }

        if (m_ptr->r_idx == MON_BEGGAR || m_ptr->r_idx == MON_LEPER) {
            chg_virtue(this->target_ptr, V_COMPASSION, -1);
        }

        if ((r_ptr->flags3 & RF3_GOOD) && ((r_ptr->level) / 10 + (3 * this->target_ptr->current_floor_ptr->dun_level) >= randint1(100)))
            chg_virtue(this->target_ptr, V_UNLIFE, 1);

        if (r_ptr->d_char == 'A') {
            if (r_ptr->flags1 & RF1_UNIQUE)
                chg_virtue(this->target_ptr, V_FAITH, -2);
            else if ((r_ptr->level) / 10 + (3 * this->target_ptr->current_floor_ptr->dun_level) >= randint1(100)) {
                if (r_ptr->flags3 & RF3_GOOD)
                    chg_virtue(this->target_ptr, V_FAITH, -1);
                else
                    chg_virtue(this->target_ptr, V_FAITH, 1);
            }
        } else if (r_ptr->flags3 & RF3_DEMON) {
            if (r_ptr->flags1 & RF1_UNIQUE)
                chg_virtue(this->target_ptr, V_FAITH, 2);
            else if ((r_ptr->level) / 10 + (3 * this->target_ptr->current_floor_ptr->dun_level) >= randint1(100))
                chg_virtue(this->target_ptr, V_FAITH, 1);
        }

        if ((r_ptr->flags3 & RF3_UNDEAD) && (r_ptr->flags1 & RF1_UNIQUE))
            chg_virtue(this->target_ptr, V_VITALITY, 2);

        if (r_ptr->r_deaths) {
            if (r_ptr->flags1 & RF1_UNIQUE) {
                chg_virtue(this->target_ptr, V_HONOUR, 10);
            } else if ((r_ptr->level) / 10 + (2 * this->target_ptr->current_floor_ptr->dun_level) >= randint1(100)) {
                chg_virtue(this->target_ptr, V_HONOUR, 1);
            }
        }
        if ((r_ptr->flags2 & RF2_MULTIPLY) && (r_ptr->r_akills > 1000) && one_in_(10)) {
            chg_virtue(this->target_ptr, V_VALOUR, -1);
        }

        for (auto i = 0; i < 4; i++) {
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
                chg_virtue(this->target_ptr, V_JUSTICE, 3);
            else if (1 + ((r_ptr->level) / 10 + (2 * this->target_ptr->current_floor_ptr->dun_level)) >= randint1(100))
                chg_virtue(this->target_ptr, V_JUSTICE, 1);
        } else if (innocent) {
            chg_virtue(this->target_ptr, V_JUSTICE, -1);
        }

        auto magic_ability_flags = r_ptr->ability_flags;
        magic_ability_flags.reset(RF_ABILITY_NOMAGIC_MASK);
        if ((r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags3 & RF3_EVIL) && magic_ability_flags.none()) {
            if (one_in_(4))
                chg_virtue(this->target_ptr, V_NATURE, -1);
        }

        if ((r_ptr->flags1 & RF1_UNIQUE) && record_destroy_uniq) {
            char note_buf[160];
            sprintf(note_buf, "%s%s", r_ptr->name.c_str(), m_ptr->mflag2.has(MFLAG2::CLONED) ? _("(クローン)", "(Clone)") : "");
            exe_write_diary(this->target_ptr, DIARY_UNIQUE, 0, note_buf);
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
            if (is_echizen(this->target_ptr))
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

            for (auto i = 0; i < 4; i++) {
                if (r_ptr->blow[i].method == RBM_EXPLODE)
                    explode = true;
            }

            /* Special note at death */
            if (explode)
                msg_format(_("%sは爆発して粉々になった。", "%^s explodes into tiny shreds."), m_name);
            else {
#ifdef JP
                if (is_echizen(this->target_ptr))
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
            if (is_echizen(this->target_ptr))
                msg_format("せっかくだから%sを葬り去った。", m_name);
            else
                msg_format("%sを葬り去った。", m_name);
#else
            msg_format("You have slain %s.", m_name);
#endif
        }
        if ((r_ptr->flags1 & RF1_UNIQUE) && m_ptr->mflag2.has_not(MFLAG2::CLONED) && !vanilla_town) {
            for (auto i = 0; i < MAX_BOUNTY; i++) {
                if ((current_world_ptr->bounty_r_idx[i] == m_ptr->r_idx) && m_ptr->mflag2.has_not(MFLAG2::CHAMELEON)) {
                    msg_format(_("%sの首には賞金がかかっている。", "There is a price on %s's head."), m_name);
                    break;
                }
            }
        }

        monster_death(this->target_ptr, this->m_idx, true);
        this->summon_special_unique(m_ptr);
        this->get_exp_from_mon(&exp_mon, exp_mon.max_maxhp * 2);
        *this->fear = false;
        return true;
    }

    if (monster_fear_remaining(m_ptr) && (this->dam > 0)) {
        auto fear_remining = monster_fear_remaining(m_ptr) - randint1(this->dam);
        if (set_monster_monfear(this->target_ptr, this->m_idx, fear_remining)) {
            *this->fear = false;
        }
    }

    // 恐怖の更なる加算.
    if (!monster_fear_remaining(m_ptr) && none_bits(r_ptr->flags3, RF3_NO_FEAR)) {
        int percentage = (100L * m_ptr->hp) / m_ptr->maxhp;
        if ((randint1(10) >= percentage) || ((this->dam >= m_ptr->hp) && (randint0(100) < 80))) {
            *this->fear = true;
            (void)set_monster_monfear(
                this->target_ptr, this->m_idx, (randint1(10) + (((this->dam >= m_ptr->hp) && (percentage > 7)) ? 20 : ((11 - percentage) * 5))));
        }
    }

    return false;
}

bool MonsterDamageProcessor::genocide_chaos_patron(monster_type *m_ptr)
{
    if (!monster_is_valid(m_ptr)) {
        this->m_idx = 0;
    }

    this->set_redraw();
    (void)set_monster_csleep(this->target_ptr, this->m_idx, 0);
    if (this->target_ptr->special_defense & NINJA_S_STEALTH) {
        set_superstealth(this->target_ptr, false);
    }

    return this->m_idx == 0;
}

/*
 * @brief たぬき、カメレオン、ナズグル、ユニークの死亡時処理
 * @param m_ptr ダメージを与えたモンスターの構造体参照ポインタ
 */
void MonsterDamageProcessor::death_special_flag_monster(monster_type *m_ptr)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (r_info[m_ptr->r_idx].flags7 & RF7_TANUKI) {
        r_ptr = &r_info[m_ptr->r_idx];
        m_ptr->ap_r_idx = m_ptr->r_idx;
        if (r_ptr->r_sights < MAX_SHORT) {
            r_ptr->r_sights++;
        }
    }

    if (m_ptr->mflag2.has(MFLAG2::CHAMELEON)) {
        r_ptr = real_r_ptr(m_ptr);
        if (r_ptr->r_sights < MAX_SHORT)
            r_ptr->r_sights++;
    }

    if (m_ptr->mflag2.has(MFLAG2::CLONED)) {
        return;
    }

    if (any_bits(r_ptr->flags7, RF7_NAZGUL)) {
        r_ptr->max_num--;
        return;
    }

    if (none_bits(r_ptr->flags1, RF1_UNIQUE)) {
        return;
    }

    r_ptr->max_num = 0;
    std::vector<std::tuple<monster_race_type, monster_race_type, monster_race_type>> uniques;
    uniques.push_back(std::make_tuple<monster_race_type, monster_race_type, monster_race_type>(MON_BANORLUPART, MON_BANOR, MON_LUPART));
    this->split_unite_uniques(m_ptr, uniques);
}

/*
 * @brief 分裂/合体を行う特殊ユニークの分裂/合体処理
 * @param m_ptr ダメージを与えたモンスターの構造体参照ポインタ
 * @uniques 分裂/合体を行う特殊ユニークのリスト
 */
void MonsterDamageProcessor::split_unite_uniques(
    monster_type *m_ptr, std::vector<std::tuple<monster_race_type, monster_race_type, monster_race_type>> uniques)
{
    for(const auto &unique : uniques) {
        auto united = (monster_race_type)0;
        auto split1 = (monster_race_type)0;
        auto split2 = (monster_race_type)0;
        std::tie(united, split1, split2) = unique;
        if ((m_ptr->r_idx == split1) || (m_ptr->r_idx == split2)) {
            r_info[united].max_num = 0;
            r_info[united].r_pkills++;
            r_info[united].r_akills++;
            if (r_info[united].r_tkills < MAX_SHORT) {
                r_info[united].r_tkills++;
            }

            continue;
        }

        if (m_ptr->r_idx != united) {
            continue;
        }

        r_info[split1].max_num = 0;
        r_info[split1].r_pkills++;
        r_info[split1].r_akills++;
        if (r_info[split1].r_tkills < MAX_SHORT) {
            r_info[split1].r_tkills++;
        }

        r_info[split2].max_num = 0;
        r_info[split2].r_pkills++;
        r_info[split2].r_akills++;
        if (r_info[split2].r_tkills < MAX_SHORT) {
            r_info[split2].r_tkills++;
        }
    }
}

/*!
 * @brief モンスターに与えたダメージを元に経験値を加算する /
 * Calculate experience point to be get
 * @param m_ptr ダメージを与えたモンスターの構造体参照ポインタ
 * @details
 * <pre>
 * Even the 64 bit operation is not big enough to avoid overflaw
 * unless we carefully choose orders of ENERGY_MULTIPLICATION and ENERGY_DIVISION.
 * Get the coefficient first, and multiply (potentially huge) base
 * experience point of a monster later.
 * </pre>
 */
void MonsterDamageProcessor::get_exp_from_mon(monster_type *m_ptr, HIT_POINT exp_dam)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (!monster_is_valid(m_ptr) || is_pet(m_ptr) || this->target_ptr->phase_out) {
        return;
    }

    /*
     * - Ratio of monster's level to player's level effects
     * - Varying speed effects
     * - Get a fraction in proportion of damage point
     */
    auto new_exp = r_ptr->level * SPEED_TO_ENERGY(m_ptr->mspeed) * exp_dam;
    auto new_exp_frac = 0U;
    auto div_h = 0;
    auto div_l = (uint)((this->target_ptr->max_plv + 2) * SPEED_TO_ENERGY(r_ptr->speed));

    /* Use (average maxhp * 2) as a denominator */
    auto compensation = any_bits(r_ptr->flags1, RF1_FORCE_MAXHP) ? r_ptr->hside * 2 : r_ptr->hside + 1;
    s64b_mul(&div_h, &div_l, 0, r_ptr->hdice * (ironman_nightmare ? 2 : 1) * compensation);

    /* Special penalty in the wilderness */
    if (!this->target_ptr->current_floor_ptr->dun_level && (none_bits(r_ptr->flags8, RF8_WILD_ONLY) || none_bits(r_ptr->flags1, RF1_UNIQUE))) {
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
    gain_exp_64(this->target_ptr, new_exp, new_exp_frac);
}

void MonsterDamageProcessor::set_redraw()
{
    if (this->target_ptr->health_who == this->m_idx) {
        this->target_ptr->redraw |= PR_HEALTH;
    }

    if (this->target_ptr->riding == this->m_idx) {
        this->target_ptr->redraw |= PR_UHEALTH;
    }
}

/*
 * @brief 特定ユニークを倒した時に更にユニークを特殊召喚する処理
 * @param m_ptr ダメージを与えた特定ユニークの構造体参照ポインタ
 */
void MonsterDamageProcessor::summon_special_unique(monster_type *m_ptr)
{
    bool is_special_summon = m_ptr->r_idx == MON_IKETA;
    is_special_summon |= m_ptr->r_idx == MON_DOPPIO;
    if (!is_special_summon || this->target_ptr->current_floor_ptr->inside_arena || this->target_ptr->phase_out) {
        delete_monster_idx(this->target_ptr, this->m_idx);
        return;
    }

    auto dummy_y = m_ptr->fy;
    auto dummy_x = m_ptr->fx;
    auto mode = (BIT_FLAGS)0;
    if (is_pet(m_ptr)) {
        mode |= PM_FORCE_PET;
    }

    MONRACE_IDX new_unique_idx;
    concptr mes;
    switch (m_ptr->r_idx) {
    case MON_IKETA:
        new_unique_idx = MON_BIKETAL;
        mes = _("「ハァッハッハッハ！！私がバイケタルだ！！」", "Uwa-hahaha!  *I* am Biketal!");
        break;
    case MON_DOPPIO:
        new_unique_idx = MON_DIAVOLO;
        mes = _("「これは『試練』だ　過去に打ち勝てという『試練』とオレは受けとった」", "This is a 'trial'. I took it as a 'trial' to overcome in the past.");
        break;
    default: // バグでなければ入らない.
        new_unique_idx = 0;
        mes = "";
        break;
    }

    delete_monster_idx(this->target_ptr, this->m_idx);
    if (summon_named_creature(this->target_ptr, 0, dummy_y, dummy_x, new_unique_idx, mode)) {
        msg_print(mes);
    }
}
