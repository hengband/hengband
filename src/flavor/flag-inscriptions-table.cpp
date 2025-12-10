#include "flavor/flag-inscriptions-table.h"
#include "object-enchant/tr-types.h"

/*! @brief アイテムの価値記述テーブル */
const concptr game_inscriptions[MAX_GAME_INSCRIPTIONS] = {
    nullptr, /* FEEL_NONE */
    _("壊れている", "broken"), /* FEEL_BROKEN */
    _("恐ろしい", "terrible"), /* FEEL_TERRIBLE */
    _("無価値", "worthless"), /* FEEL_WORTHLESS */
    _("呪われている", "cursed"), /* FEEL_CURSED */
    _("上質以上", "uncursed"), /* FEEL_UNCURSED */
    _("並", "average"), /* FEEL_AVERAGE */
    _("上質", "good"), /* FEEL_GOOD */
    _("高級品", "excellent"), /* FEEL_EXCELLENT */
    _("特別製", "special"), /* FEEL_SPECIAL */
};

/*! オブジェクトの特性表示記号テーブルの定義(pval要素) */
const std::vector<flag_insc_table> flag_insc_plus = {
    { { "攻", "At" }, TR_BLOWS },
    { { "速", "Sp" }, TR_SPEED },
    { { "腕", "St" }, TR_STR },
    { { "知", "In" }, TR_INT },
    { { "賢", "Wi" }, TR_WIS },
    { { "器", "Dx" }, TR_DEX },
    { { "耐", "Cn" }, TR_CON },
    { { "魅", "Ch" }, TR_CHR },
    { { "道", "Md" }, TR_MAGIC_MASTERY },
    { { "隠", "Sl" }, TR_STEALTH },
    { { "探", "Sr" }, TR_SEARCH },
    { { "赤", "If" }, TR_INFRA },
    { { "掘", "Dg" }, TR_TUNNEL },
};

/*! オブジェクトの特性表示記号テーブルの定義(免疫) */
const std::vector<flag_insc_table> flag_insc_immune = {
    { { "酸", "Ac" }, TR_IM_ACID },
    { { "電", "El" }, TR_IM_ELEC },
    { { "火", "Fi" }, TR_IM_FIRE },
    { { "冷", "Co" }, TR_IM_COLD },
    { { "暗", "Dk" }, TR_IM_DARK },
    { { "閃", "Li" }, TR_IM_LITE },
};

/*! オブジェクトの特性表示記号テーブルの定義(弱点) */
const std::vector<flag_insc_table> flag_insc_vuln = {
    { { "酸", "Ac" }, TR_VUL_ACID, TR_IM_ACID },
    { { "電", "El" }, TR_VUL_ELEC, TR_IM_ELEC },
    { { "火", "Fi" }, TR_VUL_FIRE, TR_IM_FIRE },
    { { "冷", "Co" }, TR_VUL_COLD, TR_IM_COLD },
    { { "閃", "Li" }, TR_VUL_LITE },
    { { "呪", "Cu" }, TR_VUL_CURSE }
};

/*! オブジェクトの特性表示記号テーブルの定義(耐性) */
const std::vector<flag_insc_table> flag_insc_resistance = {
    { { "酸", "Ac" }, TR_RES_ACID, TR_IM_ACID },
    { { "電", "El" }, TR_RES_ELEC, TR_IM_ELEC },
    { { "火", "Fi" }, TR_RES_FIRE, TR_IM_FIRE },
    { { "冷", "Co" }, TR_RES_COLD, TR_IM_COLD },
    { { "毒", "Po" }, TR_RES_POIS },
    { { "閃", "Li" }, TR_RES_LITE },
    { { "暗", "Dk" }, TR_RES_DARK },
    { { "破", "Sh" }, TR_RES_SHARDS },
    { { "盲", "Bl" }, TR_RES_BLIND },
    { { "乱", "Cf" }, TR_RES_CONF },
    { { "轟", "So" }, TR_RES_SOUND },
    { { "獄", "Nt" }, TR_RES_NETHER },
    { { "因", "Nx" }, TR_RES_NEXUS },
    { { "沌", "Ca" }, TR_RES_CHAOS },
    { { "劣", "Di" }, TR_RES_DISEN },
    { { "時", "Tm" }, TR_RES_TIME },
    { { "水", "Wt" }, TR_RES_WATER },
    { { "恐", "Fe" }, TR_RES_FEAR },
    { { "呪", "Cu" }, TR_RES_CURSE },
};

/*! オブジェクトの特性表示記号テーブルの定義(その他特性) */
const std::vector<flag_insc_table> flag_insc_misc = {
    { { "易", "Es" }, TR_EASY_SPELL },
    { { "減", "Dm" }, TR_DEC_MANA },
    { { "投", "Th" }, TR_THROW },
    { { "反", "Rf" }, TR_REFLECT },
    { { "麻", "Fa" }, TR_FREE_ACT },
    { { "視", "Si" }, TR_SEE_INVIS },
    { { "経", "Hl" }, TR_HOLD_EXP },
    { { "遅", "Sd" }, TR_SLOW_DIGEST },
    { { "活", "Rg" }, TR_REGEN },
    { { "浮", "Lv" }, TR_LEVITATION },
    { { "明", "Lu" }, TR_LITE_1 },
    { { "明", "Lu" }, TR_LITE_2 },
    { { "明", "Lu" }, TR_LITE_3 },
    { { "闇", "Dl" }, TR_LITE_M1 },
    { { "闇", "Dl" }, TR_LITE_M2 },
    { { "闇", "Dl" }, TR_LITE_M3 },
    { { "警", "Wr" }, TR_WARNING },
    { { "倍", "Xm" }, TR_XTRA_MIGHT },
    { { "射", "Xs" }, TR_XTRA_SHOTS },
    { { "瞬", "Te" }, TR_TELEPORT },
    { { "怒", "Ag" }, TR_AGGRAVATE },
    { { "祝", "Bs" }, TR_BLESSED },
    { { "忌", "Ty" }, TR_TY_CURSE },
    { { "呪", "C-" }, TR_ADD_L_CURSE },
    { { "詛", "C+" }, TR_ADD_H_CURSE },
    { { "焼", "F" }, TR_SELF_FIRE },
    { { "凍", "Co" }, TR_SELF_COLD },
    { { "電", "E" }, TR_SELF_ELEC },
};

