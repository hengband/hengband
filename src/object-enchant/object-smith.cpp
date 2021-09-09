#include "object-enchant/object-smith.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/smith-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "object/item-tester-hooker.h"
#include "object/object-flags.h"
#include "perception/object-perception.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {

/*!
 * @brief エッセンスの順序リスト。エッセンスの表示順等で使用する。
 */
const std::vector<SmithEssence> essence_list_order = {
    SmithEssence::STR,
    SmithEssence::INT,
    SmithEssence::WIS,
    SmithEssence::DEX,
    SmithEssence::CON,
    SmithEssence::CHR,
    SmithEssence::MAGIC_MASTERY,
    SmithEssence::STEALTH,
    SmithEssence::SEARCH,
    SmithEssence::INFRA,
    SmithEssence::TUNNEL,
    SmithEssence::SPEED,
    SmithEssence::BLOWS,
    SmithEssence::CHAOTIC,
    SmithEssence::VAMPIRIC,
    SmithEssence::EATHQUAKE,
    SmithEssence::BRAND_POIS,
    SmithEssence::BRAND_ACID,
    SmithEssence::BRAND_ELEC,
    SmithEssence::BRAND_FIRE,
    SmithEssence::BRAND_COLD,
    SmithEssence::SUST_STATUS,
    SmithEssence::IMMUNITY,
    SmithEssence::REFLECT,
    SmithEssence::FREE_ACT,
    SmithEssence::HOLD_EXP,
    SmithEssence::RES_ACID,
    SmithEssence::RES_ELEC,
    SmithEssence::RES_FIRE,
    SmithEssence::RES_COLD,
    SmithEssence::RES_POIS,
    SmithEssence::RES_FEAR,
    SmithEssence::RES_LITE,
    SmithEssence::RES_DARK,
    SmithEssence::RES_BLIND,
    SmithEssence::RES_CONF,
    SmithEssence::RES_SOUND,
    SmithEssence::RES_SHARDS,
    SmithEssence::RES_NETHER,
    SmithEssence::RES_NEXUS,
    SmithEssence::RES_CHAOS,
    SmithEssence::RES_DISEN,
    SmithEssence::NO_MAGIC,
    SmithEssence::WARNING,
    SmithEssence::LEVITATION,
    SmithEssence::LITE,
    SmithEssence::SEE_INVIS,
    SmithEssence::TELEPATHY,
    SmithEssence::SLOW_DIGEST,
    SmithEssence::REGEN,
    SmithEssence::TELEPORT,
    SmithEssence::SLAY_EVIL,
    SmithEssence::SLAY_ANIMAL,
    SmithEssence::SLAY_UNDEAD,
    SmithEssence::SLAY_DEMON,
    SmithEssence::SLAY_ORC,
    SmithEssence::SLAY_TROLL,
    SmithEssence::SLAY_GIANT,
    SmithEssence::SLAY_DRAGON,
    SmithEssence::SLAY_HUMAN,
    SmithEssence::ATTACK,
    SmithEssence::AC,
};

/*!
 * @brief SmithEssence からエッセンスの表記名を引く連想配列
 */
const std::unordered_map<SmithEssence, concptr> essence_to_name = {
    // clang-format off
    { SmithEssence::STR, _("腕力", "strength") },
    { SmithEssence::INT, _("知能", "intelligen.") },
    { SmithEssence::WIS, _("賢さ", "wisdom") },
    { SmithEssence::DEX, _("器用さ", "dexterity") },
    { SmithEssence::CON, _("耐久力", "constitut.") },
    { SmithEssence::CHR, _("魅力", "charisma") },
    { SmithEssence::MAGIC_MASTERY, _("魔力支配", "magic mast.") },
    { SmithEssence::STEALTH, _("隠密", "stealth") },
    { SmithEssence::SEARCH, _("探索", "searching") },
    { SmithEssence::INFRA, _("赤外線視力", "infravision") },
    { SmithEssence::TUNNEL, _("採掘", "digging") },
    { SmithEssence::SPEED, _("スピード", "speed") },
    { SmithEssence::BLOWS, _("追加攻撃", "extra atk") },
    { SmithEssence::CHAOTIC, _("カオス攻撃", "chaos brand") },
    { SmithEssence::VAMPIRIC, _("吸血攻撃", "vampiric") },
    { SmithEssence::EATHQUAKE, _("地震", "quake") },
    { SmithEssence::BRAND_POIS, _("毒殺", "pois. brand") },
    { SmithEssence::BRAND_ACID, _("溶解", "acid brand") },
    { SmithEssence::BRAND_ELEC, _("電撃", "elec. brand") },
    { SmithEssence::BRAND_FIRE, _("焼棄", "fire brand") },
    { SmithEssence::BRAND_COLD, _("凍結", "cold brand") },
    { SmithEssence::SUST_STATUS, _("能力維持", "sustain") },
    { SmithEssence::IMMUNITY, _("免疫", "immunity") },
    { SmithEssence::REFLECT, _("反射", "reflection") },
    { SmithEssence::FREE_ACT, _("麻痺知らず", "free action") },
    { SmithEssence::HOLD_EXP, _("経験値維持", "hold exp") },
    { SmithEssence::RES_ACID, _("耐酸", "res. acid") },
    { SmithEssence::RES_ELEC, _("耐電撃", "res. elec.") },
    { SmithEssence::RES_FIRE, _("耐火炎", "res. fire") },
    { SmithEssence::RES_COLD, _("耐冷気", "res. cold") },
    { SmithEssence::RES_POIS, _("耐毒", "res. poison") },
    { SmithEssence::RES_FEAR, _("耐恐怖", "res. fear") },
    { SmithEssence::RES_LITE, _("耐閃光", "res. light") },
    { SmithEssence::RES_DARK, _("耐暗黒", "res. dark") },
    { SmithEssence::RES_BLIND, _("耐盲目", "res. blind") },
    { SmithEssence::RES_CONF, _("耐混乱", "res.confuse") },
    { SmithEssence::RES_SOUND, _("耐轟音", "res. sound") },
    { SmithEssence::RES_SHARDS, _("耐破片", "res. shard") },
    { SmithEssence::RES_NETHER, _("耐地獄", "res. nether") },
    { SmithEssence::RES_NEXUS, _("耐因果混乱", "res. nexus") },
    { SmithEssence::RES_CHAOS, _("耐カオス", "res. chaos") },
    { SmithEssence::RES_DISEN, _("耐劣化", "res. disen.") },
    { SmithEssence::NO_MAGIC, _("反魔法", "anti magic") },
    { SmithEssence::WARNING, _("警告", "warning") },
    { SmithEssence::LEVITATION, _("浮遊", "levitation") },
    { SmithEssence::LITE, _("永久光源", "perm. light") },
    { SmithEssence::SEE_INVIS, _("可視透明", "see invis.") },
    { SmithEssence::TELEPATHY, _("テレパシー", "telepathy") },
    { SmithEssence::SLOW_DIGEST, _("遅消化", "slow dige.") },
    { SmithEssence::REGEN, _("急速回復", "regen.") },
    { SmithEssence::TELEPORT, _("テレポート", "teleport") },
    { SmithEssence::SLAY_EVIL, _("邪悪倍打", "slay animal") },
    { SmithEssence::SLAY_ANIMAL, _("動物倍打", "slay evil") },
    { SmithEssence::SLAY_UNDEAD, _("不死倍打", "slay undead") },
    { SmithEssence::SLAY_DEMON, _("悪魔倍打", "slay demon") },
    { SmithEssence::SLAY_ORC, _("オーク倍打", "slay orc") },
    { SmithEssence::SLAY_TROLL, _("トロル倍打", "slay troll") },
    { SmithEssence::SLAY_GIANT, _("巨人倍打", "slay giant") },
    { SmithEssence::SLAY_DRAGON, _("竜倍打", "slay dragon") },
    { SmithEssence::SLAY_HUMAN, _("人間倍打", "slay human") },
    { SmithEssence::ATTACK, _("攻撃", "weapon enc.") },
    { SmithEssence::AC, _("防御", "armor enc.") },
    // clang-format on
};

