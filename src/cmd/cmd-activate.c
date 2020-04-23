/*!
* @file cmd-activate.c
* @brief プレイヤーの発動コマンド実装
* @date 2018/09/07
* @details
* cmd6.cより分離。
*/

#include "angband.h"
#include "main/sound-definitions-table.h"

#include "cmd/cmd-activate.h"
#include "cmd/cmd-basic.h"
#include "cmd/cmd-save.h"
#include "object-hook.h"
#include "sort.h"
#include "artifact.h"
#include "avatar.h"
#include "spells-summon.h"
#include "spells-status.h"
#include "spells-object.h"
#include "spells-floor.h"
#include "player-effects.h"
#include "realm-hex.h"
#include "player-damage.h"
#include "player-inventory.h"
#include "monster-status.h"
#include "files.h"
#include "object/object-kind.h"
#include "object-ego.h"
#include "targeting.h"
#include "world.h"

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
	{ TR_RES_SHARDS, GF_SHARDS, _("破片", "shard") },
	{ TR_RES_CONF, GF_CONFUSION, _("混乱", "confusion") },
	{ TR_RES_SOUND, GF_SOUND, _("轟音", "sound") },
	{ TR_RES_NEXUS, GF_NEXUS, _("因果混乱", "nexus") },
	{ TR_RES_NETHER, GF_NETHER, _("地獄", "nether") },
	{ TR_RES_CHAOS, GF_CHAOS, _("カオス", "chaos") },
	{ TR_RES_DISEN, GF_DISENCHANT, _("劣化", "disenchant") },
	{ 0, 0, NULL }
};

/*!
 * @brief アイテムの発動効果テーブル /
 * Define flags, levels, values of activations
 */
