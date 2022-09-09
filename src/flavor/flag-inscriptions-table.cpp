#include "flavor/flag-inscriptions-table.h"
#include "object-enchant/tr-types.h"

#ifdef JP
#define N(JAPANESE, ENGLISH) JAPANESE, ENGLISH
#else
#define N(JAPANESE, ENGLISH) ENGLISH
#endif

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
    { N("攻", "At"), TR_BLOWS },
    { N("速", "Sp"), TR_SPEED },
    { N("腕", "St"), TR_STR },
    { N("知", "In"), TR_INT },
    { N("賢", "Wi"), TR_WIS },
    { N("器", "Dx"), TR_DEX },
    { N("耐", "Cn"), TR_CON },
    { N("魅", "Ch"), TR_CHR },
    { N("道", "Md"), TR_MAGIC_MASTERY },
    { N("隠", "Sl"), TR_STEALTH },
    { N("探", "Sr"), TR_SEARCH },
    { N("赤", "If"), TR_INFRA },
    { N("掘", "Dg"), TR_TUNNEL },
};

/*! オブジェクトの特性表示記号テーブルの定義(免疫) */
const std::vector<flag_insc_table> flag_insc_immune = {
    { N("酸", "Ac"), TR_IM_ACID },
    { N("電", "El"), TR_IM_ELEC },
    { N("火", "Fi"), TR_IM_FIRE },
    { N("冷", "Co"), TR_IM_COLD },
    { N("暗", "Dk"), TR_IM_DARK }
};

/*! オブジェクトの特性表示記号テーブルの定義(弱点) */
const std::vector<flag_insc_table> flag_insc_vuln = {
    { N("酸", "Ac"), TR_VUL_ACID, TR_IM_ACID },
    { N("電", "El"), TR_VUL_ELEC, TR_IM_ELEC },
    { N("火", "Fi"), TR_VUL_FIRE, TR_IM_FIRE },
    { N("冷", "Co"), TR_VUL_COLD, TR_IM_COLD },
    { N("閃", "Li"), TR_VUL_LITE },
    { N("呪", "Cu"), TR_VUL_CURSE }
};

/*! オブジェクトの特性表示記号テーブルの定義(耐性) */
const std::vector<flag_insc_table> flag_insc_resistance = {
    { N("酸", "Ac"), TR_RES_ACID, TR_IM_ACID },
    { N("電", "El"), TR_RES_ELEC, TR_IM_ELEC },
    { N("火", "Fi"), TR_RES_FIRE, TR_IM_FIRE },
    { N("冷", "Co"), TR_RES_COLD, TR_IM_COLD },
    { N("毒", "Po"), TR_RES_POIS },
    { N("閃", "Li"), TR_RES_LITE },
    { N("暗", "Dk"), TR_RES_DARK },
    { N("破", "Sh"), TR_RES_SHARDS },
    { N("盲", "Bl"), TR_RES_BLIND },
    { N("乱", "Cf"), TR_RES_CONF },
    { N("轟", "So"), TR_RES_SOUND },
    { N("獄", "Nt"), TR_RES_NETHER },
    { N("因", "Nx"), TR_RES_NEXUS },
    { N("沌", "Ca"), TR_RES_CHAOS },
    { N("劣", "Di"), TR_RES_DISEN },
    { N("時", "Tm"), TR_RES_TIME },
    { N("水", "Wt"), TR_RES_WATER },
    { N("恐", "Fe"), TR_RES_FEAR },
    { N("呪", "Cu"), TR_RES_CURSE },
};

/*! オブジェクトの特性表示記号テーブルの定義(その他特性) */
const std::vector<flag_insc_table> flag_insc_misc = {
    { N("易", "Es"), TR_EASY_SPELL },
    { N("減", "Dm"), TR_DEC_MANA },
    { N("投", "Th"), TR_THROW },
    { N("反", "Rf"), TR_REFLECT },
    { N("麻", "Fa"), TR_FREE_ACT },
    { N("視", "Si"), TR_SEE_INVIS },
    { N("経", "Hl"), TR_HOLD_EXP },
    { N("遅", "Sd"), TR_SLOW_DIGEST },
    { N("活", "Rg"), TR_REGEN },
    { N("浮", "Lv"), TR_LEVITATION },
    { N("明", "Lu"), TR_LITE_1 },
    { N("明", "Lu"), TR_LITE_2 },
    { N("明", "Lu"), TR_LITE_3 },
    { N("闇", "Dl"), TR_LITE_M1 },
    { N("闇", "Dl"), TR_LITE_M2 },
    { N("闇", "Dl"), TR_LITE_M3 },
    { N("警", "Wr"), TR_WARNING },
    { N("倍", "Xm"), TR_XTRA_MIGHT },
    { N("射", "Xs"), TR_XTRA_SHOTS },
    { N("瞬", "Te"), TR_TELEPORT },
    { N("怒", "Ag"), TR_AGGRAVATE },
    { N("祝", "Bs"), TR_BLESSED },
    { N("忌", "Ty"), TR_TY_CURSE },
    { N("呪", "C-"), TR_ADD_L_CURSE },
    { N("詛", "C+"), TR_ADD_H_CURSE },
    { N("焼", "F"), TR_SELF_FIRE },
    { N("凍", "Co"), TR_SELF_COLD },
    { N("電", "E"), TR_SELF_ELEC },
};