/*!
 * @brief エッセンス抽出情報構造体
 */
struct essence_drain_type {
    tr_type tr_flag; //!< 抽出する対象アイテムの持つ特性フラグ
    std::vector<SmithEssence> essences; //!< 抽出されるエッセンスのリスト
    int amount; //! エッセンス抽出量。ただしマイナスのものは抽出時のペナルティ源として扱う
};

/*!
 * @brief エッセンス抽出情報テーブル
 */
const std::vector<essence_drain_type> essence_drain_info_table = {
    // clang-format off
    { TR_STR, { SmithEssence::STR }, 10 },
    { TR_INT, { SmithEssence::INT }, 10 },
    { TR_WIS, { SmithEssence::WIS }, 10 },
    { TR_DEX, { SmithEssence::DEX }, 10 },
    { TR_CON, { SmithEssence::CON }, 10 },
    { TR_CHR, { SmithEssence::CHR }, 10 },
    { TR_MAGIC_MASTERY, { SmithEssence::MAGIC_MASTERY }, 10 },
    { TR_FORCE_WEAPON, { SmithEssence::INT, SmithEssence::WIS }, 5 },
    { TR_STEALTH, { SmithEssence::STEALTH }, 10 },
    { TR_SEARCH, { SmithEssence::SEARCH }, 10 },
    { TR_INFRA, { SmithEssence::INFRA }, 10 },
    { TR_TUNNEL, { SmithEssence::TUNNEL }, 10 },
    { TR_SPEED, { SmithEssence::SPEED }, 10 },
    { TR_BLOWS, { SmithEssence::BLOWS }, 10 },
    { TR_CHAOTIC, { SmithEssence::CHAOTIC }, 10 },
    { TR_VAMPIRIC, { SmithEssence::VAMPIRIC }, 10 },
    { TR_SLAY_ANIMAL, { SmithEssence::SLAY_ANIMAL }, 10 },
    { TR_SLAY_EVIL, { SmithEssence::SLAY_EVIL }, 10 },
    { TR_SLAY_UNDEAD, { SmithEssence::SLAY_UNDEAD }, 10 },
    { TR_SLAY_DEMON, { SmithEssence::SLAY_DEMON }, 10 },
    { TR_SLAY_ORC, { SmithEssence::SLAY_ORC }, 10 },
    { TR_SLAY_TROLL, { SmithEssence::SLAY_TROLL }, 10 },
    { TR_SLAY_GIANT, { SmithEssence::SLAY_GIANT }, 10 },
    { TR_SLAY_DRAGON, { SmithEssence::SLAY_DRAGON }, 10 },
    { TR_KILL_DRAGON, { SmithEssence::SLAY_DRAGON }, 10 },
    { TR_VORPAL, { SmithEssence::BRAND_POIS, SmithEssence::BRAND_ACID, SmithEssence::BRAND_ELEC, SmithEssence::BRAND_FIRE, SmithEssence::BRAND_COLD }, 5},
    { TR_EARTHQUAKE, { SmithEssence::EATHQUAKE }, 10 },
    { TR_BRAND_POIS, { SmithEssence::BRAND_POIS }, 10 },
    { TR_BRAND_ACID, { SmithEssence::BRAND_ACID }, 10 },
    { TR_BRAND_ELEC, { SmithEssence::BRAND_ELEC }, 10 },
    { TR_BRAND_FIRE, { SmithEssence::BRAND_FIRE }, 10 },
    { TR_BRAND_COLD, { SmithEssence::BRAND_COLD }, 10 },
    { TR_SUST_STR, { SmithEssence::SUST_STATUS }, 10 },
    { TR_SUST_INT, { SmithEssence::SUST_STATUS }, 10 },
    { TR_SUST_WIS, { SmithEssence::SUST_STATUS }, 10 },
    { TR_SUST_DEX, { SmithEssence::SUST_STATUS }, 10 },
    { TR_SUST_CON, { SmithEssence::SUST_STATUS }, 10 },
    { TR_SUST_CHR, { SmithEssence::SUST_STATUS }, 10 },
    { TR_RIDING, {}, 0 },
    { TR_EASY_SPELL, {}, 0 },
    { TR_IM_ACID, { SmithEssence::IMMUNITY }, 10 },
    { TR_IM_ELEC, { SmithEssence::IMMUNITY }, 10 },
    { TR_IM_FIRE, { SmithEssence::IMMUNITY }, 10 },
    { TR_IM_COLD, { SmithEssence::IMMUNITY }, 10 },
    { TR_THROW, {}, 0 },
    { TR_REFLECT, { SmithEssence::REFLECT }, 10 },
    { TR_FREE_ACT, { SmithEssence::FREE_ACT }, 10 },
    { TR_HOLD_EXP, { SmithEssence::HOLD_EXP }, 10 },
    { TR_RES_ACID, { SmithEssence::RES_ACID }, 10 },
    { TR_RES_ELEC, { SmithEssence::RES_ELEC }, 10 },
    { TR_RES_FIRE, { SmithEssence::RES_FIRE }, 10 },
    { TR_RES_COLD, { SmithEssence::RES_COLD }, 10 },
    { TR_RES_POIS, { SmithEssence::RES_POIS }, 10 },
    { TR_RES_FEAR, { SmithEssence::RES_FEAR }, 10 },
    { TR_RES_LITE, { SmithEssence::RES_LITE }, 10 },
    { TR_RES_DARK, { SmithEssence::RES_DARK }, 10 },
    { TR_RES_BLIND, { SmithEssence::RES_BLIND }, 10 },
    { TR_RES_CONF, { SmithEssence::RES_CONF }, 10 },
    { TR_RES_SOUND, { SmithEssence::RES_SOUND }, 10 },
    { TR_RES_SHARDS, { SmithEssence::RES_SHARDS }, 10 },
    { TR_RES_NETHER, { SmithEssence::RES_NETHER }, 10 },
    { TR_RES_NEXUS, { SmithEssence::RES_NEXUS }, 10 },
    { TR_RES_CHAOS, { SmithEssence::RES_CHAOS }, 10 },
    { TR_RES_DISEN, { SmithEssence::RES_DISEN }, 10 },
    { TR_SH_FIRE, { SmithEssence::BRAND_FIRE, SmithEssence::RES_FIRE }, 10 },
    { TR_SH_ELEC, { SmithEssence::BRAND_ELEC, SmithEssence::RES_ELEC }, 10 },
    { TR_SLAY_HUMAN, { SmithEssence::SLAY_HUMAN }, 10 },
    { TR_SH_COLD, { SmithEssence::BRAND_COLD, SmithEssence::RES_COLD }, 10 },
    { TR_NO_TELE, {}, -1 },
    { TR_NO_MAGIC, { SmithEssence::NO_MAGIC }, 10 },
    { TR_DEC_MANA, { SmithEssence::INT }, 10 },
    { TR_TY_CURSE, {}, -1 },
    { TR_WARNING, { SmithEssence::WARNING }, 10 },
    { TR_HIDE_TYPE, {}, 0 },
    { TR_SHOW_MODS, {}, 0 },
    { TR_SLAY_GOOD, {}, 0 }, // todo
    { TR_LEVITATION, { SmithEssence::LEVITATION }, 10 },
    { TR_LITE_1, { SmithEssence::LITE }, 10 },
    { TR_SEE_INVIS, { SmithEssence::SEE_INVIS }, 10 },
    { TR_TELEPATHY, { SmithEssence::TELEPATHY }, 10 },
    { TR_SLOW_DIGEST, { SmithEssence::SLOW_DIGEST }, 10 },
    { TR_REGEN, { SmithEssence::REGEN }, 10 },
    { TR_XTRA_MIGHT, { SmithEssence::STR }, 10 },
    { TR_XTRA_SHOTS, { SmithEssence::DEX }, 10 },
    { TR_IGNORE_ACID, {}, 0 },
    { TR_IGNORE_ELEC, {}, 0 },
    { TR_IGNORE_FIRE, {}, 0 },
    { TR_IGNORE_COLD, {}, 0 },
    { TR_ACTIVATE, {}, 0 },
    { TR_DRAIN_EXP, {}, -1 },
    { TR_TELEPORT, { SmithEssence::TELEPORT }, 10 },
    { TR_AGGRAVATE, {}, -1 },
    { TR_BLESSED, {}, 0 },
    { TR_ES_ATTACK, {}, 0 },
    { TR_ES_AC, {}, 0 },
    { TR_KILL_GOOD, {}, 0 }, // todo

    { TR_KILL_ANIMAL, { SmithEssence::SLAY_ANIMAL }, 10 },
    { TR_KILL_EVIL, { SmithEssence::SLAY_EVIL }, 10 },
    { TR_KILL_UNDEAD, { SmithEssence::SLAY_UNDEAD }, 10 },
    { TR_KILL_DEMON, { SmithEssence::SLAY_DEMON }, 10 },
    { TR_KILL_ORC, { SmithEssence::SLAY_ORC }, 10 },
    { TR_KILL_TROLL, { SmithEssence::SLAY_TROLL }, 10 },
    { TR_KILL_GIANT, { SmithEssence::SLAY_GIANT }, 10 },
    { TR_KILL_HUMAN, { SmithEssence::SLAY_HUMAN }, 10 },
    { TR_ESP_ANIMAL, { SmithEssence::SLAY_ANIMAL }, 10 },
    { TR_ESP_UNDEAD, { SmithEssence::SLAY_UNDEAD }, 10 },
    { TR_ESP_DEMON, { SmithEssence::SLAY_DEMON }, 10 },
    { TR_ESP_ORC, { SmithEssence::SLAY_ORC}, 10 },
    { TR_ESP_TROLL, { SmithEssence::SLAY_TROLL }, 10 },
    { TR_ESP_GIANT, { SmithEssence::SLAY_GIANT }, 10 },
    { TR_ESP_DRAGON, { SmithEssence::SLAY_DRAGON }, 10 },
    { TR_ESP_HUMAN, { SmithEssence::SLAY_HUMAN }, 10 },
    { TR_ESP_EVIL, { SmithEssence::SLAY_EVIL }, 10 },
    { TR_ESP_GOOD, {}, 0 }, // TODO
    { TR_ESP_NONLIVING, {}, 0 }, // TODO
    { TR_ESP_UNIQUE, {}, 0 }, // TODO
    { TR_FULL_NAME, {}, 0 },
    { TR_FIXED_FLAVOR, {}, 0 },
    { TR_ADD_L_CURSE, {}, -1 },
    { TR_ADD_H_CURSE, {}, -1 },
    { TR_DRAIN_HP, {}, -1 },
    { TR_DRAIN_MANA, {}, -1 },
    { TR_LITE_2, { SmithEssence::LITE }, 20 },
    { TR_LITE_3, { SmithEssence::LITE }, 30 },
    { TR_LITE_M1, {}, 0 },
    { TR_LITE_M2, {}, 0 },
    { TR_LITE_M3, {}, 0 },
    { TR_LITE_FUEL, {}, 0 },
    { TR_CALL_ANIMAL, {}, -1 },
    { TR_CALL_DEMON, {}, -1 },
    { TR_CALL_DRAGON, {}, -1 },
    { TR_CALL_UNDEAD, {}, -1 },
    { TR_COWARDICE, {}, -1 },
    { TR_LOW_MELEE, {}, -1 },
    { TR_LOW_AC, {}, -1 },
    { TR_HARD_SPELL, {}, 0 },
    { TR_FAST_DIGEST, {}, -1 },
    { TR_SLOW_REGEN, {}, -1 },
    { TR_MIGHTY_THROW, {}, 0 },
    { TR_EASY2_WEAPON, { SmithEssence::DEX }, 20 },
    { TR_DOWN_SAVING, {}, 0 },
    { TR_NO_AC, {}, 0 },
    { TR_HEAVY_SPELL, {}, 0 },
    { TR_RES_TIME, {}, 0 }, // TODO
    { TR_RES_WATER, {}, 0 }, // TODO
    { TR_INVULN_ARROW, {}, 0 },
    { TR_DARK_SOURCE, {}, 0 },
    { TR_SUPPORTIVE, {}, 0 },
    { TR_RES_CURSE, {}, 0 }, // TODO
    { TR_BERS_RAGE, {}, 0 },
    { TR_BRAND_MAGIC, {}, 0 },
    { TR_IMPACT, {}, 0 },
    { TR_VUL_ACID, {}, 0 },
    { TR_VUL_COLD, {}, 0 },
    { TR_VUL_ELEC, {}, 0 },
    { TR_VUL_FIRE, {}, 0 },
    { TR_VUL_LITE, {}, 0 },
    { TR_IM_DARK, {}, 0 },
    // clang-format on
};

