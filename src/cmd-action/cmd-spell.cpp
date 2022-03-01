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
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/spell-selector.h"
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
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

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
 * @brief 魔法の効果値をダイス目形式で出力する
 * @param str キャプション
 * @param dice ダイス数
 * @param sides ダイス目
 * @param base 固定値
 * @return フォーマットに従い整形された文字列
 * @details 固定値、ダイス、ダイス＋定数値の3種類
 */
concptr info_string_dice(concptr str, DICE_NUMBER dice, DICE_SID sides, int base)
{
    if (!dice) {
        return format("%s%d", str, base);
    } else if (!base) {
        return format("%s%dd%d", str, dice, sides);
    } else {
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
concptr info_damage(DICE_NUMBER dice, DICE_SID sides, int base)
{
    return info_string_dice(_("損傷:", "dam "), dice, sides, base);
}

/*!
 * @brief 魔法の効果時間を出力する / Generate duration info string such as "dur 20+1d20"
 * @param base 固定値
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
concptr info_duration(int base, DICE_SID sides)
{
    return format(_("期間:%d+1d%d", "dur %d+1d%d"), base, sides);
}

/*!
 * @brief 魔法の効果範囲を出力する / Generate range info string such as "range 5"
 * @param range 効果範囲
 * @return フォーマットに従い整形された文字列
 */
concptr info_range(POSITION range)
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
concptr info_heal(DICE_NUMBER dice, DICE_SID sides, int base)
{
    return info_string_dice(_("回復:", "heal "), dice, sides, base);
}

/*!
 * @brief 魔法効果発動までの遅延ターンを出力する / Generate delay info string such as "delay 15+1d15"
 * @param base 固定値
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
concptr info_delay(int base, DICE_SID sides)
{
    return format(_("遅延:%d+1d%d", "delay %d+1d%d"), base, sides);
}

/*!
 * @brief 魔法によるダメージを出力する(固定値＆複数回処理) / Generate multiple-damage info string such as "dam 25 each"
 * @param dam 固定値
 * @return フォーマットに従い整形された文字列
 */
concptr info_multi_damage(int dam)
{
    return format(_("損傷:各%d", "dam %d each"), dam);
}

/*!
 * @brief 魔法によるダメージを出力する(ダイスのみ＆複数回処理) / Generate multiple-damage-dice info string such as "dam 5d2 each"
 * @param dice ダイス数
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
concptr info_multi_damage_dice(DICE_NUMBER dice, DICE_SID sides)
{
    return format(_("損傷:各%dd%d", "dam %dd%d each"), dice, sides);
}

/*!
 * @brief 魔法による一般的な効力値を出力する（固定値） / Generate power info string such as "power 100"
 * @param power 固定値
 * @return フォーマットに従い整形された文字列
 */
concptr info_power(int power)
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
concptr info_power_dice(DICE_NUMBER dice, DICE_SID sides)
{
    return format(_("効力:%dd%d", "power %dd%d"), dice, sides);
}

/*!
 * @brief 魔法の効果半径を出力する / Generate radius info string such as "rad 100"
 * @param rad 効果半径
 * @return フォーマットに従い整形された文字列
 */
concptr info_radius(POSITION rad)
{
    return format(_("半径:%d", "rad %d"), rad);
}

/*!
 * @brief 魔法効果の限界重量を出力する / Generate weight info string such as "max wgt 15"
 * @param weight 最大重量
 * @return フォーマットに従い整形された文字列
 */
concptr info_weight(WEIGHT weight)
{
#ifdef JP
    return format("最大重量:%d.%dkg", lb_to_kg_integer(weight), lb_to_kg_fraction(weight));
#else
    return format("max wgt %d", weight / 10);
#endif
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
    if (repeat_pull(&code) && (code == INVEN_FORCE)) {
        browse_only ? do_cmd_mind_browse(player_ptr) : do_cmd_mind(player_ptr);
        return;
    }

    prt(_("('w'練気術, ESC) 'w'かESCを押してください。 ", "(w for the Force, ESC) Hit 'w' or ESC. "), 0, 0);
    while (true) {
        which = inkey();
        if (which == ESCAPE) {
            break;
        } else if (which == 'w') {
            repeat_push(INVEN_FORCE);
            break;
        }
    }

    prt("", 0, 0);
    if (which == 'w') {
        browse_only ? do_cmd_mind_browse(player_ptr) : do_cmd_mind(player_ptr);
    }
}

static FuncItemTester get_castable_spellbook_tester(PlayerType *player_ptr)
{
    return FuncItemTester([](auto p_ptr, auto o_ptr) { return check_book_realm(p_ptr, o_ptr->tval, o_ptr->sval); }, player_ptr);
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
    OBJECT_IDX item;
    OBJECT_SUBTYPE_VALUE sval;
    int16_t use_realm = 0;
    int j, line;
    SPELL_IDX spell = -1;
    int num = 0;
    SPELL_IDX spells[64];
    char temp[62 * 4];
    ObjectType *o_ptr;
    concptr q, s;
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

    auto item_tester = get_learnable_spellbook_tester(player_ptr);
    q = _("どの本を読みますか? ", "Browse which book? ");
    s = _("読める本がない。", "You have no books that you can read.");
    o_ptr = choose_object(player_ptr, &item, q, s, USE_INVEN | USE_FLOOR | (pc.equals(PlayerClassType::FORCETRAINER) ? USE_FORCE : 0), item_tester);
    if (!o_ptr) {
        if (item == INVEN_FORCE) /* the_force */
        {
            do_cmd_mind_browse(player_ptr);
            return;
        }
        return;
    }

    sval = o_ptr->sval;
    use_realm = tval2realm(o_ptr->tval);
    object_kind_track(player_ptr, o_ptr->k_idx);
    handle_stuff(player_ptr);
    for (spell = 0; spell < 32; spell++) {
        if ((fake_spell_flags[sval] & (1UL << spell))) {
            spells[num++] = spell;
        }
    }

    screen_save();
    prt("", 0, 0);
    while (true) {
        if (!get_spell(player_ptr, &spell, _("読む", "browse"), o_ptr->sval, true, use_realm)) {
            if (spell == -1) {
                break;
            }

            print_spells(player_ptr, 0, spells, num, 1, 15, use_realm);
            if (use_realm == REALM_HISSATSU) {
                prt(_("読める技がない。", "No techniques to browse."), 0, 0);
            } else {
                prt(_("読める呪文がない。", "No spells to browse."), 0, 0);
            }

            (void)inkey();
            screen_load();
            return;
        }

        term_erase(14, 14, 255);
        term_erase(14, 13, 255);
        term_erase(14, 12, 255);
        term_erase(14, 11, 255);
        shape_buffer(exe_spell(player_ptr, use_realm, spell, SpellProcessType::DESCRIPTION), 62, temp, sizeof(temp));
        for (j = 0, line = 11; temp[j]; j += 1 + strlen(&temp[j])) {
            prt(&temp[j], line, 15);
            line++;
        }
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
    char tmp[80];
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
    sprintf(tmp, _("魔法の領域を%sから%sに変更した。", "changed magic realm from %s to %s."), realm_names[player_ptr->realm2], realm_names[next_realm]);
    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, tmp);
    player_ptr->old_realm |= 1U << (player_ptr->realm2 - 1);
    player_ptr->realm2 = next_realm;
    player_ptr->update |= (PU_REORDER);
    player_ptr->update |= (PU_SPELLS);
    handle_stuff(player_ptr);
    autopick_load_pref(player_ptr, false);
}

/*!
 * @brief 魔法を学習するコマンドのメインルーチン /
 * Study a book to gain a new spell/prayer
 */
void do_cmd_study(PlayerType *player_ptr)
{
    int i;
    OBJECT_IDX item;
    OBJECT_SUBTYPE_VALUE sval;
    int increment = 0;
    bool learned = false;
    SPELL_IDX spell = -1;
    concptr p = spell_category_name(mp_ptr->spell_book);
    ObjectType *o_ptr;
    concptr q, s;
    if (!player_ptr->realm1) {
        msg_print(_("本を読むことができない！", "You cannot read books!"));
        return;
    }

    if (cmd_limit_blind(player_ptr)) {
        return;
    }

    if (cmd_limit_confused(player_ptr)) {
        return;
    }

    if (!(player_ptr->new_spells)) {
        msg_format(_("新しい%sを覚えることはできない！", "You cannot learn any new %ss!"), p);
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });
#ifdef JP
    if (player_ptr->new_spells < 10) {
        msg_format("あと %d つの%sを学べる。", player_ptr->new_spells, p);
    } else {
        msg_format("あと %d 個の%sを学べる。", player_ptr->new_spells, p);
    }
#else
    msg_format("You can learn %d new %s%s.", player_ptr->new_spells, p, (player_ptr->new_spells == 1 ? "" : "s"));
#endif

    msg_print(nullptr);
    auto item_tester = get_learnable_spellbook_tester(player_ptr);
    q = _("どの本から学びますか? ", "Study which book? ");
    s = _("読める本がない。", "You have no books that you can read.");
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), item_tester);
    if (!o_ptr) {
        return;
    }

    sval = o_ptr->sval;
    if (o_ptr->tval == get_realm2_book(player_ptr)) {
        increment = 32;
    } else if (o_ptr->tval != get_realm1_book(player_ptr)) {
        if (!get_check(_("本当に魔法の領域を変更しますか？", "Really, change magic realm? "))) {
            return;
        }

        change_realm2(player_ptr, tval2realm(o_ptr->tval));
        increment = 32;
    }

    object_kind_track(player_ptr, o_ptr->k_idx);
    handle_stuff(player_ptr);
    if (mp_ptr->spell_book != ItemKindType::LIFE_BOOK) {
        if (!get_spell(player_ptr, &spell, _("学ぶ", "study"), sval, false, tval2realm(o_ptr->tval)) && (spell == -1)) {
            return;
        }
    } else {
        int k = 0;
        int gift = -1;
        for (spell = 0; spell < 32; spell++) {
            if ((fake_spell_flags[sval] & (1UL << spell))) {
                if (!spell_okay(player_ptr, spell, false, true, (increment ? player_ptr->realm2 : player_ptr->realm1))) {
                    continue;
                }

                k++;
                if (one_in_(k)) {
                    gift = spell;
                }
            }
        }

        spell = gift;
    }

    if (spell < 0) {
        msg_format(_("その本には学ぶべき%sがない。", "You cannot learn any %ss in that book."), p);
        return;
    }

    if (increment) {
        spell += increment;
    }

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
        int old_exp = player_ptr->spell_exp[spell];
        concptr name = exe_spell(player_ptr, increment ? player_ptr->realm2 : player_ptr->realm1, spell % 32, SpellProcessType::NAME);

        if (old_exp >= max_exp) {
            msg_format(_("その%sは完全に使いこなせるので学ぶ必要はない。", "You don't need to study this %s anymore."), p);
            return;
        }
