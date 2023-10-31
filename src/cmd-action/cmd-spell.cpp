/*!
 * @brief 魔法のインターフェイスと発動 / Purpose: Do everything for each spell
 * @date 2013/12/31
 * @author
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "cmd-action/cmd-spell.h"
#include "action/action-limited.h"
#include "autopick/autopick-reader-writer.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-mind.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "locale/japanese.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-hook/hook-magic.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/samurai-data-type.h"
#include "player-info/self-info.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/eldritch-horror.h"
#include "player/player-damage.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "realm/realm-names-table.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-realm/spells-hex.h"
#include "spell/range-calc.h"
#include "spell/spell-info.h"
#include "spell/spells-describer.h"
#include "spell/spells-execution.h"
#include "spell/spells-summon.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/experience.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-util.h"
#include <string_view>

static const int extra_magic_gain_exp = 4;

concptr KWD_DAM = _("損傷:", "dam ");
concptr KWD_RANGE = _("射程:", "rng ");
concptr KWD_DURATION = _("期間:", "dur ");
concptr KWD_SPHERE = _("範囲:", "range ");
concptr KWD_HEAL = _("回復:", "heal ");
concptr KWD_MANA = _("MP回復:", "heal SP ");
concptr KWD_POWER _("効力:", "power ");
concptr KWD_RANDOM = _("ランダム", "random");

/*!
 * 魔法領域フラグ管理テーブル /
 * Zangband uses this array instead of the spell flags table, as there
 * are 5 realms of magic, each with 4 spellbooks and 8 spells per book -- TY
 */
const uint32_t fake_spell_flags[4] = { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };

/*!
 * @brief
 * 魔法の効果を「キャプション:ダイス＋定数値」のフォーマットで出力する / Generate dice info string such as "foo 2d10"
 * @param str キャプション
 * @param dice ダイス数
 * @param sides ダイス目
 * @param base 固定値
 * @return フォーマットに従い整形された文字列
 */
std::string info_string_dice(concptr str, DICE_NUMBER dice, DICE_SID sides, int base)
{
    /* Fix value */
    if (!dice) {
        return format("%s%d", str, base);
    }

    /* Dice only */
    else if (!base) {
        return format("%s%dd%d", str, dice, sides);
    }

    /* Dice plus base value */
    else {
        return format("%s%dd%d%+d", str, dice, sides, base);
    }
}

/*!
 * @brief 魔法によるダメージを出力する / Generate damage-dice info string such as "dam 2d10"
 * @param dice ダイス数
 * @param sides ダイス目
 * @param base 固定値
 * @return フォーマットに従い整形された文字列
 */
std::string info_damage(DICE_NUMBER dice, DICE_SID sides, int base)
{
    return info_string_dice(_("損傷:", "dam "), dice, sides, base);
}

/*!
 * @brief 魔法の効果時間を出力する / Generate duration info string such as "dur 20+1d20"
 * @param base 固定値
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
std::string info_duration(int base, DICE_SID sides)
{
    return format(_("期間:%d+1d%d", "dur %d+1d%d"), base, sides);
}

/*!
 * @brief 魔法の効果範囲を出力する / Generate range info string such as "range 5"
 * @param range 効果範囲
 * @return フォーマットに従い整形された文字列
 */
std::string info_range(POSITION range)
{
    return format(_("範囲:%d", "range %d"), range);
}

/*!
 * @brief 魔法による回復量を出力する / Generate heal info string such as "heal 2d8"
 * @param dice ダイス数
 * @param sides ダイス目
 * @param base 固定値
 * @return フォーマットに従い整形された文字列
 */
std::string info_heal(DICE_NUMBER dice, DICE_SID sides, int base)
{
    return info_string_dice(_("回復:", "heal "), dice, sides, base);
}

/*!
 * @brief 魔法効果発動までの遅延ターンを出力する / Generate delay info string such as "delay 15+1d15"
 * @param base 固定値
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
std::string info_delay(int base, DICE_SID sides)
{
    return format(_("遅延:%d+1d%d", "delay %d+1d%d"), base, sides);
}

/*!
 * @brief 魔法によるダメージを出力する(固定値＆複数回処理) / Generate multiple-damage info string such as "dam 25 each"
 * @param dam 固定値
 * @return フォーマットに従い整形された文字列
 */
std::string info_multi_damage(int dam)
{
    return format(_("損傷:各%d", "dam %d each"), dam);
}

/*!
 * @brief 魔法によるダメージを出力する(ダイスのみ＆複数回処理) / Generate multiple-damage-dice info string such as "dam 5d2 each"
 * @param dice ダイス数
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
std::string info_multi_damage_dice(DICE_NUMBER dice, DICE_SID sides)
{
    return format(_("損傷:各%dd%d", "dam %dd%d each"), dice, sides);
}

/*!
 * @brief 魔法による一般的な効力値を出力する（固定値） / Generate power info string such as "power 100"
 * @param power 固定値
 * @return フォーマットに従い整形された文字列
 */