const activation_type activation_info[] =
{
	{ "SUNLIGHT", ACT_SUNLIGHT, 10, 250, {10, 0},
	  _("太陽光線", "beam of sunlight") },
	{ "BO_MISS_1", ACT_BO_MISS_1, 10, 250, {2, 0},
	  _("マジック・ミサイル(2d6)", "magic missile (2d6)") },
	{ "BA_POIS_1", ACT_BA_POIS_1, 10, 300, {4, 0},
	  _("悪臭雲(12)", "stinking cloud (12)") },
	{ "BO_ELEC_1", ACT_BO_ELEC_1, 20, 250, {5, 0},
	  _("サンダー・ボルト(4d8)", "lightning bolt (4d8)") },
	{ "BO_ACID_1", ACT_BO_ACID_1, 20, 250, {6, 0},
	  _("アシッド・ボルト(5d8)", "acid bolt (5d8)") },
	{ "BO_COLD_1", ACT_BO_COLD_1, 20, 250, {7, 0},
	  _("アイス・ボルト(6d8)", "frost bolt (6d8)") },
	{ "BO_FIRE_1", ACT_BO_FIRE_1, 20, 250, {8, 0},
	  _("ファイア・ボルト(9d8)", "fire bolt (9d8)") },
	{ "BA_COLD_1", ACT_BA_COLD_1, 30, 750, {6, 0},
	  _("アイス・ボール(48)", "ball of cold (48)") },
	{ "BA_COLD_2", ACT_BA_COLD_2, 40, 1000, {12, 0},
	  _("アイス・ボール(100)", "ball of cold (100)") },
	{ "BA_COLD_3", ACT_BA_COLD_3, 70, 2500, {50, 0},
	  _("巨大アイス・ボール(400)", "ball of cold (400)") },
	{ "BA_FIRE_1", ACT_BA_FIRE_1, 30, 1000, {9, 0},
	  _("ファイア・ボール(72)", "ball of fire (72)") },
	{ "BA_FIRE_2", ACT_BA_FIRE_2, 40, 1500, {15, 0},
	  _("巨大ファイア・ボール(120)", "large fire ball (120)") },
	{ "BA_FIRE_3", ACT_BA_FIRE_3, 60, 1750, {40, 0},
	  _("巨大ファイア・ボール(300)", "fire ball (300)") },
	{ "BA_FIRE_4", ACT_BA_FIRE_4, 40, 1000, {12, 0},
	  _("ファイア・ボール(100)", "fire ball (100)") },
	{ "BA_ELEC_2", ACT_BA_ELEC_2, 40, 1000, {12, 0},
	  _("サンダー・ボール(100)", "ball of lightning (100)") },
	{ "BA_ELEC_3", ACT_BA_ELEC_3, 70, 2500, {70, 0},
	  _("巨大サンダー・ボール(500)", "ball of lightning (500)") },
	{ "BA_ACID_1", ACT_BA_ACID_1, 30, 1000, {12, 0},
	  _("アシッド・ボール(100)", "ball of acid (100)") },
	{ "BA_NUKE_1", ACT_BA_NUKE_1, 50, 1000, {12, 0},
	  _("放射能球(100)", "ball of nuke (100)") },
	{ "HYPODYNAMIA_1", ACT_HYPODYNAMIA_1, 30, 500, {12, 0},
	  _("窒息攻撃(100)", "a strangling attack (100)") },
	{ "HYPODYNAMIA_2", ACT_HYPODYNAMIA_2, 40, 750, {15, 0},
	  _("衰弱の矢(120)", "hypodynamic bolt (120)") },
	{ "DRAIN_1", ACT_DRAIN_1, 40, 1000, {20, 0},
	  _("吸収の矢(3*50)", "drain bolt (3*50)") },
	{ "BO_MISS_2", ACT_BO_MISS_2, 40, 1000, {20, 0},
	  _("矢(150)", "arrows (150)") },
	{ "WHIRLWIND", ACT_WHIRLWIND, 50, 7500, {25, 0},
	  _("カマイタチ", "whirlwind attack") },
	{ "DRAIN_2", ACT_DRAIN_2, 50, 2500, {40, 0},
	  _("吸収の矢(3*100)", "drain bolt (3*100)") },
	{ "CALL_CHAOS", ACT_CALL_CHAOS, 70, 5000, {35, 0},
	  _("混沌召来", "call chaos") },
	{ "ROCKET", ACT_ROCKET, 70, 5000, {20, 0},
	  _("ロケット(120+レベル)", "launch rocket (120+level)") },
	{ "DISP_EVIL", ACT_DISP_EVIL, 50, 4000, {50, 0},
	  _("邪悪退散(x5)", "dispel evil (x5)") },
	{ "BA_MISS_3", ACT_BA_MISS_3, 50, 1500, {50, 0},
	  _("エレメントのブレス(300)", "elemental breath (300)") },
	{ "DISP_GOOD", ACT_DISP_GOOD, 50, 3500, {50, 0},
	  _("善良退散(x5)", "dispel good (x5)") },
	{ "BO_MANA", ACT_BO_MANA, 40, 1500, {20, 0},
	  _("魔法の矢(150)", "a magical arrow (150)") },
	{ "BA_WATER", ACT_BA_WATER, 50, 2000, {25, 0},
	  _("ウォーター・ボール(200)", "water ball (200)") },
	{ "BA_STAR", ACT_BA_STAR, 50, 2200, {25, 0},
	  _("巨大スター・ボール(200)", "large star ball (200)") },
	{ "BA_DARK", ACT_BA_DARK, 50, 2200, {30, 0},
	  _("暗黒の嵐(250)", "darkness storm (250)") },
	{ "BA_MANA", ACT_BA_MANA, 70, 2500, {30, 0},
	  _("魔力の嵐(250)", "a mana storm (250)") },
	{ "PESTICIDE", ACT_PESTICIDE, 10, 500, {10, 0},
	  _("害虫の駆除", "dispel small life") },
	{ "BLINDING_LIGHT", ACT_BLINDING_LIGHT, 30, 5000, {40, 0},
	  _("眩しい光", "blinding light") },
	{ "BIZARRE", ACT_BIZARRE, 90, 10000, {50, 0},
	  _("信じ難いこと", "bizarre things") },
	{ "CAST_BA_STAR", ACT_CAST_BA_STAR, 70, 7500, {100, 0},
	  _("スター・ボール・ダスト(150)", "cast star balls (150)") },
	{ "BLADETURNER", ACT_BLADETURNER, 80, 20000, {80, 0},
	  _("エレメントのブレス(300), 士気高揚、祝福、耐性", "breathe elements (300), hero, bless, and resistance") },
	{ "BR_FIRE", ACT_BR_FIRE, 50, 5000, {-1, 0},
	  _("火炎のブレス (200)", "fire breath (200)") },
	{ "BR_COLD", ACT_BR_COLD, 50, 5000, {-1, 0},
	  _("冷気のブレス (200)", "cold breath (200)") },
	{ "BR_DRAGON", ACT_BR_DRAGON, 70, 10000, { 30, 0 },
	  "" /* built by item_activation_dragon_breath() */ },

	{ "CONFUSE", ACT_CONFUSE, 10, 500, {10, 0},
	  _("パニック・モンスター", "confuse monster") },
	{ "SLEEP", ACT_SLEEP, 10, 750, {15, 0},
	  _("周囲のモンスターを眠らせる", "sleep nearby monsters") },
	{ "QUAKE", ACT_QUAKE, 30, 600, {20, 0},
	  _("地震", "earthquake") },
	{ "TERROR", ACT_TERROR, 20, 2500, {-1, 0},
	  _("恐慌", "terror") },
	{ "TELE_AWAY", ACT_TELE_AWAY, 20, 2000, {15, 0},
	  _("テレポート・アウェイ", "teleport away") },
	{ "BANISH_EVIL", ACT_BANISH_EVIL, 40, 2000, {250, 0},
	  _("邪悪消滅", "banish evil") },
	{ "GENOCIDE", ACT_GENOCIDE, 50, 10000, {500, 0},
	  _("抹殺", "genocide") },
	{ "MASS_GENO", ACT_MASS_GENO, 50, 10000, {1000, 0},
	  _("周辺抹殺", "mass genocide") },
	{ "SCARE_AREA", ACT_SCARE_AREA, 20, 2500, {20, 0},
	  _("モンスター恐慌", "frighten monsters") },
	{ "AGGRAVATE", ACT_AGGRAVATE, 0, 100, {0, 0},
	  _("モンスターを怒らせる", "aggravete monsters") },

	{ "CHARM_ANIMAL", ACT_CHARM_ANIMAL, 40, 7500, {200, 0},
	  _("動物魅了", "charm animal") },
	{ "CHARM_UNDEAD", ACT_CHARM_UNDEAD, 40, 10000, {333, 0},
	  _("アンデッド従属", "enslave undead") },
	{ "CHARM_OTHER", ACT_CHARM_OTHER, 40, 10000, {400, 0},
	  _("モンスター魅了", "charm monster") },
	{ "CHARM_ANIMALS", ACT_CHARM_ANIMALS, 40, 12500, {500, 0},
	  _("動物友和", "animal friendship") },
	{ "CHARM_OTHERS", ACT_CHARM_OTHERS, 40, 17500, {750, 0},
	  _("周辺魅了", "mass charm") },
	{ "SUMMON_ANIMAL", ACT_SUMMON_ANIMAL, 50, 10000, {200, 300},
	  _("動物召喚", "summon animal") },
	{ "SUMMON_PHANTOM", ACT_SUMMON_PHANTOM, 50, 12000, {200, 200},
	  _("幻霊召喚", "summon phantasmal servant") },
	{ "SUMMON_ELEMENTAL", ACT_SUMMON_ELEMENTAL, 50, 15000, {750, 0},
	  _("エレメンタル召喚", "summon elemental") },
	{ "SUMMON_DEMON", ACT_SUMMON_DEMON, 50, 20000, {666, 0},
	  _("悪魔召喚", "summon demon") },
	{ "SUMMON_UNDEAD", ACT_SUMMON_UNDEAD, 50, 20000, {666, 0},
	  _("アンデッド召喚", "summon undead") },
	{ "SUMMON_HOUND", ACT_SUMMON_HOUND, 50, 15000, {300, 0},
	  _("ハウンド召喚", "summon hound") },
	{ "SUMMON_DAWN", ACT_SUMMON_DAWN, 50, 15000, {500, 0},
	  _("暁の師団召喚", "summon the Legion of the Dawn") },
	{ "SUMMON_OCTOPUS", ACT_SUMMON_OCTOPUS, 50, 15000, {300, 0},
	  _("蛸の大群召喚", "summon octopus") },

	{ "CHOIR_SINGS", ACT_CHOIR_SINGS, 60, 20000, {300, 0},
	  _("回復(777)、癒し、士気高揚", "heal 777 hit points, curing and HEROism") },
	{ "CURE_LW", ACT_CURE_LW, 10, 500, {10, 0},
	  _("恐怖除去/体力回復(30)", "remove fear and heal 30 hp") },
	{ "CURE_MW", ACT_CURE_MW, 20, 750, {3, 3},
	  _("傷回復(4d8)", "heal 4d8 and wounds") },
	{ "CURE_POISON", ACT_CURE_POISON, 10, 1000, {5, 0},
	  _("恐怖除去/毒消し", "remove fear and cure poison") },
	{ "REST_LIFE", ACT_REST_EXP, 40, 7500, {450, 0},
	  _("経験値復活", "restore experience") },
	{ "REST_ALL", ACT_REST_ALL, 30, 15000, {750, 0},
	  _("全ステータスと経験値復活", "restore stats and experience") },
	{ "CURE_700", ACT_CURE_700, 40, 10000, {250, 0},
	  _("体力回復(700)", "heal 700 hit points") },
	{ "CURE_1000", ACT_CURE_1000, 50, 15000, {888, 0},
	  _("体力回復(1000)", "heal 1000 hit points") },
	{ "CURING", ACT_CURING, 30, 5000, {100, 0},
	  _("癒し", "curing") },
	{ "CURE_MANA_FULL", ACT_CURE_MANA_FULL, 60, 20000, {777, 0},
	  _("魔力復活", "restore mana") },

	{ "ESP", ACT_ESP, 30, 1500, {100, 0},
	  _("テレパシー(期間 25+d30)", "telepathy (dur 25+d30)") },
	{ "BERSERK", ACT_BERSERK, 10, 800, {75, 0},
	  _("狂戦士化(25+d25ターン)", "berserk (25+d25 turns)") },
	{ "PROT_EVIL", ACT_PROT_EVIL, 30, 5000, {100, 0},
	  _("対邪悪結界(期間 3*レベル+d25)", "protect evil (dur level*3 + d25)") },
	{ "RESIST_ALL", ACT_RESIST_ALL, 30, 5000, {111, 0},
	  _("全耐性(期間 20+d20)", "resist elements (dur 20+d20)") },
	{ "SPEED", ACT_SPEED, 40, 15000, {250, 0},
	  _("加速(期間 20+d20)", "speed (dur 20+d20)") },
	{ "XTRA_SPEED", ACT_XTRA_SPEED, 40, 25000, {200, 200},
	  _("加速(期間 75+d75)", "speed (dur 75+d75)") },
	{ "WRAITH", ACT_WRAITH, 90, 25000, {1000, 0},
	  _("幽体化(期間 (レベル/2)+d(レベル/2))", "wraith form (dur level/2 + d(level/2))") },
	{ "INVULN", ACT_INVULN, 90, 25000, {1000, 0},
	  _("無敵化(期間 8+d8)", "invulnerability (dur 8+d8)") },
	{ "HERO", ACT_HERO, 10, 500, {30, 30},
	  _("士気高揚", "heroism") },
	{ "HERO_SPEED", ACT_HERO_SPEED, 30, 20000, {100, 200},
	  _("士気高揚, スピード(期間 50+d50ターン)", "hero and +10 to speed (50)") },
	{ "RESIST_ACID", ACT_RESIST_ACID, 20, 2000, {40, 40},
	  _("酸への耐性(期間 20+d20)", "resist acid (dur 20+d20)") },
	{ "RESIST_FIRE", ACT_RESIST_FIRE, 20, 2000, {40, 40},
	  _("火炎への耐性(期間 20+d20)", "resist fire (dur 20+d20)") },
	{ "RESIST_COLD", ACT_RESIST_COLD, 20, 2000, {40, 40},
	  _("冷気への耐性(期間 20+d20)", "resist cold (dur 20+d20)") },
	{ "RESIST_ELEC", ACT_RESIST_ELEC, 20, 2000, {40, 40},
	  _("電撃への耐性(期間 20+d20)", "resist elec (dur 20+d20)") },
	{ "RESIST_POIS", ACT_RESIST_POIS, 20, 2000, {40, 40},
	  _("毒への耐性(期間 20+d20)", "resist poison (dur 20+d20)") },

	{ "LIGHT", ACT_LIGHT, 10, 150, {10, 10},
	  _("イルミネーション", "light area (dam 2d15)") },
	{ "MAP_LIGHT", ACT_MAP_LIGHT, 30, 500, {50, 50},
	  _("魔法の地図と光", "light (dam 2d15) & map area") },
	{ "DETECT_ALL", ACT_DETECT_ALL, 30, 1000, {55, 55},
	  _("全感知", "detection") },
	{ "DETECT_XTRA", ACT_DETECT_XTRA, 50, 12500, {100, 0},
	  _("全感知、探索、*鑑定*", "detection, probing and identify true") },
	{ "ID_FULL", ACT_ID_FULL, 50, 10000, {75, 0},
	  _("*鑑定*", "identify true") },
	{ "ID_PLAIN", ACT_ID_PLAIN, 20, 1250, {10, 0},
	  _("鑑定", "identify spell") },
	{ "RUNE_EXPLO", ACT_RUNE_EXPLO, 40, 4000, {200, 0},
	  _("爆発のルーン", "explosive rune") },
	{ "RUNE_PROT", ACT_RUNE_PROT, 60, 10000, {400, 0},
	  _("守りのルーン", "rune of protection") },
	{ "SATIATE", ACT_SATIATE, 10, 2000, {200, 0},
	  _("空腹充足", "satisfy hunger") },
	{ "DEST_DOOR", ACT_DEST_DOOR, 10, 100, {10, 0},
	  _("ドア破壊", "destroy doors") },
	{ "STONE_MUD", ACT_STONE_MUD, 20, 1000, {3, 0},
	  _("岩石溶解", "stone to mud") },
	{ "RECHARGE", ACT_RECHARGE, 30, 1000, {70, 0},
	  _("魔力充填", "recharging") },
	{ "ALCHEMY", ACT_ALCHEMY, 50, 10000, {500, 0},
	  _("錬金術", "alchemy") },
	{ "DIM_DOOR", ACT_DIM_DOOR, 50, 10000, {100, 0},
	  _("次元の扉", "dimension door") },
	{ "TELEPORT", ACT_TELEPORT, 10, 2000, {25, 0},
	  _("テレポート", "teleport") },
	{ "RECALL", ACT_RECALL, 30, 7500, {200, 0},
	  _("帰還の詔", "word of recall") },
	{ "JUDGE", ACT_JUDGE, 90, 50000, {20, 20},
	  _("体力と引き替えに千里眼と帰還", "a telekinesis (500 lb)") },
	{ "TELEKINESIS", ACT_TELEKINESIS, 20, 5500, {25, 25},
	  _("物体を引き寄せる(重量25kgまで)", "clairvoyance and recall, draining you") },
	{ "DETECT_UNIQUE", ACT_DETECT_UNIQUE, 40, 10000, {200, 0},
	  _("この階にいるユニークモンスターを表示", "list of the uniques on the level") },
	{ "ESCAPE", ACT_ESCAPE, 10, 3000, {35, 0},
	  _("逃走", "a getaway") },
	{ "DISP_CURSE_XTRA", ACT_DISP_CURSE_XTRA, 40, 30000, {0, 0},
	  _("*解呪*と調査", "dispel curse and probing") },
	{ "BRAND_FIRE_BOLTS", ACT_BRAND_FIRE_BOLTS, 40, 20000, {999, 0},
	  _("刃先のファイア・ボルト", "fire branding of bolts") },
	{ "RECHARGE_XTRA", ACT_RECHARGE_XTRA, 70, 30000, {200, 0},
	  _("魔力充填", "recharge item") },
	{ "LORE", ACT_LORE, 10, 30000, {0, 0},
	  _("危険を伴う鑑定", "perilous identify") },
	{ "SHIKOFUMI", ACT_SHIKOFUMI, 10, 10000, {100, 100},
	  _("四股踏み", "shiko") },
	{ "PHASE_DOOR", ACT_PHASE_DOOR, 10, 1500, {10, 0},
	  _("ショート・テレポート", "blink") },
	{ "DETECT_ALL_MONS", ACT_DETECT_ALL_MONS, 30, 3000, {150, 0},
	  _("全モンスター感知", "detect all monsters") },
	{ "ULTIMATE_RESIST", ACT_ULTIMATE_RESIST, 90, 20000, {777, 0},
	  _("士気高揚、祝福、究極の耐性", "hero, bless, and ultimate resistance") },

	{ "CAST_OFF", ACT_CAST_OFF, 30, 15000, {100, 0},
	  _("脱衣と小宇宙燃焼", "cast it off and cosmic heroism") },
	{ "FISHING", ACT_FISHING, 0, 100, {0, 0},
	  _("釣りをする", "fishing") },
	{ "INROU", ACT_INROU, 40, 15000, {150, 150},
	  _("例のアレ", "reveal your identity") },
	{ "MURAMASA", ACT_MURAMASA, 0, 0, {-1, 0},
	  _("腕力の上昇", "increase STR") },
	{ "BLOODY_MOON", ACT_BLOODY_MOON, 0, 0, {3333, 0},
	  _("属性変更", "change zokusei") },
	{ "CRIMSON", ACT_CRIMSON, 0, 50000, {15, 0},
	  _("ファイア！", "fire!") },

	{ "STRAIN_HASTE", ACT_STRAIN_HASTE, 10, 1000, {120, 100},
	  _("体力と引き換えに加速", "haste with strain") },
	{ "GRAND_CROSS", ACT_GRAND_CROSS, 30, 15000, {250, 200},
	  _("グランド・クロス", "grand cross") },
	{ "TELEPORT_LEVEL", ACT_TELEPORT_LEVEL, 10, 1500, {100, 200},
	  _("テレポート・レベル", "teleort level") },
	{ "ARTS_FALLING_STAR", ACT_FALLING_STAR, 20, 5500, {30, 50},
	  _("魔剣・流れ星", "blade arts 'falling star'") },
	{ NULL, 0, 0, 0, {0, 0},
	  "" }
};