#ifdef JP
        if (!get_check(format("%sの%sをさらに学びます。よろしいですか？", name, p)))
#else
        if (!get_check(format("You will study a %s of %s again. Are you sure? ", p, name)))
#endif
        {
            return;
        }

        auto new_rank = PlayerSkill(player_ptr).gain_spell_skill_exp_over_learning(spell);
        auto new_rank_str = PlayerSkill::skill_rank_str(new_rank);
        msg_format(_("%sの熟練度が%sに上がった。", "Your proficiency of %s is now %s rank."), name, new_rank_str);
    } else {
        for (i = 0; i < 64; i++) {
            if (player_ptr->spell_order[i] == 99) {
                break;
            }
        }

        player_ptr->spell_order[i++] = spell;
#ifdef JP
        if (mp_ptr->spell_book == ItemKindType::MUSIC_BOOK) {
            msg_format("%sを学んだ。", exe_spell(player_ptr, increment ? player_ptr->realm2 : player_ptr->realm1, spell % 32, SpellProcessType::NAME));
        } else {
            msg_format("%sの%sを学んだ。", exe_spell(player_ptr, increment ? player_ptr->realm2 : player_ptr->realm1, spell % 32, SpellProcessType::NAME), p);
        }
#else
        msg_format("You have learned the %s of %s.", p, exe_spell(player_ptr, increment ? player_ptr->realm2 : player_ptr->realm1, spell % 32, SpellProcessType::NAME));
#endif
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    switch (mp_ptr->spell_book) {
    case ItemKindType::LIFE_BOOK:
        chg_virtue(player_ptr, V_FAITH, 1);
        break;
    case ItemKindType::DEATH_BOOK:
        chg_virtue(player_ptr, V_UNLIFE, 1);
        break;
    case ItemKindType::NATURE_BOOK:
        chg_virtue(player_ptr, V_NATURE, 1);
        break;
    default:
        chg_virtue(player_ptr, V_KNOWLEDGE, 1);
        break;
    }

    sound(SOUND_STUDY);
    player_ptr->learned_spells++;
    player_ptr->update |= (PU_SPELLS);
    update_creature(player_ptr);
    player_ptr->window_flags |= (PW_OBJECT);
}

