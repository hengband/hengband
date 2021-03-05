﻿#include "object-hook/hook-enchant.h"
#include "mind/mind-weaponsmith.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"

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
 * @brief オブジェクトがアーティファクトかを返す /
 * Check if an object is artifact
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return アーティファクトならばTRUEを返す
 */
bool object_is_artifact(object_type *o_ptr) { return object_is_fixed_artifact(o_ptr) || (o_ptr->art_name != 0); }

/*!
 * @brief オブジェクトがランダムアーティファクトかを返す /
 * Check if an object is random artifact
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return ランダムアーティファクトならばTRUEを返す
 */
bool object_is_random_artifact(object_type *o_ptr) { return object_is_artifact(o_ptr) && !object_is_fixed_artifact(o_ptr); }

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

    return !object_is_artifact(o_ptr) && !object_is_ego(o_ptr) && !object_is_smith(player_ptr, o_ptr);
}

/*
 * Artifacts use the "name1" field
 */
bool object_is_fixed_artifact(object_type *o_ptr) { return o_ptr->name1 != 0; }

/*
 * Ego-Items use the "name2" field
 */
bool object_is_ego(object_type *o_ptr) { return o_ptr->name2 != 0; }
