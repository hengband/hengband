/*
 * The values for the "tval" field of various objects.
 *
 * This value is the primary means by which items are sorted in the
 * player inventory_list, followed by "sval" and "cost".
 *
 * Note that a "BOW" with tval = 19 and sval S = 10*N+P takes a missile
 * weapon with tval = 16+N, and does (xP) damage when so combined.  This
 * fact is not actually used in the source, but it kind of interesting.
 *
 * Note that as of 2.7.8, the "item flags" apply to all items, though
 * only armor and weapons and a few other items use any of these flags.
 */

#pragma once

#include "util/enum-converter.h"

enum class ItemKindType : short {
    NONE = 0,
    SKELETON = 1, /* Skeletons ('s'), not specified */
    BOTTLE = 2, /* Empty bottles ('!') */
    JUNK = 3, /* Sticks, Pottery, etc ('~') */
    WHISTLE = 4, /* Whistle ('~') */
    SPIKE = 5, /* Spikes ('~') */
    CHEST = 7, /* Chests ('&') */
    FIGURINE = 8, /* Magical figurines */
    STATUE = 9, /* Statue, what a silly object... */
    CORPSE = 10, /* Corpses and Skeletons, specific */
    CAPTURE = 11, /* Monster ball */
    NO_AMMO = 15, /* Ammo for crimson */
    SHOT = 16, /* Ammo for slings */
    ARROW = 17, /* Ammo for bows */
    BOLT = 18, /* Ammo for x-bows */
    BOW = 19, /* Slings/Bows/Xbows */
    DIGGING = 20, /* Shovels/Picks */
    HAFTED = 21, /* Priest Weapons */
    POLEARM = 22, /* Axes and Pikes */
    SWORD = 23, /* Edged Weapons */
    BOOTS = 30, /* Boots */
    GLOVES = 31, /* Gloves */
    HELM = 32, /* Helms */
    CROWN = 33, /* Crowns */
    SHIELD = 34, /* Shields */
    CLOAK = 35, /* Cloaks */
    SOFT_ARMOR = 36, /* Soft Armor */
    HARD_ARMOR = 37, /* Hard Armor */
    DRAG_ARMOR = 38, /* Dragon Scale Mail */
    LITE = 39, /* Lites (including Specials) */
    AMULET = 40, /* Amulets (including Specials) */
    RING = 45, /* Rings (including Specials) */
    CARD = 50,
    STAFF = 55,
    WAND = 65,
    ROD = 66,
    PARCHMENT = 69,
    SCROLL = 70,
    POTION = 75,
    FLASK = 77,
    FOOD = 80,
    LIFE_BOOK = 90,
    SORCERY_BOOK = 91,
    NATURE_BOOK = 92,
    CHAOS_BOOK = 93,
    DEATH_BOOK = 94,
    TRUMP_BOOK = 95,
    ARCANE_BOOK = 96,
    CRAFT_BOOK = 97,
    DEMON_BOOK = 98,
    CRUSADE_BOOK = 99,
    MUSIC_BOOK = 105,
    HISSATSU_BOOK = 106,
    HEX_BOOK = 107,
    GOLD = 127, /* Gold can only be picked up by players */
};

#define TV_EQUIP_BEGIN ItemKindType::SHOT
#define TV_EQUIP_END ItemKindType::CARD
#define TV_MISSILE_BEGIN ItemKindType::SHOT
#define TV_MISSILE_END ItemKindType::BOLT
#define TV_WEARABLE_BEGIN ItemKindType::BOW
#define TV_WEARABLE_END ItemKindType::CARD
#define TV_WEAPON_BEGIN ItemKindType::BOW
#define TV_WEAPON_END ItemKindType::SWORD
#define TV_ARMOR_BEGIN ItemKindType::BOOTS
#define TV_ARMOR_END ItemKindType::DRAG_ARMOR
