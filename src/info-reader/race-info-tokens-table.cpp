#include "info-reader/race-info-tokens-table.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-race/race-ability-flags.h"

/*!
  * モンスターの打撃手段トークンの定義 /
  * Monster Blow Methods
  */
const std::unordered_map<std::string_view, rbm_type> r_info_blow_method = {
	{ "HIT", RBM_HIT },
	{ "TOUCH", RBM_TOUCH },
	{ "PUNCH", RBM_PUNCH },
	{ "KICK", RBM_KICK },
	{ "CLAW", RBM_CLAW },
	{ "BITE", RBM_BITE },
	{ "STING", RBM_STING },
	{ "SLASH", RBM_SLASH },
	{ "BUTT", RBM_BUTT },
	{ "CRUSH", RBM_CRUSH },
	{ "ENGULF", RBM_ENGULF },
	{ "CHARGE", RBM_CHARGE },
	{ "CRAWL", RBM_CRAWL },
	{ "DROOL", RBM_DROOL },
	{ "SPIT", RBM_SPIT },
	{ "EXPLODE", RBM_EXPLODE },
	{ "GAZE", RBM_GAZE },
	{ "WAIL", RBM_WAIL },
	{ "SPORE", RBM_SPORE },
	{ "XXX4", RBM_XXX4 },
	{ "BEG", RBM_BEG },
	{ "INSULT", RBM_INSULT },
	{ "MOAN", RBM_MOAN },
	{ "SHOW", RBM_SHOW },
	{ "SHOOT", RBM_SHOOT },
};

/*!
 * モンスターの打撃属性トークンの定義 /
 * Monster Blow Effects
 */
const std::unordered_map<std::string_view, RaceBlowEffectType> r_info_blow_effect = {
	{ "HURT", RaceBlowEffectType::HURT },
	{ "POISON", RaceBlowEffectType::POISON },
	{ "UN_BONUS", RaceBlowEffectType::UN_BONUS },
	{ "UN_POWER", RaceBlowEffectType::UN_POWER },
	{ "EAT_GOLD", RaceBlowEffectType::EAT_GOLD },
	{ "EAT_ITEM", RaceBlowEffectType::EAT_ITEM },
	{ "EAT_FOOD", RaceBlowEffectType::EAT_FOOD },
	{ "EAT_LITE", RaceBlowEffectType::EAT_LITE },
	{ "ACID", RaceBlowEffectType::ACID },
	{ "ELEC", RaceBlowEffectType::ELEC },
	{ "FIRE", RaceBlowEffectType::FIRE },
	{ "COLD", RaceBlowEffectType::COLD },
	{ "BLIND", RaceBlowEffectType::BLIND },
	{ "CONFUSE", RaceBlowEffectType::CONFUSE },
	{ "TERRIFY", RaceBlowEffectType::TERRIFY },
	{ "PARALYZE", RaceBlowEffectType::PARALYZE },
	{ "LOSE_STR", RaceBlowEffectType::LOSE_STR },
	{ "LOSE_INT", RaceBlowEffectType::LOSE_INT },
	{ "LOSE_WIS", RaceBlowEffectType::LOSE_WIS },
	{ "LOSE_DEX", RaceBlowEffectType::LOSE_DEX },
	{ "LOSE_CON", RaceBlowEffectType::LOSE_CON },
	{ "LOSE_CHR", RaceBlowEffectType::LOSE_CHR },
	{ "LOSE_ALL", RaceBlowEffectType::LOSE_ALL },
	{ "SHATTER", RaceBlowEffectType::SHATTER },
	{ "EXP_10", RaceBlowEffectType::EXP_10 },
	{ "EXP_20", RaceBlowEffectType::EXP_20 },
	{ "EXP_40", RaceBlowEffectType::EXP_40 },
	{ "EXP_80", RaceBlowEffectType::EXP_80 },
	{ "DISEASE", RaceBlowEffectType::DISEASE },
	{ "TIME", RaceBlowEffectType::TIME },
	{ "EXP_VAMP", RaceBlowEffectType::DR_LIFE },
	{ "DR_MANA", RaceBlowEffectType::DR_MANA },
	{ "SUPERHURT", RaceBlowEffectType::SUPERHURT },
	{ "INERTIA", RaceBlowEffectType::INERTIA },
    { "STUN", RaceBlowEffectType::STUN },
    { "HUNGRY", RaceBlowEffectType::HUNGRY },
    { "FLAVOR", RaceBlowEffectType::FLAVOR },
};