/*!
 * @brief 鍛冶情報の構造体
 */
struct smith_info_type {
    SmithEffect effect; //!< 鍛冶で与える効果の種類
    concptr name; //!< 鍛冶で与える能力の名称
    SmithCategory category; //!< 鍛冶で与える能力が所属するグループ
    std::vector<SmithEssence> need_essences; //!< 能力を与えるのに必要なエッセンスのリスト
    int consumption; //!< 能力を与えるのに必要な消費量(need_essencesに含まれるエッセンスそれぞれについてこの量を消費)
    TrFlags add_flags; //!< 鍛冶で能力を与えることにより付与されるアイテム特性フラグ
};

/*!
 * @brief 鍛冶情報テーブル
 */
const std::vector<smith_info_type> smith_info_table = {
    { SmithEffect::NONE, _("なし", "None"), SmithCategory::NONE, { SmithEssence::NONE }, 0, {} },
    { SmithEffect::STR, _("腕力", "strength"), SmithCategory::PVAL, { SmithEssence::STR }, 20, { TR_STR } },
    { SmithEffect::INT, _("知能", "intelligence"), SmithCategory::PVAL, { SmithEssence::INT }, 20, { TR_INT } },
    { SmithEffect::WIS, _("賢さ", "wisdom"), SmithCategory::PVAL, { SmithEssence::WIS }, 20, { TR_WIS } },
    { SmithEffect::DEX, _("器用さ", "dexterity"), SmithCategory::PVAL, { SmithEssence::DEX }, 20, { TR_DEX } },
    { SmithEffect::CON, _("耐久力", "constitution"), SmithCategory::PVAL, { SmithEssence::CON }, 20, { TR_CON } },
    { SmithEffect::CHR, _("魅力", "charisma"), SmithCategory::PVAL, { SmithEssence::CHR }, 20, { TR_CHR } },
    { SmithEffect::MAGIC_MASTERY, _("魔力支配", "magic mastery"), SmithCategory::PVAL, { SmithEssence::MAGIC_MASTERY }, 20, { TR_MAGIC_MASTERY } },
    { SmithEffect::STEALTH, _("隠密", "stealth"), SmithCategory::PVAL, { SmithEssence::STEALTH }, 40, { TR_STEALTH } },
    { SmithEffect::SEARCH, _("探索", "searching"), SmithCategory::PVAL, { SmithEssence::SEARCH }, 15, { TR_SEARCH } },
    { SmithEffect::INFRA, _("赤外線視力", "infravision"), SmithCategory::PVAL, { SmithEssence::INFRA }, 15, { TR_INFRA } },
    { SmithEffect::TUNNEL, _("採掘", "digging"), SmithCategory::PVAL, { SmithEssence::TUNNEL }, 15, { TR_TUNNEL } },
    { SmithEffect::SPEED, _("スピード", "speed"), SmithCategory::PVAL, { SmithEssence::SPEED }, 12, { TR_SPEED } },
    { SmithEffect::BLOWS, _("追加攻撃", "extra attack"), SmithCategory::WEAPON_ATTR, { SmithEssence::BLOWS }, 20, { TR_BLOWS } },
    { SmithEffect::CHAOTIC, _("カオス攻撃", "chaos brand"), SmithCategory::WEAPON_ATTR, { SmithEssence::CHAOTIC }, 15, { TR_CHAOTIC } },
    { SmithEffect::VAMPIRIC, _("吸血攻撃", "vampiric brand"), SmithCategory::WEAPON_ATTR, { SmithEssence::VAMPIRIC }, 60, { TR_VAMPIRIC } },
    { SmithEffect::EARTHQUAKE, _("地震発動", "quake activation"), SmithCategory::ETC, { SmithEssence::EATHQUAKE }, 15, { TR_EARTHQUAKE, TR_ACTIVATE } },
    { SmithEffect::BRAND_POIS, _("毒殺", "poison brand"), SmithCategory::WEAPON_ATTR, { SmithEssence::BRAND_POIS }, 20, { TR_BRAND_POIS } },
    { SmithEffect::BRAND_ACID, _("溶解", "acid brand"), SmithCategory::WEAPON_ATTR, { SmithEssence::BRAND_ACID }, 20, { TR_BRAND_ACID } },
    { SmithEffect::BRAND_ELEC, _("電撃", "electric brand"), SmithCategory::WEAPON_ATTR, { SmithEssence::BRAND_ELEC }, 20, { TR_BRAND_ELEC } },
    { SmithEffect::BRAND_FIRE, _("焼棄", "fire brand"), SmithCategory::WEAPON_ATTR, { SmithEssence::BRAND_FIRE }, 20, { TR_BRAND_FIRE } },
    { SmithEffect::BRAND_COLD, _("凍結", "cold brand"), SmithCategory::WEAPON_ATTR, { SmithEssence::BRAND_COLD }, 20, { TR_BRAND_COLD } },
    { SmithEffect::SUST_STR, _("腕力維持", "sustain strength"), SmithCategory::ABILITY, { SmithEssence::SUST_STATUS }, 15, { TR_SUST_STR } },
    { SmithEffect::SUST_INT, _("知能維持", "sustain intelligence"), SmithCategory::ABILITY, { SmithEssence::SUST_STATUS }, 15, { TR_SUST_INT } },
    { SmithEffect::SUST_WIS, _("賢さ維持", "sustain wisdom"), SmithCategory::ABILITY, { SmithEssence::SUST_STATUS }, 15, { TR_SUST_WIS } },
    { SmithEffect::SUST_DEX, _("器用さ維持", "sustain dexterity"), SmithCategory::ABILITY, { SmithEssence::SUST_STATUS }, 15, { TR_SUST_DEX } },
    { SmithEffect::SUST_CON, _("耐久力維持", "sustain constitution"), SmithCategory::ABILITY, { SmithEssence::SUST_STATUS }, 15, { TR_SUST_CON } },
    { SmithEffect::SUST_CHR, _("魅力維持", "sustain charisma"), SmithCategory::ABILITY, { SmithEssence::SUST_STATUS }, 15, { TR_SUST_CHR } },
    { SmithEffect::IM_ACID, _("酸免疫", "acid immunity"), SmithCategory::RESISTANCE, { SmithEssence::IMMUNITY }, 20, { TR_IM_ACID } },
    { SmithEffect::IM_ELEC, _("電撃免疫", "electric immunity"), SmithCategory::RESISTANCE, { SmithEssence::IMMUNITY }, 20, { TR_IM_ELEC } },
    { SmithEffect::IM_FIRE, _("火炎免疫", "fire immunity"), SmithCategory::RESISTANCE, { SmithEssence::IMMUNITY }, 20, { TR_IM_FIRE } },
    { SmithEffect::IM_COLD, _("冷気免疫", "cold immunity"), SmithCategory::RESISTANCE, { SmithEssence::IMMUNITY }, 20, { TR_IM_COLD } },
    { SmithEffect::REFLECT, _("反射", "reflection"), SmithCategory::RESISTANCE, { SmithEssence::REFLECT }, 20, { TR_REFLECT } },
    { SmithEffect::FREE_ACT, _("麻痺知らず", "free action"), SmithCategory::ABILITY, { SmithEssence::FREE_ACT }, 20, { TR_FREE_ACT } },
    { SmithEffect::HOLD_EXP, _("経験値維持", "hold experience"), SmithCategory::ABILITY, { SmithEssence::HOLD_EXP }, 20, { TR_HOLD_EXP } },
    { SmithEffect::RES_ACID, _("耐酸", "resistance to acid"), SmithCategory::RESISTANCE, { SmithEssence::RES_ACID }, 15, { TR_RES_ACID } },
    { SmithEffect::RES_ELEC, _("耐電撃", "resistance to electric"), SmithCategory::RESISTANCE, { SmithEssence::RES_ELEC }, 15, { TR_RES_ELEC } },
    { SmithEffect::RES_FIRE, _("耐火炎", "resistance to fire"), SmithCategory::RESISTANCE, { SmithEssence::RES_FIRE }, 15, { TR_RES_FIRE } },
    { SmithEffect::RES_COLD, _("耐冷気", "resistance to cold"), SmithCategory::RESISTANCE, { SmithEssence::RES_COLD }, 15, { TR_RES_COLD } },
    { SmithEffect::RES_POIS, _("耐毒", "resistance to poison"), SmithCategory::RESISTANCE, { SmithEssence::RES_POIS }, 25, { TR_RES_POIS } },
    { SmithEffect::RES_FEAR, _("耐恐怖", "resistance to fear"), SmithCategory::RESISTANCE, { SmithEssence::RES_FEAR }, 20, { TR_RES_FEAR } },
    { SmithEffect::RES_LITE, _("耐閃光", "resistance to light"), SmithCategory::RESISTANCE, { SmithEssence::RES_LITE }, 20, { TR_RES_LITE } },
    { SmithEffect::RES_DARK, _("耐暗黒", "resistance to dark"), SmithCategory::RESISTANCE, { SmithEssence::RES_DARK }, 20, { TR_RES_DARK } },
    { SmithEffect::RES_BLIND, _("耐盲目", "resistance to blind"), SmithCategory::RESISTANCE, { SmithEssence::RES_BLIND }, 20, { TR_RES_BLIND } },
    { SmithEffect::RES_CONF, _("耐混乱", "resistance to confusion"), SmithCategory::RESISTANCE, { SmithEssence::RES_CONF }, 20, { TR_RES_CONF } },
    { SmithEffect::RES_SOUND, _("耐轟音", "resistance to sound"), SmithCategory::RESISTANCE, { SmithEssence::RES_SOUND }, 20, { TR_RES_SOUND } },
    { SmithEffect::RES_SHARDS, _("耐破片", "resistance to shard"), SmithCategory::RESISTANCE, { SmithEssence::RES_SHARDS }, 20, { TR_RES_SHARDS } },
    { SmithEffect::RES_NETHER, _("耐地獄", "resistance to nether"), SmithCategory::RESISTANCE, { SmithEssence::RES_NETHER }, 20, { TR_RES_NETHER } },
    { SmithEffect::RES_NEXUS, _("耐因果混乱", "resistance to nexus"), SmithCategory::RESISTANCE, { SmithEssence::RES_NEXUS }, 20, { TR_RES_NEXUS } },
    { SmithEffect::RES_CHAOS, _("耐カオス", "resistance to chaos"), SmithCategory::RESISTANCE, { SmithEssence::RES_CHAOS }, 20, { TR_RES_CHAOS } },
    { SmithEffect::RES_DISEN, _("耐劣化", "resistance to disenchantment"), SmithCategory::RESISTANCE, { SmithEssence::RES_DISEN }, 20, { TR_RES_DISEN } },
    { SmithEffect::NO_MAGIC, _("反魔法", "anti magic"), SmithCategory::ABILITY, { SmithEssence::NO_MAGIC }, 15, { TR_NO_MAGIC } },
    { SmithEffect::WARNING, _("警告", "warning"), SmithCategory::ABILITY, { SmithEssence::WARNING }, 20, { TR_WARNING } },
    { SmithEffect::LEVITATION, _("浮遊", "levitation"), SmithCategory::ABILITY, { SmithEssence::LEVITATION }, 20, { TR_LEVITATION } },
    { SmithEffect::LITE, _("永久光源", "permanent light"), SmithCategory::ABILITY, { SmithEssence::LITE }, 15, { TR_LITE_1 } },
    { SmithEffect::SEE_INVIS, _("可視透明", "see invisible"), SmithCategory::ABILITY, { SmithEssence::SEE_INVIS }, 20, { TR_SEE_INVIS } },
    { SmithEffect::TELEPATHY, _("テレパシー", "telepathy"), SmithCategory::ESP, { SmithEssence::TELEPATHY }, 15, { TR_TELEPATHY } },
    { SmithEffect::SLOW_DIGEST, _("遅消化", "slow digestion"), SmithCategory::ABILITY, { SmithEssence::SLOW_DIGEST }, 15, { TR_SLOW_DIGEST } },
    { SmithEffect::REGEN, _("急速回復", "regeneration"), SmithCategory::ABILITY, { SmithEssence::REGEN }, 20, { TR_REGEN } },
    { SmithEffect::TELEPORT, _("テレポート", "teleport"), SmithCategory::ABILITY, { SmithEssence::TELEPORT }, 25, { TR_TELEPORT } },
    { SmithEffect::SLAY_EVIL, _("邪悪倍打", "slay evil"), SmithCategory::SLAYING, { SmithEssence::SLAY_EVIL }, 100, { TR_SLAY_EVIL } },
    { SmithEffect::KILL_EVIL, _("邪悪倍倍打", "kill evil"), SmithCategory::NONE, { SmithEssence::SLAY_EVIL }, 0,
        { TR_KILL_EVIL } }, // 強力すぎるため無効(SmithCategory::NONE)
    { SmithEffect::SLAY_ANIMAL, _("動物倍打", "slay animal"), SmithCategory::SLAYING, { SmithEssence::SLAY_ANIMAL }, 20, { TR_SLAY_ANIMAL } },
    { SmithEffect::KILL_ANIMAL, _("動物倍倍打", "kill animal"), SmithCategory::SLAYING, { SmithEssence::SLAY_ANIMAL }, 60, { TR_KILL_ANIMAL } },
    { SmithEffect::SLAY_UNDEAD, _("不死倍打", "slay undead"), SmithCategory::SLAYING, { SmithEssence::SLAY_UNDEAD }, 20, { TR_SLAY_UNDEAD } },
    { SmithEffect::KILL_UNDEAD, _("不死倍倍打", "kill undead"), SmithCategory::SLAYING, { SmithEssence::SLAY_UNDEAD }, 60, { TR_KILL_UNDEAD } },
    { SmithEffect::SLAY_DEMON, _("悪魔倍打", "slay demon"), SmithCategory::SLAYING, { SmithEssence::SLAY_DEMON }, 20, { TR_SLAY_DEMON } },
    { SmithEffect::KILL_DEMON, _("悪魔倍倍打", "kill demon"), SmithCategory::SLAYING, { SmithEssence::SLAY_DEMON }, 60, { TR_KILL_DEMON } },
    { SmithEffect::SLAY_ORC, _("オーク倍打", "slay orc"), SmithCategory::SLAYING, { SmithEssence::SLAY_ORC }, 20, { TR_SLAY_ORC } },
    { SmithEffect::KILL_ORC, _("オーク倍倍打", "kill orc"), SmithCategory::SLAYING, { SmithEssence::SLAY_ORC }, 60, { TR_KILL_ORC } },
    { SmithEffect::SLAY_TROLL, _("トロル倍打", "slay troll"), SmithCategory::SLAYING, { SmithEssence::SLAY_TROLL }, 20, { TR_SLAY_TROLL } },
    { SmithEffect::KILL_TROLL, _("トロル倍倍打", "kill troll"), SmithCategory::SLAYING, { SmithEssence::SLAY_TROLL }, 60, { TR_KILL_TROLL } },
    { SmithEffect::SLAY_GIANT, _("巨人倍打", "slay giant"), SmithCategory::SLAYING, { SmithEssence::SLAY_GIANT }, 20, { TR_SLAY_GIANT } },
    { SmithEffect::KILL_GIANT, _("巨人倍倍打", "kill giant"), SmithCategory::SLAYING, { SmithEssence::SLAY_GIANT }, 60, { TR_KILL_GIANT } },
    { SmithEffect::SLAY_DRAGON, _("竜倍打", "slay dragon"), SmithCategory::SLAYING, { SmithEssence::SLAY_DRAGON }, 20, { TR_SLAY_DRAGON } },
    { SmithEffect::KILL_DRAGON, _("竜倍倍打", "kill dragon"), SmithCategory::SLAYING, { SmithEssence::SLAY_DRAGON }, 60, { TR_KILL_DRAGON } },
    { SmithEffect::SLAY_HUMAN, _("人間倍打", "slay human"), SmithCategory::SLAYING, { SmithEssence::SLAY_HUMAN }, 20, { TR_SLAY_HUMAN } },
    { SmithEffect::KILL_HUMAN, _("人間倍倍打", "kill human"), SmithCategory::SLAYING, { SmithEssence::SLAY_HUMAN }, 60, { TR_KILL_HUMAN } },
    { SmithEffect::ESP_ANIMAL, _("動物ESP", "sense animal"), SmithCategory::ESP, { SmithEssence::SLAY_ANIMAL }, 40, { TR_ESP_ANIMAL } },
    { SmithEffect::ESP_UNDEAD, _("不死ESP", "sense undead"), SmithCategory::ESP, { SmithEssence::SLAY_UNDEAD }, 40, { TR_ESP_UNDEAD } },
    { SmithEffect::ESP_DEMON, _("悪魔ESP", "sense demon"), SmithCategory::ESP, { SmithEssence::SLAY_DEMON }, 40, { TR_ESP_DEMON } },
    { SmithEffect::ESP_ORC, _("オークESP", "sense orc"), SmithCategory::ESP, { SmithEssence::SLAY_ORC }, 40, { TR_ESP_ORC } },
    { SmithEffect::ESP_TROLL, _("トロルESP", "sense troll"), SmithCategory::ESP, { SmithEssence::SLAY_TROLL }, 40, { TR_ESP_TROLL } },
    { SmithEffect::ESP_GIANT, _("巨人ESP", "sense giant"), SmithCategory::ESP, { SmithEssence::SLAY_GIANT }, 40, { TR_ESP_GIANT } },
    { SmithEffect::ESP_DRAGON, _("竜ESP", "sense dragon"), SmithCategory::ESP, { SmithEssence::SLAY_DRAGON }, 40, { TR_ESP_DRAGON } },
    { SmithEffect::ESP_HUMAN, _("人間ESP", "sense human"), SmithCategory::ESP, { SmithEssence::SLAY_HUMAN }, 40, { TR_ESP_HUMAN } },
    { SmithEffect::ATTACK, _("攻撃", "weapon enchant"), SmithCategory::ENCHANT, { SmithEssence::ATTACK }, 30, {} },
    { SmithEffect::AC, _("防御", "armor enchant"), SmithCategory::ENCHANT, { SmithEssence::AC }, 15, {} },
    { SmithEffect::TMP_RES_ACID, _("酸耐性発動", "resist acid activation"), SmithCategory::ETC, { SmithEssence::RES_ACID }, 50, { TR_RES_ACID, TR_ACTIVATE } },
    { SmithEffect::TMP_RES_ELEC, _("電撃耐性発動", "resist electricity activation"), SmithCategory::ETC, { SmithEssence::RES_ELEC }, 50,
        { TR_RES_ELEC, TR_ACTIVATE } },
    { SmithEffect::TMP_RES_FIRE, _("火炎耐性発動", "resist fire activation"), SmithCategory::ETC, { SmithEssence::RES_FIRE }, 50,
        { TR_RES_FIRE, TR_ACTIVATE } },
    { SmithEffect::TMP_RES_COLD, _("冷気耐性発動", "resist cold activation"), SmithCategory::ETC, { SmithEssence::RES_COLD }, 50,
        { TR_RES_COLD, TR_ACTIVATE } },
    { SmithEffect::SH_FIRE, _("火炎オーラ", "fiery sheath"), SmithCategory::ETC, { SmithEssence::RES_FIRE, SmithEssence::BRAND_FIRE }, 50,
        { TR_RES_FIRE, TR_SH_FIRE } },
    { SmithEffect::SH_ELEC, _("電撃オーラ", "electric sheath"), SmithCategory::ETC, { SmithEssence::RES_ELEC, SmithEssence::BRAND_ELEC }, 50,
        { TR_RES_ELEC, TR_SH_ELEC } },
    { SmithEffect::SH_COLD, _("冷気オーラ", "sheath of coldness"), SmithCategory::ETC, { SmithEssence::RES_COLD, SmithEssence::BRAND_COLD }, 50,
        { TR_RES_COLD, TR_SH_COLD } },
    { SmithEffect::RESISTANCE, _("全耐性", "resistance"), SmithCategory::RESISTANCE,
        { SmithEssence::RES_ACID, SmithEssence::RES_ELEC, SmithEssence::RES_FIRE, SmithEssence::RES_COLD }, 150,
        { TR_RES_ACID, TR_RES_ELEC, TR_RES_FIRE, TR_RES_COLD } },
    { SmithEffect::SUSTAIN, _("装備保持", "elements proof"), SmithCategory::ENCHANT,
        { SmithEssence::RES_ACID, SmithEssence::RES_ELEC, SmithEssence::RES_FIRE, SmithEssence::RES_COLD }, 10, {} },
    { SmithEffect::SLAY_GLOVE, _("殺戮の小手", "gauntlets of slaying"), SmithCategory::WEAPON_ATTR, { SmithEssence::ATTACK }, 200, {} },
};

