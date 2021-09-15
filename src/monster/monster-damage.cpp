/*
 * @brief モンスターがダメージを受けた時の処理と経験値の加算処理
 * @date 2021/08/04
 * @author Hourier
 */

#include <algorithm>

#include "monster/monster-damage.h"
#include "avatar/avatar-changer.h"
#include "core/player-redraw-types.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
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

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx ダメージを与えたモンスターのID
 * @param dam 与えたダメージ量
 * @param fear ダメージによってモンスターが恐慌状態に陥ったならばtrue
 * @param note モンスターが倒された際の特別なメッセージ述語
 */
MonsterDamageProcessor::MonsterDamageProcessor(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *fear)
    : player_ptr(player_ptr)
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
    auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    auto exp_mon = *m_ptr;

    auto exp_dam = (m_ptr->hp > this->dam) ? this->dam : m_ptr->hp;

    this->get_exp_from_mon(&exp_mon, exp_dam);
    if (this->genocide_chaos_patron()) {
        return true;
    }

    m_ptr->hp -= this->dam;
    m_ptr->dealt_damage += this->dam;
    if (m_ptr->dealt_damage > m_ptr->max_maxhp * 100) {
        m_ptr->dealt_damage = m_ptr->max_maxhp * 100;
    }

    if (allow_debug_options) {
        msg_format(_("合計%d/%dのダメージを与えた。", "You do %d (out of %d) damage."), m_ptr->dealt_damage, m_ptr->maxhp);
    }

    if (this->process_dead_exp_virtue(note, &exp_mon)) {
        return true;
    }

    this->add_monster_fear();
    return false;
}

bool MonsterDamageProcessor::genocide_chaos_patron()
{
    auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    if (!monster_is_valid(m_ptr)) {
        this->m_idx = 0;
    }

    this->set_redraw();
    (void)set_monster_csleep(this->player_ptr, this->m_idx, 0);
    if (this->player_ptr->special_defense & NINJA_S_STEALTH) {
        set_superstealth(this->player_ptr, false);
    }

    return this->m_idx == 0;
}

bool MonsterDamageProcessor::process_dead_exp_virtue(concptr note, monster_type *exp_mon)
{
    auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (m_ptr->hp >= 0) {
        return false;
    }

    this->death_special_flag_monster();
    if (r_ptr->r_akills < MAX_SHORT) {
        r_ptr->r_akills++;
    }

    this->increase_kill_numbers();
    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(this->player_ptr, m_name, m_ptr, MD_TRUE_NAME);
    this->death_amberites(m_name);
    this->dying_scream(m_name);
    AvatarChanger ac(player_ptr, m_ptr);
    ac.change_virtue();
    if (any_bits(r_ptr->flags1, RF1_UNIQUE) && record_destroy_uniq) {
        char note_buf[160];
        sprintf(note_buf, "%s%s", r_ptr->name.c_str(), m_ptr->mflag2.has(MFLAG2::CLONED) ? _("(クローン)", "(Clone)") : "");
        exe_write_diary(this->player_ptr, DIARY_UNIQUE, 0, note_buf);
    }

    sound(SOUND_KILL);
    this->show_kill_message(note, m_name);
    this->show_bounty_message(m_name);
    monster_death(this->player_ptr, this->m_idx, true);
    this->summon_special_unique();
    this->get_exp_from_mon(exp_mon, exp_mon->max_maxhp * 2);
    *this->fear = false;
    return true;
}

/*
 * @brief たぬき、カメレオン、ナズグル、ユニークの死亡時処理
 * @param m_ptr ダメージを与えたモンスターの構造体参照ポインタ
 */