/*!
 * モンスター特性トークンの定義1 /
 * Monster race flags
 */
const std::unordered_map<std::string_view, race_flags1> r_info_flags1 = {
	{ "UNIQUE", RF1_UNIQUE },
	{ "QUESTOR", RF1_QUESTOR },
	{ "MALE", RF1_MALE },
	{ "FEMALE", RF1_FEMALE },
	{ "CHAR_CLEAR", RF1_CHAR_CLEAR },
	{ "SHAPECHANGER", RF1_SHAPECHANGER },
	{ "ATTR_CLEAR", RF1_ATTR_CLEAR },
	{ "ATTR_MULTI", RF1_ATTR_MULTI },
	{ "FORCE_DEPTH", RF1_FORCE_DEPTH },
	{ "FORCE_MAXHP", RF1_FORCE_MAXHP },
	{ "PREVENT_SUDDEN_MAGIC", RF1_PREVENT_SUDDEN_MAGIC },
	{ "FORCE_EXTRA", RF1_FORCE_EXTRA },
	{ "ATTR_SEMIRAND", RF1_ATTR_SEMIRAND },
	{ "FRIENDS", RF1_FRIENDS },
	{ "ESCORT", RF1_ESCORT },
	{ "ESCORTS", RF1_ESCORTS },
	{ "NEVER_BLOW", RF1_NEVER_BLOW },
	{ "NEVER_MOVE", RF1_NEVER_MOVE },
	{ "RAND_25", RF1_RAND_25 },
	{ "RAND_50", RF1_RAND_50 },
	{ "ONLY_GOLD", RF1_ONLY_GOLD },
	{ "ONLY_ITEM", RF1_ONLY_ITEM },
	{ "DROP_60", RF1_DROP_60 },
	{ "DROP_90", RF1_DROP_90 },
	{ "DROP_1D2", RF1_DROP_1D2 },
	{ "DROP_2D2", RF1_DROP_2D2 },
	{ "DROP_3D2", RF1_DROP_3D2 },
	{ "DROP_4D2", RF1_DROP_4D2 },
	{ "DROP_GOOD", RF1_DROP_GOOD },
	{ "DROP_GREAT", RF1_DROP_GREAT },
};

/*!
 * モンスター特性トークンの定義2 /
 * Monster race flags
 */
const std::unordered_map<std::string_view, race_flags2> r_info_flags2 = {
	{ "STUPID", RF2_STUPID },
	{ "SMART", RF2_SMART },
	{ "CAN_SPEAK", RF2_CAN_SPEAK },
	{ "REFLECTING", RF2_REFLECTING },
	{ "INVISIBLE", RF2_INVISIBLE },
	{ "COLD_BLOOD", RF2_COLD_BLOOD },
	{ "EMPTY_MIND", RF2_EMPTY_MIND },
	{ "WEIRD_MIND", RF2_WEIRD_MIND },
	{ "MULTIPLY", RF2_MULTIPLY },
	{ "REGENERATE", RF2_REGENERATE },
	{ "CHAR_MULTI", RF2_CHAR_MULTI },
	{ "ATTR_ANY", RF2_ATTR_ANY },
	{ "POWERFUL", RF2_POWERFUL },
	{ "ELDRITCH_HORROR", RF2_ELDRITCH_HORROR },
	{ "FLAGS2_XX14", RF2_XX14 },
	{ "FLAGS2_XX15", RF2_XX15 },
	{ "OPEN_DOOR", RF2_OPEN_DOOR },
	{ "BASH_DOOR", RF2_BASH_DOOR },
	{ "PASS_WALL", RF2_PASS_WALL },
	{ "KILL_WALL", RF2_KILL_WALL },
	{ "MOVE_BODY", RF2_MOVE_BODY },
	{ "KILL_BODY", RF2_KILL_BODY },
	{ "TAKE_ITEM", RF2_TAKE_ITEM },
	{ "KILL_ITEM", RF2_KILL_ITEM },
	{ "HUMAN", RF2_HUMAN },
	{ "QUANTUM", RF2_QUANTUM }
};