/**
 * @brief 所持しているエッセンスで付与可能な回数を取得する
 *
 * @param essence_list 使用するエッセンスのリスト
 * @param consumption 1種あたりに使用する消費量
 * @return 所持しているエッセンスで付与可能な回数を返す
 */
int addable_count(const player_type *player_ptr, std::vector<SmithEssence> essence_list, int consumption)
{
    if (consumption <= 0) {
        return 0;
    }

    std::vector<int> addable_count;
    for (auto essence : essence_list) {
        int own_amount = player_ptr->magic_num1[enum2i(essence)];
        addable_count.push_back(own_amount / consumption);
    }
    return *std::min_element(addable_count.begin(), addable_count.end());
}

/*!
 * @brief 引数で指定した鍛冶効果の鍛冶情報を得る
 *
 * @param effect 情報を得る鍛冶効果
 * @return 鍛冶情報構造体へのポインタを保持する std::optional オブジェクトを返す
 */
std::optional<const smith_info_type *> find_smith_info(SmithEffect effect)
{
    // 何度も呼ぶので線形探索を避けるため鍛冶効果から鍛冶情報のテーブルを引けるmapを作成しておく。
    static std::unordered_map<SmithEffect, const smith_info_type *> search_map;
    if (search_map.empty()) {
        for (const auto &info : smith_info_table) {
            search_map.emplace(info.effect, &info);
        }
    }

    auto it = search_map.find(effect);
    if (it == search_map.end()) {
        return std::nullopt;
    }

    return it->second;
}

}

