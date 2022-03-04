/*
 * @brief 呪文一覧から唱えたい呪文をキーで選択する処理
 * @date 2022/03/01
 * @author Hourier
 */

#include "io/spell-selector.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/text-display-options.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "realm/realm-names-table.h"
#include "spell/spell-info.h"
#include "spell/spells-describer.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

#ifdef JP
#include "locale/japanese.h"
#endif

/*!
 * 魔法領域フラグ管理テーブル /
 * Zangband uses this array instead of the spell flags table, as there
 * are 5 realms of magic, each with 4 spellbooks and 8 spells per book -- TY
 */
const uint32_t fake_spell_flags[4] = { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };

SpellSelector::SpellSelector(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief 魔法が利用可能かどうかを返す /
 * Determine if a spell is "okay" for the player to cast or study
 * The spell must be legible, not forgotten, and also, to cast,
 * it must be known, and to study, it must not be known.
 * @param spell 呪文ID
 * @param learned 使用可能な判定ならばTRUE、学習可能かどうかの判定ならばFALSE
 * @param study_pray 祈りの学習判定目的ならばTRUE
 * @param use_realm 魔法領域ID
 * @return 選択した魔法が利用可能か否か
 */
bool SpellSelector::spell_okay(const int spell, const bool tmp_learned, const bool study_pray, const short tmp_use_realm)
{
    this->learned = tmp_learned;
    this->use_realm = tmp_use_realm;
    const magic_type *s_ptr;
    if (!is_magic(this->use_realm)) {
        s_ptr = &technic_info[this->use_realm - MIN_TECHNIC][spell];
    } else {
        s_ptr = &mp_ptr->info[this->use_realm - 1][spell];
    }

    if (s_ptr->slevel > this->player_ptr->lev) {
        return false;
    }

    if ((this->use_realm == this->player_ptr->realm2) ? (this->player_ptr->spell_forgotten2 & (1UL << spell)) : (this->player_ptr->spell_forgotten1 & (1UL << spell))) {
        return false;
    }

    if (PlayerClass(this->player_ptr).is_every_magic()) {
        return true;
    }

    if ((this->use_realm == this->player_ptr->realm2) ? (this->player_ptr->spell_learned2 & (1UL << spell)) : (this->player_ptr->spell_learned1 & (1UL << spell))) {
        return !study_pray;
    }

    return !this->learned;
}

/*!
 * @brief 領域魔法の閲覧、学習、使用選択するインターフェイス処理
 * Allow user to choose a spell/prayer from the given book.
 * @param sn 選択した魔法IDを返す参照ポインタ
 * @param prompt 魔法を利用する際の動詞表記
 * @param sval 魔道書のsval
 * @param learned 閲覧/使用選択ならばTRUE、学習処理ならFALSE
 * @param use_realm 魔法領域ID
 * @return
 * <pre>
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE
 * If the user hits escape, returns FALSE, and set '*sn' to -1
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2
 * The "prompt" should be "cast", "recite", or "study"
 * The "known" should be TRUE for cast/pray, FALSE for study
 * </pre>
 */
bool SpellSelector::get_spell(concptr tmp_prompt, OBJECT_SUBTYPE_VALUE sval, const bool tmp_learned, const short tmp_use_realm, int tmp_sn)
{
    this->learned = tmp_learned;
    this->prompt = tmp_prompt;
    this->use_realm = tmp_use_realm;
    this->sn = tmp_sn;
    if (this->can_repeat()) {
        return true;
    }

    this->spell_category = spell_category_name(mp_ptr->spell_book);
    for (auto i = 0; i < MAX_SPELLS_PER_REALM; i++) {
        if (fake_spell_flags[sval] & (1U << i)) {
            this->spells[this->num++] = i;
        }
    }

    if (!this->is_learned() || !this->need_learning(sval)) {
        return false;
    }

    this->sn = -1;
    this->player_ptr->window_flags |= (PW_SPELL);
    handle_stuff(this->player_ptr);
    if (!this->display_selection()) {
        return false;
    }

    if (this->redraw) {
        screen_load();
    }

    this->player_ptr->window_flags |= (PW_SPELL);
    handle_stuff(this->player_ptr);
    if (!this->flag) {
        return false;
    }

    this->sn = this->selected_spell;
    repeat_push(static_cast<short>(this->selected_spell));
    return true;
}

int SpellSelector::get_selected_spell()
{
    return this->sn;
}

bool SpellSelector::on_key_down()
{
    this->menu_line = use_menu ? 1 : 0;
    switch (this->choice) {
    case '0':
        screen_load();
        return false;
    case '8':
    case 'k':
    case 'K':
        this->menu_line += (this->num - 1);
        return true;
    case '2':
    case 'j':
    case 'J':
        this->menu_line++;
        return true;
    case 'x':
    case 'X':
    case '\r':
    case '\n':
        this->spell_num = this->menu_line - 1;
        this->ask = false;
        return true;
    default:
        return true;
    }
}

bool SpellSelector::decide_redraw()
{
    if ((this->choice != ' ') && (this->choice != '*') && (this->choice != '?')) {
        return false;
    }

    if (!this->redraw) {
        this->redraw = true;
        screen_save();
        this->menu_line = use_menu ? 1 : 0;
        print_spells(this->player_ptr, this->menu_line, this->spells, this->num, 1, 15, this->use_realm);
        return true;
    }

    if (use_menu) {
        return true;
    }

    this->redraw = false;
    screen_load();
    return true;
}

process_result SpellSelector::select_spell_number()
{
    if (use_menu && this->choice != ' ') {
        if (!this->on_key_down()) {
            return PROCESS_FALSE;
        }

        if (this->menu_line > this->num) {
            this->menu_line -= this->num;
        }

        print_spells(this->player_ptr, this->menu_line, this->spells, this->num, 1, 15, this->use_realm);
        if (this->ask) {
            return PROCESS_LOOP_CONTINUE;
        }
    }

    if (this->decide_redraw()) {
        return PROCESS_LOOP_CONTINUE;
    }

    this->ask = isupper(this->choice) != 0;
    if (this->ask) {
        this->choice = static_cast<char>(tolower(this->choice));
    }

    this->spell_num = islower(this->choice) ? A2I(this->choice) : -1;
    return PROCESS_CONTINUE;
}

bool SpellSelector::can_use()
{
    if (this->spell_okay(this->selected_spell, this->learned, false, this->use_realm)) {
        return true;
    }

    bell();
#ifdef JP
    msg_format("その%sを%sことはできません。", this->spell_category, this->prompt);
#else
    msg_format("You may not %s that %s.", this->prompt, this->spell_category);
#endif
    return false;
}

bool SpellSelector::ask_capital()
{
    if (!this->ask) {
        return true;
    }

    char tmp_val[160];
    const magic_type *s_ptr;
    if (!is_magic(this->use_realm)) {
        s_ptr = &technic_info[this->use_realm - MIN_TECHNIC][this->selected_spell];
    } else {
        s_ptr = &mp_ptr->info[this->use_realm - 1][this->selected_spell];
    }

    int need_mana;
    if (this->use_realm == REALM_HISSATSU) {
        need_mana = s_ptr->smana;
    } else {
        need_mana = mod_need_mana(this->player_ptr, s_ptr->smana, this->selected_spell, this->use_realm);
    }

#ifdef JP
    jverb(this->prompt, this->jverb_buf, JVERB_AND);
    (void)strnfmt(tmp_val, 78, "%s(MP%d, 失敗率%d%%)を%sますか? ", exe_spell(this->player_ptr, this->use_realm, this->selected_spell, SpellProcessType::NAME), need_mana,
        spell_chance(this->player_ptr, this->selected_spell, this->use_realm), this->jverb_buf);
#else
    (void)strnfmt(tmp_val, 78, "%^s %s (%d mana, %d%% fail)? ", this->prompt, exe_spell(this->player_ptr, this->use_realm, this->selected_spell, SpellProcessType::NAME), need_mana,
        spell_chance(this->player_ptr, this->selected_spell, this->use_realm));
#endif
    if (!get_check(tmp_val)) {
        return false;
    }

    return true;
}

bool SpellSelector::loop_key_input(char *out_val)
{
    while (!this->flag) {
        if (this->choice == ESCAPE) {
            this->choice = ' ';
        } else if (!get_com(out_val, &this->choice, true)) {
            return true;
        }

        this->ask = true;
        switch (this->select_spell_number()) {
        case PROCESS_FALSE:
            return false;
        case PROCESS_CONTINUE:
            break;
        case PROCESS_LOOP_CONTINUE:
            continue;
        default:
            throw("Invalid process result returns!");
        }

        if ((this->spell_num < 0) || (this->spell_num >= this->num)) {
            bell();
            continue;
        }

        this->selected_spell = this->spells[this->spell_num];
        if (!this->can_use()) {
            continue;
        }

        if (!this->ask_capital()) {
            continue;
        }

        this->flag = true;
    }

    return true;
}

bool SpellSelector::need_learning(OBJECT_SUBTYPE_VALUE sval)
{
    PlayerClass pc(this->player_ptr);
    auto is_every_magic = pc.is_every_magic();
    if ((this->use_realm != this->player_ptr->realm1) && (this->use_realm != this->player_ptr->realm2) && !is_every_magic) {
        return false;
    }

    if (is_every_magic && !is_magic(this->use_realm)) {
        return false;
    }

    if (pc.equals(PlayerClassType::RED_MAGE) && (this->use_realm != REALM_ARCANE) && (sval > 1)) {
        return false;
    }

    return true;
}

bool SpellSelector::is_learned()
{
    this->sn = -2;
    for (this->spell_num = 0; this->spell_num < this->num; this->spell_num++) {
        if (this->spell_okay(this->spells[this->spell_num], this->learned, false, this->use_realm)) {
            return true;
        }
    }

    return false;
}

bool SpellSelector::display_selection()
{
    char out_val[160];
#ifdef JP
    jverb(this->prompt, this->jverb_buf, JVERB_AND);
    (void)strnfmt(out_val, 78, "(%^s:%c-%c, '*'で一覧, ESCで中断) どの%sを%^sますか? ", this->spell_category, I2A(0), I2A(this->num - 1), this->spell_category, this->jverb_buf);
#else
    (void)strnfmt(out_val, 78, "(%^ss %c-%c, *=List, ESC=exit) %^s which %s? ", this->spell_category, I2A(0), I2A(this->num - 1), this->prompt, this->spell_category);
#endif

    this->choice = (always_show_list || use_menu) ? ESCAPE : '\1';
    this->selected_spell = MAX_SPELLS_PER_REALM;
    return this->loop_key_input(out_val);
}

bool SpellSelector::can_repeat()
{
    short code;
    if (!repeat_pull(&code)) {
        return false;
    }

    this->sn = code;
    return this->spell_okay(this->sn, this->learned, false, this->use_realm);
}