/*!
 * @brief 装備を発動するコマンドのサブルーチン /
 * Activate a wielded object.  Wielded objects never stack.
 * And even if they did, activatable objects never stack.
 * @param item 発動するオブジェクトの所持品ID
 * @return なし
 * @details
 * <pre>
 * Currently, only (some) artifacts, and Dragon Scale Mail, can be activated.
 * But one could, for example, easily make an activatable "Ring of Plasma".
 * Note that it always takes a turn to activate an artifact, even if
 * the user hits "escape" at the "direction" prompt.
 * </pre>
 */
void exe_activate(player_type *user_ptr, INVENTORY_IDX item)
{
	DIRECTION dir;
	DEPTH lev;
	int chance, fail;
	object_type *o_ptr;
	bool success;

	o_ptr = REF_ITEM(user_ptr, user_ptr->current_floor_ptr, item);
	take_turn(user_ptr, 100);
	lev = k_info[o_ptr->k_idx].level;

	/* Hack -- use artifact level instead */
	if (object_is_fixed_artifact(o_ptr)) lev = a_info[o_ptr->name1].level;
	else if (object_is_random_artifact(o_ptr))
	{
		const activation_type* const act_ptr = find_activation_info(o_ptr);
		if (act_ptr) {
			lev = act_ptr->level;
		}
	}
	else if (((o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET)) && o_ptr->name2) lev = e_info[o_ptr->name2].level;

	/* Base chance of success */
	chance = user_ptr->skill_dev;

	/* Confusion hurts skill */
	if (user_ptr->confused) chance = chance / 2;

	fail = lev + 5;
	if (chance > fail) fail -= (chance - fail) * 2;
	else chance -= (fail - chance) * 2;
	if (fail < USE_DEVICE) fail = USE_DEVICE;
	if (chance < USE_DEVICE) chance = USE_DEVICE;

	if (cmd_limit_time_walk(user_ptr)) return;

	if (user_ptr->pclass == CLASS_BERSERKER) success = FALSE;
	else if (chance > fail)
	{
		if (randint0(chance * 2) < fail) success = FALSE;
		else success = TRUE;
	}
	else
	{
		if (randint0(fail * 2) < chance) success = TRUE;
		else success = FALSE;
	}

	/* Roll for usage */
	if (!success)
	{
		if (flush_failure) flush();
		msg_print(_("うまく始動させることができなかった。", "You failed to activate it properly."));
		sound(SOUND_FAIL);
		return;
	}

	/* Check the recharge */
	if (o_ptr->timeout)
	{
		msg_print(_("それは微かに音を立て、輝き、消えた...", "It whines, glows and fades..."));
		return;
	}

	/* Some lights need enough fuel for activation */
	if (!o_ptr->xtra4 && (o_ptr->tval == TV_FLASK) &&
		((o_ptr->sval == SV_LITE_TORCH) || (o_ptr->sval == SV_LITE_LANTERN)))
	{
		msg_print(_("燃料がない。", "It has no fuel."));
		free_turn(user_ptr);
		return;
	}

	/* Activate the artifact */
	msg_print(_("始動させた...", "You activate it..."));

	sound(SOUND_ZAP);

	/* Activate object */
	if (activation_index(o_ptr))
	{
		(void)activate_artifact(user_ptr, o_ptr);

		user_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Success */
		return;
	}

	/* Special items */
	else if (o_ptr->tval == TV_WHISTLE)
	{
		if (music_singing_any(user_ptr)) stop_singing(user_ptr);
		if (hex_spelling_any(user_ptr)) stop_hex_spell_all(user_ptr);

		{
			MONSTER_IDX pet_ctr, i;
			MONSTER_IDX *who;
			int max_pet = 0;
			u16b dummy_why;

			/* Allocate the "who" array */
			C_MAKE(who, current_world_ptr->max_m_idx, MONSTER_IDX);

			/* Process the monsters (backwards) */
			for (pet_ctr = user_ptr->current_floor_ptr->m_max - 1; pet_ctr >= 1; pet_ctr--)
			{
				if (is_pet(&user_ptr->current_floor_ptr->m_list[pet_ctr]) && (user_ptr->riding != pet_ctr))
					who[max_pet++] = pet_ctr;
			}

			ang_sort(who, &dummy_why, max_pet, ang_sort_comp_pet, ang_sort_swap_hook);

			/* Process the monsters (backwards) */
			for (i = 0; i < max_pet; i++)
			{
				pet_ctr = who[i];
				teleport_monster_to(user_ptr, pet_ctr, user_ptr->y, user_ptr->x, 100, TELEPORT_PASSIVE);
			}

			/* Free the "who" array */
			C_KILL(who, current_world_ptr->max_m_idx, MONSTER_IDX);
		}
		o_ptr->timeout = 100 + randint1(100);
		return;
	}
	else if (o_ptr->tval == TV_CAPTURE)
	{
		if (!o_ptr->pval)
		{
			bool old_target_pet = target_pet;
			target_pet = TRUE;
			if (!get_aim_dir(user_ptr, &dir))
			{
				target_pet = old_target_pet;
				return;
			}
			target_pet = old_target_pet;

			if (fire_ball(user_ptr, GF_CAPTURE, dir, 0, 0))
			{
				o_ptr->pval = (PARAMETER_VALUE)cap_mon;
				o_ptr->xtra3 = (XTRA8)cap_mspeed;
				o_ptr->xtra4 = (XTRA16)cap_hp;
				o_ptr->xtra5 = (XTRA16)cap_maxhp;
				if (cap_nickname)
				{
					concptr t;
					char *s;
					char buf[80] = "";

					if (o_ptr->inscription)
						strcpy(buf, quark_str(o_ptr->inscription));
					s = buf;
					for (s = buf; *s && (*s != '#'); s++)
					{
#ifdef JP
						if (iskanji(*s)) s++;
#endif
					}
					*s = '#';
					s++;
#ifdef JP
					/*nothing*/
#else
					*s++ = '\'';
#endif
					t = quark_str(cap_nickname);
					while (*t)
					{
						*s = *t;
						s++;
						t++;
					}
#ifdef JP
					/*nothing*/
#else
					*s++ = '\'';
#endif
					*s = '\0';
					o_ptr->inscription = quark_add(buf);
				}
			}
		}
		else
		{
			success = FALSE;
			if (!get_direction(user_ptr, &dir, FALSE, FALSE)) return;
			if (monster_can_enter(user_ptr, user_ptr->y + ddy[dir], user_ptr->x + ddx[dir], &r_info[o_ptr->pval], 0))
			{
				if (place_monster_aux(user_ptr, 0, user_ptr->y + ddy[dir], user_ptr->x + ddx[dir], o_ptr->pval, (PM_FORCE_PET | PM_NO_KAGE)))
				{
					if (o_ptr->xtra3) user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].mspeed = o_ptr->xtra3;
					if (o_ptr->xtra5) user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].max_maxhp = o_ptr->xtra5;
					if (o_ptr->xtra4) user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].hp = o_ptr->xtra4;
					user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].maxhp = user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].max_maxhp;
					if (o_ptr->inscription)
					{
						char buf[80];
						concptr t;
#ifdef JP
#else
						bool quote = FALSE;
#endif

						t = quark_str(o_ptr->inscription);
						for (t = quark_str(o_ptr->inscription); *t && (*t != '#'); t++)
						{
#ifdef JP
							if (iskanji(*t)) t++;
#endif
						}
						if (*t)
						{
							char *s = buf;
							t++;
#ifdef JP
							/* nothing */
#else
							if (*t == '\'')
							{
								t++;
								quote = TRUE;
							}
#endif
							while (*t)
							{
								*s = *t;
								t++;
								s++;
							}
#ifdef JP
							/* nothing */
#else
							if (quote && *(s - 1) == '\'')
								s--;
#endif
							*s = '\0';
							user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].nickname = quark_add(buf);
							t = quark_str(o_ptr->inscription);
							s = buf;
							while (*t && (*t != '#'))
							{
								*s = *t;
								t++;
								s++;
							}
							*s = '\0';
							o_ptr->inscription = quark_add(buf);
						}
					}
					o_ptr->pval = 0;
					o_ptr->xtra3 = 0;
					o_ptr->xtra4 = 0;
					o_ptr->xtra5 = 0;
					success = TRUE;
				}
			}
			if (!success)
				msg_print(_("おっと、解放に失敗した。", "Oops.  You failed to release your pet."));
		}
		calc_android_exp(user_ptr);
		return;
	}

	/* Mistake */
	msg_print(_("おっと、このアイテムは始動できない。", "Oops.  That object cannot be activated."));
}

