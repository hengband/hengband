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
bool SpellSelector::spell_okay(int spell, bool learned, bool study_pray, int use_realm)
{
    const magic_type *s_ptr;
    if (!is_magic(use_realm)) {
        s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
    } else {
        s_ptr = &mp_ptr->info[use_realm - 1][spell];
    }

    if (s_ptr->slevel > player_ptr->lev) {
        return false;
    }

    if ((use_realm == player_ptr->realm2) ? (player_ptr->spell_forgotten2 & (1UL << spell)) : (player_ptr->spell_forgotten1 & (1UL << spell))) {
        return false;
    }

    if (PlayerClass(player_ptr).is_every_magic()) {
        return true;
    }

    if ((use_realm == player_ptr->realm2) ? (player_ptr->spell_learned2 & (1UL << spell)) : (player_ptr->spell_learned1 & (1UL << spell))) {
        return !study_pray;
    }

    return !learned;
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
bool SpellSelector::get_spell(concptr prompt, OBJECT_SUBTYPE_VALUE sval, bool learned, int16_t use_realm, int tmp_sn)
{
    this->sn = tmp_sn;
    short code;
    if (repeat_pull(&code)) {
        this->sn = code;
        if (spell_okay(this->sn, learned, false, use_realm)) {
            return true;
        }
    }

    auto p = spell_category_name(mp_ptr->spell_book);
    auto num = 0;
    int spells[64]{};
    int spell = -1; // @todo よくあるバッドプラクティス.
    for (spell = 0; spell < 32; spell++) {
        if ((fake_spell_flags[sval] & (1UL << spell))) {
            spells[num++] = spell;
        }
    }

    auto okay = false;
    this->sn = -2;
    int spell_num;
    for (spell_num = 0; spell_num < num; spell_num++) {
        if (spell_okay(spells[spell_num], learned, false, use_realm)) {
            okay = true;
        }
    }

    if (!okay) {
        return false;
    }

    PlayerClass pc(player_ptr);
    auto is_every_magic = pc.is_every_magic();
    if (((use_realm) != player_ptr->realm1) && ((use_realm) != player_ptr->realm2) && !is_every_magic) {
        return false;
    }

    if (is_every_magic && !is_magic(use_realm)) {
        return false;
    }

    if (pc.equals(PlayerClassType::RED_MAGE) && ((use_realm) != REALM_ARCANE) && (sval > 1)) {
        return false;
    }

    this->sn = -1;
    auto flag = false;
    auto redraw = false;
    player_ptr->window_flags |= (PW_SPELL);
    handle_stuff(player_ptr);
    char out_val[160];
#ifdef JP
    char jverb_buf[128];
    jverb(prompt, jverb_buf, JVERB_AND);
    (void)strnfmt(out_val, 78, "(%^s:%c-%c, '*'で一覧, ESCで中断) どの%sを%^sますか? ", p, I2A(0), I2A(num - 1), p, jverb_buf);
#else
    (void)strnfmt(out_val, 78, "(%^ss %c-%c, *=List, ESC=exit) %^s which %s? ", p, I2A(0), I2A(num - 1), prompt, p);
#endif

    auto choice = (always_show_list || use_menu) ? ESCAPE : '\1';
    while (!flag) {
        if (choice == ESCAPE) {
            choice = ' ';
        } else if (!get_com(out_val, &choice, true)) {
            break;
        }

        auto ask = true;
        if (use_menu && choice != ' ') {
            auto menu_line = use_menu ? 1 : 0;
            switch (choice) {
            case '0':
                screen_load();
                return false;
            case '8':
            case 'k':
            case 'K':
                menu_line += (num - 1);
                break;
            case '2':
            case 'j':
            case 'J':
                menu_line++;
                break;
            case 'x':
            case 'X':
            case '\r':
            case '\n':
                spell_num = menu_line - 1;
                ask = false;
                break;
            }

            if (menu_line > num) {
                menu_line -= num;
            }

            print_spells(player_ptr, menu_line, spells, num, 1, 15, use_realm);
            if (ask) {
                continue;
            }
        } else {
            if ((choice == ' ') || (choice == '*') || (choice == '?')) {
                if (!redraw) {
                    redraw = true;
                    screen_save();
                    auto menu_line = use_menu ? 1 : 0;
                    print_spells(player_ptr, menu_line, spells, num, 1, 15, use_realm);
                } else {
                    if (use_menu) {
                        continue;
                    }

                    redraw = false;
                    screen_load();
                }

                continue;
            }

            ask = isupper(choice) != 0;
            if (ask) {
                choice = (char)tolower(choice);
            }

            spell_num = (islower(choice) ? A2I(choice) : -1);
        }

        if ((spell_num < 0) || (spell_num >= num)) {
            bell();
            continue;
        }

        spell = spells[spell_num];
        if (!spell_okay(spell, learned, false, use_realm)) {
            bell();
#ifdef JP
            msg_format("その%sを%sことはできません。", p, prompt);
#else
            msg_format("You may not %s that %s.", prompt, p);
#endif

            continue;
        }

        if (ask) {
            char tmp_val[160];
            const magic_type *s_ptr;
            if (!is_magic(use_realm)) {
                s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
            } else {
                s_ptr = &mp_ptr->info[use_realm - 1][spell];
            }

            int need_mana;
            if (use_realm == REALM_HISSATSU) {
                need_mana = s_ptr->smana;
            } else {
                need_mana = mod_need_mana(player_ptr, s_ptr->smana, spell, use_realm);
            }

#ifdef JP
            jverb(prompt, jverb_buf, JVERB_AND);
            (void)strnfmt(tmp_val, 78, "%s(MP%d, 失敗率%d%%)を%sますか? ", exe_spell(player_ptr, use_realm, spell, SpellProcessType::NAME), need_mana,
                spell_chance(player_ptr, spell, use_realm), jverb_buf);
#else
            (void)strnfmt(tmp_val, 78, "%^s %s (%d mana, %d%% fail)? ", prompt, exe_spell(player_ptr, use_realm, spell, SpellProcessType::NAME), need_mana,
                spell_chance(player_ptr, spell, use_realm));
#endif
            if (!get_check(tmp_val)) {
                continue;
            }
        }

        flag = true;
    }

    if (redraw) {
        screen_load();
    }

    player_ptr->window_flags |= (PW_SPELL);
    handle_stuff(player_ptr);
    if (!flag) {
        return false;
    }

    this->sn = spell;
    repeat_push((COMMAND_CODE)spell);
    return true;
}

int SpellSelector::get_selected_spell()
{
    return this->sn;
}