/*!
 * @brief 鍛冶クラスコンストラクタ
 */
Smith::Smith(player_type *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief 指定したエッセンスの表記名を取得する
 *
 * @param essence 表記名を取得するエッセンス
 * @return 表記名を表す文字列へのポインタ
 */
concptr Smith::get_essence_name(SmithEssence essence)
{
    auto it = essence_to_name.find(essence);
    auto essence_name = it != essence_to_name.end() ? it->second : _("不明", "Unknown");
    return essence_name;
}

/**
 * @brief 指定した鍛冶効果の表記名を取得する
 *
 * @param effect 表記名を取得する鍛冶効果
 * @return 表記名を表す文字列へのポインタ
 */
concptr Smith::get_effect_name(SmithEffect effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return _("不明", "Unknown");
    }

    return info.value()->name;
}

/*!
 * @brief 指定した鍛冶効果の付与に必要なエッセンスの表記を取得する。複数のエッセンスが必要な場合は "+" で連結されたものとなる。
 *
 * @param effect 鍛冶効果
 * @return 鍛冶効果の付与に必要なエッセンスを表記する std::string オブジェクト
 */
std::string Smith::get_need_essences_desc(SmithEffect effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value() || info.value()->need_essences.empty()) {
        return _("不明", "Unknown");
    }

    const auto &need_essences = info.value()->need_essences;
    std::stringstream ss;
    for (auto i = 0U; i < need_essences.size(); i++) {
        ss << Smith::get_essence_name(need_essences[i]);
        if (i < need_essences.size() - 1) {
            ss << _("+", " + ");
        }
    }

    return ss.str();
}

