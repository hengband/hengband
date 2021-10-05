/*!
 * @brief プレイヤーの食べるコマンド実装
 * @date 2018/09/07
 @ @author deskull
 */

#include "cmd-item/cmd-eat.h"
#include "avatar/avatar.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "hpmp/hp-mp-processor.h"
#include "inventory/inventory-object.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-expendable.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-other-types.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

/*!
 * @brief 食料タイプの食料を食べたときの効果を発動
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr 食べるオブジェクト
 * @return 鑑定されるならTRUE、されないならFALSE
 */
bool exe_eat_food_type_object(player_type *player_ptr, object_type *o_ptr)
{
    if (o_ptr->tval != TV_FOOD)
        return false;

    BadStatusSetter bss(player_ptr);
    switch (o_ptr->sval) {
    case SV_FOOD_POISON:
        return (!(has_resist_pois(player_ptr) || is_oppose_pois(player_ptr))) && bss.mod_poison(randint0(10) + 10);
    case SV_FOOD_BLINDNESS:
        return !has_resist_blind(player_ptr) && bss.mod_blindness(randint0(200) + 200);
    case SV_FOOD_PARANOIA:
        return !has_resist_fear(player_ptr) && bss.mod_afraidness(randint0(10) + 10);
    case SV_FOOD_CONFUSION:
        return !has_resist_conf(player_ptr) && bss.mod_confusion(randint0(10) + 10);
    case SV_FOOD_HALLUCINATION:
        return !has_resist_chaos(player_ptr) && bss.mod_hallucination(randint0(250) + 250);
    case SV_FOOD_PARALYSIS:
        return !player_ptr->free_act && bss.mod_paralysis(randint0(10) + 10);
    case SV_FOOD_WEAKNESS:
        take_hit(player_ptr, DAMAGE_NOESCAPE, damroll(6, 6), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(player_ptr, A_STR);
        return true;
    case SV_FOOD_SICKNESS:
        take_hit(player_ptr, DAMAGE_NOESCAPE, damroll(6, 6), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(player_ptr, A_CON);
        return true;
    case SV_FOOD_STUPIDITY:
        take_hit(player_ptr, DAMAGE_NOESCAPE, damroll(8, 8), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(player_ptr, A_INT);
        return true;
    case SV_FOOD_NAIVETY:
        take_hit(player_ptr, DAMAGE_NOESCAPE, damroll(8, 8), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(player_ptr, A_WIS);
        return true;
    case SV_FOOD_UNHEALTH:
        take_hit(player_ptr, DAMAGE_NOESCAPE, damroll(10, 10), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(player_ptr, A_CON);
        return true;
    case SV_FOOD_DISEASE:
        take_hit(player_ptr, DAMAGE_NOESCAPE, damroll(10, 10), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(player_ptr, A_STR);
        return true;
    case SV_FOOD_CURE_POISON:
        return bss.poison(0);
    case SV_FOOD_CURE_BLINDNESS:
        return bss.blindness(0);
    case SV_FOOD_CURE_PARANOIA:
        return bss.afraidness(0);
    case SV_FOOD_CURE_CONFUSION:
        return bss.confusion(0);
    case SV_FOOD_CURE_SERIOUS:
        return cure_serious_wounds(player_ptr, 4, 8);
    case SV_FOOD_RESTORE_STR:
        return do_res_stat(player_ptr, A_STR);
    case SV_FOOD_RESTORE_CON:
        return do_res_stat(player_ptr, A_CON);
    case SV_FOOD_RESTORING:
        return restore_all_status(player_ptr);
    case SV_FOOD_BISCUIT:
        msg_print(_("甘くてサクサクしてとてもおいしい。", "That is sweetly and shortly delicious."));
        return true;
    case SV_FOOD_JERKY:
        msg_print(_("歯ごたえがあっておいしい。", "That is chewy and delicious."));
        return true;
    case SV_FOOD_SLIME_MOLD:
        msg_print(_("これはなんとも形容しがたい味だ。", "That is an indescribable taste."));
        return true;
    case SV_FOOD_RATION:
        msg_print(_("これはおいしい。", "That tastes good."));
        return true;
    case SV_FOOD_WAYBREAD:
        msg_print(_("これはひじょうに美味だ。", "That tastes very good."));
        (void)bss.poison(0);
        (void)hp_player(player_ptr, damroll(4, 8));
        return true;
    case SV_FOOD_PINT_OF_ALE:
    case SV_FOOD_PINT_OF_WINE:
        msg_print(_("のどごし爽やかだ。", "That is refreshing through the throat."));
        return true;
    default:
        return false;
    }
}

/*!
 * @brief 魔法道具のチャージをの食料として食べたときの効果を発動
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr 食べるオブジェクト
 * @param item オブジェクトのインベントリ番号
 * @return 食べようとしたらTRUE、しなかったらFALSE
 */
bool exe_eat_charge_of_magic_device(player_type *player_ptr, object_type *o_ptr, INVENTORY_IDX item)
{
    if (o_ptr->tval != TV_STAFF && o_ptr->tval != TV_WAND)
        return false;

    if (PlayerRace(player_ptr).food() == PlayerRaceFood::MANA) {
        concptr staff;

        if (o_ptr->tval == TV_STAFF && (item < 0) && (o_ptr->number > 1)) {
            msg_print(_("まずは杖を拾わなければ。", "You must first pick up the staffs."));
            return true;
        }

        staff = (o_ptr->tval == TV_STAFF) ? _("杖", "staff") : _("魔法棒", "wand");

        /* "Eat" charges */
        if (o_ptr->pval == 0) {
            msg_format(_("この%sにはもう魔力が残っていない。", "The %s has no charges left."), staff);
            o_ptr->ident |= (IDENT_EMPTY);
            player_ptr->window_flags |= (PW_INVEN);
            return true;
        }

        msg_format(_("あなたは%sの魔力をエネルギー源として吸収した。", "You absorb mana of the %s as your energy."), staff);

        /* Use a single charge */
        o_ptr->pval--;

        /* Eat a charge */
        set_food(player_ptr, player_ptr->food + 5000);

        /* XXX Hack -- unstack if necessary */
        if (o_ptr->tval == TV_STAFF && (item >= 0) && (o_ptr->number > 1)) {
            object_type forge;
            object_type *q_ptr;
            q_ptr = &forge;
            q_ptr->copy_from(o_ptr);

            /* Modify quantity */
            q_ptr->number = 1;

            /* Restore the charges */
            o_ptr->pval++;

            /* Unstack the used item */
            o_ptr->number--;
            item = store_item_to_inventory(player_ptr, q_ptr);

            msg_format(_("杖をまとめなおした。", "You unstack your staff."));
        }

        if (item >= 0) {
            inven_item_charges(player_ptr, item);
        } else {
            floor_item_charges(player_ptr->current_floor_ptr, 0 - item);
        }

        player_ptr->window_flags |= (PW_INVEN | PW_EQUIP);
        return true;
    }

    return false;
}

/*!
 * @brief 食料を食べるコマンドのサブルーチン
 * @param item 食べるオブジェクトの所持品ID
 */
void exe_eat_food(player_type *player_ptr, INVENTORY_IDX item)
{
    if (music_singing_any(player_ptr))
        stop_singing(player_ptr);

    SpellHex spell_hex(player_ptr);
    if (spell_hex.is_spelling_any()) {
        (void)spell_hex.stop_all_spells();
    }

    object_type *o_ptr = ref_item(player_ptr, item);

    sound(SOUND_EAT);

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    /* Object level */
    int lev = k_info[o_ptr->k_idx].level;

    /* Identity not known yet */
    int ident = exe_eat_food_type_object(player_ptr, o_ptr);

    /*
     * Store what may have to be updated for the inventory (including
     * autodestroy if set by something else).  Then turn off those flags
     * so that updates triggered by calling gain_exp() or set_food() below
     * do not rearrange the inventory before the food item is destroyed in
     * the pack.
     */
    BIT_FLAGS inventory_flags = (PU_COMBINE | PU_REORDER | (player_ptr->update & PU_AUTODESTROY));
    player_ptr->update &= ~(PU_COMBINE | PU_REORDER | PU_AUTODESTROY);

    if (!(o_ptr->is_aware())) {
        chg_virtue(player_ptr, V_KNOWLEDGE, -1);
        chg_virtue(player_ptr, V_PATIENCE, -1);
        chg_virtue(player_ptr, V_CHANCE, 1);
    }

    /* We have tried it */
    if (o_ptr->tval == TV_FOOD)
        object_tried(o_ptr);

    /* The player is now aware of the object */
    if (ident && !o_ptr->is_aware()) {
        object_aware(player_ptr, o_ptr);
        gain_exp(player_ptr, (lev + (player_ptr->lev >> 1)) / player_ptr->lev);
    }

    player_ptr->window_flags |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

    /* Undeads drain recharge of magic device */
    if (exe_eat_charge_of_magic_device(player_ptr, o_ptr, item)) {
        player_ptr->update |= inventory_flags;
        return;
    }

    auto food_type = PlayerRace(player_ptr).food();

    /* Balrogs change humanoid corpses to energy */
    if (food_type == PlayerRaceFood::CORPSE && (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_CORPSE && angband_strchr("pht", r_info[o_ptr->pval].d_char))) {
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        msg_format(_("%sは燃え上り灰になった。精力を吸収した気がする。", "%^s is burnt to ashes.  You absorb its vitality!"), o_name);
        (void)set_food(player_ptr, PY_FOOD_MAX - 1);

        player_ptr->update |= inventory_flags;
        vary_item(player_ptr, item, -1);
        return;
    }

    if (PlayerRace(player_ptr).equals(PlayerRaceType::SKELETON)) {
        if (!((o_ptr->sval == SV_FOOD_WAYBREAD) || (o_ptr->sval < SV_FOOD_BISCUIT))) {
            object_type forge;
            object_type *q_ptr = &forge;

            msg_print(_("食べ物がアゴを素通りして落ちた！", "The food falls through your jaws!"));
            q_ptr->prep(lookup_kind(o_ptr->tval, o_ptr->sval));

            /* Drop the object from heaven */
            (void)drop_near(player_ptr, q_ptr, -1, player_ptr->y, player_ptr->x);
        } else {
            msg_print(_("食べ物がアゴを素通りして落ち、消えた！", "The food falls through your jaws and vanishes!"));
        }
    } else if (food_type == PlayerRaceFood::BLOOD) {
        /* Vampires are filled only by bloods, so reduced nutritional benefit */
        (void)set_food(player_ptr, player_ptr->food + (o_ptr->pval / 10));
        msg_print(_("あなたのような者にとって食糧など僅かな栄養にしかならない。", "Mere victuals hold scant sustenance for a being such as yourself."));

        if (player_ptr->food < PY_FOOD_ALERT) /* Hungry */
            msg_print(_("あなたの飢えは新鮮な血によってのみ満たされる！", "Your hunger can only be satisfied with fresh blood!"));
    } else if (food_type == PlayerRaceFood::WATER) {
        msg_print(_("動物の食物はあなたにとってほとんど栄養にならない。", "The food of animals is poor sustenance for you."));
        set_food(player_ptr, player_ptr->food + ((o_ptr->pval) / 20));
    } else if (food_type != PlayerRaceFood::RATION) {
        msg_print(_("生者の食物はあなたにとってほとんど栄養にならない。", "The food of mortals is poor sustenance for you."));
        set_food(player_ptr, player_ptr->food + ((o_ptr->pval) / 20));
    } else {
        if (o_ptr->tval == TV_FOOD && o_ptr->sval == SV_FOOD_WAYBREAD) {
            /* Waybread is always fully satisfying. */
            set_food(player_ptr, MAX(player_ptr->food, PY_FOOD_MAX - 1));
        } else {
            /* Food can feed the player */
            (void)set_food(player_ptr, player_ptr->food + o_ptr->pval);
        }
    }

    player_ptr->update |= inventory_flags;
    vary_item(player_ptr, item, -1);
}

/*!
 * @brief 食料を食べるコマンドのメインルーチン /
 * Eat some food (from the pack or floor)
 */
void do_cmd_eat_food(player_type *player_ptr)
{
    OBJECT_IDX item;
    concptr q, s;

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStance::MUSOU, SamuraiStance::KOUKIJIN });

    q = _("どれを食べますか? ", "Eat which item? ");
    s = _("食べ物がない。", "You have nothing to eat.");

    if (!choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(item_tester_hook_eatable, player_ptr)))
        return;

    exe_eat_food(player_ptr, item);
}