/*!
 * モンスター特性トークンの定義3 /
 * Monster race flags
 */
const std::unordered_map<std::string_view, race_flags3> r_info_flags3 = {
	{ "ORC", RF3_ORC },
	{ "TROLL", RF3_TROLL },
	{ "GIANT", RF3_GIANT },
	{ "DRAGON", RF3_DRAGON },
	{ "DEMON", RF3_DEMON },
	{ "UNDEAD", RF3_UNDEAD },
	{ "EVIL", RF3_EVIL },
	{ "ANIMAL", RF3_ANIMAL },
	{ "AMBERITE", RF3_AMBERITE },
	{ "GOOD", RF3_GOOD },
	{ "FLAGS3_XX10", RF3_XX10 },
	{ "NONLIVING", RF3_NONLIVING },
	{ "HURT_LITE", RF3_HURT_LITE },
	{ "HURT_ROCK", RF3_HURT_ROCK },
	{ "HURT_FIRE", RF3_HURT_FIRE },
	{ "HURT_COLD", RF3_HURT_COLD },
	{ "ANGEL", RF3_ANGEL },
	{ "NO_FEAR", RF3_NO_FEAR },
	{ "NO_STUN", RF3_NO_STUN },
	{ "NO_CONF", RF3_NO_CONF },
	{ "NO_SLEEP", RF3_NO_SLEEP }
};

/*!
 * モンスター特性トークン (発動型能力) /
 * Monster race flags
 */