/*!
 * @brief 鍛冶効果を付与するのに必要なエッセンスのリストを返す
 *
 * @param effect 鍛冶効果
 * @return 鍛冶効果を付与するのに必要なエッセンスのリスト
 */
std::vector<SmithEssence> Smith::get_need_essences(SmithEffect effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return {};
    }

    return info.value()->need_essences;
}

/*!
 * @brief 鍛冶効果を付与する時のエッセンス消費量を取得する
 * 複数のエッセンスを消費する鍛冶効果の場合は全てのエッセンスからこの量が消費される
 * アイテムが複数ある場合はアイテムの個数倍の消費量となる
 *
 * @param effect 鍛冶効果
 * @param o_ptr 鍛冶効果を付与するアイテムへのポインタ。nullptrの場合はデフォルトの消費量が返される。
 * @return 鍛冶効果を付与する時のエッセンス消費量
 */
int Smith::get_essence_consumption(SmithEffect effect, const object_type *o_ptr)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return 0;
    }

    auto consumption = info.value()->consumption;
    if (o_ptr == nullptr) {
        return consumption;
    }

    if ((o_ptr->tval >= TV_SHOT) && (o_ptr->tval <= TV_BOLT)) {
        consumption = (consumption + 9) / 10;
    }

    consumption *= o_ptr->number;

    return consumption;
}

