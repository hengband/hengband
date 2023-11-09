#include "spell-realm/spells-hex.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-race/monster-race.h"
#include "player-base/player-class.h"
#include "player-info/spell-hex-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "realm/realm-hex-numbers.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-song.h"
#include "spell/spell-info.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "system/angband-exceptions.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#ifdef JP
#else
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#endif

/*!< 呪術の最大詠唱数 */
constexpr int MAX_KEEP = 4;

SpellHex::SpellHex(PlayerType *player_ptr)
    : player_ptr(player_ptr)
    , spell_hex_data(PlayerClass(player_ptr).get_specific_data<spell_hex_data_type>())
{
    if (!this->spell_hex_data) {
        return;
    }

    HexSpellFlagGroup::get_flags(this->spell_hex_data->casting_spells, std::back_inserter(this->casting_spells));

    if (this->casting_spells.size() > MAX_KEEP) {
        THROW_EXCEPTION(std::logic_error, "Invalid numbers of hex magics keep!");
    }
}

SpellHex::SpellHex(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
    : player_ptr(player_ptr)
    , monap_ptr(monap_ptr)
{
}

/*!
 * @brief プレイヤーが詠唱中の全呪術を停止する
 */
void SpellHex::stop_all_spells()
{
    for (auto spell : this->casting_spells) {
        exe_spell(this->player_ptr, REALM_HEX, spell, SpellProcessType::STOP);
    }

    this->spell_hex_data->casting_spells.clear();
    if (this->player_ptr->action == ACTION_SPELL) {
        set_action(this->player_ptr, ACTION_NONE);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    rfu.set_flags(flags_mwrf);
}

/*!
 * @brief プレイヤーが詠唱中の呪術から選択式で一つまたは全てを停止する
 * @return 停止したらtrue、停止をキャンセルしたらfalse
 */
bool SpellHex::stop_spells_with_selection()
{
    if (!this->is_spelling_any()) {
        msg_print(_("呪文を詠唱していません。", "You are not casting a spell."));
        return false;
    }

    auto casting_num = this->get_casting_num();
    if ((casting_num == 1) || (this->player_ptr->lev < 35)) {
        this->stop_all_spells();
        return true;
    }

    constexpr auto fmt = _("どの呪文の詠唱を中断しますか？(呪文 %c-%c, 'l'全て, ESC)", "Which spell do you stop casting? (Spell %c-%c, 'l' to all, ESC)");
    const auto prompt = format(fmt, I2A(0), I2A(casting_num - 1));
    screen_save();
    const auto &[is_all, choice] = select_spell_stopping(prompt);
    if (is_all) {
        return true;
    }

    screen_load();
    if (choice) {
        auto n = this->casting_spells[A2I(*choice)];
        exe_spell(this->player_ptr, REALM_HEX, n, SpellProcessType::STOP);
        this->reset_casting_flag(i2enum<spell_hex_type>(n));
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    rfu.set_flags(flags_mwrf);
    return choice.has_value();
}

/*!
 * @brief 中断する呪術を選択する
 * @param out_val 呪文名
 * @return
 * Item1: 全ての呪文を中断するならばtrue、1つの呪文を中断するならばfalse
 * Item2: 選択が完了したらtrue、キャンセルならばfalse
 * Item3: 選択した呪文番号 (a～d、lの5択)
 */
std::pair<bool, std::optional<char>> SpellHex::select_spell_stopping(std::string_view prompt)
{
    while (true) {
        this->display_casting_spells_list();
        const auto choice_opt = input_command(prompt, true);
        if (!choice_opt) {
            return { false, std::nullopt };
        }

        auto choice = choice_opt.value();
        if (isupper(choice)) {
            choice = static_cast<char>(tolower(choice));
        }

        if (choice == 'l') {
            screen_load();
            this->stop_all_spells();
            return { true, choice };
        }

        if ((choice < I2A(0)) || (choice > I2A(this->get_casting_num() - 1))) {
            continue;
        }

        return { false, choice };
    }
}

void SpellHex::display_casting_spells_list()
{
    constexpr auto y = 1;
    constexpr auto x = 20;
    auto n = 0;
    term_erase(x, y);
    prt(_("     名前", "     Name"), y, x + 5);
    for (auto spell : this->casting_spells) {
        term_erase(x, y + n + 1);
        const auto spell_name = exe_spell(this->player_ptr, REALM_HEX, spell, SpellProcessType::NAME);
        put_str(format("%c)  %s", I2A(n), spell_name->data()), y + n + 1, x + 2);
        n++;
    }
}

/*!
 * @brief 一定時間毎に呪術で消費するMPを処理する
 */
void SpellHex::decrease_mana()
{
    if (!this->spell_hex_data) {
        return;
    }

    if (this->spell_hex_data->casting_spells.none() && this->spell_hex_data->interrupting_spells.none()) {
        return;
    }

    auto need_restart = this->check_restart();
    if (this->player_ptr->anti_magic) {
        this->stop_all_spells();
        return;
    }

    if (!this->process_mana_cost(need_restart)) {
        return;
    }

    this->gain_exp();
    for (auto spell : this->casting_spells) {
        exe_spell(this->player_ptr, REALM_HEX, spell, SpellProcessType::CONTNUATION);
    }
}

/*!
 * @brief 継続的な呪文の詠唱が可能な程度にMPが残っているか確認し、残量に応じて継続・中断を行う
 * @param need_restart 詠唱を再開するか否か
 * @return MPが足りているか否か
 * @todo 64ビットの割り算をしなければいけない箇所には見えない. 調査の後不要ならば消すこと.
 */
bool SpellHex::process_mana_cost(const bool need_restart)
{
    auto need_mana = this->calc_need_mana();
    uint need_mana_frac = 0;
    s64b_div(&need_mana, &need_mana_frac, 0, 3); /* Divide by 3 */
    need_mana += this->get_casting_num() - 1;

    auto enough_mana = s64b_cmp(this->player_ptr->csp, this->player_ptr->csp_frac, need_mana, need_mana_frac) >= 0;
    if (!enough_mana) {
        this->stop_all_spells();
        return false;
    }

    s64b_sub(&(this->player_ptr->csp), &(this->player_ptr->csp_frac), need_mana, need_mana_frac);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MP);
    if (!need_restart) {
        return true;
    }

    msg_print(_("詠唱を再開した。", "You restart casting."));
    this->player_ptr->action = ACTION_SPELL;
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::MAP,
        MainWindowRedrawingFlag::TIMED_EFFECT,
        MainWindowRedrawingFlag::ACTION,
    };
    rfu.set_flags(flags_mwrf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
    return true;
}

bool SpellHex::check_restart()
{
    if (this->spell_hex_data->interrupting_spells.none()) {
        return false;
    }

    this->spell_hex_data->casting_spells = this->spell_hex_data->interrupting_spells;
    this->spell_hex_data->interrupting_spells.clear();
    return true;
}

int SpellHex::calc_need_mana()
{
    auto need_mana = 0;
    for (auto spell : this->casting_spells) {
        const auto *s_ptr = &technic_info[REALM_HEX - MIN_TECHNIC][spell];
        need_mana += mod_need_mana(this->player_ptr, s_ptr->smana, spell, REALM_HEX);
    }

    return need_mana;
}

void SpellHex::gain_exp()
{
    PlayerSkill ps(player_ptr);
    for (auto spell : this->casting_spells) {
        if (!this->is_spelling_specific(spell)) {
            continue;
        }

        ps.gain_continuous_spell_skill_exp(REALM_HEX, spell);
    }
}

/*!
 * @brief プレイヤーの呪術詠唱枠がすでに最大かどうかを返す
 * @return すでに全枠を利用しているならTRUEを返す
 */
bool SpellHex::is_casting_full_capacity() const
{
    auto k_max = (this->player_ptr->lev / 15) + 1;
    k_max = std::min(k_max, MAX_KEEP);
    return this->get_casting_num() >= k_max;
}

/*!
 * @brief 一定ゲームターン毎に復讐処理の残り期間の判定を行う
 */
void SpellHex::continue_revenge()
{
    if (!this->spell_hex_data || (this->get_revenge_turn() == 0)) {
        return;
    }

    switch (this->get_revenge_type()) {
    case SpellHexRevengeType::PATIENCE:
        exe_spell(this->player_ptr, REALM_HEX, HEX_PATIENCE, SpellProcessType::CONTNUATION);
        return;
    case SpellHexRevengeType::REVENGE:
        exe_spell(this->player_ptr, REALM_HEX, HEX_REVENGE, SpellProcessType::CONTNUATION);
        return;
    default:
        return;
    }
}

/*!
 * @brief 復讐ダメージの追加を行う
 * @param dam 蓄積されるダメージ量
 */
void SpellHex::store_vengeful_damage(int dam)
{
    if (!this->spell_hex_data || (this->get_revenge_turn() == 0)) {
        return;
    }

    this->set_revenge_power(dam, false);
}

/*!
 * @brief 呪術結界の判定
 * @param m_idx 判定の対象となるモンスターID
 * @return 呪術の効果が適用されるならTRUEを返す
 * @details v3.0.0現在は反テレポート・反魔法・反増殖の3種類
 */
bool SpellHex::check_hex_barrier(MONSTER_IDX m_idx, spell_hex_type type) const
{
    const auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[m_idx];
    const auto *r_ptr = &m_ptr->get_monrace();
    return this->is_spelling_specific(type) && ((this->player_ptr->lev * 3 / 2) >= randint1(r_ptr->level));
}

bool SpellHex::is_spelling_specific(int hex) const
{
    return this->spell_hex_data && this->spell_hex_data->casting_spells.has(i2enum<spell_hex_type>(hex));
}

bool SpellHex::is_spelling_any() const
{
    return this->spell_hex_data && (this->get_casting_num() > 0);
}

void SpellHex::interrupt_spelling()
{
    this->spell_hex_data->interrupting_spells = this->spell_hex_data->casting_spells;
    this->spell_hex_data->casting_spells.clear();
}

/*!
 * @brief 呪術「目には目を」の効果処理
 * @param this->player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void SpellHex::eyes_on_eyes()
{
    if (this->monap_ptr == nullptr) {
        THROW_EXCEPTION(std::logic_error, "Invalid constructor was used!");
    }

    const auto is_eyeeye_finished = (this->player_ptr->tim_eyeeye == 0) && !this->is_spelling_specific(HEX_EYE_FOR_EYE);
    if (is_eyeeye_finished || (this->monap_ptr->get_damage == 0) || this->player_ptr->is_dead) {
        return;
    }

#ifdef JP
    msg_format("攻撃が%s自身を傷つけた！", this->monap_ptr->m_name);
#else
    const auto m_name_self = monster_desc(this->player_ptr, this->monap_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
    msg_format("The attack of %s has wounded %s!", this->monap_ptr->m_name, m_name_self.data());
#endif
    const auto y = this->monap_ptr->m_ptr->fy;
    const auto x = this->monap_ptr->m_ptr->fx;
    project(this->player_ptr, 0, 0, y, x, this->monap_ptr->get_damage, AttributeType::MISSILE, PROJECT_KILL);
    if (this->player_ptr->tim_eyeeye) {
        set_tim_eyeeye(this->player_ptr, this->player_ptr->tim_eyeeye - 5, true);
    }
}

void SpellHex::thief_teleport()
{
    if (this->monap_ptr == nullptr) {
        THROW_EXCEPTION(std::logic_error, "Invalid constructor was used!");
    }

    if (!this->monap_ptr->blinked || !this->monap_ptr->alive || this->player_ptr->is_dead) {
        return;
    }

    if (this->check_hex_barrier(this->monap_ptr->m_idx, HEX_ANTI_TELE)) {
        msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
    } else {
        msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
        teleport_away(this->player_ptr, this->monap_ptr->m_idx, MAX_PLAYER_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
    }
}

void SpellHex::set_casting_flag(spell_hex_type type)
{
    this->spell_hex_data->casting_spells.set(type);
}

void SpellHex::reset_casting_flag(spell_hex_type type)
{
    this->spell_hex_data->casting_spells.reset(type);
}

int32_t SpellHex::get_casting_num() const
{
    return this->spell_hex_data->casting_spells.count();
}

int32_t SpellHex::get_revenge_power() const
{
    return this->spell_hex_data->revenge_power;
}

void SpellHex::set_revenge_power(int32_t power, bool substitution)
{
    if (substitution) {
        this->spell_hex_data->revenge_power = power;
    } else {
        this->spell_hex_data->revenge_power += power;
    }
}

byte SpellHex::get_revenge_turn() const
{
    return this->spell_hex_data->revenge_turn;
}

/*!
 * @brief 復讐の残りターンをセットするか、残りターン数を減らす
 * @param turn 残りターン (非負整数であること)
 * @param substitution セットならtrue、ターン減少ならfalse
 */
void SpellHex::set_revenge_turn(byte turn, bool substitution)
{
    if (substitution) {
        this->spell_hex_data->revenge_turn = turn;
    } else {
        this->spell_hex_data->revenge_turn -= turn;
    }
}

SpellHexRevengeType SpellHex::get_revenge_type() const
{
    return this->spell_hex_data->revenge_type;
}

void SpellHex::set_revenge_type(SpellHexRevengeType type)
{
    this->spell_hex_data->revenge_type = type;
}