/* clang-format off */
const std::unordered_map<std::string_view, MonsterAbilityType> r_info_ability_flags = {
	{"SHRIEK", MonsterAbilityType::SHRIEK },
	{"XXX1", MonsterAbilityType::XXX1 },
	{"DISPEL", MonsterAbilityType::DISPEL },
	{"ROCKET", MonsterAbilityType::ROCKET },
	{"SHOOT", MonsterAbilityType::SHOOT },
	{"XXX2", MonsterAbilityType::XXX2 },
	{"XXX3", MonsterAbilityType::XXX3 },
	{"XXX4", MonsterAbilityType::XXX4 },
	{"BR_ACID", MonsterAbilityType::BR_ACID },
	{"BR_ELEC", MonsterAbilityType::BR_ELEC },
	{"BR_FIRE", MonsterAbilityType::BR_FIRE },
	{"BR_COLD", MonsterAbilityType::BR_COLD },
	{"BR_POIS", MonsterAbilityType::BR_POIS },
	{"BR_NETH", MonsterAbilityType::BR_NETH },
	{"BR_LITE", MonsterAbilityType::BR_LITE },
	{"BR_DARK", MonsterAbilityType::BR_DARK },
	{"BR_CONF", MonsterAbilityType::BR_CONF },
	{"BR_SOUN", MonsterAbilityType::BR_SOUN },
	{"BR_CHAO", MonsterAbilityType::BR_CHAO },
	{"BR_DISE", MonsterAbilityType::BR_DISE },
	{"BR_NEXU", MonsterAbilityType::BR_NEXU },
	{"BR_TIME", MonsterAbilityType::BR_TIME },
	{"BR_INER", MonsterAbilityType::BR_INER },
	{"BR_GRAV", MonsterAbilityType::BR_GRAV },
	{"BR_SHAR", MonsterAbilityType::BR_SHAR },
	{"BR_PLAS", MonsterAbilityType::BR_PLAS },
	{"BR_FORC", MonsterAbilityType::BR_FORC },
	{"BR_MANA", MonsterAbilityType::BR_MANA },
	{"BA_NUKE", MonsterAbilityType::BA_NUKE },
	{"BR_NUKE", MonsterAbilityType::BR_NUKE },
	{"BA_CHAO", MonsterAbilityType::BA_CHAO },
	{"BR_DISI", MonsterAbilityType::BR_DISI },

	{"BA_ACID", MonsterAbilityType::BA_ACID },
	{"BA_ELEC", MonsterAbilityType::BA_ELEC },
	{"BA_FIRE", MonsterAbilityType::BA_FIRE },
	{"BA_COLD", MonsterAbilityType::BA_COLD },
	{"BA_POIS", MonsterAbilityType::BA_POIS },
	{"BA_NETH", MonsterAbilityType::BA_NETH },
	{"BA_WATE", MonsterAbilityType::BA_WATE },
	{"BA_MANA", MonsterAbilityType::BA_MANA },
	{"BA_DARK", MonsterAbilityType::BA_DARK },
	{"DRAIN_MANA", MonsterAbilityType::DRAIN_MANA },
	{"MIND_BLAST", MonsterAbilityType::MIND_BLAST },
	{"BRAIN_SMASH", MonsterAbilityType::BRAIN_SMASH },
	{"CAUSE_1", MonsterAbilityType::CAUSE_1 },
	{"CAUSE_2", MonsterAbilityType::CAUSE_2 },
	{"CAUSE_3", MonsterAbilityType::CAUSE_3 },
	{"CAUSE_4", MonsterAbilityType::CAUSE_4 },
	{"BO_ACID", MonsterAbilityType::BO_ACID },
	{"BO_ELEC", MonsterAbilityType::BO_ELEC },
	{"BO_FIRE", MonsterAbilityType::BO_FIRE },
	{"BO_COLD", MonsterAbilityType::BO_COLD },
	{"BA_LITE", MonsterAbilityType::BA_LITE },
	{"BO_NETH", MonsterAbilityType::BO_NETH },
	{"BO_WATE", MonsterAbilityType::BO_WATE },
	{"BO_MANA", MonsterAbilityType::BO_MANA },
	{"BO_PLAS", MonsterAbilityType::BO_PLAS },
	{"BO_ICEE", MonsterAbilityType::BO_ICEE },
	{"MISSILE", MonsterAbilityType::MISSILE },
	{"SCARE", MonsterAbilityType::SCARE },
	{"BLIND", MonsterAbilityType::BLIND },
	{"CONF", MonsterAbilityType::CONF },
	{"SLOW", MonsterAbilityType::SLOW },
	{"HOLD", MonsterAbilityType::HOLD },

	{"HASTE", MonsterAbilityType::HASTE },
	{"HAND_DOOM", MonsterAbilityType::HAND_DOOM },
	{"HEAL", MonsterAbilityType::HEAL },
	{"INVULNER", MonsterAbilityType::INVULNER },
	{"BLINK", MonsterAbilityType::BLINK },
	{"TPORT", MonsterAbilityType::TPORT },
	{"WORLD", MonsterAbilityType::WORLD },
	{"SPECIAL", MonsterAbilityType::SPECIAL },
	{"TELE_TO", MonsterAbilityType::TELE_TO },
	{"TELE_AWAY", MonsterAbilityType::TELE_AWAY },
	{"TELE_LEVEL", MonsterAbilityType::TELE_LEVEL },
	{"PSY_SPEAR", MonsterAbilityType::PSY_SPEAR },
	{"DARKNESS", MonsterAbilityType::DARKNESS },
	{"TRAPS", MonsterAbilityType::TRAPS },
	{"FORGET", MonsterAbilityType::FORGET },
	{"ANIM_DEAD", MonsterAbilityType::RAISE_DEAD /* ToDo: Implement ANIM_DEAD */ },
	{"S_KIN", MonsterAbilityType::S_KIN },
	{"S_CYBER", MonsterAbilityType::S_CYBER },
	{"S_MONSTER", MonsterAbilityType::S_MONSTER },
	{"S_MONSTERS", MonsterAbilityType::S_MONSTERS },
	{"S_ANT", MonsterAbilityType::S_ANT },
	{"S_SPIDER", MonsterAbilityType::S_SPIDER },
	{"S_HOUND", MonsterAbilityType::S_HOUND },
	{"S_HYDRA", MonsterAbilityType::S_HYDRA },
	{"S_ANGEL", MonsterAbilityType::S_ANGEL },
	{"S_DEMON", MonsterAbilityType::S_DEMON },
	{"S_UNDEAD", MonsterAbilityType::S_UNDEAD },
	{"S_DRAGON", MonsterAbilityType::S_DRAGON },
	{"S_HI_UNDEAD", MonsterAbilityType::S_HI_UNDEAD },
	{"S_HI_DRAGON", MonsterAbilityType::S_HI_DRAGON },
	{"S_AMBERITES", MonsterAbilityType::S_AMBERITES },
	{"S_UNIQUE", MonsterAbilityType::S_UNIQUE },
};
/* clang-format on */