/*!
 * @brief 魔法を詠唱するコマンドのメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 詠唱したらtrue
 */
bool do_cmd_cast(PlayerType *player_ptr)
{
    OBJECT_IDX item;
    OBJECT_SUBTYPE_VALUE sval;
    SPELL_IDX spell;
    int16_t realm;
    int chance;
    int increment = 0;
    int16_t use_realm;
    MANA_POINT need_mana;
    concptr prayer;
    ObjectType *o_ptr;
    const magic_type *s_ptr;
    concptr q, s;
    bool over_exerted = false;
    PlayerClass pc(player_ptr);
    auto is_every_magic = pc.is_every_magic();
    if (!player_ptr->realm1 && !is_every_magic) {
        msg_print(_("呪文を唱えられない！", "You cannot cast spells!"));
        return false;
    }

    if (player_ptr->blind || no_lite(player_ptr)) {
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

    // @todo 錬気キャンセル時の処理がない.
    if (pc.equals(PlayerClassType::FORCETRAINER)) {
        if (player_has_no_spellbooks(player_ptr)) {
            confirm_use_force(player_ptr, false);
            return true;
        }
    }

    prayer = spell_category_name(mp_ptr->spell_book);
    q = _("どの呪文書を使いますか? ", "Use which book? ");
    s = _("呪文書がない！", "You have no spell books!");
    auto item_tester = get_castable_spellbook_tester(player_ptr);
    o_ptr = choose_object(player_ptr, &item, q, s, USE_INVEN | USE_FLOOR | (pc.equals(PlayerClassType::FORCETRAINER) ? USE_FORCE : 0), item_tester);

    // @todo 錬気キャンセル時の処理がない.
    if (!o_ptr) {
        if (item == INVEN_FORCE) {
            do_cmd_mind(player_ptr);
            return true;
        }
        return false;
    }

    sval = o_ptr->sval;
    if (!is_every_magic && (o_ptr->tval == get_realm2_book(player_ptr))) {
        increment = 32;
    }

    object_kind_track(player_ptr, o_ptr->k_idx);
    handle_stuff(player_ptr);
    if (is_every_magic) {
        realm = tval2realm(o_ptr->tval);
    } else if (increment) {
        realm = player_ptr->realm2;
    } else {
        realm = player_ptr->realm1;
    }

#ifdef JP
    if (!get_spell(player_ptr, &spell,
            ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK)       ? "詠唱する"
                : (mp_ptr->spell_book == ItemKindType::MUSIC_BOOK) ? "歌う"
                                                                   : "唱える"),
            sval, true, realm)) {
        if (spell == -2) {
            msg_format("その本には知っている%sがない。", prayer);
        }

        return false;
    }