/*!
 * @brief 鍛冶効果の対象となるアイテムを絞り込むための ItemTester クラスを取得する
 *
 * @param effect 鍛冶効果
 * @return ItemTesterクラスのオブジェクトをstd::unique_ptrに格納したものを返す
 */
std::unique_ptr<ItemTester> Smith::get_item_tester(SmithEffect effect)
{
    auto category = SmithCategory::NONE;
    if (auto info = find_smith_info(effect); info.has_value()) {
        category = info.value()->category;
    }

    if (effect == SmithEffect::SLAY_GLOVE) {
        return std::make_unique<TvalItemTester>(TV_GLOVES);
    }
    if (category == SmithCategory::WEAPON_ATTR || category == SmithCategory::SLAYING) {
        return std::make_unique<FuncItemTester>(&object_type::is_melee_ammo);
    }
    if (effect == SmithEffect::ATTACK) {
        return std::make_unique<FuncItemTester>(&object_type::allow_enchant_weapon);
    }
    if (effect == SmithEffect::AC) {
        return std::make_unique<FuncItemTester>(&object_type::is_armour);
    }

    return std::make_unique<FuncItemTester>(&object_type::is_weapon_armour_ammo);
}

/*!
 * @brief 鍛冶効果により得られる特性フラグを取得する
 *
 * @param effect 鍛冶効果
 * @return 鍛冶効果により得られる特性フラグがONになったTrFlagsオブジェクト
 */
TrFlags Smith::get_effect_tr_flags(SmithEffect effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return {};
    }

    return info.value()->add_flags;
}

/*!
 * @brief 鍛冶効果により得られる発動IDを得る
 *
 * @param effect 鍛冶効果
 * @return 鍛冶効果により得られる発動ID(random_art_activation_type型)
 * 鍛冶効果により得られる発動効果が無い場合は std::nullopt
 */
std::optional<random_art_activation_type> Smith::get_effect_activation(SmithEffect effect)
{
    switch (effect) {
    case SmithEffect::TMP_RES_ACID:
        return ACT_RESIST_ACID;
    case SmithEffect::TMP_RES_ELEC:
        return ACT_RESIST_ELEC;
    case SmithEffect::TMP_RES_FIRE:
        return ACT_RESIST_FIRE;
    case SmithEffect::TMP_RES_COLD:
        return ACT_RESIST_COLD;
    case SmithEffect::EARTHQUAKE:
        return ACT_QUAKE;
    default:
        return std::nullopt;
    }
}

/*!
 * @brief アイテムに付与されている鍛冶効果を取得する
 *
 * @param o_ptr アイテム構造体へのポインタ
 * @return アイテムに付与されている鍛冶効果を保持する std::optional オブジェクト返す。
 * 鍛冶効果が付与できないアイテムか、何も付与されていなければ std::nullopt を返す。
 */
std::optional<SmithEffect> Smith::object_effect(const object_type *o_ptr)
{
    auto effect = static_cast<SmithEffect>(o_ptr->xtra3);
    if (!o_ptr->is_weapon_armour_ammo() || effect == SmithEffect::NONE) {
        return std::nullopt;
    }

    return effect;
}

/*!
 * @brief 指定した鍛冶カテゴリの鍛冶効果のリストを取得する
 *
 * @param category 鍛冶カテゴリ
 * @return 指定した鍛冶カテゴリの鍛冶効果のリスト
 */
std::vector<SmithEffect> Smith::get_effect_list(SmithCategory category)
{
    std::vector<SmithEffect> result;

    for (const auto &info : smith_info_table) {
        if (info.category == category) {
            result.push_back(info.effect);
        }
    }

    return result;
}

/**
 * @brief 指定した鍛冶効果のエッセンスを付与できる回数を取得する
 *
 * @param effect 鍛冶効果
 * @param item_number 同時に付与するスタックしたアイテム数。スタックしている場合アイテム数倍の数だけエッセンスが必要となる。
 * @return エッセンスを付与できる回数を返す
 */
int Smith::get_addable_count(SmithEffect effect, int item_number) const
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return 0;
    }

    return addable_count(this->player_ptr, info.value()->need_essences, info.value()->consumption * item_number);
}

/*!
 * @brief 鍛冶エッセンスの全リストを取得する
 *
 * @return 鍛冶エッセンスの全リスト
 */
const std::vector<SmithEssence> &Smith::get_essence_list()
{
    return essence_list_order;
}

/*!
 * @brief 指定したエッセンスの所持量を取得する
 *
 * @param essence 所持量を取得するエッセンス
 * @return エッセンスの所持量
 */
int Smith::get_essence_num_of_posessions(SmithEssence essence) const
{
    return this->player_ptr->magic_num1[enum2i(essence)];
}

/*!
 * @brief エッセンスの抽出を行う
 *
 * @param o_ptr エッセンスの抽出を行うアイテムへのポインタ
 * @return 抽出したエッセンスと抽出した量のタプルのリストを返す
 */
