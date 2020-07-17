#include "store/service-checker.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags3.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "object/object-value.h"
#include "store/store-util.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"

/*!
 * @brief オブジェクトが祝福されているかの判定を返す /
 * @param o_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが祝福されたアイテムならばTRUEを返す
 */
static bool is_blessed_item(player_type *player_ptr, object_type *o_ptr)
{
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(player_ptr, o_ptr, flgs);
    return have_flag(flgs, TR_BLESSED);
}

static bool check_store_general(object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_POTION:
        if (o_ptr->sval != SV_POTION_WATER)
            return FALSE;
    case TV_WHISTLE:
    case TV_FOOD:
    case TV_LITE:
    case TV_FLASK:
    case TV_SPIKE:
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
    case TV_DIGGING:
    case TV_CLOAK:
    case TV_BOTTLE:
    case TV_FIGURINE:
    case TV_STATUE:
    case TV_CAPTURE:
    case TV_CARD:
        return TRUE;
    default:
        return FALSE;
    }
}

static bool check_store_armoury(object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_CROWN:
    case TV_HELM:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
        return TRUE;
    default:
        return FALSE;
    }
}

static bool check_store_weapon(object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_SHOT:
    case TV_BOLT:
    case TV_ARROW:
    case TV_BOW:
    case TV_DIGGING:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_HISSATSU_BOOK:
        return TRUE;
    case TV_HAFTED:
        return o_ptr->sval != SV_WIZSTAFF;
    default:
        return FALSE;
    }
}

static bool check_store_temple(player_type *player_ptr, object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_LIFE_BOOK:
    case TV_CRUSADE_BOOK:
    case TV_SCROLL:
    case TV_POTION:
    case TV_HAFTED:
        return TRUE;
    case TV_FIGURINE:
    case TV_STATUE: {
        monster_race *r_ptr = &r_info[o_ptr->pval];
        if (!(r_ptr->flags3 & RF3_EVIL))
            if (((r_ptr->flags3 & RF3_GOOD) != 0) || ((r_ptr->flags3 & RF3_ANIMAL) != 0) || (angband_strchr("?!", r_ptr->d_char) != '\0'))
                return TRUE;
    }
        /* Fall through */
    case TV_POLEARM:
    case TV_SWORD:
        if (is_blessed_item(player_ptr, o_ptr))
            return TRUE;

        /* Fall through */
    default:
        return FALSE;
    }
}

static bool check_store_alchemist(object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_SCROLL:
    case TV_POTION:
        return TRUE;
    default:
        return FALSE;
    }
}

static bool check_store_magic(object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_SORCERY_BOOK:
    case TV_NATURE_BOOK:
    case TV_CHAOS_BOOK:
    case TV_DEATH_BOOK:
    case TV_TRUMP_BOOK:
    case TV_ARCANE_BOOK:
    case TV_CRAFT_BOOK:
    case TV_DEMON_BOOK:
    case TV_MUSIC_BOOK:
    case TV_HEX_BOOK:
    case TV_AMULET:
    case TV_RING:
    case TV_STAFF:
    case TV_WAND:
    case TV_ROD:
    case TV_SCROLL:
    case TV_POTION:
    case TV_FIGURINE:
        return TRUE;
    case TV_HAFTED:
        return o_ptr->sval == SV_WIZSTAFF;
    default:
        return FALSE;
    }
}

static bool check_store_book(object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_SORCERY_BOOK:
    case TV_NATURE_BOOK:
    case TV_CHAOS_BOOK:
    case TV_DEATH_BOOK:
    case TV_LIFE_BOOK:
    case TV_TRUMP_BOOK:
    case TV_ARCANE_BOOK:
    case TV_CRAFT_BOOK:
    case TV_DEMON_BOOK:
    case TV_CRUSADE_BOOK:
    case TV_MUSIC_BOOK:
    case TV_HEX_BOOK:
        return TRUE;
    default:
        return FALSE;
    }
}

/*!
 * @brief オブジェクトが所定の店舗で引き取れるかどうかを返す /
 * Determine if the current store will purchase the given item
 * @param o_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが買い取れるならばTRUEを返す
 * @note
 * Note that a shop-keeper must refuse to buy "worthless" items
 */
bool store_will_buy(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;
    if ((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM))
        return TRUE;

    switch (cur_store_num) {
    case STORE_GENERAL:
        if (!check_store_general(o_ptr))
            return FALSE;

        break;
    case STORE_ARMOURY:
        if (!check_store_armoury(o_ptr))
            return FALSE;

        break;
    case STORE_WEAPON:
        if (!check_store_weapon(o_ptr))
            return FALSE;

        break;
    case STORE_TEMPLE:
        if (!check_store_temple(player_ptr, o_ptr))
            return FALSE;

        break;
    case STORE_ALCHEMIST:
        if (!check_store_alchemist(o_ptr))
            return FALSE;

        break;
    case STORE_MAGIC:
        if (!check_store_magic(o_ptr))
            return FALSE;

        break;
    case STORE_BOOK:
        if (!check_store_book(o_ptr))
            return FALSE;

        break;
    }

    return !object_value(player_ptr, o_ptr) <= 0;
}