/*!
 * モンスター特性トークンの定義7 /
 * Monster race flags
 * "GUARDIAN" ... init.c d_infoの FINAL_GUARDIAN_* にて自動指定
 */
const std::unordered_map<std::string_view, race_flags7> r_info_flags7 = {
	{ "AQUATIC", RF7_AQUATIC },
	{ "CAN_SWIM", RF7_CAN_SWIM },
	{ "CAN_FLY", RF7_CAN_FLY },
	{ "FRIENDLY", RF7_FRIENDLY },
	{ "NAZGUL", RF7_NAZGUL },
	{ "UNIQUE2", RF7_UNIQUE2 },
	{ "RIDING", RF7_RIDING },
	{ "KAGE", RF7_KAGE },
	{ "HAS_LITE_1", RF7_HAS_LITE_1 },
	{ "SELF_LITE_1", RF7_SELF_LITE_1 },
	{ "HAS_LITE_2", RF7_HAS_LITE_2 },
	{ "SELF_LITE_2", RF7_SELF_LITE_2 },
	{ "CHAMELEON", RF7_CHAMELEON },
	{ "TANUKI", RF7_TANUKI },
	{ "HAS_DARK_1", RF7_HAS_DARK_1 },
	{ "SELF_DARK_1", RF7_SELF_DARK_1 },
	{ "HAS_DARK_2", RF7_HAS_DARK_2 },
	{ "SELF_DARK_2", RF7_SELF_DARK_2 },
};

/*!
 * モンスター特性トークンの定義8 /
 * Monster race flags
 */
const std::unordered_map<std::string_view, race_flags8> r_info_flags8 = {
	{ "WILD_ONLY", RF8_WILD_ONLY },
	{ "WILD_TOWN", RF8_WILD_TOWN },
    { "NO_QUEST", RF8_NO_QUEST },
    { "WILD_SHORE", RF8_WILD_SHORE },
	{ "WILD_OCEAN", RF8_WILD_OCEAN },
	{ "WILD_WASTE", RF8_WILD_WASTE },
	{ "WILD_WOOD", RF8_WILD_WOOD },
	{ "WILD_VOLCANO", RF8_WILD_VOLCANO },
	{ "WILD_MOUNTAIN", RF8_WILD_MOUNTAIN },
	{ "WILD_GRASS", RF8_WILD_GRASS },
	{ "WILD_SWAMP", RF8_WILD_SWAMP },
	{ "WILD_ALL", RF8_WILD_ALL },
};

/*!
 * モンスター特性トークンの定義9 /
 * Monster race flags
 */
const std::unordered_map<std::string_view, race_flags9> r_info_flags9 = {
	{ "DROP_CORPSE", RF9_DROP_CORPSE },
	{ "DROP_SKELETON", RF9_DROP_SKELETON },
	{ "EAT_BLIND", RF9_EAT_BLIND },
	{ "EAT_CONF", RF9_EAT_CONF },
	{ "EAT_MANA", RF9_EAT_MANA },
	{ "EAT_NEXUS", RF9_EAT_NEXUS },
	// { "EAT_BLINK", RF9_EAT_BLINK }, //<! @note フラグ未定義
	{ "EAT_SLEEP", RF9_EAT_SLEEP },
	{ "EAT_BERSERKER", RF9_EAT_BERSERKER },
	{ "EAT_ACIDIC", RF9_EAT_ACIDIC },
	{ "EAT_SPEED", RF9_EAT_SPEED },
	{ "EAT_CURE", RF9_EAT_CURE },
	{ "EAT_FIRE_RES", RF9_EAT_FIRE_RES },
	{ "EAT_COLD_RES", RF9_EAT_COLD_RES },
	{ "EAT_ACID_RES", RF9_EAT_ACID_RES },
	{ "EAT_ELEC_RES", RF9_EAT_ELEC_RES },
	{ "EAT_POIS_RES", RF9_EAT_POIS_RES },
	{ "EAT_INSANITY", RF9_EAT_INSANITY },
	{ "EAT_DRAIN_EXP", RF9_EAT_DRAIN_EXP },
	{ "EAT_POISONOUS", RF9_EAT_POISONOUS },
	{ "EAT_GIVE_STR", RF9_EAT_GIVE_STR },
	{ "EAT_GIVE_INT", RF9_EAT_GIVE_INT },
	{ "EAT_GIVE_WIS", RF9_EAT_GIVE_WIS },
	{ "EAT_GIVE_DEX", RF9_EAT_GIVE_DEX },
	{ "EAT_GIVE_CON", RF9_EAT_GIVE_CON },
	{ "EAT_GIVE_CHR", RF9_EAT_GIVE_CHR },
	{ "EAT_LOSE_STR", RF9_EAT_LOSE_STR },
	{ "EAT_LOSE_INT", RF9_EAT_LOSE_INT },
	{ "EAT_LOSE_WIS", RF9_EAT_LOSE_WIS },
	{ "EAT_LOSE_DEX", RF9_EAT_LOSE_DEX },
	{ "EAT_LOSE_CON", RF9_EAT_LOSE_CON },
	{ "EAT_LOSE_CHR", RF9_EAT_LOSE_CHR },
	{ "EAT_DRAIN_MANA", RF9_EAT_DRAIN_MANA },
};