std::string info_power(int power)
{
    return format(_("効力:%d", "power %d"), power);
}

/*!
 * @brief 魔法による一般的な効力値を出力する（ダイス値） / Generate power info string such as "power 100"
 * @param dice ダイス数
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
/*
 * Generate power info string such as "power 1d100"
 */
std::string info_power_dice(DICE_NUMBER dice, DICE_SID sides)
{
    return format(_("効力:%dd%d", "power %dd%d"), dice, sides);
}

/*!
 * @brief 魔法の効果半径を出力する / Generate radius info string such as "rad 100"
 * @param rad 効果半径
 * @return フォーマットに従い整形された文字列
 */
std::string info_radius(POSITION rad)
{
    return format(_("半径:%d", "rad %d"), rad);
}

/*!
 * @brief 魔法効果の限界重量を出力する / Generate weight info string such as "max wgt 15"
 * @param weight 最大重量
 * @return フォーマットに従い整形された文字列
 */
std::string info_weight(WEIGHT weight)
{
#ifdef JP
    return format("最大重量:%d.%dkg", lb_to_kg_integer(weight), lb_to_kg_fraction(weight));
#else
    return format("max wgt %d", weight / 10);
#endif
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
 * @return 失敗率(%)
 */
static bool spell_okay(PlayerType *player_ptr, int spell, bool learned, bool study_pray, int use_realm)
{
    const magic_type *s_ptr;

    /* Access the spell */
    if (!is_magic(use_realm)) {
        s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
    } else {
        s_ptr = &mp_ptr->info[use_realm - 1][spell];
    }

    /* Spell is illegal */
    if (s_ptr->slevel > player_ptr->lev) {
        return false;
    }

    /* Spell is forgotten */
    if ((use_realm == player_ptr->realm2) ? (player_ptr->spell_forgotten2 & (1UL << spell)) : (player_ptr->spell_forgotten1 & (1UL << spell))) {
        /* Never okay */
        return false;
    }

    if (PlayerClass(player_ptr).is_every_magic()) {
        return true;
    }

    /* Spell is learned */
    if ((use_realm == player_ptr->realm2) ? (player_ptr->spell_learned2 & (1UL << spell)) : (player_ptr->spell_learned1 & (1UL << spell))) {
        /* Always true */
        return !study_pray;
    }

    /* Okay to study, not to cast */
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
static int get_spell(PlayerType *player_ptr, SPELL_IDX *sn, std::string_view prompt_verb, int sval, bool learned, int16_t use_realm)
{
    int i;
    SPELL_IDX spell = -1;
    int num = 0;
    SPELL_IDX spells[64]{};
    COMMAND_CODE code;
    int menu_line = (use_menu ? 1 : 0);

    /* Get the spell, if available */
    if (repeat_pull(&code)) {
        *sn = (SPELL_IDX)code;
        /* Verify the spell */
        if (spell_okay(player_ptr, *sn, learned, false, use_realm)) {
            /* Success */
            return true;
        }
    }

    /* Extract spells */
    for (spell = 0; spell < 32; spell++) {
        /* Check for this spell */
        if ((fake_spell_flags[sval] & (1UL << spell))) {
            /* Collect this spell */
            spells[num++] = spell;
        }
    }

    /* Assume no usable spells */
    auto okay = false;

    /* Assume no spells available */
    (*sn) = -2;

    /* Check for "okay" spells */
    for (i = 0; i < num; i++) {
        /* Look for "okay" spells */
        if (spell_okay(player_ptr, spells[i], learned, false, use_realm)) {
            okay = true;
        }
    }

    /* No "okay" spells */
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

    /* Assume cancelled */
    *sn = (-1);

    auto flag = false;
    auto redraw = false;

    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::SPELL);
    handle_stuff(player_ptr);

    const auto spell_category = spell_category_name(mp_ptr->spell_book);
    constexpr auto fmt = _("(%s^:%c-%c, '*'で一覧, ESCで中断) どの%sを%s^ますか? ", "(%s^s %c-%c, *=List, ESC=exit) %s^ which %s? ");
#ifdef JP
    const auto verb = conjugate_jverb(prompt_verb, JVerbConjugationType::AND);
    const auto prompt = format(fmt, spell_category.data(), I2A(0), I2A(num - 1), spell_category.data(), verb.data());
#else
    const auto prompt = format(fmt, spell_category.data(), I2A(0), I2A(num - 1), prompt_verb.data(), spell_category.data());
#endif

    auto choice = (always_show_list || use_menu) ? ESCAPE : '\1';
    while (!flag) {
        if (choice == ESCAPE) {
            choice = ' ';
        } else {
            const auto new_choice = input_command(prompt, true);
            if (!new_choice.has_value()) {
                break;
            }

            choice = new_choice.value();
        }

        auto should_redraw_cursor = true;
        if (use_menu && choice != ' ') {
            switch (choice) {
            case '0': {
                screen_load();
                return false;
            }

            case '8':
            case 'k':
            case 'K': {
                menu_line += (num - 1);
                break;
            }

            case '2':
            case 'j':
            case 'J': {
                menu_line++;
                break;
            }

            case 'x':
            case 'X':
            case '\r':
            case '\n': {
                i = menu_line - 1;
                should_redraw_cursor = false;
                break;
            }
            }
            if (menu_line > num) {
                menu_line -= num;
            }
            /* Display a list of spells */
            print_spells(player_ptr, menu_line, spells, num, 1, 15, use_realm);
            if (should_redraw_cursor) {
                continue;
            }
        } else {
            /* Request redraw */
            if ((choice == ' ') || (choice == '*') || (choice == '?')) {
                /* Show the list */
                if (!redraw) {
                    redraw = true;
                    screen_save();

                    /* Display a list of spells */
                    print_spells(player_ptr, menu_line, spells, num, 1, 15, use_realm);
                }

                /* Hide the list */
                else {
                    if (use_menu) {
                        continue;
                    }

                    /* Hide list */
                    redraw = false;
                    screen_load();
                }

                /* Redo asking */
                continue;
            }

            i = A2I(choice);
        }

        /* Totally Illegal */
        if ((i < 0) || (i >= num)) {
            bell();
            continue;
        }

        /* Save the spell index */
        spell = spells[i];

        /* Require "okay" spells */
        if (!spell_okay(player_ptr, spell, learned, false, use_realm)) {
            bell();
#ifdef JP
            msg_format("その%sを%sことはできません。", spell_category.data(), prompt_verb.data());
#else
            msg_format("You may not %s that %s.", prompt.data(), spell_category.data());
#endif

            continue;
        }

        /* Stop the loop */
        flag = true;
    }

    if (redraw) {
        screen_load();
    }

    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::SPELL);
    handle_stuff(player_ptr);

    /* Abort if needed */
    if (!flag) {
        return false;
    }

    /* Save the choice */
    (*sn) = spell;

    repeat_push((COMMAND_CODE)spell);

    /* Success */
    return true;
}

