#include "wizard/tval-descriptions-table.h"
#include "object/tval-types.h"

/*!
 * ベースアイテムの大項目IDの種別名定義 / A list of tvals and their textual names
 */
const std::vector<tval_desc> tvals = {
    { ItemPrimaryType::TV_SWORD, _("刀剣", "Sword") },
    { ItemPrimaryType::TV_POLEARM, _("長柄/斧", "Polearm") },
    { ItemPrimaryType::TV_HAFTED, _("鈍器", "Hafted Weapon") },
    { ItemPrimaryType::TV_BOW, _("弓", "Bow") },
    { ItemPrimaryType::TV_ARROW, _("矢", "Arrows") },
    { ItemPrimaryType::TV_BOLT, _("クロスボウの矢", "Bolts") },
    { ItemPrimaryType::TV_SHOT, _("弾", "Shots") },
    { ItemPrimaryType::TV_SHIELD, _("盾", "Shield") },
    { ItemPrimaryType::TV_CROWN, _("冠", "Crown") },
    { ItemPrimaryType::TV_HELM, _("兜", "Helm") },
    { ItemPrimaryType::TV_GLOVES, _("籠手", "Gloves") },
    { ItemPrimaryType::TV_BOOTS, _("靴", "Boots") },
    { ItemPrimaryType::TV_CLOAK, _("クローク", "Cloak") },
    { ItemPrimaryType::TV_DRAG_ARMOR, _("ドラゴン・スケイルメイル", "Dragon Scale Mail") },
    { ItemPrimaryType::TV_HARD_ARMOR, _("重鎧", "Hard Armor") },
    { ItemPrimaryType::TV_SOFT_ARMOR, _("軽鎧", "Soft Armor") },
    { ItemPrimaryType::TV_RING, _("指輪", "Ring") },
    { ItemPrimaryType::TV_AMULET, _("アミュレット", "Amulet") },
    { ItemPrimaryType::TV_LITE, _("光源", "Lite") },
    { ItemPrimaryType::TV_POTION, _("薬", "Potion") },
    { ItemPrimaryType::TV_SCROLL, _("巻物", "Scroll") },
    { ItemPrimaryType::TV_WAND, _("魔法棒", "Wand") },
    { ItemPrimaryType::TV_STAFF, _("杖", "Staff") },
    { ItemPrimaryType::TV_ROD, _("ロッド", "Rod") },
    { ItemPrimaryType::TV_LIFE_BOOK, _("生命の魔法書", "Life Spellbook") },
    { ItemPrimaryType::TV_SORCERY_BOOK, _("仙術の魔法書", "Sorcery Spellbook") },
    { ItemPrimaryType::TV_NATURE_BOOK, _("自然の魔法書", "Nature Spellbook") },
    { ItemPrimaryType::TV_CHAOS_BOOK, _("カオスの魔法書", "Chaos Spellbook") },
    { ItemPrimaryType::TV_DEATH_BOOK, _("暗黒の魔法書", "Death Spellbook") },
    { ItemPrimaryType::TV_TRUMP_BOOK, _("トランプの魔法書", "Trump Spellbook") },
    { ItemPrimaryType::TV_ARCANE_BOOK, _("秘術の魔法書", "Arcane Spellbook") },
    { ItemPrimaryType::TV_CRAFT_BOOK, _("匠の魔法書", "Craft Spellbook") },
    { ItemPrimaryType::TV_DEMON_BOOK, _("悪魔の魔法書", "Daemon Spellbook") },
    { ItemPrimaryType::TV_CRUSADE_BOOK, _("破邪の魔法書", "Crusade Spellbook") },
    { ItemPrimaryType::TV_MUSIC_BOOK, _("歌集", "Music Spellbook") },
    { ItemPrimaryType::TV_HISSATSU_BOOK, _("武芸の書", "Book of Kendo") },
    { ItemPrimaryType::TV_HEX_BOOK, _("呪術の魔法書", "Hex Spellbook") },
    { ItemPrimaryType::TV_PARCHMENT, _("冒険者のための中つ国ガイド", "Parchment") },
    { ItemPrimaryType::TV_WHISTLE, _("笛", "Whistle") },
    { ItemPrimaryType::TV_SPIKE, _("くさび", "Spikes") },
    { ItemPrimaryType::TV_DIGGING, _("シャベル/つるはし", "Digger") },
    { ItemPrimaryType::TV_CHEST, _("箱", "Chest") },
    { ItemPrimaryType::TV_CAPTURE, _("モンスター・ボール", "Capture Ball") },
    { ItemPrimaryType::TV_CARD, _("エクスプレスカード", "Express Card") },
    { ItemPrimaryType::TV_FIGURINE, _("人形", "Magical Figurine") },
    { ItemPrimaryType::TV_STATUE, _("像", "Statue") },
    { ItemPrimaryType::TV_CORPSE, _("死体", "Corpse") },
    { ItemPrimaryType::TV_FOOD, _("食料", "Food") },
    { ItemPrimaryType::TV_FLASK, _("ランタン", "Flask") },
    { ItemPrimaryType::TV_JUNK, _("がらくた", "Junk") },
    { ItemPrimaryType::TV_SKELETON, _("骨", "Skeleton") },
    { ItemPrimaryType::TV_NONE, nullptr },
};

/*!
 * 選択処理用キーコード /
 * Global array for converting numbers to a logical list symbol
 */
const std::vector<char> listsym = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '\0'
};
