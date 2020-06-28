/*!
 * @brief オブジェクトに関する汎用判定処理
 * @date 2018/09/24
 * @author deskull
 */

#include "object/object-hook.h"
#include "art-definition/art-accessory-types.h"
#include "art-definition/art-armor-types.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/quest.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "object-enchant/artifact.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "realm/realm-names-table.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "world/world.h"

/*
 * Used during calls to "get_item()" and "show_inven()" and "show_equip()", and the choice window routines.
 */
bool (*item_tester_hook)(player_type *, object_type *);

/*!
 * @brief エッセンスの付加可能な武器や矢弾かを返す
 * @param o_ptr チェックしたいオブジェクトの構造体参照ポインタ
 * @return エッセンスの付加可能な武器か矢弾ならばTRUEを返す。
 */
bool item_tester_hook_melee_ammo(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    switch (o_ptr->tval) {
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_DIGGING:
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT: {
        return TRUE;
    }
    case TV_SWORD: {
        if (o_ptr->sval != SV_POISON_NEEDLE)
            return TRUE;
    }
    }

    return FALSE;
}

/*!
 * @brief 呪術領域の武器呪縛の対象にできる武器かどうかを返す。 / An "item_tester_hook" for offer
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @return 呪縛可能な武器ならばTRUEを返す
 */
bool item_tester_hook_weapon_except_bow(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    switch (o_ptr->tval) {
    case TV_SWORD:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_DIGGING: {
        return TRUE;
    }
    }

    return FALSE;
}

/*!
 * @brief オブジェクトが賞金首の報酬対象になるかを返す
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが報酬対象になるならTRUEを返す
 */
bool object_is_bounty(player_type *player_ptr, object_type *o_ptr)
{
    int i;

    /* Require corpse or skeleton */
    if (o_ptr->tval != TV_CORPSE)
        return FALSE;

    /* No wanted monsters in vanilla town */
    if (vanilla_town)
        return FALSE;

    /* Today's wanted */
    if (player_ptr->today_mon > 0 && (streq(r_name + r_info[o_ptr->pval].name, r_name + r_info[today_mon].name)))
        return TRUE;

    /* Tsuchinoko */
    if (o_ptr->pval == MON_TSUCHINOKO)
        return TRUE;

    /* Unique monster */
    for (i = 0; i < MAX_BOUNTY; i++)
        if (o_ptr->pval == current_world_ptr->bounty_r_idx[i])
            break;
    if (i < MAX_BOUNTY)
        return TRUE;

    /* Not wanted */
    return FALSE;
}

/*!
 * @brief オブジェクトがレアアイテムかどうかを返す /
 * Rare weapons/aromors including Blade of Chaos, Dragon armors, etc.
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return レアアイテムならばTRUEを返す
 */
bool object_is_rare(object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_HAFTED:
        if (o_ptr->sval == SV_MACE_OF_DISRUPTION || o_ptr->sval == SV_WIZSTAFF)
            return TRUE;
        break;

    case TV_POLEARM:
        if (o_ptr->sval == SV_SCYTHE_OF_SLICING || o_ptr->sval == SV_DEATH_SCYTHE)
            return TRUE;
        break;

    case TV_SWORD:
        if (o_ptr->sval == SV_BLADE_OF_CHAOS || o_ptr->sval == SV_DIAMOND_EDGE || o_ptr->sval == SV_POISON_NEEDLE || o_ptr->sval == SV_HAYABUSA)
            return TRUE;
        break;

    case TV_SHIELD:
        if (o_ptr->sval == SV_DRAGON_SHIELD || o_ptr->sval == SV_MIRROR_SHIELD)
            return TRUE;
        break;

    case TV_HELM:
        if (o_ptr->sval == SV_DRAGON_HELM)
            return TRUE;
        break;

    case TV_BOOTS:
        if (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)
            return TRUE;
        break;

    case TV_CLOAK:
        if (o_ptr->sval == SV_ELVEN_CLOAK || o_ptr->sval == SV_ETHEREAL_CLOAK || o_ptr->sval == SV_SHADOW_CLOAK)
            return TRUE;
        break;

    case TV_GLOVES:
        if (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES)
            return TRUE;
        break;

    case TV_SOFT_ARMOR:
        if (o_ptr->sval == SV_KUROSHOUZOKU || o_ptr->sval == SV_ABUNAI_MIZUGI)
            return TRUE;
        break;

    case TV_DRAG_ARMOR:
        return TRUE;

    default:
        break;
    }

    /* Any others are not "rare" objects. */
    return FALSE;
}