/*! オブジェクトの特性表示記号テーブルの定義(オーラ) */
const std::vector<flag_insc_table> flag_insc_aura = {
    { { "炎", "F" }, TR_SH_FIRE },
    { { "電", "E" }, TR_SH_ELEC },
    { { "冷", "C" }, TR_SH_COLD },
    { { "魔", "M" }, TR_NO_MAGIC },
    { { "瞬", "T" }, TR_NO_TELE },
};

/*! オブジェクトの特性表示記号テーブルの定義(属性スレイ) */
const std::vector<flag_insc_table> flag_insc_brand = {
    { { "酸", "A" }, TR_BRAND_ACID },
    { { "電", "E" }, TR_BRAND_ELEC },
    { { "焼", "F" }, TR_BRAND_FIRE },
    { { "凍", "Co" }, TR_BRAND_COLD },
    { { "毒", "P" }, TR_BRAND_POIS },
    { { "沌", "Ca" }, TR_CHAOTIC },
    { { "魔", "Ma" }, TR_BRAND_MAGIC },
    { { "吸", "V" }, TR_VAMPIRIC },
    { { "震", "Q" }, TR_EARTHQUAKE },
    { { "切", "Sl" }, TR_VORPAL },
    { { "強", "Sm" }, TR_IMPACT },
    { { "理", "Mf" }, TR_FORCE_WEAPON }
};

/*! オブジェクトの特性表示記号テーブルの定義(種族スレイ) */
const std::vector<flag_insc_table> flag_insc_kill = {
    { { "邪", "*" }, TR_KILL_EVIL },
    { { "善", "A" }, TR_KILL_GOOD },
    { { "人", "p" }, TR_KILL_HUMAN },
    { { "龍", "D" }, TR_KILL_DRAGON },
    { { "オ", "o" }, TR_KILL_ORC },
    { { "ト", "T" }, TR_KILL_TROLL },
    { { "巨", "P" }, TR_KILL_GIANT },
    { { "デ", "U" }, TR_KILL_DEMON },
    { { "死", "L" }, TR_KILL_UNDEAD },
    { { "動", "Z" }, TR_KILL_ANIMAL },
};

/*! オブジェクトの特性表示記号テーブルの定義(種族*スレイ*) */
const std::vector<flag_insc_table> flag_insc_slay = {
    { { "邪", "*" }, TR_SLAY_EVIL, TR_KILL_EVIL },
    { { "善", "A" }, TR_SLAY_GOOD, TR_KILL_GOOD },
    { { "人", "p" }, TR_SLAY_HUMAN, TR_KILL_HUMAN },
    { { "竜", "D" }, TR_SLAY_DRAGON, TR_KILL_DRAGON },
    { { "オ", "o" }, TR_SLAY_ORC, TR_KILL_ORC },
    { { "ト", "T" }, TR_SLAY_TROLL, TR_KILL_TROLL },
    { { "巨", "P" }, TR_SLAY_GIANT, TR_KILL_GIANT },
    { { "デ", "U" }, TR_SLAY_DEMON, TR_KILL_DEMON },
    { { "死", "L" }, TR_SLAY_UNDEAD, TR_KILL_UNDEAD },
    { { "動", "Z" }, TR_SLAY_ANIMAL, TR_KILL_ANIMAL },
};

/*! オブジェクトの特性表示記号テーブルの定義(ESP1) */
const std::vector<flag_insc_table> flag_insc_esp1 = {
    { { "感", "Tele" }, TR_TELEPATHY },
    { { "邪", "Evil" }, TR_ESP_EVIL },
    { { "善", "Good" }, TR_ESP_GOOD },
    { { "無", "Nolv" }, TR_ESP_NONLIVING },
    { { "個", "Uniq" }, TR_ESP_UNIQUE },
};

/*! オブジェクトの特性表示記号テーブルの定義(ESP2) */
const std::vector<flag_insc_table> flag_insc_esp2 = {
    { { "人", "p" }, TR_ESP_HUMAN },
    { { "竜", "D" }, TR_ESP_DRAGON },
    { { "オ", "o" }, TR_ESP_ORC },
    { { "ト", "T" }, TR_ESP_TROLL },
    { { "巨", "P" }, TR_ESP_GIANT },
    { { "デ", "U" }, TR_ESP_DEMON },
    { { "死", "L" }, TR_ESP_UNDEAD },
    { { "動", "Z" }, TR_ESP_ANIMAL },
};

/*! オブジェクトの特性表示記号テーブルの定義(能力維持) */
const std::vector<flag_insc_table> flag_insc_sust = {
    { { "腕", "St" }, TR_SUST_STR },
    { { "知", "In" }, TR_SUST_INT },
    { { "賢", "Wi" }, TR_SUST_WIS },
    { { "器", "Dx" }, TR_SUST_DEX },
    { { "耐", "Cn" }, TR_SUST_CON },
    { { "魅", "Ch" }, TR_SUST_CHR },
};