/*!
 * モンスター特性トークンの定義R(耐性) /
 * Monster race flags
 */
const std::unordered_map<std::string_view, race_flags_resistance> r_info_flagsr = {
	{ "IM_ACID", RFR_IM_ACID },
	{ "IM_ELEC", RFR_IM_ELEC },
	{ "IM_FIRE", RFR_IM_FIRE },
	{ "IM_COLD", RFR_IM_COLD },
	{ "IM_POIS", RFR_IM_POIS },
	{ "RES_LITE", RFR_RES_LITE },
	{ "RES_DARK", RFR_RES_DARK },
	{ "RES_NETH", RFR_RES_NETH },
	{ "RES_WATE", RFR_RES_WATE },
	{ "RES_PLAS", RFR_RES_PLAS },
	{ "RES_SHAR", RFR_RES_SHAR },
	{ "RES_SOUN", RFR_RES_SOUN },
	{ "RES_CHAO", RFR_RES_CHAO },
	{ "RES_NEXU", RFR_RES_NEXU },
	{ "RES_DISE", RFR_RES_DISE },
	{ "RES_WALL", RFR_RES_WALL },
	{ "RES_INER", RFR_RES_INER },
	{ "RES_TIME", RFR_RES_TIME },
	{ "RES_GRAV", RFR_RES_GRAV },
	{ "RES_ALL", RFR_RES_ALL },
	{ "RES_TELE", RFR_RES_TELE },
};

const std::unordered_map<std::string_view, MonsterAuraType> r_info_aura_flags = {
    { "AURA_FIRE", MonsterAuraType::FIRE },
    { "AURA_COLD", MonsterAuraType::COLD },
    { "AURA_ELEC", MonsterAuraType::ELEC },
    { "AURA_ACID", MonsterAuraType::ACID },
    { "AURA_POISON", MonsterAuraType::POISON },
    { "AURA_NUKE", MonsterAuraType::NUKE },
    { "AURA_PLASMA", MonsterAuraType::PLASMA },
    { "AURA_WATER", MonsterAuraType::WATER },
    { "AURA_ICEE", MonsterAuraType::ICEE },
    { "AURA_LITE", MonsterAuraType::LITE },
    { "AURA_DARK", MonsterAuraType::DARK },
    { "AURA_SHARDS", MonsterAuraType::SHARDS },
    { "AURA_FORCE", MonsterAuraType::FORCE },
    { "AURA_MANA", MonsterAuraType::MANA },
    { "AURA_METEOR", MonsterAuraType::METEOR },
    { "AURA_CHAOS", MonsterAuraType::CHAOS },
    { "AURA_HOLINESS", MonsterAuraType::HOLINESS },
    { "AURA_NETHER", MonsterAuraType::NETHER },
    { "AURA_DISENCHANT", MonsterAuraType::DISENCHANT },
    { "AURA_NEXUS", MonsterAuraType::NEXUS },
    { "AURA_TIME", MonsterAuraType::TIME },
    { "AURA_GRAVITY", MonsterAuraType::GRAVITY },
    { "AURA_VOIDS", MonsterAuraType::VOIDS },
    { "AURA_ABYSS", MonsterAuraType::ABYSS },
};
