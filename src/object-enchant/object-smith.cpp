#include "object-enchant/object-smith.h"
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
const std::vector<Smith::Essence> essence_list_order = {
    Smith::Essence::STR,
    Smith::Essence::INT,
    Smith::Essence::WIS,
    Smith::Essence::DEX,
    Smith::Essence::CON,
    Smith::Essence::CHR,
    Smith::Essence::MAGIC_MASTERY,
    Smith::Essence::STEALTH,
    Smith::Essence::SEARCH,
    Smith::Essence::INFRA,
    Smith::Essence::TUNNEL,
    Smith::Essence::SPEED,
    Smith::Essence::BLOWS,
    Smith::Essence::CHAOTIC,
    Smith::Essence::VAMPIRIC,
    Smith::Essence::EATHQUAKE,
    Smith::Essence::BRAND_POIS,
    Smith::Essence::BRAND_ACID,
    Smith::Essence::BRAND_ELEC,
    Smith::Essence::BRAND_FIRE,
    Smith::Essence::BRAND_COLD,
    Smith::Essence::SUST_STATUS,
    Smith::Essence::IMMUNITY,
    Smith::Essence::REFLECT,
    Smith::Essence::FREE_ACT,
    Smith::Essence::HOLD_EXP,
    Smith::Essence::RES_ACID,
    Smith::Essence::RES_ELEC,
    Smith::Essence::RES_FIRE,
    Smith::Essence::RES_COLD,
    Smith::Essence::RES_POIS,
    Smith::Essence::RES_FEAR,
    Smith::Essence::RES_LITE,
    Smith::Essence::RES_DARK,
    Smith::Essence::RES_BLIND,
    Smith::Essence::RES_CONF,
    Smith::Essence::RES_SOUND,
    Smith::Essence::RES_SHARDS,
    Smith::Essence::RES_NETHER,
    Smith::Essence::RES_NEXUS,
    Smith::Essence::RES_CHAOS,
    Smith::Essence::RES_DISEN,
    Smith::Essence::NO_MAGIC,
    Smith::Essence::WARNING,
    Smith::Essence::LEVITATION,
    Smith::Essence::LITE,
    Smith::Essence::SEE_INVIS,
    Smith::Essence::TELEPATHY,
    Smith::Essence::SLOW_DIGEST,
    Smith::Essence::REGEN,
    Smith::Essence::TELEPORT,
    Smith::Essence::SLAY_EVIL,
    Smith::Essence::SLAY_ANIMAL,
    Smith::Essence::SLAY_UNDEAD,
    Smith::Essence::SLAY_DEMON,
    Smith::Essence::SLAY_ORC,
    Smith::Essence::SLAY_TROLL,
    Smith::Essence::SLAY_GIANT,
    Smith::Essence::SLAY_DRAGON,
    Smith::Essence::SLAY_HUMAN,
    Smith::Essence::ATTACK,
    Smith::Essence::AC,
};

/*!
 * @brief Smith::Essence からエッセンスの表記名を引く連想配列
 */
const std::unordered_map<Smith::Essence, concptr> essence_to_name = {
    // clang-format off
    { Smith::Essence::STR, _("腕力", "strength") },
    { Smith::Essence::INT, _("知能", "intelligen.") },
    { Smith::Essence::WIS, _("賢さ", "wisdom") },
    { Smith::Essence::DEX, _("器用さ", "dexterity") },
    { Smith::Essence::CON, _("耐久力", "constitut.") },
    { Smith::Essence::CHR, _("魅力", "charisma") },
    { Smith::Essence::MAGIC_MASTERY, _("魔力支配", "magic mast.") },
    { Smith::Essence::STEALTH, _("隠密", "stealth") },
    { Smith::Essence::SEARCH, _("探索", "searching") },
    { Smith::Essence::INFRA, _("赤外線視力", "infravision") },
    { Smith::Essence::TUNNEL, _("採掘", "digging") },
    { Smith::Essence::SPEED, _("スピード", "speed") },
    { Smith::Essence::BLOWS, _("追加攻撃", "extra atk") },
    { Smith::Essence::CHAOTIC, _("カオス攻撃", "chaos brand") },
    { Smith::Essence::VAMPIRIC, _("吸血攻撃", "vampiric") },
    { Smith::Essence::EATHQUAKE, _("地震", "quake") },
    { Smith::Essence::BRAND_POIS, _("毒殺", "pois. brand") },
    { Smith::Essence::BRAND_ACID, _("溶解", "acid brand") },
    { Smith::Essence::BRAND_ELEC, _("電撃", "elec. brand") },
    { Smith::Essence::BRAND_FIRE, _("焼棄", "fire brand") },
    { Smith::Essence::BRAND_COLD, _("凍結", "cold brand") },
    { Smith::Essence::SUST_STATUS, _("能力維持", "sustain") },
    { Smith::Essence::IMMUNITY, _("免疫", "immunity") },
    { Smith::Essence::REFLECT, _("反射", "reflection") },
    { Smith::Essence::FREE_ACT, _("麻痺知らず", "free action") },
    { Smith::Essence::HOLD_EXP, _("経験値維持", "hold exp") },
    { Smith::Essence::RES_ACID, _("耐酸", "res. acid") },
    { Smith::Essence::RES_ELEC, _("耐電撃", "res. elec.") },
    { Smith::Essence::RES_FIRE, _("耐火炎", "res. fire") },
    { Smith::Essence::RES_COLD, _("耐冷気", "res. cold") },
    { Smith::Essence::RES_POIS, _("耐毒", "res. poison") },
    { Smith::Essence::RES_FEAR, _("耐恐怖", "res. fear") },
    { Smith::Essence::RES_LITE, _("耐閃光", "res. light") },
    { Smith::Essence::RES_DARK, _("耐暗黒", "res. dark") },
    { Smith::Essence::RES_BLIND, _("耐盲目", "res. blind") },
    { Smith::Essence::RES_CONF, _("耐混乱", "res.confuse") },
    { Smith::Essence::RES_SOUND, _("耐轟音", "res. sound") },
    { Smith::Essence::RES_SHARDS, _("耐破片", "res. shard") },
    { Smith::Essence::RES_NETHER, _("耐地獄", "res. nether") },
    { Smith::Essence::RES_NEXUS, _("耐因果混乱", "res. nexus") },
    { Smith::Essence::RES_CHAOS, _("耐カオス", "res. chaos") },
    { Smith::Essence::RES_DISEN, _("耐劣化", "res. disen.") },
    { Smith::Essence::NO_MAGIC, _("反魔法", "anti magic") },
    { Smith::Essence::WARNING, _("警告", "warning") },
    { Smith::Essence::LEVITATION, _("浮遊", "levitation") },
    { Smith::Essence::LITE, _("永久光源", "perm. light") },
    { Smith::Essence::SEE_INVIS, _("可視透明", "see invis.") },
    { Smith::Essence::TELEPATHY, _("テレパシー", "telepathy") },
    { Smith::Essence::SLOW_DIGEST, _("遅消化", "slow dige.") },
    { Smith::Essence::REGEN, _("急速回復", "regen.") },
    { Smith::Essence::TELEPORT, _("テレポート", "teleport") },
    { Smith::Essence::SLAY_EVIL, _("邪悪倍打", "slay animal") },
    { Smith::Essence::SLAY_ANIMAL, _("動物倍打", "slay evil") },
    { Smith::Essence::SLAY_UNDEAD, _("不死倍打", "slay undead") },
    { Smith::Essence::SLAY_DEMON, _("悪魔倍打", "slay demon") },
    { Smith::Essence::SLAY_ORC, _("オーク倍打", "slay orc") },
    { Smith::Essence::SLAY_TROLL, _("トロル倍打", "slay troll") },
    { Smith::Essence::SLAY_GIANT, _("巨人倍打", "slay giant") },
    { Smith::Essence::SLAY_DRAGON, _("竜倍打", "slay dragon") },
    { Smith::Essence::SLAY_HUMAN, _("人間倍打", "slay human") },
    { Smith::Essence::ATTACK, _("攻撃", "weapon enc.") },
    { Smith::Essence::AC, _("防御", "armor enc.") },
    // clang-format on
};