/*!
 * @brief プレイヤーの職業が練気術師の時、領域魔法と練気術を切り換える処理のインターフェイス
 * @param browse_only 魔法と技能の閲覧を行うならばTRUE
 * @return 魔道書を一冊も持っていないならTRUEを返す
 */
static void confirm_use_force(PlayerType *player_ptr, bool browse_only)
{
    char which;
    COMMAND_CODE code;

    /* Get the item index */
    if (repeat_pull(&code) && (code == INVEN_FORCE)) {
        browse_only ? do_cmd_mind_browse(player_ptr) : do_cmd_mind(player_ptr);
        return;
    }

    /* Show the prompt */
    prt(_("('w'練気術, ESC) 'w'かESCを押してください。 ", "(w for the Force, ESC) Hit 'w' or ESC. "), 0, 0);

    while (true) {
        /* Get a key */
        which = inkey();

        if (which == ESCAPE) {
            break;
        } else if (which == 'w') {
            repeat_push(INVEN_FORCE);
            break;
        }
    }

    /* Clear the prompt line */
    prt("", 0, 0);

    if (which == 'w') {
        browse_only ? do_cmd_mind_browse(player_ptr) : do_cmd_mind(player_ptr);
    }
}

static FuncItemTester get_castable_spellbook_tester(PlayerType *player_ptr)
{
    return FuncItemTester([](auto p_ptr, auto o_ptr) { return check_book_realm(p_ptr, o_ptr->bi_key); }, player_ptr);
}

static FuncItemTester get_learnable_spellbook_tester(PlayerType *player_ptr)
{
    if (player_ptr->realm2 == REALM_NONE) {
        return get_castable_spellbook_tester(player_ptr);
    } else {
        return FuncItemTester(item_tester_learn_spell, player_ptr);
    }
}

/*!
 * @brief プレイヤーの魔法と技能を閲覧するコマンドのメインルーチン /
 * Peruse the spells/prayers in a book
 * @details
 * <pre>
 * Note that *all* spells in the book are listed
 *
 * Note that browsing is allowed while confused or blind,
 * and in the dark, primarily to allow browsing in stores.
 * </pre>
 */
