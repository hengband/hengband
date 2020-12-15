#include "object-hook/hook-expendable.h"
#include "artifact/fixed-art-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "monster-race/monster-race.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player/mimic-info-table.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "system/object-type-definition.h"
#include "util/string-processor.h"

/*!
 * @brief オブジェクトをプレイヤーが食べることができるかを判定する /
 * Hook to determine if an object is eatable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 食べることが可能ならばTRUEを返す
 */
bool item_tester_hook_eatable(player_type *player_ptr, object_type *o_ptr)
{
    if (o_ptr->tval == TV_FOOD)
        return TRUE;

    if (is_specific_player_race(player_ptr, RACE_SKELETON) || is_specific_player_race(player_ptr, RACE_GOLEM)
        || is_specific_player_race(player_ptr, RACE_ZOMBIE) || is_specific_player_race(player_ptr, RACE_SPECTRE)) {
        if (o_ptr->tval == TV_STAFF || o_ptr->tval == TV_WAND)
            return TRUE;
    } else if (is_specific_player_race(player_ptr, RACE_BALROG) || (mimic_info[player_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_DEMON)) {
        if (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_CORPSE && angband_strchr("pht", r_info[o_ptr->pval].d_char))
            return TRUE;
    }

    return FALSE;
}

/*!
 * @brief オブジェクトをプレイヤーが飲むことができるかを判定する /
 * Hook to determine if an object can be quaffed
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 飲むことが可能ならばTRUEを返す
 */
bool item_tester_hook_quaff(player_type *player_ptr, object_type *o_ptr)
{
    if (o_ptr->tval == TV_POTION)
        return TRUE;

    if (is_specific_player_race(player_ptr, RACE_ANDROID) && (o_ptr->tval == TV_FLASK) && (o_ptr->sval == SV_FLASK_OIL))
        return TRUE;

    return FALSE;
}

/*!
 * @brief オブジェクトをプレイヤーが読むことができるかを判定する /
 * Hook to determine if an object is readable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 読むことが可能ならばTRUEを返す
 */
bool item_tester_hook_readable(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if ((o_ptr->tval == TV_SCROLL) || (o_ptr->tval == TV_PARCHMENT) || (o_ptr->name1 == ART_GHB) || (o_ptr->name1 == ART_POWER))
        return TRUE;

    return FALSE;
}

/*!
 * @brief オブジェクトがランタンの燃料になるかどうかを判定する
 * An "item_tester_hook" for refilling lanterns
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトがランタンの燃料になるならばTRUEを返す
 */
bool item_tester_refill_lantern(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if ((o_ptr->tval == TV_FLASK) || ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_LANTERN)))
        return TRUE;

    return FALSE;
}

/*!
 * @brief オブジェクトが松明に束ねられるかどうかを判定する
 * An "item_tester_hook" for refilling torches
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトが松明に束ねられるならばTRUEを返す
 */
bool object_can_refill_torch(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_TORCH))
        return TRUE;

    return FALSE;
}

/*!
 * @brief 破壊可能なアイテムかを返す /
 * Determines whether an object can be destroyed, and makes fake inscription.
 * @param o_ptr 破壊可能かを確認したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトが破壊可能ならばTRUEを返す
 */
bool can_player_destroy_object(player_type *player_ptr, object_type *o_ptr)
{
    /* Artifacts cannot be destroyed */
    if (!object_is_artifact(o_ptr))
        return TRUE;

    if (!object_is_known(o_ptr)) {
        byte feel = FEEL_SPECIAL;
        if (object_is_cursed(o_ptr) || object_is_broken(o_ptr))
            feel = FEEL_TERRIBLE;

        o_ptr->feeling = feel;
        o_ptr->ident |= IDENT_SENSE;
        player_ptr->update |= (PU_COMBINE);
        player_ptr->window |= (PW_INVEN | PW_EQUIP);
        return FALSE;
    }

    return FALSE;
}

/*!
 * @brief オブジェクトが薬であるかを返す
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが薬ならばTRUEを返す
 */
bool object_is_potion(object_type *o_ptr) { return (k_info[o_ptr->k_idx].tval == TV_POTION); }