/*!
 * @brief エッセンス抽出情報構造体
 */
struct essence_drain_type {
    tr_type tr_flag; //!< 抽出する対象アイテムの持つ特性フラグ
    const std::vector<Smith::Essence> essences; //!< 抽出されるエッセンスのリスト
    int amount; //! エッセンス抽出量。ただしマイナスのものは抽出時のペナルティ源として扱う
};

/*!
 * @brief エッセンス抽出情報テーブル
 */
const std::vector<essence_drain_type> essence_drain_info_table = {
    // clang-format off
    { TR_STR, { Smith::Essence::STR }, 10 },
    { TR_INT, { Smith::Essence::INT }, 10 },
    { TR_WIS, { Smith::Essence::WIS }, 10 },
    { TR_DEX, { Smith::Essence::DEX }, 10 },
    { TR_CON, { Smith::Essence::CON }, 10 },
    { TR_CHR, { Smith::Essence::CHR }, 10 },
    { TR_MAGIC_MASTERY, { Smith::Essence::MAGIC_MASTERY }, 10 },
    { TR_FORCE_WEAPON, { Smith::Essence::INT, Smith::Essence::WIS }, 5 },
    { TR_STEALTH, { Smith::Essence::STEALTH }, 10 },
    { TR_SEARCH, { Smith::Essence::SEARCH }, 10 },
    { TR_INFRA, { Smith::Essence::INFRA }, 10 },
    { TR_TUNNEL, { Smith::Essence::TUNNEL }, 10 },
    { TR_SPEED, { Smith::Essence::SPEED }, 10 },
    { TR_BLOWS, { Smith::Essence::BLOWS }, 10 },
    { TR_CHAOTIC, { Smith::Essence::CHAOTIC }, 10 },
    { TR_VAMPIRIC, { Smith::Essence::VAMPIRIC }, 10 },
    { TR_SLAY_ANIMAL, { Smith::Essence::SLAY_ANIMAL }, 10 },
    { TR_SLAY_EVIL, { Smith::Essence::SLAY_EVIL }, 10 },
    { TR_SLAY_UNDEAD, { Smith::Essence::SLAY_UNDEAD }, 10 },
    { TR_SLAY_DEMON, { Smith::Essence::SLAY_DEMON }, 10 },
    { TR_SLAY_ORC, { Smith::Essence::SLAY_ORC }, 10 },
    { TR_SLAY_TROLL, { Smith::Essence::SLAY_TROLL }, 10 },
    { TR_SLAY_GIANT, { Smith::Essence::SLAY_GIANT }, 10 },
    { TR_SLAY_DRAGON, { Smith::Essence::SLAY_DRAGON }, 10 },
    { TR_KILL_DRAGON, { Smith::Essence::SLAY_DRAGON }, 10 },
    { TR_VORPAL, { Smith::Essence::BRAND_POIS, Smith::Essence::BRAND_ACID, Smith::Essence::BRAND_ELEC, Smith::Essence::BRAND_FIRE, Smith::Essence::BRAND_COLD }, 5},
    { TR_EARTHQUAKE, { Smith::Essence::EATHQUAKE }, 10 },
    { TR_BRAND_POIS, { Smith::Essence::BRAND_POIS }, 10 },
    { TR_BRAND_ACID, { Smith::Essence::BRAND_ACID }, 10 },
    { TR_BRAND_ELEC, { Smith::Essence::BRAND_ELEC }, 10 },
    { TR_BRAND_FIRE, { Smith::Essence::BRAND_FIRE }, 10 },
    { TR_BRAND_COLD, { Smith::Essence::BRAND_COLD }, 10 },
    { TR_SUST_STR, { Smith::Essence::SUST_STATUS }, 10 },
    { TR_SUST_INT, { Smith::Essence::SUST_STATUS }, 10 },
    { TR_SUST_WIS, { Smith::Essence::SUST_STATUS }, 10 },
    { TR_SUST_DEX, { Smith::Essence::SUST_STATUS }, 10 },
    { TR_SUST_CON, { Smith::Essence::SUST_STATUS }, 10 },
    { TR_SUST_CHR, { Smith::Essence::SUST_STATUS }, 10 },
    { TR_RIDING, {}, 0 },
    { TR_EASY_SPELL, {}, 0 },
    { TR_IM_ACID, { Smith::Essence::IMMUNITY }, 10 },
    { TR_IM_ELEC, { Smith::Essence::IMMUNITY }, 10 },
    { TR_IM_FIRE, { Smith::Essence::IMMUNITY }, 10 },
    { TR_IM_COLD, { Smith::Essence::IMMUNITY }, 10 },
    { TR_THROW, {}, 0 },
    { TR_REFLECT, { Smith::Essence::REFLECT }, 10 },
    { TR_FREE_ACT, { Smith::Essence::FREE_ACT }, 10 },
    { TR_HOLD_EXP, { Smith::Essence::HOLD_EXP }, 10 },
    { TR_RES_ACID, { Smith::Essence::RES_ACID }, 10 },
    { TR_RES_ELEC, { Smith::Essence::RES_ELEC }, 10 },
    { TR_RES_FIRE, { Smith::Essence::RES_FIRE }, 10 },
    { TR_RES_COLD, { Smith::Essence::RES_COLD }, 10 },
    { TR_RES_POIS, { Smith::Essence::RES_POIS }, 10 },
    { TR_RES_FEAR, { Smith::Essence::RES_FEAR }, 10 },
    { TR_RES_LITE, { Smith::Essence::RES_LITE }, 10 },
    { TR_RES_DARK, { Smith::Essence::RES_DARK }, 10 },
    { TR_RES_BLIND, { Smith::Essence::RES_BLIND }, 10 },
    { TR_RES_CONF, { Smith::Essence::RES_CONF }, 10 },
    { TR_RES_SOUND, { Smith::Essence::RES_SOUND }, 10 },
    { TR_RES_SHARDS, { Smith::Essence::RES_SHARDS }, 10 },
    { TR_RES_NETHER, { Smith::Essence::RES_NETHER }, 10 },
    { TR_RES_NEXUS, { Smith::Essence::RES_NEXUS }, 10 },
    { TR_RES_CHAOS, { Smith::Essence::RES_CHAOS }, 10 },
    { TR_RES_DISEN, { Smith::Essence::RES_DISEN }, 10 },
    { TR_SH_FIRE, { Smith::Essence::BRAND_FIRE, Smith::Essence::RES_FIRE }, 10 },
    { TR_SH_ELEC, { Smith::Essence::BRAND_ELEC, Smith::Essence::RES_ELEC }, 10 },
    { TR_SLAY_HUMAN, { Smith::Essence::SLAY_HUMAN }, 10 },
    { TR_SH_COLD, { Smith::Essence::BRAND_COLD, Smith::Essence::RES_COLD }, 10 },
    { TR_NO_TELE, {}, -1 },
    { TR_NO_MAGIC, { Smith::Essence::NO_MAGIC }, 10 },
    { TR_DEC_MANA, { Smith::Essence::INT }, 10 },
    { TR_TY_CURSE, {}, -1 },
    { TR_WARNING, { Smith::Essence::WARNING }, 10 },
    { TR_HIDE_TYPE, {}, 0 },
    { TR_SHOW_MODS, {}, 0 },
    { TR_SLAY_GOOD, {}, 0 }, // todo
    { TR_LEVITATION, { Smith::Essence::LEVITATION }, 10 },
    { TR_LITE_1, { Smith::Essence::LITE }, 10 },
    { TR_SEE_INVIS, { Smith::Essence::SEE_INVIS }, 10 },
    { TR_TELEPATHY, { Smith::Essence::TELEPATHY }, 10 },
    { TR_SLOW_DIGEST, { Smith::Essence::SLOW_DIGEST }, 10 },
    { TR_REGEN, { Smith::Essence::REGEN }, 10 },
    { TR_XTRA_MIGHT, { Smith::Essence::STR }, 10 },
    { TR_XTRA_SHOTS, { Smith::Essence::DEX }, 10 },
    { TR_IGNORE_ACID, {}, 0 },
    { TR_IGNORE_ELEC, {}, 0 },
    { TR_IGNORE_FIRE, {}, 0 },
    { TR_IGNORE_COLD, {}, 0 },
    { TR_ACTIVATE, {}, 0 },
    { TR_DRAIN_EXP, {}, -1 },
    { TR_TELEPORT, { Smith::Essence::TELEPORT }, 10 },
    { TR_AGGRAVATE, {}, -1 },
    { TR_BLESSED, {}, 0 },
    { TR_ES_ATTACK, {}, 0 },
    { TR_ES_AC, {}, 0 },
    { TR_KILL_GOOD, {}, 0 }, // todo

    { TR_KILL_ANIMAL, { Smith::Essence::SLAY_ANIMAL }, 10 },
    { TR_KILL_EVIL, { Smith::Essence::SLAY_EVIL }, 10 },
    { TR_KILL_UNDEAD, { Smith::Essence::SLAY_UNDEAD }, 10 },
    { TR_KILL_DEMON, { Smith::Essence::SLAY_DEMON }, 10 },
    { TR_KILL_ORC, { Smith::Essence::SLAY_ORC }, 10 },
    { TR_KILL_TROLL, { Smith::Essence::SLAY_TROLL }, 10 },
    { TR_KILL_GIANT, { Smith::Essence::SLAY_GIANT }, 10 },
    { TR_KILL_HUMAN, { Smith::Essence::SLAY_HUMAN }, 10 },
    { TR_ESP_ANIMAL, { Smith::Essence::SLAY_ANIMAL }, 10 },
    { TR_ESP_UNDEAD, { Smith::Essence::SLAY_UNDEAD }, 10 },
    { TR_ESP_DEMON, { Smith::Essence::SLAY_DEMON }, 10 },
    { TR_ESP_ORC, { Smith::Essence::SLAY_ORC}, 10 },
    { TR_ESP_TROLL, { Smith::Essence::SLAY_TROLL }, 10 },
    { TR_ESP_GIANT, { Smith::Essence::SLAY_GIANT }, 10 },
    { TR_ESP_DRAGON, { Smith::Essence::SLAY_DRAGON }, 10 },
    { TR_ESP_HUMAN, { Smith::Essence::SLAY_HUMAN }, 10 },
    { TR_ESP_EVIL, { Smith::Essence::SLAY_EVIL }, 10 },
    { TR_ESP_GOOD, {}, 0 }, // TODO
    { TR_ESP_NONLIVING, {}, 0 }, // TODO
    { TR_ESP_UNIQUE, {}, 0 }, // TODO
    { TR_FULL_NAME, {}, 0 },
    { TR_FIXED_FLAVOR, {}, 0 },
    { TR_ADD_L_CURSE, {}, -1 },
    { TR_ADD_H_CURSE, {}, -1 },
    { TR_DRAIN_HP, {}, -1 },
    { TR_DRAIN_MANA, {}, -1 },
    { TR_LITE_2, { Smith::Essence::LITE }, 20 },
    { TR_LITE_3, { Smith::Essence::LITE }, 30 },
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
    { TR_EASY2_WEAPON, { Smith::Essence::DEX }, 20 },
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
    Smith::Effect effect; //!< 鍛冶で与える効果の種類
    concptr name; //!< 鍛冶で与える能力の名称
    Smith::Category category; //!< 鍛冶で与える能力が所属するグループ
    const std::vector<Smith::Essence> need_essences; //!< 能力を与えるのに必要なエッセンスのリスト
    int consumption; //!< 能力を与えるのに必要な消費量(need_essencesに含まれるエッセンスそれぞれについてこの量を消費)
    TrFlags add_flags; //!< 鍛冶で能力を与えることにより付与されるアイテム特性フラグ
};