void do_cmd_browse(PlayerType *player_ptr)
{
    SPELL_IDX spell = -1;
    int num = 0;

    SPELL_IDX spells[64];

    /* Warriors are illiterate */
    PlayerClass pc(player_ptr);
    if (!(player_ptr->realm1 || player_ptr->realm2) && !pc.is_every_magic()) {
        msg_print(_("本を読むことができない！", "You cannot read books!"));
        return;
    }

    pc.break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (pc.equals(PlayerClassType::FORCETRAINER)) {
        if (player_has_no_spellbooks(player_ptr)) {
            confirm_use_force(player_ptr, true);
            return;
        }
    }

    /* Restrict choices to "useful" books */
    auto item_tester = get_learnable_spellbook_tester(player_ptr);

    constexpr auto q = _("どの本を読みますか? ", "Browse which book? ");
    constexpr auto s = _("読める本がない。", "You have no books that you can read.");
    constexpr auto options = USE_INVEN | USE_FLOOR;
    short i_idx;
    const auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, options | (pc.equals(PlayerClassType::FORCETRAINER) ? USE_FORCE : 0), item_tester);
    if (o_ptr == nullptr) {
        if (i_idx == INVEN_FORCE) /* the_force */
        {
            do_cmd_mind_browse(player_ptr);
            return;
        }
        return;
    }

    /* Access the item's sval */
    const auto tval = o_ptr->bi_key.tval();
    const auto sval = o_ptr->bi_key.sval().value();
    short use_realm = tval2realm(tval);

    /* Track the object kind */
    object_kind_track(player_ptr, o_ptr->bi_id);
    handle_stuff(player_ptr);

    /* Extract spells */
    for (spell = 0; spell < 32; spell++) {
        /* Check for this spell */
        if ((fake_spell_flags[sval] & (1UL << spell))) {
            /* Collect this spell */
            spells[num++] = spell;
        }
    }

    screen_save();
    prt("", 0, 0);

    /* Keep browsing spells.  Exit browsing on cancel. */
    while (true) {
        /* Ask for a spell, allow cancel */
        if (!get_spell(player_ptr, &spell, _("読む", "browse"), sval, true, use_realm)) {
            /* If cancelled, leave immediately. */
            if (spell == -1) {
                break;
            }

            /* Display a list of spells */
            print_spells(player_ptr, 0, spells, num, 1, 15, use_realm);

            /* Notify that there's nothing to see, and wait. */
            if (use_realm == REALM_HISSATSU) {
                prt(_("読める技がない。", "No techniques to browse."), 0, 0);
            } else {
                prt(_("読める呪文がない。", "No spells to browse."), 0, 0);
            }
            (void)inkey();

            screen_load();

            return;
        }

        /* Clear lines, position cursor  (really should use strlen here) */
        term_erase(14, 14);
        term_erase(14, 13);
        term_erase(14, 12);
        term_erase(14, 11);

        const auto spell_desc = exe_spell(player_ptr, use_realm, spell, SpellProcessType::DESCRIPTION);
        display_wrap_around(spell_desc.value(), 62, 11, 15);
    }
    screen_load();
}

/*!
 * @brief プレイヤーの第二魔法領域を変更する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param next_realm 変更先の魔法領域ID
 */
static void change_realm2(PlayerType *player_ptr, int16_t next_realm)
{
    int i, j = 0;
    for (i = 0; i < 64; i++) {
        player_ptr->spell_order[j] = player_ptr->spell_order[i];
        if (player_ptr->spell_order[i] < 32) {
            j++;
        }
    }
    for (; j < 64; j++) {
        player_ptr->spell_order[j] = 99;
    }

    for (i = 32; i < 64; i++) {
        player_ptr->spell_exp[i] = PlayerSkill::spell_exp_at(PlayerSkillRank::UNSKILLED);
    }
    player_ptr->spell_learned2 = 0L;
    player_ptr->spell_worked2 = 0L;
    player_ptr->spell_forgotten2 = 0L;

    constexpr auto fmt_realm = _("魔法の領域を%sから%sに変更した。", "changed magic realm from %s to %s.");
    const auto mes = format(fmt_realm, realm_names[player_ptr->realm2], realm_names[next_realm]);
    exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 0, mes);
    player_ptr->old_realm |= 1U << (player_ptr->realm2 - 1);
    player_ptr->realm2 = next_realm;

    static constexpr auto flags = {
        StatusRecalculatingFlag::REORDER,
        StatusRecalculatingFlag::SPELLS,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    handle_stuff(player_ptr);

    /* Load an autopick preference file */
    autopick_load_pref(player_ptr, false);
}

/*!
 * @brief 魔法を学習するコマンドのメインルーチン /
 * Study a book to gain a new spell/prayer
 */
