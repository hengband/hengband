#include "wizard/tval-descriptions-table.h"
#include "object/tval-types.h"

/*!
 * ベースアイテムの大項目IDの種別名定義 / A list of tvals and their textual names
 */
const std::vector<tval_desc> tvals = {
    { ItemKindType::SWORD, _("刀剣", "Sword") },
    { ItemKindType::POLEARM, _("長柄/斧", "Polearm") },
    { ItemKindType::HAFTED, _("鈍器", "Hafted Weapon") },
    { ItemKindType::BOW, _("弓", "Bow") },
    { ItemKindType::ARROW, _("矢", "Arrows") },
    { ItemKindType::BOLT, _("クロスボウの矢", "Bolts") },
    { ItemKindType::SHOT, _("弾", "Shots") },
    { ItemKindType::SHIELD, _("盾", "Shield") },
    { ItemKindType::CROWN, _("冠", "Crown") },
    { ItemKindType::HELM, _("兜", "Helm") },
    { ItemKindType::GLOVES, _("籠手", "Gloves") },
    { ItemKindType::BOOTS, _("靴", "Boots") },
    { ItemKindType::CLOAK, _("クローク", "Cloak") },
    { ItemKindType::DRAG_ARMOR, _("ドラゴン・スケイルメイル", "Dragon Scale Mail") },
    { ItemKindType::HARD_ARMOR, _("重鎧", "Hard Armor") },
    { ItemKindType::SOFT_ARMOR, _("軽鎧", "Soft Armor") },
    { ItemKindType::RING, _("指輪", "Ring") },
    { ItemKindType::AMULET, _("アミュレット", "Amulet") },
    { ItemKindType::LITE, _("光源", "Lite") },
    { ItemKindType::POTION, _("薬", "Potion") },
    { ItemKindType::SCROLL, _("巻物", "Scroll") },
    { ItemKindType::WAND, _("魔法棒", "Wand") },
    { ItemKindType::STAFF, _("杖", "Staff") },
    { ItemKindType::ROD, _("ロッド", "Rod") },
    { ItemKindType::LIFE_BOOK, _("生命の魔法書", "Life Spellbook") },
    { ItemKindType::SORCERY_BOOK, _("仙術の魔法書", "Sorcery Spellbook") },
    { ItemKindType::NATURE_BOOK, _("自然の魔法書", "Nature Spellbook") },
    { ItemKindType::CHAOS_BOOK, _("カオスの魔法書", "Chaos Spellbook") },
    { ItemKindType::DEATH_BOOK, _("暗黒の魔法書", "Death Spellbook") },
    { ItemKindType::TRUMP_BOOK, _("トランプの魔法書", "Trump Spellbook") },
    { ItemKindType::ARCANE_BOOK, _("秘術の魔法書", "Arcane Spellbook") },
    { ItemKindType::CRAFT_BOOK, _("匠の魔法書", "Craft Spellbook") },
    { ItemKindType::DEMON_BOOK, _("悪魔の魔法書", "Daemon Spellbook") },
    { ItemKindType::CRUSADE_BOOK, _("破邪の魔法書", "Crusade Spellbook") },
    { ItemKindType::MUSIC_BOOK, _("歌集", "Music Spellbook") },
    { ItemKindType::HISSATSU_BOOK, _("武芸の書", "Book of Kendo") },
    { ItemKindType::HEX_BOOK, _("呪術の魔法書", "Hex Spellbook") },
    { ItemKindType::PARCHMENT, _("羊皮紙", "Parchment") },
    { ItemKindType::WHISTLE, _("笛", "Whistle") },
    { ItemKindType::SPIKE, _("くさび", "Spikes") },
    { ItemKindType::DIGGING, _("シャベル/つるはし", "Digger") },
    { ItemKindType::CHEST, _("箱", "Chest") },
    { ItemKindType::CAPTURE, _("モンスター・ボール", "Capture Ball") },
    { ItemKindType::CARD, _("エクスプレスカード", "Express Card") },
    { ItemKindType::FIGURINE, _("人形", "Magical Figurine") },
    { ItemKindType::STATUE, _("像", "Statue") },
    { ItemKindType::CORPSE, _("死体", "Corpse") },
    { ItemKindType::FOOD, _("食料", "Food") },
    { ItemKindType::FLASK, _("油つぼ", "Flask") },
    { ItemKindType::JUNK, _("がらくた", "Junk") },
    { ItemKindType::SKELETON, _("骨", "Skeleton") },
    { ItemKindType::NONE, nullptr },
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