void MonsterDamageProcessor::death_special_flag_monster()
{
    auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    auto r_idx = m_ptr->r_idx;
    auto *r_ptr = &r_info[r_idx];
    if (any_bits(r_info[r_idx].flags7, RF7_TANUKI)) {
        r_ptr = &r_info[r_idx];
        m_ptr->ap_r_idx = r_idx;
        if (r_ptr->r_sights < MAX_SHORT) {
            r_ptr->r_sights++;
        }
    }

    if (m_ptr->mflag2.has(MFLAG2::CHAMELEON)) {
        r_ptr = real_r_ptr(m_ptr);
        if (r_ptr->r_sights < MAX_SHORT) {
            r_ptr->r_sights++;
        }
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

    this->death_unique_monster((monster_race_type)r_idx);
}

/*
 * @brief ユニークの死亡処理
 * @param r_idx 死亡したユニークの種族番号
 */
void MonsterDamageProcessor::death_unique_monster(monster_race_type r_idx)
{
    r_info[r_idx].max_num = 0;
    std::vector<monster_race_type> combined_unique_vec;
    if (!check_combined_unique(r_idx, &combined_unique_vec)) {
        return;
    }

    combined_uniques uniques;
    const int one_unit = 3;
    for (auto i = 0U; i < combined_unique_vec.size(); i += one_unit) {
        auto unique = std::make_tuple(combined_unique_vec[i], combined_unique_vec[i + 1], combined_unique_vec[i + 2]);
        uniques.push_back(unique);
    }

    this->death_combined_uniques(r_idx, &uniques);
}

/*
 * @brief 死亡したモンスターが分裂/合体を行う特殊ユニークか否かの判定処理
 * @param r_idx 死亡したモンスターの種族番号
 * @param united_uniques 分裂/合体を行う特殊ユニーク
 * @details 合体後、合体前1、合体前2 の順にpush_backすること
 */
bool MonsterDamageProcessor::check_combined_unique(const monster_race_type r_idx, std::vector<monster_race_type> *combined_unique_vec)
{
    combined_unique_vec->push_back(MON_BANORLUPART);
    combined_unique_vec->push_back(MON_BANOR);
    combined_unique_vec->push_back(MON_LUPART);

    for (const auto &unique : *combined_unique_vec) {
        if (r_idx == unique) {
            return true;
        }
    }

    return false;
}

/*
 * @brief 分裂/合体を行う特殊ユニークの死亡処理
 * @param m_ptr ダメージを与えたモンスターの構造体参照ポインタ
 * @uniques 分裂/合体を行う特殊ユニークのリスト
 */
void MonsterDamageProcessor::death_combined_uniques(const monster_race_type r_idx, combined_uniques *combined_uniques)
{
    for (const auto &unique : *combined_uniques) {
        auto united = (monster_race_type)0;
        auto split1 = (monster_race_type)0;
        auto split2 = (monster_race_type)0;
        std::tie(united, split1, split2) = unique;
        if ((r_idx == split1) || (r_idx == split2)) {
            r_info[united].max_num = 0;
            r_info[united].r_pkills++;
            r_info[united].r_akills++;
            if (r_info[united].r_tkills < MAX_SHORT) {
                r_info[united].r_tkills++;
            }

            continue;
        }

        if (r_idx != united) {
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

void MonsterDamageProcessor::increase_kill_numbers()
{
    auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (((m_ptr->ml == 0) || this->player_ptr->hallucinated) && none_bits(r_ptr->flags1, RF1_UNIQUE)) {
        return;
    }

    if (m_ptr->mflag2.has(MFLAG2::KAGE) && (r_info[MON_KAGE].r_pkills < MAX_SHORT)) {
        r_info[MON_KAGE].r_pkills++;
    } else if (r_ptr->r_pkills < MAX_SHORT) {
        r_ptr->r_pkills++;
    }

    if (m_ptr->mflag2.has(MFLAG2::KAGE) && (r_info[MON_KAGE].r_tkills < MAX_SHORT)) {
        r_info[MON_KAGE].r_tkills++;
    } else if (r_ptr->r_tkills < MAX_SHORT) {
        r_ptr->r_tkills++;
    }

    monster_race_track(this->player_ptr, m_ptr->ap_r_idx);
}

void MonsterDamageProcessor::death_amberites(GAME_TEXT *m_name)
{
    auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (none_bits(r_ptr->flags3, RF3_AMBERITE) || one_in_(2)) {
        return;
    }

    auto curses = 1 + randint1(3);
    auto stop_ty = false;
    auto count = 0;
    msg_format(_("%^sは恐ろしい血の呪いをあなたにかけた！", "%^s puts a terrible blood curse on you!"), m_name);
    curse_equipment(this->player_ptr, 100, 50);
    do {
        stop_ty = activate_ty_curse(this->player_ptr, stop_ty, &count);
    } while (--curses);
}

void MonsterDamageProcessor::dying_scream(GAME_TEXT *m_name)
{
    auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (none_bits(r_ptr->flags2, RF2_CAN_SPEAK)) {
        return;
    }

    char line_got[1024];
    if (!get_rnd_line(_("mondeath_j.txt", "mondeath.txt"), m_ptr->r_idx, line_got)) {
        msg_format("%^s %s", m_name, line_got);
    }

#ifdef WORLD_SCORE
    if (m_ptr->r_idx == MON_SERPENT) {
        screen_dump = make_screen_dump(this->player_ptr);
    }
#endif
}

void MonsterDamageProcessor::show_kill_message(concptr note, GAME_TEXT *m_name)
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (note != nullptr) {
        msg_format("%^s%s", m_name, note);
        return;
    }

    if (!m_ptr->ml) {
        auto mes = is_echizen(this->player_ptr) ? _("せっかくだから%sを殺した。", "Because it's time, you have killed %s.")
                                                : _("%sを殺した。", "You have killed %s.");
        msg_format(mes, m_name);
        return;
    }

    if (monster_living(m_ptr->r_idx)) {
        auto mes = is_echizen(this->player_ptr) ? _("せっかくだから%sを殺した。", "Because it's time, you have slain %s.")
                                                : _("%sを殺した。", "You have slain %s.");
        msg_format(mes, m_name);
        return;
    }

    auto explode = false;
    for (auto i = 0; i < 4; i++) {
        if (r_ptr->blow[i].method == RBM_EXPLODE) {
            explode = true;
        }
    }

    if (explode) {
        msg_format(_("%sは爆発して粉々になった。", "%^s explodes into tiny shreds."), m_name);
        return;
    }

    auto mes = is_echizen(this->player_ptr) ? _("せっかくだから%sを殺した。", "Because it's time, you have destroyed %s.")
                                            : _("%sを殺した。", "You have destroyed %s.");
    msg_format(mes, m_name);
}

void MonsterDamageProcessor::show_bounty_message(GAME_TEXT *m_name)
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (none_bits(r_ptr->flags1, RF1_UNIQUE) || m_ptr->mflag2.has(MFLAG2::CLONED) || vanilla_town) {
        return;
    }

    for (auto i = 0; i < MAX_BOUNTY; i++) {
        if ((w_ptr->bounty_r_idx[i] == m_ptr->r_idx) && m_ptr->mflag2.has_not(MFLAG2::CHAMELEON)) {
            msg_format(_("%sの首には賞金がかかっている。", "There is a price on %s's head."), m_name);
            break;
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
    if (!monster_is_valid(m_ptr) || is_pet(m_ptr) || this->player_ptr->phase_out) {
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
    auto div_l = (uint)((this->player_ptr->max_plv + 2) * SPEED_TO_ENERGY(r_ptr->speed));

    /* Use (average maxhp * 2) as a denominator */
    auto compensation = any_bits(r_ptr->flags1, RF1_FORCE_MAXHP) ? r_ptr->hside * 2 : r_ptr->hside + 1;
    s64b_mul(&div_h, &div_l, 0, r_ptr->hdice * (ironman_nightmare ? 2 : 1) * compensation);

    /* Special penalty in the wilderness */
    if (!this->player_ptr->current_floor_ptr->dun_level && (none_bits(r_ptr->flags8, RF8_WILD_ONLY) || none_bits(r_ptr->flags1, RF1_UNIQUE))) {
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
    gain_exp_64(this->player_ptr, new_exp, new_exp_frac);
}

void MonsterDamageProcessor::set_redraw()
{
    if (this->player_ptr->health_who == this->m_idx) {
        this->player_ptr->redraw |= PR_HEALTH;
    }

    if (this->player_ptr->riding == this->m_idx) {
        this->player_ptr->redraw |= PR_UHEALTH;
    }
}

/*
 * @brief 特定ユニークを倒した時に更にユニークを特殊召喚する処理
 * @param m_ptr ダメージを与えた特定ユニークの構造体参照ポインタ
 */
void MonsterDamageProcessor::summon_special_unique()
{
    auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    bool is_special_summon = m_ptr->r_idx == MON_IKETA;
    is_special_summon |= m_ptr->r_idx == MON_DOPPIO;
    if (!is_special_summon || this->player_ptr->current_floor_ptr->inside_arena || this->player_ptr->phase_out) {
        delete_monster_idx(this->player_ptr, this->m_idx);
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

    delete_monster_idx(this->player_ptr, this->m_idx);
    if (summon_named_creature(this->player_ptr, 0, dummy_y, dummy_x, new_unique_idx, mode)) {
        msg_print(mes);
    }
}

void MonsterDamageProcessor::add_monster_fear()
{
    auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    if (monster_fear_remaining(m_ptr) && (this->dam > 0)) {
        auto fear_remining = monster_fear_remaining(m_ptr) - randint1(this->dam);
        if (set_monster_monfear(this->player_ptr, this->m_idx, fear_remining)) {
            *this->fear = false;
        }
    }

    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (monster_fear_remaining(m_ptr) || any_bits(r_ptr->flags3, RF3_NO_FEAR)) {
        return;
    }

    int percentage = (100L * m_ptr->hp) / m_ptr->maxhp;
    if ((randint1(10) < percentage) && ((this->dam < m_ptr->hp) || (randint0(100) >= 80))) {
        return;
    }

    *this->fear = true;
    auto fear_condition = (this->dam >= m_ptr->hp) && (percentage > 7);
    auto fear_value = randint1(10) + (fear_condition ? 20 : (11 - percentage) * 5);
    (void)set_monster_monfear(this->player_ptr, this->m_idx, fear_value);
}
