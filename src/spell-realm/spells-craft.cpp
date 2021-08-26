#include "spell-realm/spells-craft.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "object-enchant/object-ego.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "player-info/equipment-info.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "spell/spells-object.h"
#include "sv-definition/sv-protector-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"

/*!
 * @brief 一時的元素スレイの継続時間をセットする / Set a temporary elemental brand. Clear all other brands. Print status messages. -LM-
 * @param attack_type スレイのタイプID
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_ele_attack(player_type *creature_ptr, uint32_t attack_type, TIME_EFFECT v)
{
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if ((creature_ptr->special_attack & (ATTACK_ACID)) && (attack_type != ATTACK_ACID)) {
        creature_ptr->special_attack &= ~(ATTACK_ACID);
        msg_print(_("酸で攻撃できなくなった。", "Your temporary acidic brand fades away."));
    }

    if ((creature_ptr->special_attack & (ATTACK_ELEC)) && (attack_type != ATTACK_ELEC)) {
        creature_ptr->special_attack &= ~(ATTACK_ELEC);
        msg_print(_("電撃で攻撃できなくなった。", "Your temporary electrical brand fades away."));
    }

    if ((creature_ptr->special_attack & (ATTACK_FIRE)) && (attack_type != ATTACK_FIRE)) {
        creature_ptr->special_attack &= ~(ATTACK_FIRE);
        msg_print(_("火炎で攻撃できなくなった。", "Your temporary fiery brand fades away."));
    }

    if ((creature_ptr->special_attack & (ATTACK_COLD)) && (attack_type != ATTACK_COLD)) {
        creature_ptr->special_attack &= ~(ATTACK_COLD);
        msg_print(_("冷気で攻撃できなくなった。", "Your temporary frost brand fades away."));
    }

    if ((creature_ptr->special_attack & (ATTACK_POIS)) && (attack_type != ATTACK_POIS)) {
        creature_ptr->special_attack &= ~(ATTACK_POIS);
        msg_print(_("毒で攻撃できなくなった。", "Your temporary poison brand fades away."));
    }

    if ((v) && (attack_type)) {
        creature_ptr->special_attack |= (attack_type);
        creature_ptr->ele_attack = v;
#ifdef JP
        msg_format("%sで攻撃できるようになった！",
            ((attack_type == ATTACK_ACID)
                    ? "酸"
                    : ((attack_type == ATTACK_ELEC)
                            ? "電撃"
                            : ((attack_type == ATTACK_FIRE) ? "火炎"
                                                            : ((attack_type == ATTACK_COLD) ? "冷気" : ((attack_type == ATTACK_POIS) ? "毒" : "(なし)"))))));
#else
        msg_format("For a while, the blows you deal will %s",
            ((attack_type == ATTACK_ACID)
                    ? "melt with acid!"
                    : ((attack_type == ATTACK_ELEC)
                            ? "shock your foes!"
                            : ((attack_type == ATTACK_FIRE)
                                    ? "burn with fire!"
                                    : ((attack_type == ATTACK_COLD) ? "chill to the bone!"
                                                                    : ((attack_type == ATTACK_POIS) ? "poison your enemies!" : "do nothing special."))))));
#endif
    }

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->redraw |= (PR_STATUS);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);

    return true;
}

/*!
 * @brief 一時的元素免疫の継続時間をセットする / Set a temporary elemental brand.  Clear all other brands.  Print status messages. -LM-
 * @param immune_type 免疫のタイプID
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_ele_immune(player_type *creature_ptr, uint32_t immune_type, TIME_EFFECT v)
{
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if ((creature_ptr->special_defense & (DEFENSE_ACID)) && (immune_type != DEFENSE_ACID)) {
        creature_ptr->special_defense &= ~(DEFENSE_ACID);
        msg_print(_("酸の攻撃で傷つけられるようになった。。", "You are no longer immune to acid."));
    }

    if ((creature_ptr->special_defense & (DEFENSE_ELEC)) && (immune_type != DEFENSE_ELEC)) {
        creature_ptr->special_defense &= ~(DEFENSE_ELEC);
        msg_print(_("電撃の攻撃で傷つけられるようになった。。", "You are no longer immune to electricity."));
    }

    if ((creature_ptr->special_defense & (DEFENSE_FIRE)) && (immune_type != DEFENSE_FIRE)) {
        creature_ptr->special_defense &= ~(DEFENSE_FIRE);
        msg_print(_("火炎の攻撃で傷つけられるようになった。。", "You are no longer immune to fire."));
    }

    if ((creature_ptr->special_defense & (DEFENSE_COLD)) && (immune_type != DEFENSE_COLD)) {
        creature_ptr->special_defense &= ~(DEFENSE_COLD);
        msg_print(_("冷気の攻撃で傷つけられるようになった。。", "You are no longer immune to cold."));
    }

    if ((creature_ptr->special_defense & (DEFENSE_POIS)) && (immune_type != DEFENSE_POIS)) {
        creature_ptr->special_defense &= ~(DEFENSE_POIS);
        msg_print(_("毒の攻撃で傷つけられるようになった。。", "You are no longer immune to poison."));
    }

    if ((v) && (immune_type)) {
        creature_ptr->special_defense |= (immune_type);
        creature_ptr->ele_immune = v;
        msg_format(_("%sの攻撃を受けつけなくなった！", "For a while, you are immune to %s"),
            ((immune_type == DEFENSE_ACID)
                    ? _("酸", "acid!")
                    : ((immune_type == DEFENSE_ELEC)
                            ? _("電撃", "electricity!")
                            : ((immune_type == DEFENSE_FIRE)
                                    ? _("火炎", "fire!")
                                    : ((immune_type == DEFENSE_COLD)
                                            ? _("冷気", "cold!")
                                            : ((immune_type == DEFENSE_POIS) ? _("毒", "poison!") : _("(なし)", "nothing special.")))))));
    }

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->redraw |= (PR_STATUS);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);

    return true;
}

/*
 * Choose a warrior-mage elemental attack. -LM-
 */
