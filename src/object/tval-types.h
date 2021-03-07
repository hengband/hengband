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

enum tval_type {
    TV_NONE = 0,
    TV_SKELETON = 1, /* Skeletons ('s'), not specified */
    TV_BOTTLE = 2, /* Empty bottles ('!') */
    TV_JUNK = 3, /* Sticks, Pottery, etc ('~') */
    TV_WHISTLE = 4, /* Whistle ('~') */
    TV_SPIKE = 5, /* Spikes ('~') */
    TV_CHEST = 7, /* Chests ('&') */
    TV_FIGURINE = 8, /* Magical figurines */
    TV_STATUE = 9, /* Statue, what a silly object... */
    TV_CORPSE = 10, /* Corpses and Skeletons, specific */
    TV_CAPTURE = 11, /* Monster ball */
    TV_NO_AMMO = 15, /* Ammo for crimson */
    TV_SHOT = 16, /* Ammo for slings */
    TV_ARROW = 17, /* Ammo for bows */
    TV_BOLT = 18, /* Ammo for x-bows */
    TV_BOW = 19, /* Slings/Bows/Xbows */
    TV_DIGGING = 20, /* Shovels/Picks */
    TV_HAFTED = 21, /* Priest Weapons */
    TV_POLEARM = 22, /* Axes and Pikes */
    TV_SWORD = 23, /* Edged Weapons */
    TV_BOOTS = 30, /* Boots */
    TV_GLOVES = 31, /* Gloves */
    TV_HELM = 32, /* Helms */
    TV_CROWN = 33, /* Crowns */
    TV_SHIELD = 34, /* Shields */
    TV_CLOAK = 35, /* Cloaks */
    TV_SOFT_ARMOR = 36, /* Soft Armor */
    TV_HARD_ARMOR = 37, /* Hard Armor */
    TV_DRAG_ARMOR = 38, /* Dragon Scale Mail */
    TV_LITE = 39, /* Lites (including Specials) */
    TV_AMULET = 40, /* Amulets (including Specials) */
    TV_RING = 45, /* Rings (including Specials) */
    TV_CARD = 50,
    TV_STAFF = 55,
    TV_WAND = 65,
    TV_ROD = 66,
    TV_PARCHMENT = 69,
    TV_SCROLL = 70,
    TV_POTION = 75,
    TV_FLASK = 77,
    TV_FOOD = 80,
    TV_LIFE_BOOK = 90,
    TV_SORCERY_BOOK = 91,
    TV_NATURE_BOOK = 92,
    TV_CHAOS_BOOK = 93,
    TV_DEATH_BOOK = 94,
    TV_TRUMP_BOOK = 95,
    TV_ARCANE_BOOK = 96,
    TV_CRAFT_BOOK = 97,
    TV_DEMON_BOOK = 98,
    TV_CRUSADE_BOOK = 99,
    TV_MUSIC_BOOK = 105,
    TV_HISSATSU_BOOK = 106,
    TV_HEX_BOOK = 107,
    TV_GOLD = 127, /* Gold can only be picked up by players */
};

#define TV_EQUIP_BEGIN TV_SHOT
#define TV_EQUIP_END TV_CARD
#define TV_MISSILE_BEGIN TV_SHOT
#define TV_MISSILE_END TV_BOLT
#define TV_WEARABLE_BEGIN TV_BOW
#define TV_WEARABLE_END TV_CARD
#define TV_WEAPON_BEGIN TV_BOW
#define TV_WEAPON_END TV_SWORD
#define TV_ARMOR_BEGIN TV_BOOTS
#define TV_ARMOR_END TV_DRAG_ARMOR