/*!
 * @brief 鍛冶情報テーブル
 */
const std::vector<smith_info_type> smith_info_table = {
    { Smith::Effect::NONE, _("なし", "None"), Smith::Category::NONE, { Smith::Essence::NONE }, 0, {} },
    { Smith::Effect::STR, _("腕力", "strength"), Smith::Category::PVAL, { Smith::Essence::STR }, 20, { TR_STR } },
    { Smith::Effect::INT, _("知能", "intelligence"), Smith::Category::PVAL, { Smith::Essence::INT }, 20, { TR_INT } },
    { Smith::Effect::WIS, _("賢さ", "wisdom"), Smith::Category::PVAL, { Smith::Essence::WIS }, 20, { TR_WIS } },
    { Smith::Effect::DEX, _("器用さ", "dexterity"), Smith::Category::PVAL, { Smith::Essence::DEX }, 20, { TR_DEX } },
    { Smith::Effect::CON, _("耐久力", "constitution"), Smith::Category::PVAL, { Smith::Essence::CON }, 20, { TR_CON } },
    { Smith::Effect::CHR, _("魅力", "charisma"), Smith::Category::PVAL, { Smith::Essence::CHR }, 20, { TR_CHR } },
    { Smith::Effect::MAGIC_MASTERY, _("魔力支配", "magic mastery"), Smith::Category::PVAL, { Smith::Essence::MAGIC_MASTERY }, 20, { TR_MAGIC_MASTERY } },
    { Smith::Effect::STEALTH, _("隠密", "stealth"), Smith::Category::PVAL, { Smith::Essence::STEALTH }, 40, { TR_STEALTH } },
    { Smith::Effect::SEARCH, _("探索", "searching"), Smith::Category::PVAL, { Smith::Essence::SEARCH }, 15, { TR_SEARCH } },
    { Smith::Effect::INFRA, _("赤外線視力", "infravision"), Smith::Category::PVAL, { Smith::Essence::INFRA }, 15, { TR_INFRA } },
    { Smith::Effect::TUNNEL, _("採掘", "digging"), Smith::Category::PVAL, { Smith::Essence::TUNNEL }, 15, { TR_TUNNEL } },
    { Smith::Effect::SPEED, _("スピード", "speed"), Smith::Category::PVAL, { Smith::Essence::SPEED }, 12, { TR_SPEED } },
    { Smith::Effect::BLOWS, _("追加攻撃", "extra attack"), Smith::Category::WEAPON_ATTR, { Smith::Essence::BLOWS }, 20, { TR_BLOWS } },
    { Smith::Effect::CHAOTIC, _("カオス攻撃", "chaos brand"), Smith::Category::WEAPON_ATTR, { Smith::Essence::CHAOTIC }, 15, { TR_CHAOTIC } },
    { Smith::Effect::VAMPIRIC, _("吸血攻撃", "vampiric brand"), Smith::Category::WEAPON_ATTR, { Smith::Essence::VAMPIRIC }, 60, { TR_VAMPIRIC } },
    { Smith::Effect::EARTHQUAKE, _("地震発動", "quake activation"), Smith::Category::ETC, { Smith::Essence::EATHQUAKE }, 15, { TR_EARTHQUAKE, TR_ACTIVATE } },
    { Smith::Effect::BRAND_POIS, _("毒殺", "poison brand"), Smith::Category::WEAPON_ATTR, { Smith::Essence::BRAND_POIS }, 20, { TR_BRAND_POIS } },
    { Smith::Effect::BRAND_ACID, _("溶解", "acid brand"), Smith::Category::WEAPON_ATTR, { Smith::Essence::BRAND_ACID }, 20, { TR_BRAND_ACID } },
    { Smith::Effect::BRAND_ELEC, _("電撃", "electric brand"), Smith::Category::WEAPON_ATTR, { Smith::Essence::BRAND_ELEC }, 20, { TR_BRAND_ELEC } },
    { Smith::Effect::BRAND_FIRE, _("焼棄", "fire brand"), Smith::Category::WEAPON_ATTR, { Smith::Essence::BRAND_FIRE }, 20, { TR_BRAND_FIRE } },
    { Smith::Effect::BRAND_COLD, _("凍結", "cold brand"), Smith::Category::WEAPON_ATTR, { Smith::Essence::BRAND_COLD }, 20, { TR_BRAND_COLD } },
    { Smith::Effect::SUST_STR, _("腕力維持", "sustain strength"), Smith::Category::ABILITY, { Smith::Essence::SUST_STATUS }, 15, { TR_SUST_STR } },
    { Smith::Effect::SUST_INT, _("知能維持", "sustain intelligence"), Smith::Category::ABILITY, { Smith::Essence::SUST_STATUS }, 15, { TR_SUST_INT } },
    { Smith::Effect::SUST_WIS, _("賢さ維持", "sustain wisdom"), Smith::Category::ABILITY, { Smith::Essence::SUST_STATUS }, 15, { TR_SUST_WIS } },
    { Smith::Effect::SUST_DEX, _("器用さ維持", "sustain dexterity"), Smith::Category::ABILITY, { Smith::Essence::SUST_STATUS }, 15, { TR_SUST_DEX } },
    { Smith::Effect::SUST_CON, _("耐久力維持", "sustain constitution"), Smith::Category::ABILITY, { Smith::Essence::SUST_STATUS }, 15, { TR_SUST_CON } },
    { Smith::Effect::SUST_CHR, _("魅力維持", "sustain charisma"), Smith::Category::ABILITY, { Smith::Essence::SUST_STATUS }, 15, { TR_SUST_CHR } },
    { Smith::Effect::IM_ACID, _("酸免疫", "acid immunity"), Smith::Category::RESISTANCE, { Smith::Essence::IMMUNITY }, 20, { TR_IM_ACID } },
    { Smith::Effect::IM_ELEC, _("電撃免疫", "electric immunity"), Smith::Category::RESISTANCE, { Smith::Essence::IMMUNITY }, 20, { TR_IM_ELEC } },
    { Smith::Effect::IM_FIRE, _("火炎免疫", "fire immunity"), Smith::Category::RESISTANCE, { Smith::Essence::IMMUNITY }, 20, { TR_IM_FIRE } },
    { Smith::Effect::IM_COLD, _("冷気免疫", "cold immunity"), Smith::Category::RESISTANCE, { Smith::Essence::IMMUNITY }, 20, { TR_IM_COLD } },
    { Smith::Effect::REFLECT, _("反射", "reflection"), Smith::Category::RESISTANCE, { Smith::Essence::REFLECT }, 20, { TR_REFLECT } },
    { Smith::Effect::FREE_ACT, _("麻痺知らず", "free action"), Smith::Category::ABILITY, { Smith::Essence::FREE_ACT }, 20, { TR_FREE_ACT } },
    { Smith::Effect::HOLD_EXP, _("経験値維持", "hold experience"), Smith::Category::ABILITY, { Smith::Essence::HOLD_EXP }, 20, { TR_HOLD_EXP } },
    { Smith::Effect::RES_ACID, _("耐酸", "resistance to acid"), Smith::Category::RESISTANCE, { Smith::Essence::RES_ACID }, 15, { TR_RES_ACID } },
    { Smith::Effect::RES_ELEC, _("耐電撃", "resistance to electric"), Smith::Category::RESISTANCE, { Smith::Essence::RES_ELEC }, 15, { TR_RES_ELEC } },
    { Smith::Effect::RES_FIRE, _("耐火炎", "resistance to fire"), Smith::Category::RESISTANCE, { Smith::Essence::RES_FIRE }, 15, { TR_RES_FIRE } },
    { Smith::Effect::RES_COLD, _("耐冷気", "resistance to cold"), Smith::Category::RESISTANCE, { Smith::Essence::RES_COLD }, 15, { TR_RES_COLD } },
    { Smith::Effect::RES_POIS, _("耐毒", "resistance to poison"), Smith::Category::RESISTANCE, { Smith::Essence::RES_POIS }, 25, { TR_RES_POIS } },
    { Smith::Effect::RES_FEAR, _("耐恐怖", "resistance to fear"), Smith::Category::RESISTANCE, { Smith::Essence::RES_FEAR }, 20, { TR_RES_FEAR } },
    { Smith::Effect::RES_LITE, _("耐閃光", "resistance to light"), Smith::Category::RESISTANCE, { Smith::Essence::RES_LITE }, 20, { TR_RES_LITE } },
    { Smith::Effect::RES_DARK, _("耐暗黒", "resistance to dark"), Smith::Category::RESISTANCE, { Smith::Essence::RES_DARK }, 20, { TR_RES_DARK } },
    { Smith::Effect::RES_BLIND, _("耐盲目", "resistance to blind"), Smith::Category::RESISTANCE, { Smith::Essence::RES_BLIND }, 20, { TR_RES_BLIND } },
    { Smith::Effect::RES_CONF, _("耐混乱", "resistance to confusion"), Smith::Category::RESISTANCE, { Smith::Essence::RES_CONF }, 20, { TR_RES_CONF } },
    { Smith::Effect::RES_SOUND, _("耐轟音", "resistance to sound"), Smith::Category::RESISTANCE, { Smith::Essence::RES_SOUND }, 20, { TR_RES_SOUND } },
    { Smith::Effect::RES_SHARDS, _("耐破片", "resistance to shard"), Smith::Category::RESISTANCE, { Smith::Essence::RES_SHARDS }, 20, { TR_RES_SHARDS } },
    { Smith::Effect::RES_NETHER, _("耐地獄", "resistance to nether"), Smith::Category::RESISTANCE, { Smith::Essence::RES_NETHER }, 20, { TR_RES_NETHER } },
    { Smith::Effect::RES_NEXUS, _("耐因果混乱", "resistance to nexus"), Smith::Category::RESISTANCE, { Smith::Essence::RES_NEXUS }, 20, { TR_RES_NEXUS } },
    { Smith::Effect::RES_CHAOS, _("耐カオス", "resistance to chaos"), Smith::Category::RESISTANCE, { Smith::Essence::RES_CHAOS }, 20, { TR_RES_CHAOS } },
    { Smith::Effect::RES_DISEN, _("耐劣化", "resistance to disenchantment"), Smith::Category::RESISTANCE, { Smith::Essence::RES_DISEN }, 20, { TR_RES_DISEN } },
    { Smith::Effect::NO_MAGIC, _("反魔法", "anti magic"), Smith::Category::ABILITY, { Smith::Essence::NO_MAGIC }, 15, { TR_NO_MAGIC } },
    { Smith::Effect::WARNING, _("警告", "warning"), Smith::Category::ABILITY, { Smith::Essence::WARNING }, 20, { TR_WARNING } },
    { Smith::Effect::LEVITATION, _("浮遊", "levitation"), Smith::Category::ABILITY, { Smith::Essence::LEVITATION }, 20, { TR_LEVITATION } },
    { Smith::Effect::LITE, _("永久光源", "permanent light"), Smith::Category::ABILITY, { Smith::Essence::LITE }, 15, { TR_LITE_1 } },
    { Smith::Effect::SEE_INVIS, _("可視透明", "see invisible"), Smith::Category::ABILITY, { Smith::Essence::SEE_INVIS }, 20, { TR_SEE_INVIS } },
    { Smith::Effect::TELEPATHY, _("テレパシー", "telepathy"), Smith::Category::ESP, { Smith::Essence::TELEPATHY }, 15, { TR_TELEPATHY } },
    { Smith::Effect::SLOW_DIGEST, _("遅消化", "slow digestion"), Smith::Category::ABILITY, { Smith::Essence::SLOW_DIGEST }, 15, { TR_SLOW_DIGEST } },
    { Smith::Effect::REGEN, _("急速回復", "regeneration"), Smith::Category::ABILITY, { Smith::Essence::REGEN }, 20, { TR_REGEN } },
    { Smith::Effect::TELEPORT, _("テレポート", "teleport"), Smith::Category::ABILITY, { Smith::Essence::TELEPORT }, 25, { TR_TELEPORT } },
    { Smith::Effect::SLAY_EVIL, _("邪悪倍打", "slay evil"), Smith::Category::SLAYING, { Smith::Essence::SLAY_EVIL }, 100, { TR_SLAY_EVIL } },
    { Smith::Effect::KILL_EVIL, _("邪悪倍倍打", "kill evil"), Smith::Category::NONE, { Smith::Essence::SLAY_EVIL }, 0,
        { TR_KILL_EVIL } }, // 強力すぎるため無効(Smith::Category::NONE)
    { Smith::Effect::SLAY_ANIMAL, _("動物倍打", "slay animal"), Smith::Category::SLAYING, { Smith::Essence::SLAY_ANIMAL }, 20, { TR_SLAY_ANIMAL } },
    { Smith::Effect::KILL_ANIMAL, _("動物倍倍打", "kill animal"), Smith::Category::SLAYING, { Smith::Essence::SLAY_ANIMAL }, 60, { TR_KILL_ANIMAL } },
    { Smith::Effect::SLAY_UNDEAD, _("不死倍打", "slay undead"), Smith::Category::SLAYING, { Smith::Essence::SLAY_UNDEAD }, 20, { TR_SLAY_UNDEAD } },
    { Smith::Effect::KILL_UNDEAD, _("不死倍倍打", "kill undead"), Smith::Category::SLAYING, { Smith::Essence::SLAY_UNDEAD }, 60, { TR_KILL_UNDEAD } },
    { Smith::Effect::SLAY_DEMON, _("悪魔倍打", "slay demon"), Smith::Category::SLAYING, { Smith::Essence::SLAY_DEMON }, 20, { TR_SLAY_DEMON } },
    { Smith::Effect::KILL_DEMON, _("悪魔倍倍打", "kill demon"), Smith::Category::SLAYING, { Smith::Essence::SLAY_DEMON }, 60, { TR_KILL_DEMON } },
    { Smith::Effect::SLAY_ORC, _("オーク倍打", "slay orc"), Smith::Category::SLAYING, { Smith::Essence::SLAY_ORC }, 20, { TR_SLAY_ORC } },
    { Smith::Effect::KILL_ORC, _("オーク倍倍打", "kill orc"), Smith::Category::SLAYING, { Smith::Essence::SLAY_ORC }, 60, { TR_KILL_ORC } },
    { Smith::Effect::SLAY_TROLL, _("トロル倍打", "slay troll"), Smith::Category::SLAYING, { Smith::Essence::SLAY_TROLL }, 20, { TR_SLAY_TROLL } },
    { Smith::Effect::KILL_TROLL, _("トロル倍倍打", "kill troll"), Smith::Category::SLAYING, { Smith::Essence::SLAY_TROLL }, 60, { TR_KILL_TROLL } },
    { Smith::Effect::SLAY_GIANT, _("巨人倍打", "slay giant"), Smith::Category::SLAYING, { Smith::Essence::SLAY_GIANT }, 20, { TR_SLAY_GIANT } },
    { Smith::Effect::KILL_GIANT, _("巨人倍倍打", "kill giant"), Smith::Category::SLAYING, { Smith::Essence::SLAY_GIANT }, 60, { TR_KILL_GIANT } },
    { Smith::Effect::SLAY_DRAGON, _("竜倍打", "slay dragon"), Smith::Category::SLAYING, { Smith::Essence::SLAY_DRAGON }, 20, { TR_SLAY_DRAGON } },
    { Smith::Effect::KILL_DRAGON, _("竜倍倍打", "kill dragon"), Smith::Category::SLAYING, { Smith::Essence::SLAY_DRAGON }, 60, { TR_KILL_DRAGON } },
    { Smith::Effect::SLAY_HUMAN, _("人間倍打", "slay human"), Smith::Category::SLAYING, { Smith::Essence::SLAY_HUMAN }, 20, { TR_SLAY_HUMAN } },
    { Smith::Effect::KILL_HUMAN, _("人間倍倍打", "kill human"), Smith::Category::SLAYING, { Smith::Essence::SLAY_HUMAN }, 60, { TR_KILL_HUMAN } },
    { Smith::Effect::ESP_ANIMAL, _("動物ESP", "sense animal"), Smith::Category::ESP, { Smith::Essence::SLAY_ANIMAL }, 40, { TR_ESP_ANIMAL } },
    { Smith::Effect::ESP_UNDEAD, _("不死ESP", "sense undead"), Smith::Category::ESP, { Smith::Essence::SLAY_UNDEAD }, 40, { TR_ESP_UNDEAD } },
    { Smith::Effect::ESP_DEMON, _("悪魔ESP", "sense demon"), Smith::Category::ESP, { Smith::Essence::SLAY_DEMON }, 40, { TR_ESP_DEMON } },
    { Smith::Effect::ESP_ORC, _("オークESP", "sense orc"), Smith::Category::ESP, { Smith::Essence::SLAY_ORC }, 40, { TR_ESP_ORC } },
    { Smith::Effect::ESP_TROLL, _("トロルESP", "sense troll"), Smith::Category::ESP, { Smith::Essence::SLAY_TROLL }, 40, { TR_ESP_TROLL } },
    { Smith::Effect::ESP_GIANT, _("巨人ESP", "sense giant"), Smith::Category::ESP, { Smith::Essence::SLAY_GIANT }, 40, { TR_ESP_GIANT } },
    { Smith::Effect::ESP_DRAGON, _("竜ESP", "sense dragon"), Smith::Category::ESP, { Smith::Essence::SLAY_DRAGON }, 40, { TR_ESP_DRAGON } },
    { Smith::Effect::ESP_HUMAN, _("人間ESP", "sense human"), Smith::Category::ESP, { Smith::Essence::SLAY_HUMAN }, 40, { TR_ESP_HUMAN } },
    { Smith::Effect::ATTACK, _("攻撃", "weapon enchant"), Smith::Category::ENCHANT, { Smith::Essence::ATTACK }, 30, {} },
    { Smith::Effect::AC, _("防御", "armor enchant"), Smith::Category::ENCHANT, { Smith::Essence::AC }, 15, {} },
    { Smith::Effect::TMP_RES_ACID, _("酸耐性発動", "resist acid activation"), Smith::Category::ETC, { Smith::Essence::RES_ACID }, 50,
        { TR_RES_ACID, TR_ACTIVATE } },
    { Smith::Effect::TMP_RES_ELEC, _("電撃耐性発動", "resist electricity activation"), Smith::Category::ETC, { Smith::Essence::RES_ELEC }, 50,
        { TR_RES_ELEC, TR_ACTIVATE } },
    { Smith::Effect::TMP_RES_FIRE, _("火炎耐性発動", "resist fire activation"), Smith::Category::ETC, { Smith::Essence::RES_FIRE }, 50,
        { TR_RES_FIRE, TR_ACTIVATE } },
    { Smith::Effect::TMP_RES_COLD, _("冷気耐性発動", "resist cold activation"), Smith::Category::ETC, { Smith::Essence::RES_COLD }, 50,
        { TR_RES_COLD, TR_ACTIVATE } },
    { Smith::Effect::SH_FIRE, _("火炎オーラ", "fiery sheath"), Smith::Category::ETC, { Smith::Essence::RES_FIRE, Smith::Essence::BRAND_FIRE }, 50,
        { TR_RES_FIRE, TR_SH_FIRE } },
    { Smith::Effect::SH_ELEC, _("電撃オーラ", "electric sheath"), Smith::Category::ETC, { Smith::Essence::RES_ELEC, Smith::Essence::BRAND_ELEC }, 50,
        { TR_RES_ELEC, TR_SH_ELEC } },
    { Smith::Effect::SH_COLD, _("冷気オーラ", "sheath of coldness"), Smith::Category::ETC, { Smith::Essence::RES_COLD, Smith::Essence::BRAND_COLD }, 50,
        { TR_RES_COLD, TR_SH_COLD } },
    { Smith::Effect::RESISTANCE, _("全耐性", "resistance"), Smith::Category::RESISTANCE,
        { Smith::Essence::RES_ACID, Smith::Essence::RES_ELEC, Smith::Essence::RES_FIRE, Smith::Essence::RES_COLD }, 150,
        { TR_RES_ACID, TR_RES_ELEC, TR_RES_FIRE, TR_RES_COLD } },
    { Smith::Effect::SUSTAIN, _("装備保持", "elements proof"), Smith::Category::ENCHANT,
        { Smith::Essence::RES_ACID, Smith::Essence::RES_ELEC, Smith::Essence::RES_FIRE, Smith::Essence::RES_COLD }, 10, {} },
    { Smith::Effect::SLAY_GLOVE, _("殺戮の小手", "gauntlets of slaying"), Smith::Category::WEAPON_ATTR, { Smith::Essence::ATTACK }, 200, {} },
};