bool choose_ele_attack(player_type *creature_ptr)
{
    if (!has_melee_weapon(creature_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(creature_ptr, INVEN_SUB_HAND)) {
        msg_format(_("武器を持たないと魔法剣は使えない。", "You cannot use temporary branding with no weapon."));
        return false;
    }

    screen_save();
    int num = (creature_ptr->lev - 20) / 5;
    c_prt(TERM_RED, _("        a) 焼棄", "        a) Fire Brand"), 2, 14);

    if (num >= 2)
        c_prt(TERM_L_WHITE, _("        b) 凍結", "        b) Cold Brand"), 3, 14);
    else
        prt("", 3, 14);

    if (num >= 3)
        c_prt(TERM_GREEN, _("        c) 毒殺", "        c) Poison Brand"), 4, 14);
    else
        prt("", 4, 14);

    if (num >= 4)
        c_prt(TERM_L_DARK, _("        d) 溶解", "        d) Acid Brand"), 5, 14);
    else
        prt("", 5, 14);

    if (num >= 5)
        c_prt(TERM_BLUE, _("        e) 電撃", "        e) Elec Brand"), 6, 14);
    else
        prt("", 6, 14);

    prt("", 7, 14);
    prt("", 8, 14);
    prt("", 9, 14);

    prt("", 1, 0);
    prt(_("        どの元素攻撃をしますか？", "        Choose a temporary elemental brand "), 1, 14);

    char choice = inkey();

    if ((choice == 'a') || (choice == 'A'))
        set_ele_attack(creature_ptr, ATTACK_FIRE, creature_ptr->lev / 2 + randint1(creature_ptr->lev / 2));
    else if (((choice == 'b') || (choice == 'B')) && (num >= 2))
        set_ele_attack(creature_ptr, ATTACK_COLD, creature_ptr->lev / 2 + randint1(creature_ptr->lev / 2));
    else if (((choice == 'c') || (choice == 'C')) && (num >= 3))
        set_ele_attack(creature_ptr, ATTACK_POIS, creature_ptr->lev / 2 + randint1(creature_ptr->lev / 2));
    else if (((choice == 'd') || (choice == 'D')) && (num >= 4))
        set_ele_attack(creature_ptr, ATTACK_ACID, creature_ptr->lev / 2 + randint1(creature_ptr->lev / 2));
    else if (((choice == 'e') || (choice == 'E')) && (num >= 5))
        set_ele_attack(creature_ptr, ATTACK_ELEC, creature_ptr->lev / 2 + randint1(creature_ptr->lev / 2));
    else {
        msg_print(_("魔法剣を使うのをやめた。", "You cancel the temporary branding."));
        screen_load();
        return false;
    }

    screen_load();
    return true;
}
/*
 * Choose a elemental immune. -LM-
 */
bool choose_ele_immune(player_type *creature_ptr, TIME_EFFECT immune_turn)
{
    screen_save();

    c_prt(TERM_RED, _("        a) 火炎", "        a) Immunity to fire"), 2, 14);
    c_prt(TERM_L_WHITE, _("        b) 冷気", "        b) Immunity to cold"), 3, 14);
    c_prt(TERM_L_DARK, _("        c) 酸", "        c) Immunity to acid"), 4, 14);
    c_prt(TERM_BLUE, _("        d) 電撃", "        d) Immunity to elec"), 5, 14);

    prt("", 6, 14);
    prt("", 7, 14);
    prt("", 8, 14);
    prt("", 9, 14);

    prt("", 1, 0);
    prt(_("        どの元素の免疫をつけますか？", "        Choose a temporary elemental immunity "), 1, 14);

    char choice = inkey();

    if ((choice == 'a') || (choice == 'A'))
        set_ele_immune(creature_ptr, DEFENSE_FIRE, immune_turn);
    else if ((choice == 'b') || (choice == 'B'))
        set_ele_immune(creature_ptr, DEFENSE_COLD, immune_turn);
    else if ((choice == 'c') || (choice == 'C'))
        set_ele_immune(creature_ptr, DEFENSE_ACID, immune_turn);
    else if ((choice == 'd') || (choice == 'D'))
        set_ele_immune(creature_ptr, DEFENSE_ELEC, immune_turn);
    else {
        msg_print(_("免疫を付けるのをやめた。", "You cancel the temporary immunity."));
        screen_load();
        return false;
    }

    screen_load();
    return true;
}

/*!
 * @brief 盾磨き処理 /
 * pulish shield
 * @return ターン消費を要する処理を行ったならばTRUEを返す
 */
bool pulish_shield(player_type *caster_ptr)
{
    concptr q = _("どの盾を磨きますか？", "Polish which shield? ");
    concptr s = _("磨く盾がありません。", "You have no shield to polish.");

    OBJECT_IDX item;
    object_type *o_ptr = choose_object(caster_ptr, &item, q, s, USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT, TV_SHIELD);
    if (o_ptr == NULL)
        return false;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, OD_OMIT_PREFIX | OD_NAME_ONLY);
    TrFlags flgs;
    object_flags(o_ptr, flgs);

    bool is_pulish_successful = o_ptr->k_idx && !object_is_artifact(o_ptr) && !object_is_ego(o_ptr);
    is_pulish_successful &= !object_is_cursed(o_ptr);
    is_pulish_successful &= (o_ptr->sval != SV_MIRROR_SHIELD);
    if (is_pulish_successful) {
#ifdef JP
        msg_format("%sは輝いた！", o_name);
#else
        msg_format("%s %s shine%s!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif
        o_ptr->name2 = EGO_REFLECTION;
        enchant_equipment(caster_ptr, o_ptr, randint0(3) + 4, ENCH_TOAC);
        o_ptr->discount = 99;
        chg_virtue(caster_ptr, V_ENCHANT, 2);
        return true;
    }

    if (flush_failure)
        flush();

    msg_print(_("失敗した。", "Failed."));
    chg_virtue(caster_ptr, V_ENCHANT, -2);
    calc_android_exp(caster_ptr);
    return false;
}