/*!
 * @brief 装備を発動するコマンドのメインルーチン /
 * @param user_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_activate(player_type *user_ptr)
{
	OBJECT_IDX item;
	concptr q, s;

	if (user_ptr->wild_mode) return;
	if (cmd_limit_arena(user_ptr)) return;

	if (user_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(user_ptr, ACTION_NONE);
	}

	item_tester_hook = item_tester_hook_activate;

	q = _("どのアイテムを始動させますか? ", "Activate which item? ");
	s = _("始動できるアイテムを装備していない。", "You have nothing to activate.");

	if (!choose_object(user_ptr, &item, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), 0)) return;

	/* Activate the item */
	exe_activate(user_ptr, item);
}

/*!
* @brief 発動によるブレスの属性をアイテムの耐性から選択し、実行を処理する。/ Dragon breath activation
* @details 対象となる耐性は dragonbreath_info テーブルを参照のこと。
* @param user_ptr プレーヤーへの参照ポインタ
* @param o_ptr 対象のオブジェクト構造体ポインタ
* @return 発動実行の是非を返す。
*/
static bool activate_dragon_breath(player_type *user_ptr, object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE]; /* for resistance flags */
	int type[20];
	concptr name[20];
	int i, t, n = 0;
	DIRECTION dir;

	if (!get_aim_dir(user_ptr, &dir)) return FALSE;

	object_flags(o_ptr, flgs);

	for (i = 0; dragonbreath_info[i].flag != 0; i++)
	{
		if (have_flag(flgs, dragonbreath_info[i].flag))
		{
			type[n] = dragonbreath_info[i].type;
			name[n] = dragonbreath_info[i].name;
			n++;
		}
	}
	if (n == 0) return FALSE;

	/* Stop speaking */
	if (music_singing_any(user_ptr)) stop_singing(user_ptr);
	if (hex_spelling_any(user_ptr)) stop_hex_spell_all(user_ptr);

	t = randint0(n);
	msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), name[t]);
	fire_breath(user_ptr, type[t], dir, 250, 4);

	return TRUE;
}


