#include "wizard/tval-descriptions-table.h"
#include "object/tval-types.h"

/*!
 * ベースアイテムの大項目IDの種別名定義 / A list of tvals and their textual names
 */
const std::vector<tval_desc> tvals = {
    { TV_SWORD, _("刀剣", "Sword") },
    { TV_POLEARM, _("長柄/斧", "Polearm") },
    { TV_HAFTED, _("鈍器", "Hafted Weapon") },
    { TV_BOW, _("弓", "Bow") },
    { TV_ARROW, _("矢", "Arrows") },
    { TV_BOLT, _("クロスボウの矢", "Bolts") },
    { TV_SHOT, _("弾", "Shots") },
    { TV_SHIELD, _("盾", "Shield") },
    { TV_CROWN, _("冠", "Crown") },
    { TV_HELM, _("兜", "Helm") },
    { TV_GLOVES, _("籠手", "Gloves") },
    { TV_BOOTS, _("靴", "Boots") },
    { TV_CLOAK, _("クローク", "Cloak") },
    { TV_DRAG_ARMOR, _("ドラゴン・スケイルメイル", "Dragon Scale Mail") },
    { TV_HARD_ARMOR, _("重鎧", "Hard Armor") },
    { TV_SOFT_ARMOR, _("軽鎧", "Soft Armor") },
    { TV_RING, _("指輪", "Ring") },
    { TV_AMULET, _("アミュレット", "Amulet") },
    { TV_LITE, _("光源", "Lite") },
    { TV_POTION, _("薬", "Potion") },
    { TV_SCROLL, _("巻物", "Scroll") },
    { TV_WAND, _("魔法棒", "Wand") },
    { TV_STAFF, _("杖", "Staff") },
    { TV_ROD, _("ロッド", "Rod") },
    { TV_LIFE_BOOK, _("生命の魔法書", "Life Spellbook") },
    { TV_SORCERY_BOOK, _("仙術の魔法書", "Sorcery Spellbook") },
    { TV_NATURE_BOOK, _("自然の魔法書", "Nature Spellbook") },
    { TV_CHAOS_BOOK, _("カオスの魔法書", "Chaos Spellbook") },
    { TV_DEATH_BOOK, _("暗黒の魔法書", "Death Spellbook") },
    { TV_TRUMP_BOOK, _("トランプの魔法書", "Trump Spellbook") },
    { TV_ARCANE_BOOK, _("秘術の魔法書", "Arcane Spellbook") },
    { TV_CRAFT_BOOK, _("匠の魔法書", "Craft Spellbook") },
    { TV_DEMON_BOOK, _("悪魔の魔法書", "Daemon Spellbook") },
    { TV_CRUSADE_BOOK, _("破邪の魔法書", "Crusade Spellbook") },
    { TV_MUSIC_BOOK, _("歌集", "Music Spellbook") },
    { TV_HISSATSU_BOOK, _("武芸の書", "Book of Kendo") },
    { TV_HEX_BOOK, _("呪術の魔法書", "Hex Spellbook") },
    { TV_PARCHMENT, _("冒険者のための中つ国ガイド", "Parchment") },
    { TV_WHISTLE, _("笛", "Whistle") },
    { TV_SPIKE, _("くさび", "Spikes") },
    { TV_DIGGING, _("シャベル/つるはし", "Digger") },
    { TV_CHEST, _("箱", "Chest") },
    { TV_CAPTURE, _("モンスター・ボール", "Capture Ball") },
    { TV_CARD, _("エクスプレスカード", "Express Card") },
    { TV_FIGURINE, _("人形", "Magical Figurine") },
    { TV_STATUE, _("像", "Statue") },
    { TV_CORPSE, _("死体", "Corpse") },
    { TV_FOOD, _("食料", "Food") },
    { TV_FLASK, _("ランタン", "Flask") },
    { TV_JUNK, _("がらくた", "Junk") },
    { TV_SKELETON, _("骨", "Skeleton") },
    { TV_NONE, nullptr }
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