void do_cmd_study(PlayerType *player_ptr)
{
    auto increment = 0;
    auto learned = false;

    /* Spells of realm2 will have an increment of +32 */
    SPELL_IDX spell = -1;
    const auto spell_category = spell_category_name(mp_ptr->spell_book);
    if (!player_ptr->realm1) {
        msg_print(_("本を読むことができない！", "You cannot read books!"));
        return;
    }

    if (cmd_limit_blind(player_ptr) || cmd_limit_confused(player_ptr)) {
        return;
    }

    if (player_ptr->new_spells == 0) {
        msg_format(_("新しい%sを覚えることはできない！", "You cannot learn any new %ss!"), spell_category.data());
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

#ifdef JP
    if (player_ptr->new_spells < 10) {
        msg_format("あと %d つの%sを学べる。", player_ptr->new_spells, spell_category.data());
    } else {
        msg_format("あと %d 個の%sを学べる。", player_ptr->new_spells, spell_category.data());
    }
#else
    msg_format("You can learn %d new %s%s.", player_ptr->new_spells, spell_category.data(), (player_ptr->new_spells == 1 ? "" : "s"));
#endif

    msg_print(nullptr);

    /* Restrict choices to "useful" books */
    auto item_tester = get_learnable_spellbook_tester(player_ptr);

    constexpr auto q = _("どの本から学びますか? ", "Study which book? ");
    constexpr auto s = _("読める本がない。", "You have no books that you can read.");

    short i_idx;
    const auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_INVEN | USE_FLOOR), item_tester);
    if (o_ptr == nullptr) {
        return;
    }

    const auto tval = o_ptr->bi_key.tval();
    const auto sval = o_ptr->bi_key.sval().value();
    if (tval == get_realm2_book(player_ptr)) {
        increment = 32;
    } else if (tval != get_realm1_book(player_ptr)) {
        if (!input_check(_("本当に魔法の領域を変更しますか？", "Really, change magic realm? "))) {
            return;
        }

        change_realm2(player_ptr, tval2realm(tval));
        increment = 32;
    }

    /* Track the object kind */
    object_kind_track(player_ptr, o_ptr->bi_id);
    handle_stuff(player_ptr);

    /* Mage -- Learn a selected spell */
    if (mp_ptr->spell_book != ItemKindType::LIFE_BOOK) {
        /* Ask for a spell, allow cancel */
        if (!get_spell(player_ptr, &spell, _("学ぶ", "study"), sval, false, tval2realm(tval)) && (spell == -1)) {
            return;
        }
    }

    /* Priest -- Learn a random prayer */
    else {
        int k = 0;
        int gift = -1;

        /* Extract spells */
        for (spell = 0; spell < 32; spell++) {
            /* Check spells in the book */
            if ((fake_spell_flags[sval] & (1UL << spell))) {
                /* Skip non "okay" prayers */
                if (!spell_okay(player_ptr, spell, false, true, (increment ? player_ptr->realm2 : player_ptr->realm1))) {
                    continue;
                }

                /* Hack -- Prepare the randomizer */
                k++;

                /* Hack -- Apply the randomizer */
                if (one_in_(k)) {
                    gift = spell;
                }
            }
        }

        /* Accept gift */
        spell = gift;
    }

    /* Nothing to study */
    if (spell < 0) {
        msg_format(_("その本には学ぶべき%sがない。", "You cannot learn any %ss in that book."), spell_category.data());

        /* Abort */
        return;
    }

    if (increment) {
        spell += increment;
    }

    /* Learn the spell */
    if (spell < 32) {
        if (player_ptr->spell_learned1 & (1UL << spell)) {
            learned = true;
        } else {
            player_ptr->spell_learned1 |= (1UL << spell);
        }
    } else {
        if (player_ptr->spell_learned2 & (1UL << (spell - 32))) {
            learned = true;
        } else {
            player_ptr->spell_learned2 |= (1UL << (spell - 32));
        }
    }

    if (learned) {
        auto max_exp = PlayerSkill::spell_exp_at((spell < 32) ? PlayerSkillRank::MASTER : PlayerSkillRank::EXPERT);
        const auto old_exp = player_ptr->spell_exp[spell];
        const auto realm = increment ? player_ptr->realm2 : player_ptr->realm1;
        const auto spell_name = exe_spell(player_ptr, realm, spell % 32, SpellProcessType::NAME);

        if (old_exp >= max_exp) {
            msg_format(_("その%sは完全に使いこなせるので学ぶ必要はない。", "You don't need to study this %s anymore."), spell_category.data());
            return;
        }
#ifdef JP
        if (!input_check(format("%sの%sをさらに学びます。よろしいですか？", spell_name->data(), spell_category.data())))
#else
        if (!input_check(format("You will study a %s of %s again. Are you sure? ", spell_category.data(), spell_name->data())))
#endif
        {
            return;
        }

        auto new_rank = PlayerSkill(player_ptr).gain_spell_skill_exp_over_learning(spell);
        auto new_rank_str = PlayerSkill::skill_rank_str(new_rank);
        msg_format(_("%sの熟練度が%sに上がった。", "Your proficiency of %s is now %s rank."), spell_name->data(), new_rank_str);
    } else {
        /* Find the next open entry in "player_ptr->spell_order[]" */
        int i;
        for (i = 0; i < 64; i++) {
            /* Stop at the first empty space */
            if (player_ptr->spell_order[i] == 99) {
                break;
            }
        }

        /* Add the spell to the known list */
        player_ptr->spell_order[i++] = spell;

        /* Mention the result */
        const auto realm = increment ? player_ptr->realm2 : player_ptr->realm1;
        const auto spell_name = exe_spell(player_ptr, realm, spell % 32, SpellProcessType::NAME);
#ifdef JP
        if (mp_ptr->spell_book == ItemKindType::MUSIC_BOOK) {
            msg_format("%sを学んだ。", spell_name->data());
        } else {
            msg_format("%sの%sを学んだ。", spell_name->data(), spell_category.data());
        }
#else
        msg_format("You have learned the %s of %s.", spell_category.data(), spell_name->data());
#endif
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    switch (mp_ptr->spell_book) {
    case ItemKindType::LIFE_BOOK:
        chg_virtue(player_ptr, Virtue::FAITH, 1);
        break;
    case ItemKindType::DEATH_BOOK:
        chg_virtue(player_ptr, Virtue::UNLIFE, 1);
        break;
    case ItemKindType::NATURE_BOOK:
        chg_virtue(player_ptr, Virtue::NATURE, 1);
        break;
    default:
        chg_virtue(player_ptr, Virtue::KNOWLEDGE, 1);
        break;
    }

    sound(SOUND_STUDY);

    /* One less spell available */
    player_ptr->learned_spells++;

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::SPELLS);
    update_creature(player_ptr);
    rfu.set_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGE);
}