/*!
 * @brief オブジェクトが鍛冶師のエッセンス付加済みかを返す /
 * Check if an object is made by a smith's special ability
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return エッセンス付加済みならばTRUEを返す
 */
bool object_is_smith(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if (object_is_weapon_armour_ammo(player_ptr, o_ptr) && o_ptr->xtra3)
        return TRUE;

    return FALSE;
}

/*!
 * @brief オブジェクトがアーティファクトかを返す /
 * Check if an object is artifact
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return アーティファクトならばTRUEを返す
 */
bool object_is_artifact(object_type *o_ptr)
{
    if (object_is_fixed_artifact(o_ptr) || o_ptr->art_name)
        return TRUE;

    return FALSE;
}

/*!
 * @brief オブジェクトがランダムアーティファクトかを返す /
 * Check if an object is random artifact
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return ランダムアーティファクトならばTRUEを返す
 */
bool object_is_random_artifact(object_type *o_ptr)
{
    if (object_is_artifact(o_ptr) && !object_is_fixed_artifact(o_ptr))
        return TRUE;

    return FALSE;
}

/*!
 * @brief オブジェクトが通常のアイテム(アーティファクト、エゴ、鍛冶師エッセンス付加いずれでもない)かを返す /
 * Check if an object is neither artifact, ego, nor 'smith' object
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 通常のアイテムならばTRUEを返す
 */
bool object_is_nameless(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if (!object_is_artifact(o_ptr) && !object_is_ego(o_ptr) && !object_is_smith(player_ptr, o_ptr))
        return TRUE;

    return FALSE;
}

/*!
 * @brief オブジェクトがクエストの達成目的か否かを返す。
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @return 現在クエスト達成目的のアイテムならばTRUEを返す。
 */
bool object_is_quest_target(player_type *player_ptr, object_type *o_ptr)
{
    if (player_ptr->current_floor_ptr->inside_quest) {
        ARTIFACT_IDX a_idx = quest[player_ptr->current_floor_ptr->inside_quest].k_idx;
        if (a_idx) {
            artifact_type *a_ptr = &a_info[a_idx];
            if (!(a_ptr->gen_flags & TRG_INSTA_ART)) {
                if ((o_ptr->tval == a_ptr->tval) && (o_ptr->sval == a_ptr->sval)) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/*!
 * @brief アイテムがitem_tester_hookグローバル関数ポインタの条件を満たしているかを返す汎用関数
 * Check an item against the item tester info
 * @param o_ptr 判定を行いたいオブジェクト構造体参照ポインタ
 * @return item_tester_hookの参照先、その他いくつかの例外に応じてTRUE/FALSEを返す。
 */
bool item_tester_okay(player_type *player_ptr, object_type *o_ptr, tval_type tval)
{
    /* Require an item */
    if (!o_ptr->k_idx)
        return FALSE;

    /* Hack -- ignore "gold" */
    if (o_ptr->tval == TV_GOLD) {
        /* See xtra2.c */
        extern bool show_gold_on_floor;

        if (!show_gold_on_floor)
            return FALSE;
    }

    /* Check the tval */
    if (tval) {
        /* Is it a spellbook? If so, we need a hack -- TY */
        if ((tval <= TV_DEATH_BOOK) && (tval >= TV_LIFE_BOOK))
            return check_book_realm(player_ptr, o_ptr->tval, o_ptr->sval);
        else if (tval != o_ptr->tval)
            return FALSE;
    }

    /* Check the hook */
    if (item_tester_hook) {
        if (!(*item_tester_hook)(player_ptr, o_ptr))
            return FALSE;
    }

    /* Assume okay */
    return TRUE;
}

/*
 * Artifacts use the "name1" field
 */
bool object_is_fixed_artifact(object_type *o_ptr) { return o_ptr->name1 != 0; }

/*
 * Ego-Items use the "name2" field
 */
bool object_is_ego(object_type *o_ptr) { return o_ptr->name2 != 0; }