/*! オブジェクトの特性表示記号テーブルの定義(オーラ) */
const std::vector<flag_insc_table> flag_insc_aura = {
    { N("炎", "F"), TR_SH_FIRE },
    { N("電", "E"), TR_SH_ELEC },
    { N("冷", "C"), TR_SH_COLD },
    { N("魔", "M"), TR_NO_MAGIC },
    { N("瞬", "T"), TR_NO_TELE },
};

/*! オブジェクトの特性表示記号テーブルの定義(属性スレイ) */
const std::vector<flag_insc_table> flag_insc_brand = {
    { N("酸", "A"), TR_BRAND_ACID },
    { N("電", "E"), TR_BRAND_ELEC },
    { N("焼", "F"), TR_BRAND_FIRE },
    { N("凍", "Co"), TR_BRAND_COLD },
    { N("毒", "P"), TR_BRAND_POIS },
    { N("沌", "Ca"), TR_CHAOTIC },
    { N("魔", "Ma"), TR_BRAND_MAGIC },
    { N("吸", "V"), TR_VAMPIRIC },
    { N("震", "Q"), TR_EARTHQUAKE },
    { N("切", "Sl"), TR_VORPAL },
    { N("強", "Sm"), TR_IMPACT },
    { N("理", "Mf"), TR_FORCE_WEAPON }
};

/*! オブジェクトの特性表示記号テーブルの定義(種族スレイ) */
const std::vector<flag_insc_table> flag_insc_kill = {
    { N("邪", "*"), TR_KILL_EVIL },
    { N("善", "A"), TR_KILL_GOOD },
    { N("人", "p"), TR_KILL_HUMAN },
    { N("龍", "D"), TR_KILL_DRAGON },
    { N("オ", "o"), TR_KILL_ORC },
    { N("ト", "T"), TR_KILL_TROLL },
    { N("巨", "P"), TR_KILL_GIANT },
    { N("デ", "U"), TR_KILL_DEMON },
    { N("死", "L"), TR_KILL_UNDEAD },
    { N("動", "Z"), TR_KILL_ANIMAL },
};

/*! オブジェクトの特性表示記号テーブルの定義(種族*スレイ*) */
const std::vector<flag_insc_table> flag_insc_slay = {
    { N("邪", "*"), TR_SLAY_EVIL, TR_KILL_EVIL },
    { N("善", "A"), TR_SLAY_GOOD, TR_KILL_GOOD },
    { N("人", "p"), TR_SLAY_HUMAN, TR_KILL_HUMAN },
    { N("竜", "D"), TR_SLAY_DRAGON, TR_KILL_DRAGON },
    { N("オ", "o"), TR_SLAY_ORC, TR_KILL_ORC },
    { N("ト", "T"), TR_SLAY_TROLL, TR_KILL_TROLL },
    { N("巨", "P"), TR_SLAY_GIANT, TR_KILL_GIANT },
    { N("デ", "U"), TR_SLAY_DEMON, TR_KILL_DEMON },
    { N("死", "L"), TR_SLAY_UNDEAD, TR_KILL_UNDEAD },
    { N("動", "Z"), TR_SLAY_ANIMAL, TR_KILL_ANIMAL },
};

/*! オブジェクトの特性表示記号テーブルの定義(ESP1) */
const std::vector<flag_insc_table> flag_insc_esp1 = {
    { N("感", "Tele"), TR_TELEPATHY },
    { N("邪", "Evil"), TR_ESP_EVIL },
    { N("善", "Good"), TR_ESP_GOOD },
    { N("無", "Nolv"), TR_ESP_NONLIVING },
    { N("個", "Uniq"), TR_ESP_UNIQUE },
};

/*! オブジェクトの特性表示記号テーブルの定義(ESP2) */
const std::vector<flag_insc_table> flag_insc_esp2 = {
    { N("人", "p"), TR_ESP_HUMAN },
    { N("竜", "D"), TR_ESP_DRAGON },
    { N("オ", "o"), TR_ESP_ORC },
    { N("ト", "T"), TR_ESP_TROLL },
    { N("巨", "P"), TR_ESP_GIANT },
    { N("デ", "U"), TR_ESP_DEMON },
    { N("死", "L"), TR_ESP_UNDEAD },
    { N("動", "Z"), TR_ESP_ANIMAL },
};

/*! オブジェクトの特性表示記号テーブルの定義(能力維持) */
const std::vector<flag_insc_table> flag_insc_sust = {
    { N("腕", "St"), TR_SUST_STR },
    { N("知", "In"), TR_SUST_INT },
    { N("賢", "Wi"), TR_SUST_WIS },
    { N("器", "Dx"), TR_SUST_DEX },
    { N("耐", "Cn"), TR_SUST_CON },
    { N("魅", "Ch"), TR_SUST_CHR },
};
