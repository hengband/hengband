#include "spell-realm/spells-hex.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "monster-race/monster-race.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-song.h"
#include "spell/spell-info.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!< 呪術の最大詠唱数 */
constexpr int MAX_KEEP = 4;

RealmHex::RealmHex(player_type *caster_ptr)
    : caster_ptr(caster_ptr)
{
    constexpr int max_realm_spells = 32;
    for (auto spell = 0; spell < max_realm_spells; spell++) {
        if (this->is_spelling_specific(spell)) {
            this->casting_spells.push_back(spell);
        }
    }

    if (this->casting_spells.size() > MAX_KEEP) {
        throw("Invalid numbers of hex magics keep!");
    }
}

/*!
 * @brief プレイヤーが詠唱中の全呪術を停止する
 */
bool RealmHex::stop_all_spells()
{
    for (auto spell : this->casting_spells) {
        exe_spell(this->caster_ptr, REALM_HEX, spell, SPELL_STOP);
    }

    casting_hex_flags(this->caster_ptr) = 0;
    casting_hex_num(this->caster_ptr) = 0;
    if (this->caster_ptr->action == ACTION_SPELL) {
        set_action(this->caster_ptr, ACTION_NONE);
    }

    this->caster_ptr->update |= PU_BONUS | PU_HP | PU_MANA | PU_SPELLS;
    this->caster_ptr->redraw |= PR_EXTRA | PR_HP | PR_MANA;
    return true;
}

/*!
 * @brief プレイヤーが詠唱中の呪術から一つを選んで停止する
 */
bool RealmHex::stop_one_spell()
{
    if (!RealmHex(this->caster_ptr).is_spelling_any()) {
        msg_print(_("呪文を詠唱していません。", "You are not casting a spell."));
        return false;
    }

    if ((casting_hex_num(this->caster_ptr) == 1) || (this->caster_ptr->lev < 35)) {
        return this->stop_all_spells();
    }

    char out_val[160];
    strnfmt(out_val, 78, _("どの呪文の詠唱を中断しますか？(呪文 %c-%c, 'l'全て, ESC)", "Which spell do you stop casting? (Spell %c-%c, 'l' to all, ESC)"),
        I2A(0), I2A(casting_hex_num(this->caster_ptr) - 1));
    screen_save();
    char choice = 0;
    auto is_selected = select_spell_stopping(out_val, choice);
    screen_load();
    if (is_selected) {
        auto n = this->casting_spells[A2I(choice)];
        exe_spell(this->caster_ptr, REALM_HEX, n, SPELL_STOP);
        casting_hex_flags(this->caster_ptr) &= ~(1UL << n);
        casting_hex_num(this->caster_ptr)--;
    }

    this->caster_ptr->update |= PU_BONUS | PU_HP | PU_MANA | PU_SPELLS;
    this->caster_ptr->redraw |= PR_EXTRA | PR_HP | PR_MANA;
    return is_selected;
}

/*!
 * @brief 中断する呪術を選択する
 * @param spells 詠唱中の呪術リスト
 * @param out_val 呪文名
 * @param choice 選択した呪文
 * @return 選択が完了したらtrue、キャンセルならばfalse
 */
bool RealmHex::select_spell_stopping(char *out_val, char &choice)
{
    while (true) {
        this->display_casting_spells_list();
        if (!get_com(out_val, &choice, true)) {
            return false;
        }

        if (isupper(choice)) {
            choice = static_cast<char>(tolower(choice));
        }

        /* All */
        if (choice == 'l') {
            screen_load();
            return this->stop_all_spells();
        }

        if ((choice < I2A(0)) || (choice > I2A(casting_hex_num(this->caster_ptr) - 1))) {
            continue;
        }

        return true;
    }
}

void RealmHex::display_casting_spells_list()
{
    constexpr auto y = 1;
    constexpr auto x = 20;
    auto n = 0;
    term_erase(x, y, 255);
    prt(_("     名前", "     Name"), y, x + 5);
    for (auto spell : this->casting_spells) {
        term_erase(x, y + n + 1, 255);
        auto spell_result = exe_spell(this->caster_ptr, REALM_HEX, spell, SPELL_NAME);
        put_str(format("%c)  %s", I2A(n), spell_result), y + n + 1, x + 2);
        n++;
    }
}