/**
 * @brief 所持しているエッセンスで付与可能な回数を取得する
 *
 * @param essence_list 使用するエッセンスのリスト
 * @param consumption 1種あたりに使用する消費量
 * @return 所持しているエッセンスで付与可能な回数を返す
 */
int addable_count(const player_type *player_ptr, std::vector<Smith::Essence> essence_list, int consumption)
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
std::optional<const smith_info_type *> find_smith_info(Smith::Effect effect)
{
    // 何度も呼ぶので線形探索を避けるため鍛冶効果から鍛冶情報のテーブルを引けるmapを作成しておく。
    static std::unordered_map<Smith::Effect, const smith_info_type *> search_map;
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
concptr Smith::get_essence_name(Essence essence)
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
concptr Smith::get_effect_name(Effect effect)
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
std::string Smith::get_need_essences_desc(Effect effect)
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
std::vector<Smith::Essence> Smith::get_need_essences(Effect effect)
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
int Smith::get_essence_consumption(Effect effect, const object_type *o_ptr)
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
std::unique_ptr<ItemTester> Smith::get_item_tester(Effect effect)
{
    auto category = Category::NONE;
    if (auto info = find_smith_info(effect); info.has_value()) {
        category = info.value()->category;
    }

    if (effect == Smith::Effect::SLAY_GLOVE) {
        return std::make_unique<TvalItemTester>(TV_GLOVES);
    }
    if (category == Smith::Category::WEAPON_ATTR || category == Smith::Category::SLAYING) {
        return std::make_unique<FuncItemTester>(&object_type::is_melee_ammo);
    }
    if (effect == Smith::Effect::ATTACK) {
        return std::make_unique<FuncItemTester>(&object_type::allow_enchant_weapon);
    }
    if (effect == Smith::Effect::AC) {
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
TrFlags Smith::get_effect_tr_flags(Effect effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return {};
    }

    return info.value()->add_flags;
}

/*!
 * @brief 指定した鍛冶カテゴリの鍛冶効果のリストを取得する
 *
 * @param category 鍛冶カテゴリ
 * @return 指定した鍛冶カテゴリの鍛冶効果のリスト
 */
std::vector<Smith::Effect> Smith::get_effect_list(Category category)
{
    std::vector<Smith::Effect> result;

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
int Smith::get_addable_count(Effect effect, int item_number) const
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
const std::vector<Smith::Essence> &Smith::get_essence_list()
{
    return essence_list_order;
}

/*!
 * @brief 指定したエッセンスの所持量を取得する
 *
 * @param essence 所持量を取得するエッセンス
 * @return エッセンスの所持量
 */
int Smith::get_essence_num_of_posessions(Essence essence) const
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

    std::unordered_map<Essence, int> drain_values;

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
        drain_values[Smith::Essence::ATTACK] += diff(old_o.ds, o_ptr->ds) * 10;
        drain_values[Smith::Essence::ATTACK] += diff(old_o.dd, o_ptr->dd) * 10;
    }

    drain_values[Smith::Essence::ATTACK] += diff(old_o.to_h, o_ptr->to_h) * 10;
    drain_values[Smith::Essence::ATTACK] += diff(old_o.to_d, o_ptr->to_d) * 10;
    drain_values[Smith::Essence::AC] += diff(old_o.ac, o_ptr->ac) * 10;
    drain_values[Smith::Essence::AC] += diff(old_o.to_a, o_ptr->to_a) * 10;

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
    std::vector<std::tuple<Smith::Essence, int>> result;

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
bool Smith::add_essence(Effect effect, object_type *o_ptr, int number)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return false;
    }

    const auto total_consumption = this->get_essence_consumption(effect, o_ptr) * number;
    for (auto &&essence : info.value()->need_essences) {
        this->player_ptr->magic_num1[enum2i(essence)] -= total_consumption;
    }

    if (effect == Smith::Effect::SLAY_GLOVE) {
        auto get_to_h = ((number + 1) / 2 + randint0(number / 2 + 1));
        auto get_to_d = ((number + 1) / 2 + randint0(number / 2 + 1));
        o_ptr->xtra4 = (get_to_h << 8) + get_to_d;
        o_ptr->to_h += get_to_h;
        o_ptr->to_d += get_to_d;
    }

    if (effect == Smith::Effect::ATTACK) {
        const auto max_val = this->player_ptr->lev / 5 + 5;
        if ((o_ptr->to_h >= max_val) && (o_ptr->to_d >= max_val)) {
            return false;
        } else {
            o_ptr->to_h = std::min(o_ptr->to_h + 1, max_val);
            o_ptr->to_d = std::min(o_ptr->to_d + 1, max_val);
        }
    } else if (effect == Smith::Effect::AC) {
        const auto max_val = this->player_ptr->lev / 5 + 5;
        if (o_ptr->to_a >= max_val) {
            return false;
        } else {
            o_ptr->to_a = std::min(o_ptr->to_a + 1, max_val);
        }
    } else if (effect == Smith::Effect::SUSTAIN) {
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
    auto effect = static_cast<Smith::Effect>(o_ptr->xtra3);
    if (effect == Smith::Effect::SLAY_GLOVE) {
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