/*!
 * @brief アイテムの発動効果を処理する。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動実行の是非を返す。
 */
bool activate_artifact(player_type *user_ptr, object_type *o_ptr)
{
	PLAYER_LEVEL plev = user_ptr->lev;
	int k, dummy = 0;
	DIRECTION dir;
	concptr name = k_name + k_info[o_ptr->k_idx].name;
	const activation_type* const act_ptr = find_activation_info(o_ptr);
	if (!act_ptr) {
		/* Maybe forgot adding information to activation_info table ? */
		msg_print("Activation information is not found.");
		return FALSE;
	}

	/* Activate for attack */
	switch (act_ptr->index)
	{
	case ACT_SUNLIGHT:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		msg_print(_("太陽光線が放たれた。", "A line of sunlight appears."));
		(void)lite_line(user_ptr, dir, damroll(6, 8));
		break;
	}

	case ACT_BO_MISS_1:
	{
		msg_print(_("それは眩しいくらいに明るく輝いている...", "It glows extremely brightly..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_bolt(user_ptr, GF_MISSILE, dir, damroll(2, 6));
		break;
	}

	case ACT_BA_POIS_1:
	{
		msg_print(_("それは濃緑色に脈動している...", "It throbs deep green..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_POIS, dir, 12, 3);
		break;
	}

	case ACT_BO_ELEC_1:
	{
		msg_print(_("それは火花に覆われた...", "It is covered in sparks..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_bolt(user_ptr, GF_ELEC, dir, damroll(4, 8));
		break;
	}

	case ACT_BO_ACID_1:
	{
		msg_print(_("それは酸に覆われた...", "It is covered in acid..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_bolt(user_ptr, GF_ACID, dir, damroll(5, 8));
		break;
	}

	case ACT_BO_COLD_1:
	{
		msg_print(_("それは霜に覆われた...", "It is covered in frost..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_bolt(user_ptr, GF_COLD, dir, damroll(6, 8));
		break;
	}

	case ACT_BO_FIRE_1:
	{
		msg_print(_("それは炎に覆われた...", "It is covered in fire..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_bolt(user_ptr, GF_FIRE, dir, damroll(9, 8));
		break;
	}

	case ACT_BA_COLD_1:
	{
		msg_print(_("それは霜に覆われた...", "It is covered in frost..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_COLD, dir, 48, 2);
		break;
	}

	case ACT_BA_COLD_2:
	{
		msg_print(_("それは青く激しく輝いた...", "It glows an intense blue..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_COLD, dir, 100, 2);
		break;
	}

	case ACT_BA_COLD_3:
	{
		msg_print(_("明るく白色に輝いている...", "It glows bright white..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_COLD, dir, 400, 3);
		break;
	}

	case ACT_BA_FIRE_1:
	{
		msg_print(_("それは赤く激しく輝いた...", "It glows an intense red..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_FIRE, dir, 72, 2);
		break;
	}

	case ACT_BA_FIRE_2:
	{
		msg_format(_("%sから炎が吹き出した...", "The %s rages in fire..."), name);
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_FIRE, dir, 120, 3);
		break;
	}

	case ACT_BA_FIRE_3:
	{
		msg_print(_("深赤色に輝いている...", "It glows deep red..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_FIRE, dir, 300, 3);
		break;
	}

	case ACT_BA_FIRE_4:
	{
		msg_print(_("それは赤く激しく輝いた...", "It glows an intense red..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_FIRE, dir, 100, 2);
		break;
	}

	case ACT_BA_ELEC_2:
	{
		msg_print(_("電気がパチパチ音を立てた...", "It crackles with electricity..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_ELEC, dir, 100, 3);
		break;
	}

	case ACT_BA_ELEC_3:
	{
		msg_print(_("深青色に輝いている...", "It glows deep blue..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_ELEC, dir, 500, 3);
		break;
	}

	case ACT_BA_ACID_1:
	{
		msg_print(_("それは黒く激しく輝いた...", "It glows an intense black..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_ACID, dir, 100, 2);
		break;
	}

	case ACT_BA_NUKE_1:
	{
		msg_print(_("それは緑に激しく輝いた...", "It glows an intense green..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_NUKE, dir, 100, 2);
		break;
	}

	case ACT_HYPODYNAMIA_1:
	{
		msg_format(_("あなたは%sに敵を締め殺すよう命じた。", "You order the %s to strangle your opponent."), name);
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		hypodynamic_bolt(user_ptr, dir, 100);
		break;
	}

	case ACT_HYPODYNAMIA_2:
	{
		msg_print(_("黒く輝いている...", "It glows black..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		hypodynamic_bolt(user_ptr, dir, 120);
		break;
	}

	case ACT_DRAIN_1:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		for (dummy = 0; dummy < 3; dummy++)
		{
			if (hypodynamic_bolt(user_ptr, dir, 50))
				hp_player(user_ptr, 50);
		}
		break;
	}

	case ACT_BO_MISS_2:
	{
		msg_print(_("魔法のトゲが現れた...", "It grows magical spikes..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_bolt(user_ptr, GF_ARROW, dir, 150);
		break;
	}

	case ACT_WHIRLWIND:
	{
		massacre(user_ptr);
		break;
	}

	case ACT_DRAIN_2:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		for (dummy = 0; dummy < 3; dummy++)
		{
			if (hypodynamic_bolt(user_ptr, dir, 100))
				hp_player(user_ptr, 100);
		}
		break;
	}


	case ACT_CALL_CHAOS:
	{
		msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
		call_chaos(user_ptr);
		break;
	}

	case ACT_ROCKET:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		msg_print(_("ロケットを発射した！", "You launch a rocket!"));
		fire_ball(user_ptr, GF_ROCKET, dir, 250 + plev * 3, 2);
		break;
	}

	case ACT_DISP_EVIL:
	{
		msg_print(_("神聖な雰囲気が充満した...", "It floods the area with goodness..."));
		dispel_evil(user_ptr, user_ptr->lev * 5);
		break;
	}

	case ACT_BA_MISS_3:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
		fire_breath(user_ptr, GF_MISSILE, dir, 300, 4);
		break;
	}

	case ACT_DISP_GOOD:
	{
		msg_print(_("邪悪な雰囲気が充満した...", "It floods the area with evil..."));
		dispel_good(user_ptr, user_ptr->lev * 5);
		break;
	}

	case ACT_BO_MANA:
	{
		msg_format(_("%sに魔法のトゲが現れた...", "The %s grows magical spikes..."), name);
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_bolt(user_ptr, GF_ARROW, dir, 150);
		break;
	}

	case ACT_BA_WATER:
	{
		msg_format(_("%sが深い青色に鼓動している...", "The %s throbs deep blue..."), name);
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_WATER, dir, 200, 3);
		break;
	}

	case ACT_BA_DARK:
	{
		msg_format(_("%sが深い闇に覆われた...", "The %s is coverd in pitch-darkness..."), name);
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_DARK, dir, 250, 4);
		break;
	}

	case ACT_BA_MANA:
	{
		msg_format(_("%sが青白く光った．．．", "The %s glows pale..."), name);
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_ball(user_ptr, GF_MANA, dir, 250, 4);
		break;
	}

	case ACT_PESTICIDE:
	{
		msg_print(_("あなたは害虫を一掃した。", "You exterminate small life."));
		(void)dispel_monsters(user_ptr, 4);
		break;
	}

	case ACT_BLINDING_LIGHT:
	{
		msg_format(_("%sが眩しい光で輝いた...", "The %s gleams with blinding light..."), name);
		fire_ball(user_ptr, GF_LITE, 0, 300, 6);
		confuse_monsters(user_ptr, 3 * user_ptr->lev / 2);
		break;
	}

	case ACT_BIZARRE:
	{
		msg_format(_("%sは漆黒に輝いた...", "The %s glows intensely black..."), name);
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		ring_of_power(user_ptr, dir);
		break;
	}

	case ACT_CAST_BA_STAR:
	{
		HIT_POINT num = damroll(5, 3);
		POSITION y = 0, x = 0;
		int attempts;
		msg_format(_("%sが稲妻で覆われた...", "The %s is surrounded by lightning..."), name);
		for (k = 0; k < num; k++)
		{
			attempts = 1000;

			while (attempts--)
			{
				scatter(user_ptr, &y, &x, user_ptr->y, user_ptr->x, 4, 0);
				if (!cave_have_flag_bold(user_ptr->current_floor_ptr, y, x, FF_PROJECT)) continue;
				if (!player_bold(user_ptr, y, x)) break;
			}

			project(user_ptr, 0, 3, y, x, 150, GF_ELEC,
				(PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
		}

		break;
	}

	case ACT_BLADETURNER:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
		fire_breath(user_ptr, GF_MISSILE, dir, 300, 4);
		msg_print(_("鎧が様々な色に輝いた...", "Your armor glows many colours..."));
		(void)set_afraid(user_ptr, 0);
		(void)set_hero(user_ptr, randint1(50) + 50, FALSE);
		(void)hp_player(user_ptr, 10);
		(void)set_blessed(user_ptr, randint1(50) + 50, FALSE);
		(void)set_oppose_acid(user_ptr, randint1(50) + 50, FALSE);
		(void)set_oppose_elec(user_ptr, randint1(50) + 50, FALSE);
		(void)set_oppose_fire(user_ptr, randint1(50) + 50, FALSE);
		(void)set_oppose_cold(user_ptr, randint1(50) + 50, FALSE);
		(void)set_oppose_pois(user_ptr, randint1(50) + 50, FALSE);
		break;
	}

	case ACT_BR_FIRE:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_breath(user_ptr, GF_FIRE, dir, 200, 2);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES))
		{
			(void)set_oppose_fire(user_ptr, randint1(20) + 20, FALSE);
		}
		break;
	}

	case ACT_BR_COLD:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		fire_breath(user_ptr, GF_COLD, dir, 200, 2);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE))
		{
			(void)set_oppose_cold(user_ptr, randint1(20) + 20, FALSE);
		}
		break;
	}

	case ACT_BR_DRAGON:
	{
		if (!activate_dragon_breath(user_ptr, o_ptr)) return FALSE;
		break;
	}

	/* Activate for other offensive action */
	case ACT_CONFUSE:
	{
		msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		confuse_monster(user_ptr, dir, 20);
		break;
	}

	case ACT_SLEEP:
	{
		msg_print(_("深青色に輝いている...", "It glows deep blue..."));
		sleep_monsters_touch(user_ptr);
		break;
	}

	case ACT_QUAKE:
	{
		earthquake(user_ptr, user_ptr->y, user_ptr->x, 5, 0);
		break;
	}

	case ACT_TERROR:
	{
		turn_monsters(user_ptr, 40 + user_ptr->lev);
		break;
	}

	case ACT_TELE_AWAY:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		(void)fire_beam(user_ptr, GF_AWAY_ALL, dir, plev);
		break;
	}

	case ACT_BANISH_EVIL:
	{
		if (banish_evil(user_ptr, 100))
		{
			msg_print(_("アーティファクトの力が邪悪を打ち払った！", "The power of the artifact banishes evil!"));
		}
		break;
	}

	case ACT_GENOCIDE:
	{
		msg_print(_("深青色に輝いている...", "It glows deep blue..."));
		(void)symbol_genocide(user_ptr, 200, TRUE);
		break;
	}

	case ACT_MASS_GENO:
	{
		msg_print(_("ひどく鋭い音が流れ出た...", "It lets out a long, shrill note..."));
		(void)mass_genocide(user_ptr, 200, TRUE);
		break;
	}

	case ACT_SCARE_AREA:
	{
		if (music_singing_any(user_ptr)) stop_singing(user_ptr);
		if (hex_spelling_any(user_ptr)) stop_hex_spell_all(user_ptr);
		msg_print(_("あなたは力強い突風を吹き鳴らした。周囲の敵が震え上っている!",
			"You wind a mighty blast; your enemies tremble!"));
		(void)turn_monsters(user_ptr, (3 * user_ptr->lev / 2) + 10);
		break;
	}

	case ACT_AGGRAVATE:
	{
		if (o_ptr->name1 == ART_HYOUSIGI)
		{
			msg_print(_("拍子木を打った。", "You beat your wooden clappers."));
		}
		else
		{
			msg_format(_("%sは不快な物音を立てた。", "The %s sounds an unpleasant noise."), name);
		}
		aggravate_monsters(user_ptr, 0);
		break;
	}

	/* Activate for summoning / charming */

	case ACT_CHARM_ANIMAL:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		(void)charm_animal(user_ptr, dir, plev);
		break;
	}

	case ACT_CHARM_UNDEAD:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		(void)control_one_undead(user_ptr, dir, plev);
		break;
	}

	case ACT_CHARM_OTHER:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		(void)charm_monster(user_ptr, dir, plev * 2);
		break;
	}

	case ACT_CHARM_ANIMALS:
	{
		(void)charm_animals(user_ptr, plev * 2);
		break;
	}

	case ACT_CHARM_OTHERS:
	{
		charm_monsters(user_ptr, plev * 2);
		break;
	}

	case ACT_SUMMON_ANIMAL:
	{
		(void)summon_specific(user_ptr, -1, user_ptr->y, user_ptr->x, plev, SUMMON_ANIMAL_RANGER, (PM_ALLOW_GROUP | PM_FORCE_PET));
		break;
	}

	case ACT_SUMMON_PHANTOM:
	{
		msg_print(_("幻霊を召喚した。", "You summon a phantasmal servant."));
		(void)summon_specific(user_ptr, -1, user_ptr->y, user_ptr->x, user_ptr->current_floor_ptr->dun_level, SUMMON_PHANTOM, (PM_ALLOW_GROUP | PM_FORCE_PET));
		break;
	}

	case ACT_SUMMON_ELEMENTAL:
		if (!cast_summon_elemental(user_ptr, (plev * 3) / 2)) return FALSE;
		break;

	case ACT_SUMMON_DEMON:
	{
		cast_summon_demon(user_ptr, (plev * 3) / 2);
		break;
	}

	case ACT_SUMMON_UNDEAD:
		if (!cast_summon_undead(user_ptr, (plev * 3) / 2)) return FALSE;
		break;

	case ACT_SUMMON_HOUND:
		if (!cast_summon_hound(user_ptr, (plev * 3) / 2)) return FALSE;
		break;

	case ACT_SUMMON_DAWN:
	{
		msg_print(_("暁の師団を召喚した。", "You summon the Legion of the Dawn."));
		(void)summon_specific(user_ptr, -1, user_ptr->y, user_ptr->x, user_ptr->current_floor_ptr->dun_level, SUMMON_DAWN, (PM_ALLOW_GROUP | PM_FORCE_PET));
		break;
	}

	case ACT_SUMMON_OCTOPUS:
		if (!cast_summon_octopus(user_ptr)) return FALSE;
		break;

		/* Activate for healing */

	case ACT_CHOIR_SINGS:
	{
		msg_print(_("天国の歌が聞こえる...", "A heavenly choir sings..."));
		(void)cure_critical_wounds(user_ptr, 777);
		(void)set_hero(user_ptr, randint1(25) + 25, FALSE);
		break;
	}

	case ACT_CURE_LW:
	{
		(void)set_afraid(user_ptr, 0);
		(void)hp_player(user_ptr, 30);
		break;
	}

	case ACT_CURE_MW:
	{
		msg_print(_("深紫色の光を発している...", "It radiates deep purple..."));
		(void)cure_serious_wounds(user_ptr, 4, 8);
		break;
	}

	case ACT_CURE_POISON:
	{
		msg_print(_("深青色に輝いている...", "It glows deep blue..."));
		(void)set_afraid(user_ptr, 0);
		(void)set_poisoned(user_ptr, 0);
		break;
	}

	case ACT_REST_EXP:
	{
		msg_print(_("深紅に輝いている...", "It glows a deep red..."));
		restore_level(user_ptr);
		break;
	}

	case ACT_REST_ALL:
	{
		msg_print(_("濃緑色に輝いている...", "It glows a deep green..."));
		(void)restore_all_status(user_ptr);
		(void)restore_level(user_ptr);
		break;
	}

	case ACT_CURE_700:
	{
		msg_print(_("深青色に輝いている...", "It glows deep blue..."));
		msg_print(_("体内に暖かい鼓動が感じられる...", "You feel a warm tingling inside..."));
		(void)cure_critical_wounds(user_ptr, 700);
		break;
	}

	case ACT_CURE_1000:
	{
		msg_print(_("白く明るく輝いている...", "It glows a bright white..."));
		msg_print(_("ひじょうに気分がよい...", "You feel much better..."));
		(void)cure_critical_wounds(user_ptr, 1000);
		break;
	}

	case ACT_CURING:
	{
		msg_format(_("%sの優しさに癒される...", "the %s cures you affectionately ..."), name);
		true_healing(user_ptr, 0);
		break;
	}

	case ACT_CURE_MANA_FULL:
	{
		msg_format(_("%sが青白く光った．．．", "The %s glows pale..."), name);
		restore_mana(user_ptr, TRUE);
		break;
	}

	/* Activate for timed effect */

	case ACT_ESP:
	{
		(void)set_tim_esp(user_ptr, randint1(30) + 25, FALSE);
		break;
	}

	case ACT_BERSERK:
	{
		(void)berserk(user_ptr, randint1(25) + 25);
		break;
	}

	case ACT_PROT_EVIL:
	{
		msg_format(_("%sから鋭い音が流れ出た...", "The %s lets out a shrill wail..."), name);
		k = 3 * user_ptr->lev;
		(void)set_protevil(user_ptr, randint1(25) + k, FALSE);
		break;
	}

	case ACT_RESIST_ALL:
	{
		msg_print(_("様々な色に輝いている...", "It glows many colours..."));
		(void)set_oppose_acid(user_ptr, randint1(40) + 40, FALSE);
		(void)set_oppose_elec(user_ptr, randint1(40) + 40, FALSE);
		(void)set_oppose_fire(user_ptr, randint1(40) + 40, FALSE);
		(void)set_oppose_cold(user_ptr, randint1(40) + 40, FALSE);
		(void)set_oppose_pois(user_ptr, randint1(40) + 40, FALSE);
		break;
	}

	case ACT_SPEED:
	{
		msg_print(_("明るく緑色に輝いている...", "It glows bright green..."));
		(void)set_fast(user_ptr, randint1(20) + 20, FALSE);
		break;
	}

	case ACT_XTRA_SPEED:
	{
		msg_print(_("明るく輝いている...", "It glows brightly..."));
		(void)set_fast(user_ptr, randint1(75) + 75, FALSE);
		break;
	}

	case ACT_WRAITH:
	{
		set_wraith_form(user_ptr, randint1(plev / 2) + (plev / 2), FALSE);
		break;
	}

	case ACT_INVULN:
	{
		(void)set_invuln(user_ptr, randint1(8) + 8, FALSE);
		break;
	}

	case ACT_HERO:
	{
		(void)heroism(user_ptr, 25);
		break;
	}

	case ACT_HERO_SPEED:
	{
		(void)set_fast(user_ptr, randint1(50) + 50, FALSE);
		(void)heroism(user_ptr, 50);
		break;
	}

	case ACT_RESIST_ACID:
	{
		msg_format(_("%sが黒く輝いた...", "The %s grows black."), name);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ACID))
		{
			if (!get_aim_dir(user_ptr, &dir)) return FALSE;
			fire_ball(user_ptr, GF_ACID, dir, 100, 2);
		}
		(void)set_oppose_acid(user_ptr, randint1(20) + 20, FALSE);
		break;
	}

	case ACT_RESIST_FIRE:
	{
		msg_format(_("%sが赤く輝いた...", "The %s grows red."), name);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES))
		{
			if (!get_aim_dir(user_ptr, &dir)) return FALSE;
			fire_ball(user_ptr, GF_FIRE, dir, 100, 2);
		}
		(void)set_oppose_fire(user_ptr, randint1(20) + 20, FALSE);
		break;
	}

	case ACT_RESIST_COLD:
	{
		msg_format(_("%sが白く輝いた...", "The %s grows white."), name);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE))
		{
			if (!get_aim_dir(user_ptr, &dir)) return FALSE;
			fire_ball(user_ptr, GF_COLD, dir, 100, 2);
		}
		(void)set_oppose_cold(user_ptr, randint1(20) + 20, FALSE);
		break;
	}

	case ACT_RESIST_ELEC:
	{
		msg_format(_("%sが青く輝いた...", "The %s grows blue."), name);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ELEC))
		{
			if (!get_aim_dir(user_ptr, &dir)) return FALSE;
			fire_ball(user_ptr, GF_ELEC, dir, 100, 2);
		}
		(void)set_oppose_elec(user_ptr, randint1(20) + 20, FALSE);
		break;
	}

	case ACT_RESIST_POIS:
	{
		msg_format(_("%sが緑に輝いた...", "The %s grows green."), name);
		(void)set_oppose_pois(user_ptr, randint1(20) + 20, FALSE);
		break;
	}

	/* Activate for general purpose effect (detection etc.) */

	case ACT_LIGHT:
	{
		msg_format(_("%sから澄んだ光があふれ出た...", "The %s wells with clear light..."), name);
		lite_area(user_ptr, damroll(2, 15), 3);
		break;
	}

	case ACT_MAP_LIGHT:
	{
		msg_print(_("眩しく輝いた...", "It shines brightly..."));
		map_area(user_ptr, DETECT_RAD_MAP);
		lite_area(user_ptr, damroll(2, 15), 3);
		break;
	}

	case ACT_DETECT_ALL:
	{
		msg_print(_("白く明るく輝いている...", "It glows bright white..."));
		msg_print(_("心にイメージが浮かんできた...", "An image forms in your mind..."));
		detect_all(user_ptr, DETECT_RAD_DEFAULT);
		break;
	}

	case ACT_DETECT_XTRA:
	{
		msg_print(_("明るく輝いている...", "It glows brightly..."));
		detect_all(user_ptr, DETECT_RAD_DEFAULT);
		probing(user_ptr);
		identify_fully(user_ptr, FALSE);
		break;
	}

	case ACT_ID_FULL:
	{
		msg_print(_("黄色く輝いている...", "It glows yellow..."));
		identify_fully(user_ptr, FALSE);
		break;
	}

	case ACT_ID_PLAIN:
	{
		if (!ident_spell(user_ptr, FALSE)) return FALSE;
		break;
	}

	case ACT_RUNE_EXPLO:
	{
		msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
		explosive_rune(user_ptr, user_ptr->y, user_ptr->x);
		break;
	}

	case ACT_RUNE_PROT:
	{
		msg_print(_("ブルーに明るく輝いている...", "It glows light blue..."));
		warding_glyph(user_ptr);
		break;
	}

	case ACT_SATIATE:
	{
		(void)set_food(user_ptr, PY_FOOD_MAX - 1);
		break;
	}

	case ACT_DEST_DOOR:
	{
		msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
		destroy_doors_touch(user_ptr);
		break;
	}

	case ACT_STONE_MUD:
	{
		msg_print(_("鼓動している...", "It pulsates..."));
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		wall_to_mud(user_ptr, dir, 20 + randint1(30));
		break;
	}

	case ACT_RECHARGE:
	{
		recharge(user_ptr, 130);
		break;
	}

	case ACT_ALCHEMY:
	{
		msg_print(_("明るい黄色に輝いている...", "It glows bright yellow..."));
		(void)alchemy(user_ptr);
		break;
	}

	case ACT_DIM_DOOR:
	{
		msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
		if (!dimension_door(user_ptr)) return FALSE;
		break;
	}


	case ACT_TELEPORT:
	{
		msg_print(_("周りの空間が歪んでいる...", "It twists space around you..."));
		teleport_player(user_ptr, 100, 0L);
		break;
	}

	case ACT_RECALL:
	{
		msg_print(_("やわらかな白色に輝いている...", "It glows soft white..."));
		if (!recall_player(user_ptr, randint0(21) + 15)) return FALSE;
		break;
	}

	case ACT_JUDGE:
	{
		msg_format(_("%sは赤く明るく光った！", "The %s flashes bright red!"), name);
		chg_virtue(user_ptr, V_KNOWLEDGE, 1);
		chg_virtue(user_ptr, V_ENLIGHTEN, 1);
		wiz_lite(user_ptr, FALSE);

		msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
		take_hit(user_ptr, DAMAGE_LOSELIFE, damroll(3, 8), _("審判の宝石", "the Jewel of Judgement"), -1);

		(void)detect_traps(user_ptr, DETECT_RAD_DEFAULT, TRUE);
		(void)detect_doors(user_ptr, DETECT_RAD_DEFAULT);
		(void)detect_stairs(user_ptr, DETECT_RAD_DEFAULT);

		if (get_check(_("帰還の力を使いますか？", "Activate recall? ")))
		{
			(void)recall_player(user_ptr, randint0(21) + 15);
		}

		break;
	}

	case ACT_TELEKINESIS:
	{
		if (!get_aim_dir(user_ptr, &dir)) return FALSE;
		msg_format(_("%sを伸ばした。", "You stretched your %s."), name);
		fetch(user_ptr, dir, 500, TRUE);
		break;
	}

	case ACT_DETECT_UNIQUE:
	{
		int i;
		monster_type *m_ptr;
		monster_race *r_ptr;
		msg_print(_("奇妙な場所が頭の中に浮かんだ．．．", "Some strange places show up in your mind. And you see ..."));
		/* Process the monsters (backwards) */
		for (i = user_ptr->current_floor_ptr->m_max - 1; i >= 1; i--)
		{
			m_ptr = &user_ptr->current_floor_ptr->m_list[i];

			/* Ignore "dead" monsters */
			if (!monster_is_valid(m_ptr)) continue;

			r_ptr = &r_info[m_ptr->r_idx];

			if (r_ptr->flags1 & RF1_UNIQUE)
			{
				msg_format(_("%s． ", "%s. "), r_name + r_ptr->name);
			}
		}
		break;
	}

	case ACT_ESCAPE:
	{
		switch (randint1(13))
		{
		case 1: case 2: case 3: case 4: case 5:
			teleport_player(user_ptr, 10, 0L);
			break;
		case 6: case 7: case 8: case 9: case 10:
			teleport_player(user_ptr, 222, 0L);
			break;
		case 11: case 12:
			(void)stair_creation(user_ptr);
			break;
		default:
			if (get_check(_("この階を去りますか？", "Leave this level? ")))
			{
				if (autosave_l) do_cmd_save_game(user_ptr, TRUE);
				user_ptr->leaving = TRUE;
			}
		}
		break;
	}

	case ACT_DISP_CURSE_XTRA:
	{
		msg_format(_("%sが真実を照らし出す...", "The %s exhibits the truth..."), name);
		(void)remove_all_curse(user_ptr);
		(void)probing(user_ptr);
		break;
	}

	case ACT_BRAND_FIRE_BOLTS:
	{
		msg_format(_("%sが深紅に輝いた...", "Your %s glows deep red..."), name);
		brand_bolts(user_ptr);
		break;
	}

	case ACT_RECHARGE_XTRA:
	{
		msg_format(_("%sが白く輝いた．．．", "The %s gleams with blinding light..."), name);
		if (!recharge(user_ptr, 1000)) return FALSE;
		break;
	}

	case ACT_LORE:
		msg_print(_("石が隠された秘密を写し出した．．．", "The stone reveals hidden mysteries..."));
		if (!perilous_secrets(user_ptr)) return FALSE;
		break;

	case ACT_SHIKOFUMI:
	{
		msg_print(_("力強く四股を踏んだ。", "You stamp. (as if you are in a ring.)"));
		(void)set_afraid(user_ptr, 0);
		(void)set_hero(user_ptr, randint1(20) + 20, FALSE);
		dispel_evil(user_ptr, user_ptr->lev * 3);
		break;
	}

	case ACT_PHASE_DOOR:
	{
		teleport_player(user_ptr, 10, 0L);
		break;
	}

	case ACT_DETECT_ALL_MONS:
	{
		(void)detect_monsters_invis(user_ptr, 255);
		(void)detect_monsters_normal(user_ptr, 255);
		break;
	}

	case ACT_ULTIMATE_RESIST:
	{
		TIME_EFFECT v = randint1(25) + 25;
		(void)set_afraid(user_ptr, 0);
		(void)set_hero(user_ptr, v, FALSE);
		(void)hp_player(user_ptr, 10);
		(void)set_blessed(user_ptr, v, FALSE);
		(void)set_oppose_acid(user_ptr, v, FALSE);
		(void)set_oppose_elec(user_ptr, v, FALSE);
		(void)set_oppose_fire(user_ptr, v, FALSE);
		(void)set_oppose_cold(user_ptr, v, FALSE);
		(void)set_oppose_pois(user_ptr, v, FALSE);
		(void)set_ultimate_res(user_ptr, v, FALSE);
		break;
	}

	case ACT_CAST_OFF:
		cosmic_cast_off(user_ptr, o_ptr);
		break;

	case ACT_FALLING_STAR:
	{
		msg_print(_("あなたは妖刀に魅入られた…", "You are enchanted by cursed blade..."));
		msg_print(_("「狂ほしく 血のごとき 月はのぼれり 秘めおきし 魔剣 いずこぞや」", "'Behold the blade arts.'"));
		massacre(user_ptr);
		break;
	}

	case ACT_GRAND_CROSS:
	{
		msg_print(_("「闇に還れ！」", "You say, 'Return to darkness!'"));
		project(user_ptr, 0, 8, user_ptr->y, user_ptr->x, (randint1(100) + 200) * 2, GF_HOLY_FIRE, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID, -1);
		break;
	}

	case ACT_TELEPORT_LEVEL:
	{
		if (!get_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)"))) return FALSE;
		teleport_level(user_ptr, 0);
		break;
	}

	case ACT_STRAIN_HASTE:
	{
		int t;
		msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
		take_hit(user_ptr, DAMAGE_LOSELIFE, damroll(3, 8), _("加速した疲労", "the strain of haste"), -1);
		t = 25 + randint1(25);
		(void)set_fast(user_ptr, user_ptr->fast + t, FALSE);
		break;
	}

	case ACT_FISHING:
		if (!fishing(user_ptr)) return FALSE;
		break;

	case ACT_INROU:
		mitokohmon(user_ptr);
		break;

	case ACT_MURAMASA:
	{
		/* Only for Muramasa */
		if (o_ptr->name1 != ART_MURAMASA) return FALSE;
		if (get_check(_("本当に使いますか？", "Are you sure?!")))
		{
			msg_print(_("村正が震えた．．．", "The Muramasa pulsates..."));
			do_inc_stat(user_ptr, A_STR);
			if (one_in_(2))
			{
				msg_print(_("村正は壊れた！", "The Muramasa is destroyed!"));
				curse_weapon_object(user_ptr, TRUE, o_ptr);
			}
		}
		break;
	}

	case ACT_BLOODY_MOON:
	{
		/* Only for Bloody Moon */
		if (o_ptr->name1 != ART_BLOOD) return FALSE;
		msg_print(_("鎌が明るく輝いた...", "Your scythe glows brightly!"));
		get_bloody_moon_flags(o_ptr);
		if (user_ptr->prace == RACE_ANDROID) calc_android_exp(user_ptr);
		user_ptr->update |= (PU_BONUS | PU_HP);
		break;
	}

	case ACT_CRIMSON:
		if (o_ptr->name1 != ART_CRIMSON) return FALSE;
		msg_print(_("せっかくだから『クリムゾン』をぶっぱなすぜ！", "I'll fire CRIMSON! SEKKAKUDAKARA!"));
		if (!fire_crimson(user_ptr)) return FALSE;
		break;

	default:
	{
		msg_format(_("Unknown activation effect: %d.", "Unknown activation effect: %d."), act_ptr->index);
		return FALSE;
	}
	}

	/* Set activation timeout */
	if (act_ptr->timeout.constant >= 0) {
		o_ptr->timeout = (s16b)act_ptr->timeout.constant;
		if (act_ptr->timeout.dice > 0) {
			o_ptr->timeout += randint1(act_ptr->timeout.dice);
		}
	}
	else {
		/* Activations that have special timeout */
		switch (act_ptr->index) {
		case ACT_BR_FIRE:
			o_ptr->timeout = ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) ? 200 : 250;
			break;
		case ACT_BR_COLD:
			o_ptr->timeout = ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) ? 200 : 250;
			break;
		case ACT_TERROR:
			o_ptr->timeout = 3 * (user_ptr->lev + 10);
			break;
		case ACT_MURAMASA:
			/* Nothing to do */
			break;
		default:
			msg_format("Special timeout is not implemented: %d.", act_ptr->index);
			return FALSE;
		}
	}

	return TRUE;
}