/*!
 * @brief 一定時間毎に呪術で消費するMPを処理する
 */
void RealmHex::decrease_mana()
{
    /* Spells spelled by player */
    if (this->caster_ptr->realm1 != REALM_HEX) {
        return;
    }

    if (!casting_hex_flags(this->caster_ptr) && !this->caster_ptr->magic_num1[1]) {
        return;
    }

    auto need_restart = this->check_restart();
    if (this->caster_ptr->anti_magic) {
        this->stop_all_spells();
        return;
    }

    if (!this->process_mana_cost(need_restart)) {
        return;
    }

    this->gain_exp_from_hex();
    for (auto spell : this->casting_spells) {
        exe_spell(this->caster_ptr, REALM_HEX, spell, SPELL_CONT);
    }
}

/*!
 * @brief 継続的な呪文の詠唱が可能な程度にMPが残っているか確認し、残量に応じて継続・中断を行う
 * @param need_restart 詠唱を再開するか否か
 * @return MPが足りているか否か
 */
bool RealmHex::process_mana_cost(const bool need_restart)
{
    auto need_mana = this->calc_need_mana();
    uint need_mana_frac = 0;
    s64b_div(&need_mana, &need_mana_frac, 0, 3); /* Divide by 3 */
    need_mana += (casting_hex_num(this->caster_ptr) - 1);

    auto enough_mana = s64b_cmp(this->caster_ptr->csp, this->caster_ptr->csp_frac, need_mana, need_mana_frac) >= 0;
    if (!enough_mana) {
        this->stop_all_spells();
        return false;
    }

    s64b_sub(&(this->caster_ptr->csp), &(this->caster_ptr->csp_frac), need_mana, need_mana_frac);
    this->caster_ptr->redraw |= PR_MANA;
    if (!need_restart) {
        return true;
    }

    msg_print(_("詠唱を再開した。", "You restart casting."));
    this->caster_ptr->action = ACTION_SPELL;
    this->caster_ptr->update |= PU_BONUS | PU_HP;
    this->caster_ptr->redraw |= PR_MAP | PR_STATUS | PR_STATE;
    this->caster_ptr->update |= PU_MONSTERS;
    this->caster_ptr->window_flags |= PW_OVERHEAD | PW_DUNGEON;
    return true;
}

bool RealmHex::check_restart()
{
    if (this->caster_ptr->magic_num1[1] == 0) {
        return false;
    }

    this->caster_ptr->magic_num1[0] = this->caster_ptr->magic_num1[1];
    this->caster_ptr->magic_num1[1] = 0;
    return true;
}

int RealmHex::calc_need_mana()
{
    auto need_mana = 0;
    for (auto spell : this->casting_spells) {
        const auto *s_ptr = &technic_info[REALM_HEX - MIN_TECHNIC][spell];
        need_mana += mod_need_mana(this->caster_ptr, s_ptr->smana, spell, REALM_HEX);
    }

    return need_mana;
}

void RealmHex::gain_exp_from_hex()
{
    for (auto spell : this->casting_spells) {
        if (!this->is_spelling_specific(spell)) {
            continue;
        }

        if (this->caster_ptr->spell_exp[spell] < SPELL_EXP_BEGINNER) {
            this->caster_ptr->spell_exp[spell] += 5;
            continue;
        }

        if (this->gain_exp_skilled(spell)) {
            continue;
        }

        if (this->gain_exp_expert(spell)) {
            continue;
        }

        this->gain_exp_master(spell);
    }
}

bool RealmHex::gain_exp_skilled(const int spell)
{
    if (this->caster_ptr->spell_exp[spell] >= SPELL_EXP_SKILLED) {
        return false;
    }

    auto *floor_ptr = this->caster_ptr->current_floor_ptr;
    auto gain_condition = one_in_(2);
    gain_condition &= floor_ptr->dun_level > 4;
    gain_condition &= (floor_ptr->dun_level + 10) > this->caster_ptr->lev;
    if (gain_condition) {
        this->caster_ptr->spell_exp[spell]++;
    }

    return true;
}

