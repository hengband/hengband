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

// clang-format off
/*! オブジェクトの特性表示記号テーブルの定義(pval要素) */
std::vector<flag_insc_table> flag_insc_plus = {
    { N("攻", "At"), TR_BLOWS, -1 }, { N("速", "Sp"), TR_SPEED, -1 },
    { N("腕", "St"), TR_STR, -1 }, { N("知", "In"), TR_INT, -1 }, { N("賢", "Wi"), TR_WIS, -1 },
    { N("器", "Dx"), TR_DEX, -1 }, { N("耐", "Cn"), TR_CON, -1 }, { N("魅", "Ch"), TR_CHR, -1 }, { N("道", "Md"), TR_MAGIC_MASTERY, -1 },
    { N("隠", "Sl"), TR_STEALTH, -1 }, { N("探", "Sr"), TR_SEARCH, -1 }, { N("赤", "If"), TR_INFRA, -1 }, { N("掘", "Dg"), TR_TUNNEL, -1 },
};

/*! オブジェクトの特性表示記号テーブルの定義(免疫) */
std::vector<flag_insc_table> flag_insc_immune = {
    { N("酸", "Ac"), TR_IM_ACID, -1 }, { N("電", "El"), TR_IM_ELEC, -1 }, { N("火", "Fi"), TR_IM_FIRE, -1 }, { N("冷", "Co"), TR_IM_COLD, -1 },
    { N("暗", "Dk"), TR_IM_DARK, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(弱点) */
std::vector<flag_insc_table> flag_insc_vuln = {
    { N("酸", "Ac"), TR_VUL_ACID, TR_IM_ACID }, { N("電", "El"), TR_VUL_ELEC, TR_IM_ELEC }, { N("火", "Fi"), TR_VUL_FIRE, TR_IM_FIRE }, { N("冷", "Co"), TR_VUL_COLD, TR_IM_COLD },
    { N("閃", "Li"), TR_VUL_LITE, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(耐性) */
std::vector<flag_insc_table> flag_insc_resistance = {
    { N("酸", "Ac"), TR_RES_ACID, TR_IM_ACID }, { N("電", "El"), TR_RES_ELEC, TR_IM_ELEC }, { N("火", "Fi"), TR_RES_FIRE, TR_IM_FIRE }, { N("冷", "Co"), TR_RES_COLD, TR_IM_COLD },
    { N("毒", "Po"), TR_RES_POIS, -1 }, { N("閃", "Li"), TR_RES_LITE, -1 }, { N("暗", "Dk"), TR_RES_DARK, -1 }, { N("破", "Sh"), TR_RES_SHARDS, -1 },
    { N("盲", "Bl"), TR_RES_BLIND, -1 }, { N("乱", "Cf"), TR_RES_CONF, -1 }, { N("轟", "So"), TR_RES_SOUND, -1 }, { N("獄", "Nt"), TR_RES_NETHER, -1 },
    { N("因", "Nx"), TR_RES_NEXUS, -1 }, { N("沌", "Ca"), TR_RES_CHAOS, -1 }, { N("劣", "Di"), TR_RES_DISEN, -1 }, { N("時", "Tm"), TR_RES_TIME, -1 },
    { N("水", "Wt"), TR_RES_WATER, -1 }, { N("恐", "Fe"), TR_RES_FEAR, -1 }, { N("呪", "Cu"), TR_RES_CURSE, -1 },
};

/*! オブジェクトの特性表示記号テーブルの定義(その他特性) */
std::vector<flag_insc_table> flag_insc_misc = {
    { N("易", "Es"), TR_EASY_SPELL, -1 }, { N("減", "Dm"), TR_DEC_MANA, -1 },
    { N("投", "Th"), TR_THROW, -1 }, { N("反", "Rf"), TR_REFLECT, -1 }, { N("麻", "Fa"), TR_FREE_ACT, -1 }, { N("視", "Si"), TR_SEE_INVIS, -1 },
    { N("経", "Hl"), TR_HOLD_EXP, -1 }, { N("遅", "Sd"), TR_SLOW_DIGEST, -1 }, { N("活", "Rg"), TR_REGEN, -1 }, { N("浮", "Lv"), TR_LEVITATION, -1 },
    { N("明", "Lu"), TR_LITE_1, -1 }, { N("明", "Lu"), TR_LITE_2, -1 }, { N("明", "Lu"), TR_LITE_3, -1 }, { N("闇", "Dl"), TR_LITE_M1, -1 },
    { N("闇", "Dl"), TR_LITE_M2, -1 }, { N("闇", "Dl"), TR_LITE_M3, -1 }, { N("警", "Wr"), TR_WARNING, -1 }, { N("倍", "Xm"), TR_XTRA_MIGHT, -1 },
    { N("射", "Xs"), TR_XTRA_SHOTS, -1 }, { N("瞬", "Te"), TR_TELEPORT, -1 }, { N("怒", "Ag"), TR_AGGRAVATE, -1 }, { N("祝", "Bs"), TR_BLESSED, -1 },
    { N("忌", "Ty"), TR_TY_CURSE, -1 }, { N("呪", "C-"), TR_ADD_L_CURSE, -1 }, { N("詛", "C+"), TR_ADD_H_CURSE, -1 },
};

/*! オブジェクトの特性表示記号テーブルの定義(オーラ) */
std::vector<flag_insc_table> flag_insc_aura = {
    { N("炎", "F"), TR_SH_FIRE, -1 }, { N("電", "E"), TR_SH_ELEC, -1 }, { N("冷", "C"), TR_SH_COLD, -1 },
    { N("魔", "M"), TR_NO_MAGIC, -1 }, { N("瞬", "T"), TR_NO_TELE, -1 },
};

/*! オブジェクトの特性表示記号テーブルの定義(属性スレイ) */
std::vector<flag_insc_table> flag_insc_brand = {
    { N("酸", "A"), TR_BRAND_ACID, -1 }, { N("電", "E"), TR_BRAND_ELEC, -1 }, { N("焼", "F"), TR_BRAND_FIRE, -1 }, { N("凍", "Co"), TR_BRAND_COLD, -1 },
    { N("毒", "P"), TR_BRAND_POIS, -1 }, { N("沌", "Ca"), TR_CHAOTIC, -1 }, { N("魔", "Ma"), TR_BRAND_MAGIC, -1 },
    { N("吸", "V"), TR_VAMPIRIC, -1 }, { N("震", "Q"), TR_EARTHQUAKE, -1 }, { N("切", "Sl"), TR_VORPAL, -1 }, { N("強", "Sm"), TR_IMPACT, -1 },
    { N("理", "Mf"), TR_FORCE_WEAPON, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(種族スレイ) */
std::vector<flag_insc_table> flag_insc_kill = {
    { N("邪", "*"), TR_KILL_EVIL, -1 }, { N("善", "A"), TR_KILL_GOOD, -1 }, { N("人", "p"), TR_KILL_HUMAN, -1 }, { N("龍", "D"), TR_KILL_DRAGON, -1 },
    { N("オ", "o"), TR_KILL_ORC, -1 }, { N("ト", "T"), TR_KILL_TROLL, -1 }, { N("巨", "P"), TR_KILL_GIANT, -1 },
    { N("デ", "U"), TR_KILL_DEMON, -1 }, { N("死", "L"), TR_KILL_UNDEAD, -1 }, { N("動", "Z"), TR_KILL_ANIMAL, -1 },
};

/*! オブジェクトの特性表示記号テーブルの定義(種族*スレイ*) */
std::vector<flag_insc_table> flag_insc_slay = {
    { N("邪", "*"), TR_SLAY_EVIL, TR_KILL_EVIL }, { N("善", "A"), TR_SLAY_GOOD, TR_KILL_GOOD }, { N("人", "p"), TR_SLAY_HUMAN, TR_KILL_HUMAN }, { N("竜", "D"), TR_SLAY_DRAGON, TR_KILL_DRAGON },
    { N("オ", "o"), TR_SLAY_ORC, TR_KILL_ORC }, { N("ト", "T"), TR_SLAY_TROLL, TR_KILL_TROLL }, { N("巨", "P"), TR_SLAY_GIANT, TR_KILL_GIANT },
    { N("デ", "U"), TR_SLAY_DEMON, TR_KILL_DEMON }, { N("死", "L"), TR_SLAY_UNDEAD, TR_KILL_UNDEAD }, { N("動", "Z"), TR_SLAY_ANIMAL, TR_KILL_ANIMAL },
};

/*! オブジェクトの特性表示記号テーブルの定義(ESP1) */
std::vector<flag_insc_table> flag_insc_esp1 = {
    { N("感", "Tele"), TR_TELEPATHY, -1 }, { N("邪", "Evil"), TR_ESP_EVIL, -1 }, { N("善", "Good"), TR_ESP_GOOD, -1 },
    { N("無", "Nolv"), TR_ESP_NONLIVING, -1 }, { N("個", "Uniq"), TR_ESP_UNIQUE, -1 },
};

/*! オブジェクトの特性表示記号テーブルの定義(ESP2) */
std::vector<flag_insc_table> flag_insc_esp2 = {
    { N("人", "p"), TR_ESP_HUMAN, -1 }, { N("竜", "D"), TR_ESP_DRAGON, -1 }, { N("オ", "o"), TR_ESP_ORC, -1 }, { N("ト", "T"), TR_ESP_TROLL, -1 },
    { N("巨", "P"), TR_ESP_GIANT, -1 }, { N("デ", "U"), TR_ESP_DEMON, -1 }, { N("死", "L"), TR_ESP_UNDEAD, -1 }, { N("動", "Z"), TR_ESP_ANIMAL, -1 },
};

/*! オブジェクトの特性表示記号テーブルの定義(能力維持) */
std::vector<flag_insc_table> flag_insc_sust = {
    { N("腕", "St"), TR_SUST_STR, -1 }, { N("知", "In"), TR_SUST_INT, -1 }, { N("賢", "Wi"), TR_SUST_WIS, -1 },
    { N("器", "Dx"), TR_SUST_DEX, -1 }, { N("耐", "Cn"), TR_SUST_CON, -1 }, { N("魅", "Ch"), TR_SUST_CHR, -1 },
};
// clang format on
