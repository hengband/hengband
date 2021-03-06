#include "flavor/flag-inscriptions-table.h"
#include "object-enchant/tr-types.h"

/*! @brief アイテムの価値記述テーブル */
const concptr game_inscriptions[MAX_GAME_INSCRIPTIONS] = {
    NULL, /* FEEL_NONE */
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

#ifdef JP
/*! オブジェクトの特性表示記号テーブルの定義(pval要素) */
flag_insc_table flag_insc_plus[MAX_INSCRIPTIONS_PLUS]
    = { { "攻", "At", TR_BLOWS, -1 }, { "速", "Sp", TR_SPEED, -1 }, { "腕", "St", TR_STR, -1 }, { "知", "In", TR_INT, -1 },
    { "賢", "Wi", TR_WIS, -1 }, { "器", "Dx", TR_DEX, -1 }, { "耐", "Cn", TR_CON, -1 }, { "魅", "Ch", TR_CHR, -1 }, { "道", "Md", TR_MAGIC_MASTERY, -1 },
    { "隠", "Sl", TR_STEALTH, -1 }, { "探", "Sr", TR_SEARCH, -1 }, { "赤", "If", TR_INFRA, -1 }, { "掘", "Dg", TR_TUNNEL, -1 }, { NULL, NULL, 0, -1 } };

/*! オブジェクトの特性表示記号テーブルの定義(免疫) */
flag_insc_table flag_insc_immune[MAX_INSCRIPTIONS_IMMUNE]
    = { { "酸", "Ac", TR_IM_ACID, -1 }, { "電", "El", TR_IM_ELEC, -1 }, { "火", "Fi", TR_IM_FIRE, -1 }, { "冷", "Co", TR_IM_COLD, -1 }, { NULL, NULL, 0, -1 } };

/*! オブジェクトの特性表示記号テーブルの定義(耐性) */
flag_insc_table flag_insc_resistance[MAX_INSCRIPTIONS_RESISTANCE] = { { "酸", "Ac", TR_RES_ACID, TR_IM_ACID }, { "電", "El", TR_RES_ELEC, TR_IM_ELEC },
    { "火", "Fi", TR_RES_FIRE, TR_IM_FIRE }, { "冷", "Co", TR_RES_COLD, TR_IM_COLD }, { "毒", "Po", TR_RES_POIS, -1 }, { "閃", "Li", TR_RES_LITE, -1 },
    { "暗", "Dk", TR_RES_DARK, -1 }, { "破", "Sh", TR_RES_SHARDS, -1 }, { "盲", "Bl", TR_RES_BLIND, -1 }, { "乱", "Cf", TR_RES_CONF, -1 },
    { "轟", "So", TR_RES_SOUND, -1 }, { "獄", "Nt", TR_RES_NETHER, -1 }, { "因", "Nx", TR_RES_NEXUS, -1 }, { "沌", "Ca", TR_RES_CHAOS, -1 },
    { "劣", "Di", TR_RES_DISEN, -1 }, { "恐", "Fe", TR_RES_FEAR, -1 }, { NULL, NULL, 0, -1 } };

/*! オブジェクトの特性表示記号テーブルの定義(その他特性) */
flag_insc_table flag_insc_misc[MAX_INSCRIPTIONS_MISC] = { { "易", "Es", TR_EASY_SPELL, -1 }, { "減", "Dm", TR_DEC_MANA, -1 }, { "投", "Th", TR_THROW, -1 },
    { "反", "Rf", TR_REFLECT, -1 }, { "麻", "Fa", TR_FREE_ACT, -1 }, { "視", "Si", TR_SEE_INVIS, -1 }, { "経", "Hl", TR_HOLD_EXP, -1 },
    { "遅", "Sd", TR_SLOW_DIGEST, -1 }, { "活", "Rg", TR_REGEN, -1 }, { "浮", "Lv", TR_LEVITATION, -1 }, { "明", "Lu", TR_LITE_1, -1 },
    { "明", "Lu", TR_LITE_2, -1 }, { "明", "Lu", TR_LITE_3, -1 }, { "闇", "Dl", TR_LITE_M1, -1 }, { "闇", "Dl", TR_LITE_M2, -1 },
    { "闇", "Dl", TR_LITE_M3, -1 }, { "警", "Wr", TR_WARNING, -1 }, { "倍", "Xm", TR_XTRA_MIGHT, -1 }, { "射", "Xs", TR_XTRA_SHOTS, -1 },
    { "瞬", "Te", TR_TELEPORT, -1 }, { "怒", "Ag", TR_AGGRAVATE, -1 }, { "祝", "Bs", TR_BLESSED, -1 }, { "忌", "Ty", TR_TY_CURSE, -1 },
    { "呪", "C-", TR_ADD_L_CURSE, -1 }, { "詛", "C+", TR_ADD_H_CURSE, -1 }, { NULL, NULL, 0, -1 } };

/*! オブジェクトの特性表示記号テーブルの定義(オーラ) */
flag_insc_table flag_insc_aura[MAX_INSCRIPTIONS_AURA] = { { "炎", "F", TR_SH_FIRE, -1 }, { "電", "E", TR_SH_ELEC, -1 }, { "冷", "C", TR_SH_COLD, -1 },
    { "魔", "M", TR_NO_MAGIC, -1 }, { "瞬", "T", TR_NO_TELE, -1 }, { NULL, NULL, 0, -1 } };

/*! オブジェクトの特性表示記号テーブルの定義(属性スレイ) */
flag_insc_table flag_insc_brand[MAX_INSCRIPTIONS_BRAND] = { { "酸", "A", TR_BRAND_ACID, -1 }, { "電", "E", TR_BRAND_ELEC, -1 },
    { "焼", "F", TR_BRAND_FIRE, -1 },
    { "凍", "Co", TR_BRAND_COLD, -1 }, { "毒", "P", TR_BRAND_POIS, -1 }, { "沌", "Ca", TR_CHAOTIC, -1 }, { "吸", "V", TR_VAMPIRIC, -1 },
    { "震", "Q", TR_IMPACT, -1 }, { "切", "S", TR_VORPAL, -1 }, { "理", "M", TR_FORCE_WEAPON, -1 }, { NULL, NULL, 0, -1 } };

/*! オブジェクトの特性表示記号テーブルの定義(種族スレイ) */
flag_insc_table flag_insc_kill[MAX_INSCRIPTIONS_KILL] = { { "邪", "*", TR_KILL_EVIL, -1 }, { "人", "p", TR_KILL_HUMAN, -1 }, { "龍", "D", TR_KILL_DRAGON, -1 },
    { "オ", "o", TR_KILL_ORC, -1 }, { "ト", "T", TR_KILL_TROLL, -1 }, { "巨", "P", TR_KILL_GIANT, -1 }, { "デ", "U", TR_KILL_DEMON, -1 },
    { "死", "L", TR_KILL_UNDEAD, -1 }, { "動", "Z", TR_KILL_ANIMAL, -1 }, { NULL, NULL, 0, -1 } };

/*! オブジェクトの特性表示記号テーブルの定義(種族*スレイ*) */
flag_insc_table flag_insc_slay[MAX_INSCRIPTIONS_SLAY] = { { "邪", "*", TR_SLAY_EVIL, TR_KILL_EVIL }, { "人", "p", TR_SLAY_HUMAN, TR_KILL_HUMAN },
    { "竜", "D", TR_SLAY_DRAGON, TR_KILL_DRAGON }, { "オ", "o", TR_SLAY_ORC, TR_KILL_ORC }, { "ト", "T", TR_SLAY_TROLL, TR_KILL_TROLL },
    { "巨", "P", TR_SLAY_GIANT, TR_KILL_GIANT }, { "デ", "U", TR_SLAY_DEMON, TR_KILL_DEMON }, { "死", "L", TR_SLAY_UNDEAD, TR_KILL_UNDEAD },
    { "動", "Z", TR_SLAY_ANIMAL, TR_KILL_ANIMAL }, { NULL, NULL, 0, -1 } };

/*! オブジェクトの特性表示記号テーブルの定義(ESP1) */
flag_insc_table flag_insc_esp1[MAX_INSCRIPTIONS_ESP_1] = { { "感", "Tele", TR_TELEPATHY, -1 }, { "邪", "Evil", TR_ESP_EVIL, -1 },
    { "善", "Good", TR_ESP_GOOD, -1 },
    { "無", "Nolv", TR_ESP_NONLIVING, -1 }, { "個", "Uniq", TR_ESP_UNIQUE, -1 }, { NULL, NULL, 0, -1 } };

/*! オブジェクトの特性表示記号テーブルの定義(ESP2) */
flag_insc_table flag_insc_esp2[MAX_INSCRIPTIONS_ESP_2] = { { "人", "p", TR_ESP_HUMAN, -1 }, { "竜", "D", TR_ESP_DRAGON, -1 }, { "オ", "o", TR_ESP_ORC, -1 },
    { "ト", "T", TR_ESP_TROLL, -1 }, { "巨", "P", TR_ESP_GIANT, -1 }, { "デ", "U", TR_ESP_DEMON, -1 }, { "死", "L", TR_ESP_UNDEAD, -1 },
    { "動", "Z", TR_ESP_ANIMAL, -1 }, { NULL, NULL, 0, -1 } };

/*! オブジェクトの特性表示記号テーブルの定義(能力維持) */
flag_insc_table flag_insc_sust[MAX_INSCRIPTIONS_SUSTAINER] = { { "腕", "St", TR_SUST_STR, -1 }, { "知", "In", TR_SUST_INT, -1 },
    { "賢", "Wi", TR_SUST_WIS, -1 },
    { "器", "Dx", TR_SUST_DEX, -1 }, { "耐", "Cn", TR_SUST_CON, -1 }, { "魅", "Ch", TR_SUST_CHR, -1 }, { NULL, NULL, 0, -1 } };

#else
flag_insc_table flag_insc_plus[MAX_INSCRIPTIONS_PLUS] = { { "At", TR_BLOWS, -1 }, { "Sp", TR_SPEED, -1 }, { "St", TR_STR, -1 }, { "In", TR_INT, -1 },
    { "Wi", TR_WIS, -1 },
    { "Dx", TR_DEX, -1 }, { "Cn", TR_CON, -1 }, { "Ch", TR_CHR, -1 }, { "Md", TR_MAGIC_MASTERY, -1 }, { "Sl", TR_STEALTH, -1 }, { "Sr", TR_SEARCH, -1 },
    { "If", TR_INFRA, -1 }, { "Dg", TR_TUNNEL, -1 }, { NULL, 0, -1 } };

flag_insc_table flag_insc_immune[MAX_INSCRIPTIONS_IMMUNE]
    = { { "Ac", TR_IM_ACID, -1 }, { "El", TR_IM_ELEC, -1 }, { "Fi", TR_IM_FIRE, -1 }, { "Co", TR_IM_COLD, -1 }, { NULL, 0, -1 } };

flag_insc_table flag_insc_resistance[MAX_INSCRIPTIONS_RESISTANCE] = { { "Ac", TR_RES_ACID, TR_IM_ACID }, { "El", TR_RES_ELEC, TR_IM_ELEC },
    { "Fi", TR_RES_FIRE, TR_IM_FIRE },
    { "Co", TR_RES_COLD, TR_IM_COLD }, { "Po", TR_RES_POIS, -1 }, { "Li", TR_RES_LITE, -1 }, { "Dk", TR_RES_DARK, -1 }, { "Sh", TR_RES_SHARDS, -1 },
    { "Bl", TR_RES_BLIND, -1 }, { "Cf", TR_RES_CONF, -1 }, { "So", TR_RES_SOUND, -1 }, { "Nt", TR_RES_NETHER, -1 }, { "Nx", TR_RES_NEXUS, -1 },
    { "Ca", TR_RES_CHAOS, -1 }, { "Di", TR_RES_DISEN, -1 }, { "Fe", TR_RES_FEAR, -1 }, { NULL, 0, -1 } };

flag_insc_table flag_insc_misc[MAX_INSCRIPTIONS_MISC] = { { "Es", TR_EASY_SPELL, -1 }, { "Dm", TR_DEC_MANA, -1 }, { "Th", TR_THROW, -1 },
    { "Rf", TR_REFLECT, -1 },
    { "Fa", TR_FREE_ACT, -1 }, { "Si", TR_SEE_INVIS, -1 }, { "Hl", TR_HOLD_EXP, -1 }, { "Sd", TR_SLOW_DIGEST, -1 }, { "Rg", TR_REGEN, -1 },
    { "Lv", TR_LEVITATION, -1 }, { "Lu", TR_LITE_1, -1 }, { "Lu", TR_LITE_2, -1 }, { "Lu", TR_LITE_3, -1 }, { "Dl", TR_LITE_M1, -1 }, { "Dl", TR_LITE_M2, -1 },
    { "Dl", TR_LITE_M3, -1 }, { "Wr", TR_WARNING, -1 }, { "Xm", TR_XTRA_MIGHT, -1 }, { "Xs", TR_XTRA_SHOTS, -1 }, { "Te", TR_TELEPORT, -1 },
    { "Ag", TR_AGGRAVATE, -1 }, { "Bs", TR_BLESSED, -1 }, { "Ty", TR_TY_CURSE, -1 }, { "C-", TR_ADD_L_CURSE, -1 }, { "C+", TR_ADD_H_CURSE, -1 },
    { NULL, 0, -1 } };

flag_insc_table flag_insc_aura[MAX_INSCRIPTIONS_AURA]
    = { { "F", TR_SH_FIRE, -1 }, { "E", TR_SH_ELEC, -1 }, { "C", TR_SH_COLD, -1 }, { "M", TR_NO_MAGIC, -1 }, { "T", TR_NO_TELE, -1 }, { NULL, 0, -1 } };

flag_insc_table flag_insc_brand[MAX_INSCRIPTIONS_BRAND]
    = { { "A", TR_BRAND_ACID, -1 }, { "E", TR_BRAND_ELEC, -1 }, { "F", TR_BRAND_FIRE, -1 }, { "Co", TR_BRAND_COLD, -1 }, { "P", TR_BRAND_POIS, -1 },
          { "Ca", TR_CHAOTIC, -1 }, { "V", TR_VAMPIRIC, -1 }, { "Q", TR_IMPACT, -1 }, { "S", TR_VORPAL, -1 }, { "M", TR_FORCE_WEAPON, -1 }, { NULL, 0, -1 } };

flag_insc_table flag_insc_kill[MAX_INSCRIPTIONS_KILL]
    = { { "*", TR_KILL_EVIL, -1 }, { "p", TR_KILL_HUMAN, -1 }, { "D", TR_KILL_DRAGON, -1 }, { "o", TR_KILL_ORC, -1 }, { "T", TR_KILL_TROLL, -1 },
          { "P", TR_KILL_GIANT, -1 }, { "U", TR_KILL_DEMON, -1 }, { "L", TR_KILL_UNDEAD, -1 }, { "Z", TR_KILL_ANIMAL, -1 }, { NULL, 0, -1 } };

flag_insc_table flag_insc_slay[MAX_INSCRIPTIONS_SLAY] = { { "*", TR_SLAY_EVIL, TR_KILL_EVIL }, { "p", TR_SLAY_HUMAN, TR_KILL_HUMAN },
    { "D", TR_SLAY_DRAGON, TR_KILL_DRAGON },
    { "o", TR_SLAY_ORC, TR_KILL_ORC }, { "T", TR_SLAY_TROLL, TR_KILL_TROLL }, { "P", TR_SLAY_GIANT, TR_KILL_GIANT }, { "U", TR_SLAY_DEMON, TR_KILL_DEMON },
    { "L", TR_SLAY_UNDEAD, TR_KILL_UNDEAD }, { "Z", TR_SLAY_ANIMAL, TR_KILL_ANIMAL }, { NULL, 0, -1 } };

flag_insc_table flag_insc_esp1[MAX_INSCRIPTIONS_ESP_1] = { { "Tele", TR_TELEPATHY, -1 }, { "Evil", TR_ESP_EVIL, -1 }, { "Good", TR_ESP_GOOD, -1 },
    { "Nolv", TR_ESP_NONLIVING, -1 }, { "Uniq", TR_ESP_UNIQUE, -1 }, { NULL, 0, -1 } };

flag_insc_table flag_insc_esp2[MAX_INSCRIPTIONS_ESP_2] = { { "p", TR_ESP_HUMAN, -1 }, { "D", TR_ESP_DRAGON, -1 }, { "o", TR_ESP_ORC, -1 },
    { "T", TR_ESP_TROLL, -1 },
    { "P", TR_ESP_GIANT, -1 }, { "U", TR_ESP_DEMON, -1 }, { "L", TR_ESP_UNDEAD, -1 }, { "Z", TR_ESP_ANIMAL, -1 }, { NULL, 0, -1 } };

flag_insc_table flag_insc_sust[MAX_INSCRIPTIONS_SUSTAINER] = { { "St", TR_SUST_STR, -1 }, { "In", TR_SUST_INT, -1 }, { "Wi", TR_SUST_WIS, -1 },
    { "Dx", TR_SUST_DEX, -1 },
    { "Cn", TR_SUST_CON, -1 }, { "Ch", TR_SUST_CHR, -1 }, { NULL, 0, -1 } };
#endif