bool RealmHex::gain_exp_expert(const int spell)
{
    if (this->caster_ptr->spell_exp[spell] >= SPELL_EXP_EXPERT) {
        return false;
    }

    const auto *s_ptr = &technic_info[REALM_HEX - MIN_TECHNIC][spell];
    auto *floor_ptr = this->caster_ptr->current_floor_ptr;
    auto gain_condition = one_in_(5);
    gain_condition &= (floor_ptr->dun_level + 5) > this->caster_ptr->lev;
    gain_condition &= (floor_ptr->dun_level + 5) > s_ptr->slevel;
    if (gain_condition) {
        this->caster_ptr->spell_exp[spell]++;
    }

    return true;
}

void RealmHex::gain_exp_master(const int spell)
{
    if (this->caster_ptr->spell_exp[spell] >= SPELL_EXP_MASTER) {
        return;
    }

    const auto *s_ptr = &technic_info[REALM_HEX - MIN_TECHNIC][spell];
    auto *floor_ptr = this->caster_ptr->current_floor_ptr;
    auto gain_condition = one_in_(5);
    gain_condition &= (floor_ptr->dun_level + 5) > this->caster_ptr->lev;
    gain_condition &= floor_ptr->dun_level > s_ptr->slevel;
    if (gain_condition) {
        this->caster_ptr->spell_exp[spell]++;
    }
}

/*!
 * @brief プレイヤーの呪術詠唱枠がすでに最大かどうかを返す
 * @return すでに全枠を利用しているならTRUEを返す
 */
bool RealmHex::is_casting_full_capacity() const
{
    auto k_max = (this->caster_ptr->lev / 15) + 1;
    k_max = MIN(k_max, MAX_KEEP);
    return casting_hex_num(this->caster_ptr) >= k_max;
}

/*!
 * @brief 一定ゲームターン毎に復讐処理の残り期間の判定を行う
 */
void RealmHex::continue_revenge()
{
    if ((this->caster_ptr->realm1 != REALM_HEX) || (hex_revenge_turn(this->caster_ptr) <= 0)) {
        return;
    }

    switch (hex_revenge_type(this->caster_ptr)) {
    case 1:
        exe_spell(this->caster_ptr, REALM_HEX, HEX_PATIENCE, SPELL_CONT);
        return;
    case 2:
        exe_spell(this->caster_ptr, REALM_HEX, HEX_REVENGE, SPELL_CONT);
        return;
    default:
        return;
    }
}

/*!
 * @brief 復讐ダメージの追加を行う
 * @param dam 蓄積されるダメージ量
 */
void RealmHex::store_vengeful_damage(HIT_POINT dam)
{
    if ((this->caster_ptr->realm1 != REALM_HEX) || (hex_revenge_turn(this->caster_ptr) <= 0)) {
        return;
    }

    hex_revenge_power(this->caster_ptr) += dam;
}

/*!
 * @brief 呪術結界の判定
 * @param m_idx 判定の対象となるモンスターID
 * @return 呪術の効果が適用されるならTRUEを返す
 * @details v3.0.0現在は反テレポート・反魔法・反増殖の3種類
 */
bool RealmHex::check_hex_barrier(MONSTER_IDX m_idx, realm_hex_type type) const
{
    const auto *m_ptr = &this->caster_ptr->current_floor_ptr->m_list[m_idx];
    const auto *r_ptr = &r_info[m_ptr->r_idx];
    return this->is_spelling_specific(type) && ((this->caster_ptr->lev * 3 / 2) >= randint1(r_ptr->level));
}

bool RealmHex::is_spelling_specific(int hex) const
{
    auto check = static_cast<uint32_t>(this->caster_ptr->magic_num1[0]);
    return (this->caster_ptr->realm1 == REALM_HEX) && any_bits(check, 1U << hex);
}

bool RealmHex::is_spelling_any() const
{
    return (caster_ptr->realm1 == REALM_HEX) && (caster_ptr->magic_num1[0] != 0);
}
