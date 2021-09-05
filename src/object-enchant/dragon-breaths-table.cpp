#include "object-enchant/dragon-breaths-table.h"
#include "spell/spell-types.h"

/*!
 * @brief 装備耐性に準じたブレス効果の選択テーブル /
 * Define flags, effect type, name for dragon breath activation
 */
const dragonbreath_type dragonbreath_info[] = {
	{ TR_RES_ACID, GF_ACID, _("酸", "acid") },
	{ TR_RES_ELEC, GF_ELEC, _("電撃", "lightning") },
	{ TR_RES_FIRE, GF_FIRE, _("火炎", "fire") },
	{ TR_RES_COLD, GF_COLD, _("冷気", "cold") },
	{ TR_RES_POIS, GF_POIS, _("毒", "poison") },
	{ TR_RES_LITE, GF_LITE, _("閃光", "light") },
	{ TR_RES_DARK, GF_DARK, _("暗黒", "dark") },
	{ TR_RES_SHARDS, GF_SHARDS, _("破片", "shards") },
	{ TR_RES_CONF, GF_CONFUSION, _("混乱", "confusion") },
	{ TR_RES_SOUND, GF_SOUND, _("轟音", "sound") },
	{ TR_RES_NEXUS, GF_NEXUS, _("因果混乱", "nexus") },
	{ TR_RES_NETHER, GF_NETHER, _("地獄", "nether") },
	{ TR_RES_CHAOS, GF_CHAOS, _("カオス", "chaos") },
	{ TR_RES_DISEN, GF_DISENCHANT, _("劣化", "disenchantment") },
	{ TR_STR, 0, nullptr }
};
