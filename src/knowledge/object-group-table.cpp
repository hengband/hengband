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
const std::vector<ItemPrimaryType> object_group_tval = {
    ItemPrimaryType::TV_FOOD,
    ItemPrimaryType::TV_POTION,
    ItemPrimaryType::TV_FLASK,
    ItemPrimaryType::TV_SCROLL,
    ItemPrimaryType::TV_RING,
    ItemPrimaryType::TV_AMULET,
    ItemPrimaryType::TV_WHISTLE,
    ItemPrimaryType::TV_LITE,
    ItemPrimaryType::TV_WAND,
    ItemPrimaryType::TV_STAFF,
    ItemPrimaryType::TV_ROD,
    ItemPrimaryType::TV_CARD,
    ItemPrimaryType::TV_CAPTURE,
    ItemPrimaryType::TV_PARCHMENT,
    ItemPrimaryType::TV_SPIKE,
    ItemPrimaryType::TV_CHEST,
    ItemPrimaryType::TV_FIGURINE,
    ItemPrimaryType::TV_STATUE,
    ItemPrimaryType::TV_JUNK,
    ItemPrimaryType::TV_BOTTLE,
    ItemPrimaryType::TV_SKELETON,
    ItemPrimaryType::TV_CORPSE,
    ItemPrimaryType::TV_SWORD,
    ItemPrimaryType::TV_HAFTED,
    ItemPrimaryType::TV_POLEARM,
    ItemPrimaryType::TV_DIGGING,
    ItemPrimaryType::TV_BOW,
    ItemPrimaryType::TV_SHOT,
    ItemPrimaryType::TV_ARROW,
    ItemPrimaryType::TV_BOLT,
    ItemPrimaryType::TV_SOFT_ARMOR,
    ItemPrimaryType::TV_HARD_ARMOR,
    ItemPrimaryType::TV_DRAG_ARMOR,
    ItemPrimaryType::TV_SHIELD,
    ItemPrimaryType::TV_CLOAK,
    ItemPrimaryType::TV_GLOVES,
    ItemPrimaryType::TV_HELM,
    ItemPrimaryType::TV_CROWN,
    ItemPrimaryType::TV_BOOTS,
    ItemPrimaryType::TV_LIFE_BOOK,
    ItemPrimaryType::TV_GOLD,
    ItemPrimaryType::TV_NONE,
    ItemPrimaryType::TV_NONE,
};