Smith::DrainEssenceResult Smith::drain_essence(object_type *o_ptr)
{
    // 抽出量を揃えるためKILLフラグのみ付いている場合はSLAYフラグも付ける
    auto old_flgs = object_flags(o_ptr);
    if (old_flgs.has(TR_KILL_DRAGON))
        old_flgs.set(TR_SLAY_DRAGON);
    if (old_flgs.has(TR_KILL_ANIMAL))
        old_flgs.set(TR_SLAY_ANIMAL);
    if (old_flgs.has(TR_KILL_EVIL))
        old_flgs.set(TR_SLAY_EVIL);
    if (old_flgs.has(TR_KILL_UNDEAD))
        old_flgs.set(TR_SLAY_UNDEAD);
    if (old_flgs.has(TR_KILL_DEMON))
        old_flgs.set(TR_SLAY_DEMON);
    if (old_flgs.has(TR_KILL_ORC))
        old_flgs.set(TR_SLAY_ORC);
    if (old_flgs.has(TR_KILL_TROLL))
        old_flgs.set(TR_SLAY_TROLL);
    if (old_flgs.has(TR_KILL_GIANT))
        old_flgs.set(TR_SLAY_GIANT);
    if (old_flgs.has(TR_KILL_HUMAN))
        old_flgs.set(TR_SLAY_HUMAN);
    if (old_flgs.has(TR_KILL_GOOD))
        old_flgs.set(TR_SLAY_GOOD);

    // マイナス効果のあるアイテムから抽出する時のペナルティを計算
    int dec = 4;
    if (o_ptr->curse_flags.has_any_of({ TRC::CURSED, TRC::HEAVY_CURSE, TRC::PERMA_CURSE }))
        dec--;

    for (auto &&info : essence_drain_info_table) {
        if (info.amount < 0 && old_flgs.has(info.tr_flag)) {
            dec += info.amount;
        }
    }

    // アイテムをエッセンス抽出後の状態にする
    const object_type old_o = *o_ptr;
    o_ptr->prep(o_ptr->k_idx);

    o_ptr->iy = old_o.iy;
    o_ptr->ix = old_o.ix;
    o_ptr->marked = old_o.marked;
    o_ptr->number = old_o.number;

    if (o_ptr->tval == TV_DRAG_ARMOR)
        o_ptr->timeout = old_o.timeout;
    o_ptr->ident |= (IDENT_FULL_KNOWN);
    object_aware(player_ptr, o_ptr);
    object_known(o_ptr);

    auto new_flgs = object_flags(o_ptr);

    std::unordered_map<SmithEssence, int> drain_values;

    // 特性フラグからのエッセンス抽出
    for (auto &&info : essence_drain_info_table) {
        int pval = 0;
        if (TR_PVAL_FLAG_MASK.has(info.tr_flag) && old_o.pval > 0) {
            pval = new_flgs.has(info.tr_flag) ? old_o.pval - o_ptr->pval : old_o.pval;
        }

        if ((new_flgs.has_not(info.tr_flag) || pval) && old_flgs.has(info.tr_flag)) {
            for (auto &&essence : info.essences) {
                drain_values[essence] += info.amount * std::max(pval, 1);
            }
        }
    }

    // ダイス/命中/ダメージ/ACからの抽出
    auto diff = [](int o, int n) { return std::max(o - n, 0); };

    if (o_ptr->is_weapon_ammo()) {
        drain_values[SmithEssence::ATTACK] += diff(old_o.ds, o_ptr->ds) * 10;
        drain_values[SmithEssence::ATTACK] += diff(old_o.dd, o_ptr->dd) * 10;
    }

    drain_values[SmithEssence::ATTACK] += diff(old_o.to_h, o_ptr->to_h) * 10;
    drain_values[SmithEssence::ATTACK] += diff(old_o.to_d, o_ptr->to_d) * 10;
    drain_values[SmithEssence::AC] += diff(old_o.ac, o_ptr->ac) * 10;
    drain_values[SmithEssence::AC] += diff(old_o.to_a, o_ptr->to_a) * 10;

    // 個数/矢弾/マイナス効果のペナルティによる抽出量の調整
    for (auto &&[unuse, val] : drain_values) {
        val *= o_ptr->number;
        val = val * dec / 4;
        val = std::max(val, 0);
        if (o_ptr->is_ammo()) {
            val /= 10;
        }
    }

    // 所持エッセンスに追加
    std::vector<std::tuple<SmithEssence, int>> result;

    for (auto essence : essence_list_order) {
        auto drain_value = drain_values[essence];
        if (drain_value <= 0) {
            continue;
        }

        auto i = enum2i(essence);
        this->player_ptr->magic_num1[i] = std::min(20000, this->player_ptr->magic_num1[i] + drain_value);
        result.emplace_back(essence, drain_value);
    }

    return result;
}

/*!
 * @brief 鍛冶効果を付与する
 *
 * @param effect 付与する鍛冶効果
 * @param o_ptr 鍛冶効果を付与するアイテムへのポインタ
 * @param number エッセンス付与数
 * @return 鍛冶効果の付与に成功したら ture、失敗したら false を返す
 */
bool Smith::add_essence(SmithEffect effect, object_type *o_ptr, int number)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return false;
    }

    const auto total_consumption = this->get_essence_consumption(effect, o_ptr) * number;
    for (auto &&essence : info.value()->need_essences) {
        this->player_ptr->magic_num1[enum2i(essence)] -= total_consumption;
    }

    if (effect == SmithEffect::SLAY_GLOVE) {
        HIT_PROB get_to_h = ((number + 1) / 2 + randint0(number / 2 + 1));
        HIT_POINT get_to_d = ((number + 1) / 2 + randint0(number / 2 + 1));
        o_ptr->xtra4 = (get_to_h << 8) + get_to_d;
        o_ptr->to_h += get_to_h;
        o_ptr->to_d += get_to_d;
    }

    if (effect == SmithEffect::ATTACK) {
        const auto max_val = this->player_ptr->lev / 5 + 5;
        if ((o_ptr->to_h >= max_val) && (o_ptr->to_d >= max_val)) {
            return false;
        } else {
            o_ptr->to_h = static_cast<HIT_PROB>(std::min(o_ptr->to_h + 1, max_val));
            o_ptr->to_d = static_cast<HIT_POINT>(std::min(o_ptr->to_d + 1, max_val));
        }
    } else if (effect == SmithEffect::AC) {
        const auto max_val = this->player_ptr->lev / 5 + 5;
        if (o_ptr->to_a >= max_val) {
            return false;
        } else {
            o_ptr->to_a = static_cast<ARMOUR_CLASS>(std::min(o_ptr->to_a + 1, max_val));
        }
    } else if (effect == SmithEffect::SUSTAIN) {
        o_ptr->art_flags.set(TR_IGNORE_ACID);
        o_ptr->art_flags.set(TR_IGNORE_ELEC);
        o_ptr->art_flags.set(TR_IGNORE_FIRE);
        o_ptr->art_flags.set(TR_IGNORE_COLD);
    } else {
        o_ptr->xtra3 = static_cast<decltype(o_ptr->xtra3)>(effect);
    }

    return true;
}

/*!
 * @brief 鍛冶効果を消去する
 *
 * @param o_ptr 鍛冶効果を消去するアイテムへのポインタ
 */
void Smith::erase_essence(object_type *o_ptr) const
{
    auto effect = Smith::object_effect(o_ptr);
    if (effect == SmithEffect::SLAY_GLOVE) {
        o_ptr->to_h -= (o_ptr->xtra4 >> 8);
        o_ptr->to_d -= (o_ptr->xtra4 & 0x000f);
        o_ptr->xtra4 = 0;
        if (o_ptr->to_h < 0)
            o_ptr->to_h = 0;
        if (o_ptr->to_d < 0)
            o_ptr->to_d = 0;
    }
    o_ptr->xtra3 = 0;
    auto flgs = object_flags(o_ptr);
    if (flgs.has_none_of(TR_PVAL_FLAG_MASK))
        o_ptr->pval = 0;
}
