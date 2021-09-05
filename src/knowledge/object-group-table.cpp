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
concptr object_group_text[MAX_OBJECT_GROUP_TEXT] = { _("キノコ", "Mushrooms"), _("薬", "Potions"), _("油つぼ", "Flasks"), _("巻物", "Scrolls"),
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
byte object_group_tval[MAX_OBJECT_GROUP_TVAL] = {
    TV_FOOD,
    TV_POTION,
    TV_FLASK,
    TV_SCROLL,
    TV_RING,
    TV_AMULET,
    TV_WHISTLE,
    TV_LITE,
    TV_WAND,
    TV_STAFF,
    TV_ROD,
    TV_CARD,
    TV_CAPTURE,
    TV_PARCHMENT,
    TV_SPIKE,
    TV_CHEST,
    TV_FIGURINE,
    TV_STATUE,
    TV_JUNK,
    TV_BOTTLE,
    TV_SKELETON,
    TV_CORPSE,
    TV_SWORD,
    TV_HAFTED,
    TV_POLEARM,
    TV_DIGGING,
    TV_BOW,
    TV_SHOT,
    TV_ARROW,
    TV_BOLT,
    TV_SOFT_ARMOR,
    TV_HARD_ARMOR,
    TV_DRAG_ARMOR,
    TV_SHIELD,
    TV_CLOAK,
    TV_GLOVES,
    TV_HELM,
    TV_CROWN,
    TV_BOOTS,
    TV_LIFE_BOOK,
    TV_GOLD,
    0,
    0,
};