/*!
 * @brief 魔法を詠唱するコマンドのメインルーチン /
 * Cast a spell
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 詠唱したらtrue
 */
bool do_cmd_cast(PlayerType *player_ptr)
{
    SPELL_IDX spell;
    int16_t realm;
    int chance;
    auto increment = 0;
    int16_t use_realm;
    MANA_POINT need_mana;

    const magic_type *s_ptr;
    auto over_exerted = false;

    /* Require spell ability */
    PlayerClass pc(player_ptr);
    auto is_every_magic = pc.is_every_magic();
    if (!player_ptr->realm1 && !is_every_magic) {
        msg_print(_("呪文を唱えられない！", "You cannot cast spells!"));
        return false;
    }

    if (player_ptr->effects()->blindness()->is_blind() || no_lite(player_ptr)) {
        if (pc.equals(PlayerClassType::FORCETRAINER)) {
            confirm_use_force(player_ptr, false);
        } else {
            msg_print(_("目が見えない！", "You cannot see!"));
            flush();
        }

        return false;
    }

    if (cmd_limit_confused(player_ptr)) {
        return false;
    }

    if (player_ptr->realm1 == REALM_HEX) {
        if (SpellHex(player_ptr).is_casting_full_capacity()) {
            auto flag = false;
            msg_print(_("これ以上新しい呪文を詠唱することはできない。", "Can not cast more spells."));
            flush();
            if (player_ptr->lev >= 35) {
                flag = SpellHex(player_ptr).stop_spells_with_selection();
            }

            if (!flag) {
                return false;
            }
        }
    }

    if (pc.equals(PlayerClassType::FORCETRAINER)) {
        if (player_has_no_spellbooks(player_ptr)) {
            confirm_use_force(player_ptr, false);
            return true; //!< 錬気キャンセル時の処理がない
        }
    }

    const auto prayer = spell_category_name(mp_ptr->spell_book);
    constexpr auto q = _("どの呪文書を使いますか? ", "Use which book? ");
    constexpr auto s = _("呪文書がない！", "You have no spell books!");
    auto item_tester = get_castable_spellbook_tester(player_ptr);
    const auto options = USE_INVEN | USE_FLOOR | (pc.equals(PlayerClassType::FORCETRAINER) ? USE_FORCE : 0);
    short i_idx;
    const auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, options, item_tester);
    if (o_ptr == nullptr) {
        if (i_idx == INVEN_FORCE) {
            do_cmd_mind(player_ptr);
            return true; //!< 錬気キャンセル時の処理がない
        }

        return false;
    }

    const auto tval = o_ptr->bi_key.tval();
    const auto sval = o_ptr->bi_key.sval().value();
    if (!is_every_magic && (tval == get_realm2_book(player_ptr))) {
        increment = 32;
    }

    /* Track the object kind */
    object_kind_track(player_ptr, o_ptr->bi_id);
    handle_stuff(player_ptr);

    if (is_every_magic) {
        realm = tval2realm(tval);
    } else if (increment) {
        realm = player_ptr->realm2;
    } else {
        realm = player_ptr->realm1;
    }

    /* Ask for a spell */
#ifdef JP
    if (!get_spell(player_ptr, &spell,
            ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK)       ? "詠唱する"
                : (mp_ptr->spell_book == ItemKindType::MUSIC_BOOK) ? "歌う"
                                                                   : "唱える"),
            sval, true, realm)) {
        if (spell == -2) {
            msg_format("その本には知っている%sがない。", prayer.data());
        }
        return false;
    }