#else
    if (!get_spell(player_ptr, &spell, ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "recite" : "cast"), sval, true, realm)) {
        if (spell == -2) {
            msg_format("You don't know any %ss in that book.", prayer);
        }

        return false;
    }
#endif

    use_realm = tval2realm(o_ptr->tval);
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

    need_mana = mod_need_mana(player_ptr, s_ptr->smana, spell, realm);
    if (need_mana > player_ptr->csp) {
        if (flush_failure) {
            flush();
        }

#ifdef JP
        msg_format("その%sを%sのに十分なマジックポイントがない。", prayer,
            ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK)      ? "詠唱する"
                : (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "歌う"
                                                                  : "唱える"));
#else
        msg_format("You do not have enough mana to %s this %s.", ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "recite" : "cast"), prayer);
#endif

        if (!over_exert) {
            return false;
        }

        if (!get_check_strict(player_ptr, _("それでも挑戦しますか? ", "Attempt it anyway? "), CHECK_OKAY_CANCEL)) {
            return false;
        }
    }

    chance = spell_chance(player_ptr, spell, use_realm);
    if (randint0(100) < chance) {
        if (flush_failure) {
            flush();
        }

        msg_format(_("%sをうまく唱えられなかった！", "You failed to get the %s off!"), prayer);
        sound(SOUND_FAIL);
        switch (realm) {
        case REALM_LIFE:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, V_VITALITY, -1);
            }

            break;
        case REALM_DEATH:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, V_UNLIFE, -1);
            }

            break;
        case REALM_NATURE:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, V_NATURE, -1);
            }

            break;
        case REALM_DAEMON:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, V_JUSTICE, 1);
            }

            break;
        case REALM_CRUSADE:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, V_JUSTICE, -1);
            }

            break;
        case REALM_HEX:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, V_COMPASSION, -1);
            }

            break;
        default:
            if (randint1(100) < chance) {
                chg_virtue(player_ptr, V_KNOWLEDGE, -1);
            }

            break;
        }

        exe_spell(player_ptr, realm, spell, SpellProcessType::FAIL);
        if ((o_ptr->tval == ItemKindType::CHAOS_BOOK) && (randint1(100) < spell)) {
            msg_print(_("カオス的な効果を発生した！", "You produce a chaotic effect!"));
            wild_magic(player_ptr, spell);
        } else if ((o_ptr->tval == ItemKindType::DEATH_BOOK) && (randint1(100) < spell)) {
            if ((sval == 3) && one_in_(2)) {
                sanity_blast(player_ptr, 0, true);
            } else {
                msg_print(_("痛い！", "It hurts!"));
                take_hit(player_ptr, DAMAGE_LOSELIFE, damroll(o_ptr->sval + 1, 6), _("暗黒魔法の逆流", "a miscast Death spell"));

                if ((spell > 15) && one_in_(6) && !player_ptr->hold_exp) {
                    lose_exp(player_ptr, spell * 250);
                }
            }
        } else if ((o_ptr->tval == ItemKindType::MUSIC_BOOK) && (randint1(200) < spell)) {
            msg_print(_("いやな音が響いた", "An infernal sound echoed."));
            aggravate_monsters(player_ptr, 0);
        }

        if (randint1(100) >= chance) {
            chg_virtue(player_ptr, V_CHANCE, -1);
        }
    } else {
        if (!exe_spell(player_ptr, realm, spell, SpellProcessType::CAST)) {
            return false;
        }

        if (randint1(100) < chance) {
            chg_virtue(player_ptr, V_CHANCE, 1);
        }

        if (!(increment ? (player_ptr->spell_worked2 & (1UL << spell)) : (player_ptr->spell_worked1 & (1UL << spell))) && !is_every_magic) {
            int e = s_ptr->sexp;
            if (realm == player_ptr->realm1) {
                player_ptr->spell_worked1 |= (1UL << spell);
            } else {
                player_ptr->spell_worked2 |= (1UL << spell);
            }

            gain_exp(player_ptr, e * s_ptr->slevel);
            player_ptr->window_flags |= (PW_OBJECT);
            switch (realm) {
            case REALM_LIFE:
                chg_virtue(player_ptr, V_TEMPERANCE, 1);
                chg_virtue(player_ptr, V_COMPASSION, 1);
                chg_virtue(player_ptr, V_VITALITY, 1);
                chg_virtue(player_ptr, V_DILIGENCE, 1);
                break;
            case REALM_DEATH:
                chg_virtue(player_ptr, V_UNLIFE, 1);
                chg_virtue(player_ptr, V_JUSTICE, -1);
                chg_virtue(player_ptr, V_FAITH, -1);
                chg_virtue(player_ptr, V_VITALITY, -1);
                break;
            case REALM_DAEMON:
                chg_virtue(player_ptr, V_JUSTICE, -1);
                chg_virtue(player_ptr, V_FAITH, -1);
                chg_virtue(player_ptr, V_HONOUR, -1);
                chg_virtue(player_ptr, V_TEMPERANCE, -1);
                break;
            case REALM_CRUSADE:
                chg_virtue(player_ptr, V_FAITH, 1);
                chg_virtue(player_ptr, V_JUSTICE, 1);
                chg_virtue(player_ptr, V_SACRIFICE, 1);
                chg_virtue(player_ptr, V_HONOUR, 1);
                break;
            case REALM_NATURE:
                chg_virtue(player_ptr, V_NATURE, 1);
                chg_virtue(player_ptr, V_HARMONY, 1);
                break;
            case REALM_HEX:
                chg_virtue(player_ptr, V_JUSTICE, -1);
                chg_virtue(player_ptr, V_FAITH, -1);
                chg_virtue(player_ptr, V_HONOUR, -1);
                chg_virtue(player_ptr, V_COMPASSION, -1);
                break;
            default:
                chg_virtue(player_ptr, V_KNOWLEDGE, 1);
                break;
            }
        }

        switch (realm) {
        case REALM_LIFE:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_TEMPERANCE, 1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_COMPASSION, 1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_VITALITY, 1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_DILIGENCE, 1);
            }

            break;
        case REALM_DEATH:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_UNLIFE, 1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_JUSTICE, -1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_FAITH, -1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_VITALITY, -1);
            }
            break;
        case REALM_DAEMON:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_JUSTICE, -1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_FAITH, -1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_HONOUR, -1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_TEMPERANCE, -1);
            }

            break;
        case REALM_CRUSADE:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_FAITH, 1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_JUSTICE, 1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_SACRIFICE, 1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_HONOUR, 1);
            }

            break;
        case REALM_NATURE:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_NATURE, 1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_HARMONY, 1);
            }

            break;
        case REALM_HEX:
            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_JUSTICE, -1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_FAITH, -1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_HONOUR, -1);
            }

            if (randint1(100 + player_ptr->lev) < need_mana) {
                chg_virtue(player_ptr, V_COMPASSION, -1);
            }

            break;
        }

        if (any_bits(mp_ptr->spell_xtra, extra_magic_gain_exp)) {
            PlayerSkill(player_ptr).gain_spell_skill_exp(realm, spell);
        }
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    if (need_mana <= player_ptr->csp) {
        player_ptr->csp -= need_mana;
    } else {
        over_exerted = true;
    }

    player_ptr->redraw |= (PR_MANA);
    if (over_exerted) {
        int oops = need_mana;
        player_ptr->csp = 0;
        player_ptr->csp_frac = 0;
        msg_print(_("精神を集中しすぎて気を失ってしまった！", "You faint from the effort!"));
        (void)BadStatusSetter(player_ptr).mod_paralysis(randint1(5 * oops + 1));
        switch (realm) {
        case REALM_LIFE:
            chg_virtue(player_ptr, V_VITALITY, -10);
            break;
        case REALM_DEATH:
            chg_virtue(player_ptr, V_UNLIFE, -10);
            break;
        case REALM_DAEMON:
            chg_virtue(player_ptr, V_JUSTICE, 10);
            break;
        case REALM_NATURE:
            chg_virtue(player_ptr, V_NATURE, -10);
            break;
        case REALM_CRUSADE:
            chg_virtue(player_ptr, V_JUSTICE, -10);
            break;
        case REALM_HEX:
            chg_virtue(player_ptr, V_COMPASSION, 10);
            break;
        default:
            chg_virtue(player_ptr, V_KNOWLEDGE, -10);
            break;
        }

        if (randint0(100) < 50) {
            bool perm = (randint0(100) < 25);
            msg_print(_("体を悪くしてしまった！", "You have damaged your health!"));
            (void)dec_stat(player_ptr, A_CON, 15 + randint1(10), perm);
        }
    }

    player_ptr->window_flags |= (PW_PLAYER);
    player_ptr->window_flags |= (PW_SPELL);
    return true;
}
