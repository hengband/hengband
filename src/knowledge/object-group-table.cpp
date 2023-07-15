/*
 * @brief オブジェクト種別を表すテキストの配列群
 * @date 2020/03/08
 * @author Hourier
 */

#include "knowledge/object-group-table.h"
#include "object/tval-types.h"

/*
 * Description of each monster group.
 */
const std::vector<concptr> object_group_text = { _("キノコ", "Mushrooms"), _("薬", "Potions"), _("油つぼ", "Flasks"), _("巻物", "Scrolls"),
    _("指輪", "Rings"), _("アミュレット", "Amulets"), _("笛", "Whistles"), _("光源", "Lanterns"), _("魔法棒", "Wands"), _("杖", "Staffs"), _("ロッド", "Rods"),
    _("カード", "Cards"), _("モンスター・ボール", "Capture Balls"), _("羊皮紙", "Parchments"), _("くさび", "Spikes"), _("箱", "Boxes"), _("人形", "Figurines"),
    _("像", "Statues"), _("ゴミ", "Junk"), _("空のビン", "Bottles"), _("骨", "Skeletons"), _("死体", "Corpses"), _("刀剣類", "Swords"),
    _("鈍器", "Blunt Weapons"), _("長柄武器", "Polearms"), _("採掘道具", "Diggers"), _("飛び道具", "Bows"), _("弾", "Shots"), _("矢", "Arrows"),
    _("ボルト", "Bolts"), _("軽装鎧", "Soft Armor"), _("重装鎧", "Hard Armor"), _("ドラゴン鎧", "Dragon Armor"), _("盾", "Shields"), _("クローク", "Cloaks"),
    _("籠手", "Gloves"), _("ヘルメット", "Helms"), _("冠", "Crowns"), _("ブーツ", "Boots"), _("魔法書", "Spellbooks"), _("財宝", "Treasure"),
    _("何か", "Something"), nullptr };

/*
 * TVALs of items in each group
 */
const std::vector<ItemKindType> object_group_tval = {
    ItemKindType::FOOD,
    ItemKindType::POTION,
    ItemKindType::FLASK,
    ItemKindType::SCROLL,
    ItemKindType::RING,
    ItemKindType::AMULET,
    ItemKindType::WHISTLE,
    ItemKindType::LITE,
    ItemKindType::WAND,
    ItemKindType::STAFF,
    ItemKindType::ROD,
    ItemKindType::CARD,
    ItemKindType::CAPTURE,
    ItemKindType::PARCHMENT,
    ItemKindType::SPIKE,
    ItemKindType::CHEST,
    ItemKindType::FIGURINE,
    ItemKindType::STATUE,
    ItemKindType::JUNK,
    ItemKindType::BOTTLE,
    ItemKindType::SKELETON,
    ItemKindType::CORPSE,
    ItemKindType::SWORD,
    ItemKindType::HAFTED,
    ItemKindType::POLEARM,
    ItemKindType::DIGGING,
    ItemKindType::BOW,
    ItemKindType::SHOT,
    ItemKindType::ARROW,
    ItemKindType::BOLT,
    ItemKindType::SOFT_ARMOR,
    ItemKindType::HARD_ARMOR,
    ItemKindType::DRAG_ARMOR,
    ItemKindType::SHIELD,
    ItemKindType::CLOAK,
    ItemKindType::GLOVES,
    ItemKindType::HELM,
    ItemKindType::CROWN,
    ItemKindType::BOOTS,
    ItemKindType::LIFE_BOOK,
    ItemKindType::GOLD,
    ItemKindType::NONE,
    ItemKindType::NONE,
};