#else
    if (!get_spell(player_ptr, &spell, ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "recite" : "cast"), sval, true, realm)) {
        if (spell == -2) {
            msg_format("You don't know any %ss in that book.", prayer.data());
        }
        return false;
    }
#endif

    use_realm = tval2realm(tval);
    if (use_realm == REALM_HEX) {
        if (SpellHex(player_ptr).is_spelling_specific(spell)) {
            msg_print(_("その呪文はすでに詠唱中だ。", "You are already casting it."));
            return false;
        }
    }

    if (!is_magic(use_realm)) {
        s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
    } else {
        s_ptr = &mp_ptr->info[realm - 1][spell];
    }

    /* Extract mana consumption rate */
    need_mana = mod_need_mana(player_ptr, s_ptr->smana, spell, realm);

    /* Verify "dangerous" spells */
    if (need_mana > player_ptr->csp) {
        if (flush_failure) {
            flush();
        }

        /* Warning */
#ifdef JP
        msg_format("その%sを%sのに十分なマジックポイントがない。", prayer.data(),
            ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK)      ? "詠唱する"
                : (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "歌う"
                                                                  : "唱える"));
#else
        msg_format("You do not have enough mana to %s this %s.", ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "recite" : "cast"), prayer.data());
#endif

        if (!over_exert) {
            return false;
        }

        /* Verify */
        if (!input_check_strict(player_ptr, _("それでも挑戦しますか? ", "Attempt it anyway? "), UserCheck::OKAY_CANCEL)) {
            return false;
        }
    }

    /* Spell failure chance */
    chance = spell_chance(player_ptr, spell, use_realm);

    /* Failed spell */
    if (randint0(100) < chance) {
        if (flush_failure) {
            flush();
        }

        msg_format(_("%sをうまく唱えられなかった！", "You failed to get the %s off!"), prayer.data());
        sound(SOUND_FAIL);

        switch (realm) {
        case REALM_LIFE:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, Virtue::VITALITY, -1);
            }
            break;
        case REALM_DEATH:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, Virtue::UNLIFE, -1);
            }
            break;
        case REALM_NATURE:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, Virtue::NATURE, -1);
            }
            break;
        case REALM_DAEMON:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, Virtue::JUSTICE, 1);
            }
            break;
        case REALM_CRUSADE:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, Virtue::JUSTICE, -1);
            }
            break;
        case REALM_HEX:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, Virtue::COMPASSION, -1);
            }
            break;
        default:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, Virtue::KNOWLEDGE, -1);
            }
            break;
        }

        /* Failure casting may activate some side effect */
        exe_spell(player_ptr, realm, spell, SpellProcessType::FAIL);

        if ((tval == ItemKindType::CHAOS_BOOK) && (randint1(100) < spell)) {
            msg_print(_("カオス的な効果を発生した！", "You produce a chaotic effect!"));
            wild_magic(player_ptr, spell);
        } else if ((tval == ItemKindType::DEATH_BOOK) && (randint1(100) < spell)) {
            if ((sval == 3) && one_in_(2)) {
                sanity_blast(player_ptr, 0, true);
            } else {
                msg_print(_("痛い！", "It hurts!"));
                take_hit(player_ptr, DAMAGE_LOSELIFE, damroll(sval + 1, 6), _("暗黒魔法の逆流", "a miscast Death spell"));

                if ((spell > 15) && one_in_(6) && !player_ptr->hold_exp) {
                    lose_exp(player_ptr, spell * 250);
                }
            }
        } else if ((tval == ItemKindType::MUSIC_BOOK) && (randint1(200) < spell)) {
            msg_print(_("いやな音が響いた", "An infernal sound echoed."));
            aggravate_monsters(player_ptr, 0);
        }
        if (randint1(100) >= chance) {
            chg_virtue(player_ptr, Virtue::CHANCE, -1);
        }
    }

    /* Process spell */
    else {
        /* Canceled spells cost neither a turn nor mana */
        if (!exe_spell(player_ptr, realm, spell, SpellProcessType::CAST)) {
            return false;
        }

        if (randint1(100) < chance) {
            chg_virtue(player_ptr, Virtue::CHANCE, 1);
        }

        /* A spell was cast */
        if (!(increment ? (player_ptr->spell_worked2 & (1UL << spell)) : (player_ptr->spell_worked1 & (1UL << spell))) && !is_every_magic) {
            int e = s_ptr->sexp;

            /* The spell worked */
            if (realm == player_ptr->realm1) {
                player_ptr->spell_worked1 |= (1UL << spell);
            } else {
                player_ptr->spell_worked2 |= (1UL << spell);
            }

            gain_exp(player_ptr, e * s_ptr->slevel);
            RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGE);

            switch (realm) {
            case REALM_LIFE:
                chg_virtue(player_ptr, Virtue::TEMPERANCE, 1);
                chg_virtue(player_ptr, Virtue::COMPASSION, 1);
                chg_virtue(player_ptr, Virtue::VITALITY, 1);
                chg_virtue(player_ptr, Virtue::DILIGENCE, 1);
                break;
            case REALM_DEATH:
                chg_virtue(player_ptr, Virtue::UNLIFE, 1);
                chg_virtue(player_ptr, Virtue::JUSTICE, -1);
                chg_virtue(player_ptr, Virtue::FAITH, -1);
                chg_virtue(player_ptr, Virtue::VITALITY, -1);
                break;
            case REALM_DAEMON:
                chg_virtue(player_ptr, Virtue::JUSTICE, -1);
                chg_virtue(player_ptr, Virtue::FAITH, -1);
                chg_virtue(player_ptr, Virtue::HONOUR, -1);
                chg_virtue(player_ptr, Virtue::TEMPERANCE, -1);
                break;
            case REALM_CRUSADE:
                chg_virtue(player_ptr, Virtue::FAITH, 1);
                chg_virtue(player_ptr, Virtue::JUSTICE, 1);
                chg_virtue(player_ptr, Virtue::SACRIFICE, 1);
                chg_virtue(player_ptr, Virtue::HONOUR, 1);
                break;
            case REALM_NATURE:
                chg_virtue(player_ptr, Virtue::NATURE, 1);
                chg_virtue(player_ptr, Virtue::HARMONY, 1);
                break;
            case REALM_HEX:
                chg_virtue(player_ptr, Virtue::JUSTICE, -1);
                chg_virtue(player_ptr, Virtue::FAITH, -1);
                chg_virtue(player_ptr, Virtue::HONOUR, -1);
                chg_virtue(player_ptr, Virtue::COMPASSION, -1);
                break;
            default:
                chg_virtue(player_ptr, Virtue::KNOWLEDGE, 1);
                break;
            }
        }
        switch (realm) {
        case REALM_LIFE:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::TEMPERANCE, 1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::COMPASSION, 1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::VITALITY, 1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::DILIGENCE, 1);
            }
            break;
        case REALM_DEATH:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::UNLIFE, 1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::JUSTICE, -1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::FAITH, -1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::VITALITY, -1);
            }
            break;
        case REALM_DAEMON:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::JUSTICE, -1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::FAITH, -1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::HONOUR, -1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::TEMPERANCE, -1);
            }
            break;
        case REALM_CRUSADE:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::FAITH, 1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::JUSTICE, 1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::SACRIFICE, 1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::HONOUR, 1);
            }
            break;
        case REALM_NATURE:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::NATURE, 1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::HARMONY, 1);
            }
            break;
        case REALM_HEX:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::JUSTICE, -1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::FAITH, -1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::HONOUR, -1);
            }
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, Virtue::COMPASSION, -1);
            }
            break;
        }
        if (any_bits(mp_ptr->spell_xtra, extra_magic_gain_exp)) {
            PlayerSkill(player_ptr).gain_spell_skill_exp(realm, spell);
        }
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    /* Sufficient mana */
    if (need_mana <= player_ptr->csp) {
        /* Use some mana */
        player_ptr->csp -= need_mana;
    } else {
        over_exerted = true;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);

    /* Over-exert the player */
    if (over_exerted) {
        int oops = need_mana;
        player_ptr->csp = 0;
        player_ptr->csp_frac = 0;
        msg_print(_("精神を集中しすぎて気を失ってしまった！", "You faint from the effort!"));
        (void)BadStatusSetter(player_ptr).mod_paralysis(randint1(5 * oops + 1));
        switch (realm) {
        case REALM_LIFE:
            chg_virtue(player_ptr, Virtue::VITALITY, -10);
            break;
        case REALM_DEATH:
            chg_virtue(player_ptr, Virtue::UNLIFE, -10);
            break;
        case REALM_DAEMON:
            chg_virtue(player_ptr, Virtue::JUSTICE, 10);
            break;
        case REALM_NATURE:
            chg_virtue(player_ptr, Virtue::NATURE, -10);
            break;
        case REALM_CRUSADE:
            chg_virtue(player_ptr, Virtue::JUSTICE, -10);
            break;
        case REALM_HEX:
            chg_virtue(player_ptr, Virtue::COMPASSION, 10);
            break;
        default:
            chg_virtue(player_ptr, Virtue::KNOWLEDGE, -10);
            break;
        }

        /* Damage CON (possibly permanently) */
        if (randint0(100) < 50) {
            bool perm = (randint0(100) < 25);

            msg_print(_("体を悪くしてしまった！", "You have damaged your health!"));

            /* Reduce constitution */
            (void)dec_stat(player_ptr, A_CON, 15 + randint1(10), perm);
        }
    }

    static constexpr auto flags = {
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::SPELL,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    return true; //!< @note 詠唱した
}
