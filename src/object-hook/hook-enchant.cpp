#include "object-hook/hook-enchant.h"
#include "mind/mind-weaponsmith.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief オブジェクトがアーティファクトかを返す /
 * Check if an object is artifact
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return アーティファクトならばTRUEを返す
 */
bool object_is_artifact(const object_type *o_ptr)
{
    return object_is_fixed_artifact(o_ptr) || (o_ptr->art_name != 0);
}

/*!
 * @brief オブジェクトがランダムアーティファクトかを返す /
 * Check if an object is random artifact
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return ランダムアーティファクトならばTRUEを返す
 */
bool object_is_random_artifact(const object_type *o_ptr)
{
    return object_is_artifact(o_ptr) && !object_is_fixed_artifact(o_ptr);
}

/*!
 * @brief オブジェクトが通常のアイテム(アーティファクト、エゴ、鍛冶師エッセンス付加いずれでもない)かを返す /
 * Check if an object is neither artifact, ego, nor 'smith' object
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 通常のアイテムならばTRUEを返す
 */
bool object_is_nameless(const object_type *o_ptr)
{
    return !object_is_artifact(o_ptr) && !o_ptr->is_ego() && !object_is_smith(o_ptr);
}

/*
 * Artifacts use the "name1" field
 */
bool object_is_fixed_artifact(const object_type *o_ptr)
{
    return o_ptr->name1 != 0;
}
