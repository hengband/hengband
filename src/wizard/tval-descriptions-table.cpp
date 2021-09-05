#include "wizard/tval-descriptions-table.h"
#include "object/tval-types.h"

/*!
 * ベースアイテムの大項目IDの種別名定義 / A list of tvals and their textual names
 */
tval_desc tvals[MAX_TVAL_DESCRIPTIONS] = { { TV_SWORD, "Sword" }, { TV_POLEARM, "Polearm" }, { TV_HAFTED, "Hafted Weapon" }, { TV_BOW, "Bow" },
    { TV_ARROW, "Arrows" }, { TV_BOLT, "Bolts" }, { TV_SHOT, "Shots" }, { TV_SHIELD, "Shield" }, { TV_CROWN, "Crown" }, { TV_HELM, "Helm" },
    { TV_GLOVES, "Gloves" }, { TV_BOOTS, "Boots" }, { TV_CLOAK, "Cloak" }, { TV_DRAG_ARMOR, "Dragon Scale Mail" }, { TV_HARD_ARMOR, "Hard Armor" },
    { TV_SOFT_ARMOR, "Soft Armor" }, { TV_RING, "Ring" }, { TV_AMULET, "Amulet" }, { TV_LITE, "Lite" }, { TV_POTION, "Potion" }, { TV_SCROLL, "Scroll" },
    { TV_WAND, "Wand" }, { TV_STAFF, "Staff" }, { TV_ROD, "Rod" }, { TV_LIFE_BOOK, "Life Spellbook" }, { TV_SORCERY_BOOK, "Sorcery Spellbook" },
    { TV_NATURE_BOOK, "Nature Spellbook" }, { TV_CHAOS_BOOK, "Chaos Spellbook" }, { TV_DEATH_BOOK, "Death Spellbook" }, { TV_TRUMP_BOOK, "Trump Spellbook" },
    { TV_ARCANE_BOOK, "Arcane Spellbook" }, { TV_CRAFT_BOOK, "Craft Spellbook" }, { TV_DEMON_BOOK, "Daemon Spellbook" },
    { TV_CRUSADE_BOOK, "Crusade Spellbook" }, { TV_MUSIC_BOOK, "Music Spellbook" }, { TV_HISSATSU_BOOK, "Book of Kendo" }, { TV_HEX_BOOK, "Hex Spellbook" },
    { TV_PARCHMENT, "Parchment" }, { TV_WHISTLE, "Whistle" }, { TV_SPIKE, "Spikes" }, { TV_DIGGING, "Digger" }, { TV_CHEST, "Chest" },
    { TV_CAPTURE, "Capture Ball" }, { TV_CARD, "Express Card" }, { TV_FIGURINE, "Magical Figurine" }, { TV_STATUE, "Statue" }, { TV_CORPSE, "Corpse" },
    { TV_FOOD, "Food" }, { TV_FLASK, "Flask" }, { TV_JUNK, "Junk" }, { TV_SKELETON, "Skeleton" }, { 0, nullptr } };

/*!
 * 選択処理用キーコード /
 * Global array for converting numbers to a logical list symbol
 */
const char listsym[MAX_DEBUG_COMMAND_SYMBOLS] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
    'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '\0' };
