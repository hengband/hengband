/*!
 * @brief プレイヤーの鍛冶コマンド実装
 * @date 2019/03/11
 * @author deskull
 */

#include "cmd-item/cmd-smith.h"
#include "action/action-limited.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/text-display-options.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "mind/mind-weaponsmith.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h" //!< @todo 相互参照している.
#include "perception/object-perception.h"
#include "player-status/player-energy.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

#include <memory>

/*!
 * エッセンス情報の構造体 / A structure for smithing
 */
typedef struct {
    int add; /* TR flag number or special essence id */
    concptr add_name; /* Name of this ability */
    int32_t type; /* Menu number */
    int essence; /* Index for carrying essences */
    int value; /* Needed value to add this ability */
} essence_type;

/*!
 * エッセンス情報テーブル Smithing type data for Weapon smith
 */
#ifdef JP
static essence_type essence_info[] = { { TR_STR, "腕力", 4, TR_STR, 20 }, { TR_INT, "知能", 4, TR_INT, 20 }, { TR_WIS, "賢さ", 4, TR_WIS, 20 },
    { TR_DEX, "器用さ", 4, TR_DEX, 20 }, { TR_CON, "耐久力", 4, TR_CON, 20 }, { TR_CHR, "魅力", 4, TR_CHR, 20 },
    { TR_MAGIC_MASTERY, "魔力支配", 4, TR_MAGIC_MASTERY, 20 }, { TR_STEALTH, "隠密", 4, TR_STEALTH, 40 }, { TR_SEARCH, "探索", 4, TR_SEARCH, 15 },
    { TR_INFRA, "赤外線視力", 4, TR_INFRA, 15 }, { TR_TUNNEL, "採掘", 4, TR_TUNNEL, 15 }, { TR_SPEED, "スピード", 4, TR_SPEED, 12 },
    { TR_BLOWS, "追加攻撃", 1, TR_BLOWS, 20 }, { TR_CHAOTIC, "カオス攻撃", 1, TR_CHAOTIC, 15 }, { TR_VAMPIRIC, "吸血攻撃", 1, TR_VAMPIRIC, 60 },
    { TR_EARTHQUAKE, "地震発動", 7, TR_EARTHQUAKE, 15 }, { TR_BRAND_POIS, "毒殺", 1, TR_BRAND_POIS, 20 }, { TR_BRAND_ACID, "溶解", 1, TR_BRAND_ACID, 20 },
    { TR_BRAND_ELEC, "電撃", 1, TR_BRAND_ELEC, 20 }, { TR_BRAND_FIRE, "焼棄", 1, TR_BRAND_FIRE, 20 }, { TR_BRAND_COLD, "凍結", 1, TR_BRAND_COLD, 20 },
    { TR_SUST_STR, "腕力維持", 3, TR_SUST_STR, 15 }, { TR_SUST_INT, "知能維持", 3, TR_SUST_STR, 15 }, { TR_SUST_WIS, "賢さ維持", 3, TR_SUST_STR, 15 },
    { TR_SUST_DEX, "器用さ維持", 3, TR_SUST_STR, 15 }, { TR_SUST_CON, "耐久力維持", 3, TR_SUST_STR, 15 }, { TR_SUST_CHR, "魅力維持", 3, TR_SUST_STR, 15 },
    { TR_IM_ACID, "酸免疫", 2, TR_IM_ACID, 20 }, { TR_IM_ELEC, "電撃免疫", 2, TR_IM_ACID, 20 }, { TR_IM_FIRE, "火炎免疫", 2, TR_IM_ACID, 20 },
    { TR_IM_COLD, "冷気免疫", 2, TR_IM_ACID, 20 }, { TR_REFLECT, "反射", 2, TR_REFLECT, 20 }, { TR_FREE_ACT, "麻痺知らず", 3, TR_FREE_ACT, 20 },
    { TR_HOLD_EXP, "経験値維持", 3, TR_HOLD_EXP, 20 }, { TR_RES_ACID, "耐酸", 2, TR_RES_ACID, 15 }, { TR_RES_ELEC, "耐電撃", 2, TR_RES_ELEC, 15 },
    { TR_RES_FIRE, "耐火炎", 2, TR_RES_FIRE, 15 }, { TR_RES_COLD, "耐冷気", 2, TR_RES_COLD, 15 }, { TR_RES_POIS, "耐毒", 2, TR_RES_POIS, 25 },
    { TR_RES_FEAR, "耐恐怖", 2, TR_RES_FEAR, 20 }, { TR_RES_LITE, "耐閃光", 2, TR_RES_LITE, 20 }, { TR_RES_DARK, "耐暗黒", 2, TR_RES_DARK, 20 },
    { TR_RES_BLIND, "耐盲目", 2, TR_RES_BLIND, 20 }, { TR_RES_CONF, "耐混乱", 2, TR_RES_CONF, 20 }, { TR_RES_SOUND, "耐轟音", 2, TR_RES_SOUND, 20 },
    { TR_RES_SHARDS, "耐破片", 2, TR_RES_SHARDS, 20 }, { TR_RES_NETHER, "耐地獄", 2, TR_RES_NETHER, 20 }, { TR_RES_NEXUS, "耐因果混乱", 2, TR_RES_NEXUS, 20 },
    { TR_RES_CHAOS, "耐カオス", 2, TR_RES_CHAOS, 20 }, { TR_RES_DISEN, "耐劣化", 2, TR_RES_DISEN, 20 }, { TR_SH_FIRE, "", 0, -2, 0 },
    { TR_SH_ELEC, "", 0, -2, 0 }, { TR_SH_COLD, "", 0, -2, 0 }, { TR_NO_MAGIC, "反魔法", 3, TR_NO_MAGIC, 15 }, { TR_WARNING, "警告", 3, TR_WARNING, 20 },
    { TR_LEVITATION, "浮遊", 3, TR_LEVITATION, 20 }, { TR_LITE_1, "永久光源", 3, TR_LITE_1, 15 }, { TR_LITE_2, "", 0, -2, 0 }, { TR_LITE_3, "", 0, -2, 0 },
    { TR_SEE_INVIS, "可視透明", 3, TR_SEE_INVIS, 20 }, { TR_TELEPATHY, "テレパシー", 6, TR_TELEPATHY, 15 }, { TR_SLOW_DIGEST, "遅消化", 3, TR_SLOW_DIGEST, 15 },
    { TR_REGEN, "急速回復", 3, TR_REGEN, 20 }, { TR_TELEPORT, "テレポート", 3, TR_TELEPORT, 25 },

    { TR_SLAY_EVIL, "邪悪倍打", 5, TR_SLAY_EVIL, 100 }, { TR_KILL_EVIL, "邪悪倍倍打", 0, TR_SLAY_EVIL, 60 },
    { TR_SLAY_ANIMAL, "動物倍打", 5, TR_SLAY_ANIMAL, 20 }, { TR_KILL_ANIMAL, "動物倍倍打", 5, TR_SLAY_ANIMAL, 60 },
    { TR_SLAY_UNDEAD, "不死倍打", 5, TR_SLAY_UNDEAD, 20 }, { TR_KILL_UNDEAD, "不死倍倍打", 5, TR_SLAY_UNDEAD, 60 },
    { TR_SLAY_DEMON, "悪魔倍打", 5, TR_SLAY_DEMON, 20 }, { TR_KILL_DEMON, "悪魔倍倍打", 5, TR_SLAY_DEMON, 60 },
    { TR_SLAY_ORC, "オーク倍打", 5, TR_SLAY_ORC, 15 }, { TR_KILL_ORC, "オーク倍倍打", 5, TR_SLAY_ORC, 60 },
    { TR_SLAY_TROLL, "トロル倍打", 5, TR_SLAY_TROLL, 15 }, { TR_KILL_TROLL, "トロル倍倍打", 5, TR_SLAY_TROLL, 60 },
    { TR_SLAY_GIANT, "巨人倍打", 5, TR_SLAY_GIANT, 20 }, { TR_KILL_GIANT, "巨人倍倍打", 5, TR_SLAY_GIANT, 60 },
    { TR_SLAY_DRAGON, "竜倍打", 5, TR_SLAY_DRAGON, 20 }, { TR_KILL_DRAGON, "竜倍倍打", 5, TR_SLAY_DRAGON, 60 },
    { TR_SLAY_HUMAN, "人間倍打", 5, TR_SLAY_HUMAN, 20 }, { TR_KILL_HUMAN, "人間倍倍打", 5, TR_SLAY_HUMAN, 60 },

    { TR_ESP_ANIMAL, "動物ESP", 6, TR_SLAY_ANIMAL, 40 }, { TR_ESP_UNDEAD, "不死ESP", 6, TR_SLAY_UNDEAD, 40 }, { TR_ESP_DEMON, "悪魔ESP", 6, TR_SLAY_DEMON, 40 },
    { TR_ESP_ORC, "オークESP", 6, TR_SLAY_ORC, 40 }, { TR_ESP_TROLL, "トロルESP", 6, TR_SLAY_TROLL, 40 }, { TR_ESP_GIANT, "巨人ESP", 6, TR_SLAY_GIANT, 40 },
    { TR_ESP_DRAGON, "竜ESP", 6, TR_SLAY_DRAGON, 40 }, { TR_ESP_HUMAN, "人間ESP", 6, TR_SLAY_HUMAN, 40 },

    { ESSENCE_ATTACK, "攻撃", 10, TR_ES_ATTACK, 30 }, { ESSENCE_AC, "防御", 10, TR_ES_AC, 15 }, { ESSENCE_TMP_RES_ACID, "酸耐性発動", 7, TR_RES_ACID, 50 },
    { ESSENCE_TMP_RES_ELEC, "電撃耐性発動", 7, TR_RES_ELEC, 50 }, { ESSENCE_TMP_RES_FIRE, "火炎耐性発動", 7, TR_RES_FIRE, 50 },
    { ESSENCE_TMP_RES_COLD, "冷気耐性発動", 7, TR_RES_COLD, 50 }, { ESSENCE_SH_FIRE, "火炎オーラ", 7, -1, 50 }, { ESSENCE_SH_ELEC, "電撃オーラ", 7, -1, 50 },
    { ESSENCE_SH_COLD, "冷気オーラ", 7, -1, 50 }, { ESSENCE_RESISTANCE, "全耐性", 2, -1, 150 }, { ESSENCE_SUSTAIN, "装備保持", 10, -1, 10 },
    { ESSENCE_SLAY_GLOVE, "殺戮の小手", 1, TR_ES_ATTACK, 200 },

    { -1, nullptr, 0, -1, 0 } };
#else
static essence_type essence_info[] = { { TR_STR, "strength", 4, TR_STR, 20 }, { TR_INT, "intelligence", 4, TR_INT, 20 }, { TR_WIS, "wisdom", 4, TR_WIS, 20 },
    { TR_DEX, "dexterity", 4, TR_DEX, 20 }, { TR_CON, "constitution", 4, TR_CON, 20 }, { TR_CHR, "charisma", 4, TR_CHR, 20 },
    { TR_MAGIC_MASTERY, "magic mastery", 4, TR_MAGIC_MASTERY, 20 }, { TR_STEALTH, "stealth", 4, TR_STEALTH, 40 }, { TR_SEARCH, "searching", 4, TR_SEARCH, 15 },
    { TR_INFRA, "infravision", 4, TR_INFRA, 15 }, { TR_TUNNEL, "digging", 4, TR_TUNNEL, 15 }, { TR_SPEED, "speed", 4, TR_SPEED, 12 },
    { TR_BLOWS, "extra attack", 1, TR_BLOWS, 20 }, { TR_CHAOTIC, "chaos brand", 1, TR_CHAOTIC, 15 }, { TR_VAMPIRIC, "vampiric brand", 1, TR_VAMPIRIC, 60 },
    { TR_IMPACT, "quake activation", 7, TR_IMPACT, 15 }, { TR_BRAND_POIS, "poison brand", 1, TR_BRAND_POIS, 20 },
    { TR_BRAND_ACID, "acid brand", 1, TR_BRAND_ACID, 20 }, { TR_BRAND_ELEC, "electric brand", 1, TR_BRAND_ELEC, 20 },
    { TR_BRAND_FIRE, "fire brand", 1, TR_BRAND_FIRE, 20 }, { TR_BRAND_COLD, "cold brand", 1, TR_BRAND_COLD, 20 },
    { TR_SUST_STR, "sustain strength", 3, TR_SUST_STR, 15 }, { TR_SUST_INT, "sustain intelligence", 3, TR_SUST_STR, 15 },
    { TR_SUST_WIS, "sustain wisdom", 3, TR_SUST_STR, 15 }, { TR_SUST_DEX, "sustain dexterity", 3, TR_SUST_STR, 15 },
    { TR_SUST_CON, "sustain constitution", 3, TR_SUST_STR, 15 }, { TR_SUST_CHR, "sustain charisma", 3, TR_SUST_STR, 15 },
    { TR_IM_ACID, "acid immunity", 2, TR_IM_ACID, 20 }, { TR_IM_ELEC, "electric immunity", 2, TR_IM_ACID, 20 },
    { TR_IM_FIRE, "fire immunity", 2, TR_IM_ACID, 20 }, { TR_IM_COLD, "cold immunity", 2, TR_IM_ACID, 20 }, { TR_REFLECT, "reflection", 2, TR_REFLECT, 20 },
    { TR_FREE_ACT, "free action", 3, TR_FREE_ACT, 20 }, { TR_HOLD_EXP, "hold experience", 3, TR_HOLD_EXP, 20 },
    { TR_RES_ACID, "resistance to acid", 2, TR_RES_ACID, 15 }, { TR_RES_ELEC, "resistance to electric", 2, TR_RES_ELEC, 15 },
    { TR_RES_FIRE, "resistance to fire", 2, TR_RES_FIRE, 15 }, { TR_RES_COLD, "resistance to cold", 2, TR_RES_COLD, 15 },
    { TR_RES_POIS, "resistance to poison", 2, TR_RES_POIS, 25 }, { TR_RES_FEAR, "resistance to fear", 2, TR_RES_FEAR, 20 },
    { TR_RES_LITE, "resistance to light", 2, TR_RES_LITE, 20 }, { TR_RES_DARK, "resistance to dark", 2, TR_RES_DARK, 20 },
    { TR_RES_BLIND, "resistance to blind", 2, TR_RES_BLIND, 20 }, { TR_RES_CONF, "resistance to confusion", 2, TR_RES_CONF, 20 },
    { TR_RES_SOUND, "resistance to sound", 2, TR_RES_SOUND, 20 }, { TR_RES_SHARDS, "resistance to shard", 2, TR_RES_SHARDS, 20 },
    { TR_RES_NETHER, "resistance to nether", 2, TR_RES_NETHER, 20 }, { TR_RES_NEXUS, "resistance to nexus", 2, TR_RES_NEXUS, 20 },
    { TR_RES_CHAOS, "resistance to chaos", 2, TR_RES_CHAOS, 20 }, { TR_RES_DISEN, "resistance to disenchantment", 2, TR_RES_DISEN, 20 },
    { TR_SH_FIRE, "", 0, -2, 0 }, { TR_SH_ELEC, "", 0, -2, 0 }, { TR_SH_COLD, "", 0, -2, 0 }, { TR_NO_MAGIC, "anti magic", 3, TR_NO_MAGIC, 15 },
    { TR_WARNING, "warning", 3, TR_WARNING, 20 }, { TR_LEVITATION, "levitation", 3, TR_LEVITATION, 20 }, { TR_LITE_1, "permanent light", 3, TR_LITE_1, 15 },
    { TR_LITE_2, "", 0, -2, 0 }, { TR_LITE_3, "", 0, -2, 0 }, { TR_SEE_INVIS, "see invisible", 3, TR_SEE_INVIS, 20 },
    { TR_TELEPATHY, "telepathy", 6, TR_TELEPATHY, 15 }, { TR_SLOW_DIGEST, "slow digestion", 3, TR_SLOW_DIGEST, 15 },
    { TR_REGEN, "regeneration", 3, TR_REGEN, 20 }, { TR_TELEPORT, "teleport", 3, TR_TELEPORT, 25 },

    { TR_SLAY_EVIL, "slay evil", 5, TR_SLAY_EVIL, 100 }, { TR_KILL_EVIL, "kill evil", 0, TR_SLAY_EVIL, 60 },
    { TR_SLAY_ANIMAL, "slay animal", 5, TR_SLAY_ANIMAL, 20 }, { TR_KILL_ANIMAL, "kill animal", 5, TR_SLAY_ANIMAL, 60 },
    { TR_SLAY_UNDEAD, "slay undead", 5, TR_SLAY_UNDEAD, 20 }, { TR_KILL_UNDEAD, "kill undead", 5, TR_SLAY_UNDEAD, 60 },
    { TR_SLAY_DEMON, "slay demon", 5, TR_SLAY_DEMON, 20 }, { TR_KILL_DEMON, "kill demon", 5, TR_SLAY_DEMON, 60 },
    { TR_SLAY_ORC, "slay orc", 5, TR_SLAY_ORC, 15 }, { TR_KILL_ORC, "kill orc", 5, TR_SLAY_ORC, 60 }, { TR_SLAY_TROLL, "slay troll", 5, TR_SLAY_TROLL, 15 },
    { TR_KILL_TROLL, "kill troll", 5, TR_SLAY_TROLL, 60 }, { TR_SLAY_GIANT, "slay giant", 5, TR_SLAY_GIANT, 20 },
    { TR_KILL_GIANT, "kill giant", 5, TR_SLAY_GIANT, 60 }, { TR_SLAY_DRAGON, "slay dragon", 5, TR_SLAY_DRAGON, 20 },
    { TR_KILL_DRAGON, "kill dragon", 5, TR_SLAY_DRAGON, 60 }, { TR_SLAY_HUMAN, "slay human", 5, TR_SLAY_HUMAN, 20 },
    { TR_KILL_HUMAN, "kill human", 5, TR_SLAY_HUMAN, 60 },

    { TR_ESP_ANIMAL, "sense animal", 6, TR_SLAY_ANIMAL, 40 }, { TR_ESP_UNDEAD, "sense undead", 6, TR_SLAY_UNDEAD, 40 },
    { TR_ESP_DEMON, "sense demon", 6, TR_SLAY_DEMON, 40 }, { TR_ESP_ORC, "sense orc", 6, TR_SLAY_ORC, 40 },
    { TR_ESP_TROLL, "sense troll", 6, TR_SLAY_TROLL, 40 }, { TR_ESP_GIANT, "sense giant", 6, TR_SLAY_GIANT, 40 },
    { TR_ESP_DRAGON, "sense dragon", 6, TR_SLAY_DRAGON, 40 }, { TR_ESP_HUMAN, "sense human", 6, TR_SLAY_HUMAN, 40 },

    { ESSENCE_ATTACK, "weapon enchant", 10, TR_ES_ATTACK, 30 }, { ESSENCE_AC, "armor enchant", 10, TR_ES_AC, 15 },
    { ESSENCE_TMP_RES_ACID, "resist acid activation", 7, TR_RES_ACID, 50 }, { ESSENCE_TMP_RES_ELEC, "resist electricity activation", 7, TR_RES_ELEC, 50 },
    { ESSENCE_TMP_RES_FIRE, "resist fire activation", 7, TR_RES_FIRE, 50 }, { ESSENCE_TMP_RES_COLD, "resist cold activation", 7, TR_RES_COLD, 50 },
    { ESSENCE_SH_FIRE, "fiery sheath", 7, -1, 50 }, { ESSENCE_SH_ELEC, "electric sheath", 7, -1, 50 }, { ESSENCE_SH_COLD, "sheath of coldness", 7, -1, 50 },
    { ESSENCE_RESISTANCE, "resistance", 2, -1, 150 }, { ESSENCE_SUSTAIN, "elements proof", 10, -1, 10 },
    { ESSENCE_SLAY_GLOVE, "gauntlets of slaying", 1, TR_ES_ATTACK, 200 },

    { -1, nullptr, 0, -1, 0 } };
#endif

/*!
 * エッセンス名テーブル / Essense names for Weapon smith
 */
#ifdef JP
concptr essence_name[] = { "腕力", "知能", "賢さ", "器用さ", "耐久力", "魅力", "魔力支配", "", "隠密", "探索", "赤外線視力", "採掘", "スピード", "追加攻撃",
    "カオス攻撃", "吸血攻撃", "動物倍打", "邪悪倍打", "不死倍打", "悪魔倍打", "オーク倍打", "トロル倍打", "巨人倍打", "竜倍打", "", "", "地震", "毒殺", "溶解",
    "電撃", "焼棄", "凍結", "能力維持", "", "", "", "", "", "", "", "免疫", "", "", "", "", "反射", "麻痺知らず", "経験値維持", "耐酸", "耐電撃", "耐火炎",
    "耐冷気", "耐毒", "耐恐怖", "耐閃光", "耐暗黒", "耐盲目", "耐混乱", "耐轟音", "耐破片", "耐地獄", "耐因果混乱", "耐カオス", "耐劣化", "", "", "人間倍打",
    "", "", "反魔法", "", "", "警告", "", "", "", "浮遊", "永久光源", "可視透明", "テレパシー", "遅消化", "急速回復", "", "", "", "", "", "", "", "",
    "テレポート", "", "", "攻撃", "防御",

    nullptr };

#else

concptr essence_name[] = { "strength", "intelligen.", "wisdom", "dexterity", "constitut.", "charisma", "magic mast.", "", "stealth", "searching", "infravision",
    "digging", "speed", "extra atk", "chaos brand", "vampiric", "slay animal", "slay evil", "slay undead", "slay demon", "slay orc", "slay troll", "slay giant",
    "slay dragon", "", "", "quake", "pois. brand", "acid brand", "elec. brand", "fire brand", "cold brand", "sustain", "", "", "", "", "", "", "", "immunity",
    "", "", "", "", "reflection", "free action", "hold exp", "res. acid", "res. elec.", "res. fire", "res. cold", "res. poison", "res. fear", "res. light",
    "res. dark", "res. blind", "res.confuse", "res. sound", "res. shard", "res. nether", "res. nexus", "res. chaos", "res. disen.", "", "", "slay human", "",
    "", "anti magic", "", "", "warning", "", "", "", "levitation", "perm. light", "see invis.", "telepathy", "slow dige.", "regen.", "", "", "", "", "", "", "",
    "", "teleport", "", "", "weapon enc.", "armor enc.",

    nullptr };
#endif

static concptr const kaji_tips[5] = {
#ifdef JP
    "現在持っているエッセンスの一覧を表示する。",
    "アイテムからエッセンスを取り出す。エッセンスを取られたアイテムは全く魔法がかかっていない初期状態に戻る。",
    "既にエッセンスが付加されたアイテムからエッセンスのみ消し去る。エッセンスは手に入らない。",
    "アイテムにエッセンスを付加する。既にエッセンスが付加されたアイテムやアーティファクトには付加できない。",
    "武器や防具を強化したり、攻撃で傷つかないようにしたりする。エッセンスが付加されたアイテムやアーティファクトに対しても使用できる。",
#else
    "Display essences you have.",
    "Extract essences from an item. The item become non magical.",
    "Remove added essences from equipment which was improved before. The removed essence will be ruined.",
    "Add essences to an item. The improved items or artifacts cannot be reimprove.",
    "Enchant an item or make an item element-proofed. Improved items and artifacts can be enchanted too.",
#endif
};

/*!
 * @brief 所持しているエッセンス一覧を表示する
 */
static void display_essence(player_type *creature_ptr)
{
    int i, num = 0;

    screen_save();
    for (i = 1; i < 22; i++) {
        prt("", i, 0);
    }
    prt(_("エッセンス   個数     エッセンス   個数     エッセンス   個数", "Essence      Num      Essence      Num      Essence      Num "), 1, 8);
    for (i = 0; essence_name[i]; i++) {
        if (!essence_name[i][0])
            continue;
        prt(format("%-11s %5d", essence_name[i], creature_ptr->magic_num1[i]), 2 + num % 21, 8 + num / 21 * 22);
        num++;
    }
    prt(_("現在所持しているエッセンス", "List of all essences you have."), 0, 0);
    (void)inkey();
    screen_load();
    return;
}

/*!
 * @brief エッセンスの抽出処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void drain_essence(player_type *creature_ptr)
{
    int drain_value[sizeof(creature_ptr->magic_num1) / sizeof(int32_t)];
    size_t i;
    OBJECT_IDX item;
    int dec = 4;
    bool observe = false;
    int old_ds, old_dd, old_to_h, old_to_d, old_ac, old_to_a, old_pval, old_name2;
    TIME_EFFECT old_timeout;
    object_type *o_ptr;
    concptr q, s;
    POSITION iy, ix;
    byte marked;
    ITEM_NUMBER number;

    for (i = 0; i < sizeof(drain_value) / sizeof(int); i++)
        drain_value[i] = 0;

    q = _("どのアイテムから抽出しますか？", "Extract from which item? ");
    s = _("抽出できるアイテムがありません。", "You have nothing you can extract from.");

    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), FuncItemTester(&object_type::is_weapon_armour_ammo));
    if (!o_ptr)
        return;

    if (o_ptr->is_known() && !o_ptr->is_nameless()) {
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        if (!get_check(format(_("本当に%sから抽出してよろしいですか？", "Really extract from %s? "), o_name)))
            return;
    }

    PlayerEnergy(creature_ptr).set_player_turn_energy(100);

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

    old_to_a = o_ptr->to_a;
    old_ac = o_ptr->ac;
    old_to_h = o_ptr->to_h;
    old_to_d = o_ptr->to_d;
    old_ds = o_ptr->ds;
    old_dd = o_ptr->dd;
    old_pval = o_ptr->pval;
    old_name2 = o_ptr->name2;
    old_timeout = o_ptr->timeout;
    if (o_ptr->curse_flags.has_any_of({ TRC::CURSED, TRC::HEAVY_CURSE, TRC::PERMA_CURSE }))
        dec--;
    if (old_flgs.has(TR_ADD_L_CURSE))
        dec--;
    if (old_flgs.has(TR_ADD_H_CURSE))
        dec--;
    if (old_flgs.has(TR_AGGRAVATE))
        dec--;
    if (old_flgs.has(TR_NO_TELE))
        dec--;
    if (old_flgs.has(TR_DRAIN_EXP))
        dec--;
    if (old_flgs.has(TR_DRAIN_HP))
        dec--;
    if (old_flgs.has(TR_DRAIN_MANA))
        dec--;
    if (old_flgs.has(TR_CALL_ANIMAL))
        dec--;
    if (old_flgs.has(TR_CALL_DEMON))
        dec--;
    if (old_flgs.has(TR_CALL_DRAGON))
        dec--;
    if (old_flgs.has(TR_CALL_UNDEAD))
        dec--;
    if (old_flgs.has(TR_COWARDICE))
        dec--;
    if (old_flgs.has(TR_LOW_MELEE))
        dec--;
    if (old_flgs.has(TR_LOW_AC))
        dec--;
    if (old_flgs.has(TR_HARD_SPELL))
        dec--;
    if (old_flgs.has(TR_FAST_DIGEST))
        dec--;
    if (old_flgs.has(TR_SLOW_REGEN))
        dec--;
    if (old_flgs.has(TR_TY_CURSE))
        dec--;

    iy = o_ptr->iy;
    ix = o_ptr->ix;
    marked = o_ptr->marked;
    number = o_ptr->number;

    o_ptr->prep(o_ptr->k_idx);

    o_ptr->iy = iy;
    o_ptr->ix = ix;
    o_ptr->marked = marked;
    o_ptr->number = number;
    if (o_ptr->tval == TV_DRAG_ARMOR)
        o_ptr->timeout = old_timeout;
    o_ptr->ident |= (IDENT_FULL_KNOWN);
    object_aware(creature_ptr, o_ptr);
    object_known(o_ptr);

    auto new_flgs = object_flags(o_ptr);

    for (i = 0; essence_info[i].add_name; i++) {
        essence_type *es_ptr = &essence_info[i];
        PARAMETER_VALUE pval = 0;
        const auto drain_flag = static_cast<tr_type>(es_ptr->add);

        if (es_ptr->add < TR_FLAG_MAX && TR_PVAL_FLAG_MASK.has(drain_flag) && old_pval)
            pval = new_flgs.has(drain_flag) ? old_pval - o_ptr->pval : old_pval;

        if (es_ptr->add < TR_FLAG_MAX && (new_flgs.has_not(drain_flag) || pval) && old_flgs.has(drain_flag)) {
            if (pval) {
                drain_value[es_ptr->essence] += 10 * pval;
            } else if (es_ptr->essence != -2) {
                drain_value[es_ptr->essence] += 10;
            } else if (es_ptr->add == TR_SH_FIRE) {
                drain_value[TR_BRAND_FIRE] += 10;
                drain_value[TR_RES_FIRE] += 10;
            } else if (es_ptr->add == TR_SH_ELEC) {
                drain_value[TR_BRAND_ELEC] += 10;
                drain_value[TR_RES_ELEC] += 10;
            } else if (es_ptr->add == TR_SH_COLD) {
                drain_value[TR_BRAND_COLD] += 10;
                drain_value[TR_RES_COLD] += 10;
            } else if (es_ptr->add == TR_LITE_2) {
                drain_value[TR_LITE_1] += 20;
            } else if (es_ptr->add == TR_LITE_3) {
                drain_value[TR_LITE_1] += 30;
            }
        }
    }

    if ((old_flgs.has(TR_FORCE_WEAPON)) && new_flgs.has_not(TR_FORCE_WEAPON)) {
        drain_value[TR_INT] += 5;
        drain_value[TR_WIS] += 5;
    }
    if ((old_flgs.has(TR_VORPAL)) && new_flgs.has_not(TR_VORPAL)) {
        drain_value[TR_BRAND_POIS] += 5;
        drain_value[TR_BRAND_ACID] += 5;
        drain_value[TR_BRAND_ELEC] += 5;
        drain_value[TR_BRAND_FIRE] += 5;
        drain_value[TR_BRAND_COLD] += 5;
    }
    if ((old_flgs.has(TR_DEC_MANA)) && new_flgs.has_not(TR_DEC_MANA)) {
        drain_value[TR_INT] += 10;
    }
    if ((old_flgs.has(TR_XTRA_MIGHT)) && new_flgs.has_not(TR_XTRA_MIGHT)) {
        drain_value[TR_STR] += 10;
    }
    if ((old_flgs.has(TR_XTRA_SHOTS)) && new_flgs.has_not(TR_XTRA_SHOTS)) {
        drain_value[TR_DEX] += 10;
    }
    if (old_name2 == EGO_2WEAPON) {
        drain_value[TR_DEX] += 20;
    }
    if (o_ptr->is_weapon_ammo()) {
        if (old_ds > o_ptr->ds)
            drain_value[TR_ES_ATTACK] += (old_ds - o_ptr->ds) * 10;

        if (old_dd > o_ptr->dd)
            drain_value[TR_ES_ATTACK] += (old_dd - o_ptr->dd) * 10;
    }
    if (old_to_h > o_ptr->to_h)
        drain_value[TR_ES_ATTACK] += (old_to_h - o_ptr->to_h) * 10;
    if (old_to_d > o_ptr->to_d)
        drain_value[TR_ES_ATTACK] += (old_to_d - o_ptr->to_d) * 10;
    if (old_ac > o_ptr->ac)
        drain_value[TR_ES_AC] += (old_ac - o_ptr->ac) * 10;
    if (old_to_a > o_ptr->to_a)
        drain_value[TR_ES_AC] += (old_to_a - o_ptr->to_a) * 10;

    for (i = 0; i < sizeof(drain_value) / sizeof(int); i++) {
        drain_value[i] *= number;
        drain_value[i] = drain_value[i] * dec / 4;
        drain_value[i] = MAX(drain_value[i], 0);
        if ((o_ptr->tval >= TV_SHOT) && (o_ptr->tval <= TV_BOLT))
            drain_value[i] /= 10;
        if (drain_value[i]) {
            observe = true;
        }
    }
    if (!observe) {
        msg_print(_("エッセンスは抽出できませんでした。", "You were not able to extract any essence."));
    } else {
        msg_print(_("抽出したエッセンス:", "Extracted essences:"));

        for (i = 0; essence_name[i]; i++) {
            if (!essence_name[i][0])
                continue;
            if (!drain_value[i])
                continue;

            creature_ptr->magic_num1[i] += drain_value[i];
            creature_ptr->magic_num1[i] = MIN(20000, creature_ptr->magic_num1[i]);
            msg_print(nullptr);
            msg_format("%s...%d%s", essence_name[i], drain_value[i], _("。", ". "));
        }
    }

    /* Apply autodestroy/inscription to the drained item */
    autopick_alter_item(creature_ptr, item, true);
    creature_ptr->update |= (PU_COMBINE | PU_REORDER);
    creature_ptr->window_flags |= (PW_INVEN);
}

/*!
 * @brief 付加するエッセンスの大別を選択する
 * @return 選んだエッセンスの大別ID
 */
static COMMAND_CODE choose_essence(void)
{
    COMMAND_CODE mode = 0;
    char choice;
    COMMAND_CODE menu_line = (use_menu ? 1 : 0);

#ifdef JP
    concptr menu_name[] = { "武器属性", "耐性", "能力", "数値", "スレイ", "ESP", "その他" };
#else
    concptr menu_name[] = { "Brand weapon", "Resistance", "Ability", "Magic number", "Slay", "ESP", "Others" };
#endif
    const COMMAND_CODE mode_max = 7;

    if (repeat_pull(&mode) && 1 <= mode && mode <= mode_max)
        return mode;
    mode = 0;
    if (use_menu) {
        screen_save();

        while (!mode) {
            int i;
            for (i = 0; i < mode_max; i++)
#ifdef JP
                prt(format(" %s %s", (menu_line == 1 + i) ? "》" : "  ", menu_name[i]), 2 + i, 14);
            prt("どの種類のエッセンス付加を行いますか？", 0, 0);
#else
                prt(format(" %s %s", (menu_line == 1 + i) ? "> " : "  ", menu_name[i]), 2 + i, 14);
            prt("Choose from menu.", 0, 0);
#endif

            choice = inkey();
            switch (choice) {
            case ESCAPE:
            case 'z':
            case 'Z':
                screen_load();
                return 0;
            case '2':
            case 'j':
            case 'J':
                menu_line++;
                break;
            case '8':
            case 'k':
            case 'K':
                menu_line += mode_max - 1;
                break;
            case '\r':
            case '\n':
            case 'x':
            case 'X':
                mode = menu_line;
                break;
            }
            if (menu_line > mode_max)
                menu_line -= mode_max;
        }
        screen_load();
    } else {
        screen_save();
        while (!mode) {
            int i;

            for (i = 0; i < mode_max; i++)
                prt(format("  %c) %s", 'a' + i, menu_name[i]), 2 + i, 14);

            if (!get_com(_("何を付加しますか:", "Command :"), &choice, true)) {
                screen_load();
                return 0;
            }

            if (isupper(choice))
                choice = (char)tolower(choice);

            if ('a' <= choice && choice <= 'a' + (char)mode_max - 1)
                mode = (int)choice - 'a' + 1;
        }
        screen_load();
    }

    repeat_push(mode);
    return mode;
}

/*!
 * @brief エッセンスを実際に付加する
 * @param mode エッセンスの大別ID
 */
static void add_essence(player_type *creature_ptr, int32_t mode)
{
    OBJECT_IDX item;
    int max_num = 0;
    COMMAND_CODE i;
    bool flag, redraw;
    char choice;
    concptr q, s;
    object_type *o_ptr;
    int ask = true;
    char out_val[160];
    int num[22];
    GAME_TEXT o_name[MAX_NLEN];
    int use_essence;
    essence_type *es_ptr;
    bool able[22] = { 0 };
    int menu_line = (use_menu ? 1 : 0);

    for (i = 0; essence_info[i].add_name; i++) {
        es_ptr = &essence_info[i];

        if (es_ptr->type != mode)
            continue;
        num[max_num++] = i;
    }

    if (!repeat_pull(&i) || i < 0 || i >= max_num) {
        flag = false;
        redraw = false;

        (void)strnfmt(out_val, 78, _("('*'で一覧, ESCで中断) どの能力を付加しますか？", "(*=List, ESC=exit) Add which ability? "));
        if (use_menu)
            screen_save();

        choice = (always_show_list || use_menu) ? ESCAPE : 1;
        while (!flag) {
            if (choice == ESCAPE)
                choice = ' ';
            else if (!get_com(out_val, &choice, false))
                break;

            if (use_menu && choice != ' ') {
                switch (choice) {
                case '0': {
                    screen_load();
                    return;
                }

                case '8':
                case 'k':
                case 'K': {
                    menu_line += (max_num - 1);
                    break;
                }

                case '2':
                case 'j':
                case 'J': {
                    menu_line++;
                    break;
                }

                case '4':
                case 'h':
                case 'H': {
                    menu_line = 1;
                    break;
                }
                case '6':
                case 'l':
                case 'L': {
                    menu_line = max_num;
                    break;
                }

                case 'x':
                case 'X':
                case '\r':
                case '\n': {
                    i = menu_line - 1;
                    ask = false;
                    break;
                }
                }
                if (menu_line > max_num)
                    menu_line -= max_num;
            }
            /* Request redraw */
            if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask)) {
                /* Show the list */
                if (!redraw || use_menu) {
                    byte y, x = 10;
                    int ctr;
                    char dummy[80], dummy2[160];
                    byte col;

                    strcpy(dummy, "");
                    redraw = true;
                    if (!use_menu)
                        screen_save();

                    for (y = 1; y < 24; y++)
                        prt("", y, x);

                        /* Print header(s) */
#ifdef JP
                    prt(format("   %-45s %6s/%s", "能力(必要エッセンス)", "所持数", "必要数"), 1, x);

#else
                    prt(format("   %-44s %7s/%s", "Ability (needed essence)", "Possess", "Needs"), 1, x);
#endif
                    /* Print list */
                    for (ctr = 0; ctr < max_num; ctr++) {
                        es_ptr = &essence_info[num[ctr]];

                        if (use_menu) {
                            if (ctr == (menu_line - 1))
                                strcpy(dummy, _("》 ", ">  "));
                            else
                                strcpy(dummy, "   ");

                        }
                        /* letter/number for power selection */
                        else {
                            sprintf(dummy, "%c) ", I2A(ctr));
                        }

                        strcat(dummy, es_ptr->add_name);

                        col = TERM_WHITE;
                        able[ctr] = true;

                        if (es_ptr->essence != -1) {
                            strcat(dummy, format("(%s)", essence_name[es_ptr->essence]));
                            if (creature_ptr->magic_num1[es_ptr->essence] < es_ptr->value)
                                able[ctr] = false;
                        } else {
                            switch (es_ptr->add) {
                            case ESSENCE_SH_FIRE:
                                strcat(dummy, _("(焼棄+耐火炎)", "(brand fire + res.fire)"));
                                if (creature_ptr->magic_num1[TR_BRAND_FIRE] < es_ptr->value)
                                    able[ctr] = false;
                                if (creature_ptr->magic_num1[TR_RES_FIRE] < es_ptr->value)
                                    able[ctr] = false;
                                break;
                            case ESSENCE_SH_ELEC:
                                strcat(dummy, _("(電撃+耐電撃)", "(brand elec. + res. elec.)"));
                                if (creature_ptr->magic_num1[TR_BRAND_ELEC] < es_ptr->value)
                                    able[ctr] = false;
                                if (creature_ptr->magic_num1[TR_RES_ELEC] < es_ptr->value)
                                    able[ctr] = false;
                                break;
                            case ESSENCE_SH_COLD:
                                strcat(dummy, _("(凍結+耐冷気)", "(brand cold + res. cold)"));
                                if (creature_ptr->magic_num1[TR_BRAND_COLD] < es_ptr->value)
                                    able[ctr] = false;
                                if (creature_ptr->magic_num1[TR_RES_COLD] < es_ptr->value)
                                    able[ctr] = false;
                                break;
                            case ESSENCE_RESISTANCE:
                                strcat(dummy, _("(耐火炎+耐冷気+耐電撃+耐酸)", "(r.fire+r.cold+r.elec+r.acid)"));
                                if (creature_ptr->magic_num1[TR_RES_FIRE] < es_ptr->value)
                                    able[ctr] = false;
                                if (creature_ptr->magic_num1[TR_RES_COLD] < es_ptr->value)
                                    able[ctr] = false;
                                if (creature_ptr->magic_num1[TR_RES_ELEC] < es_ptr->value)
                                    able[ctr] = false;
                                if (creature_ptr->magic_num1[TR_RES_ACID] < es_ptr->value)
                                    able[ctr] = false;
                                break;
                            case ESSENCE_SUSTAIN:
                                strcat(dummy, _("(耐火炎+耐冷気+耐電撃+耐酸)", "(r.fire+r.cold+r.elec+r.acid)"));
                                if (creature_ptr->magic_num1[TR_RES_FIRE] < es_ptr->value)
                                    able[ctr] = false;
                                if (creature_ptr->magic_num1[TR_RES_COLD] < es_ptr->value)
                                    able[ctr] = false;
                                if (creature_ptr->magic_num1[TR_RES_ELEC] < es_ptr->value)
                                    able[ctr] = false;
                                if (creature_ptr->magic_num1[TR_RES_ACID] < es_ptr->value)
                                    able[ctr] = false;
                                break;
                            }
                        }

                        if (!able[ctr])
                            col = TERM_RED;

                        if (es_ptr->essence != -1) {
                            sprintf(dummy2, "%-49s %5d/%d", dummy, (int)creature_ptr->magic_num1[es_ptr->essence], es_ptr->value);
                        } else {
                            sprintf(dummy2, "%-49s  (\?\?)/%d", dummy, es_ptr->value);
                        }

                        c_prt(col, dummy2, ctr + 2, x);
                    }
                }

                /* Hide the list */
                else {
                    /* Hide list */
                    redraw = false;
                    screen_load();
                }

                /* Redo asking */
                continue;
            }

            if (!use_menu) {
                /* Note verify */
                ask = (isupper(choice));

                /* Lowercase */
                if (ask)
                    choice = (char)tolower(choice);

                /* Extract request */
                i = (islower(choice) ? A2I(choice) : -1);
            }

            /* Totally Illegal */
            if ((i < 0) || (i >= max_num) || !able[i]) {
                bell();
                continue;
            }

            /* Verify it */
            if (ask) {
                char tmp_val[160];

                /* Prompt */
                (void)strnfmt(tmp_val, 78, _("%sを付加しますか？ ", "Add the ability of %s? "), essence_info[num[i]].add_name);

                /* Belay that order */
                if (!get_check(tmp_val))
                    continue;
            }

            /* Stop the loop */
            flag = true;
        }
        if (redraw)
            screen_load();

        if (!flag)
            return;

        repeat_push(i);
    }
    es_ptr = &essence_info[num[i]];

    auto decide_item_tester = [es_ptr, mode]() -> std::unique_ptr<ItemTester> {
        if (es_ptr->add == ESSENCE_SLAY_GLOVE)
            return std::make_unique<TvalItemTester>(TV_GLOVES);
        else if (mode == 1 || mode == 5)
            return std::make_unique<FuncItemTester>(&object_type::is_melee_ammo);
        else if (es_ptr->add == ESSENCE_ATTACK)
            return std::make_unique<FuncItemTester>(&object_type::allow_enchant_weapon);
        else if (es_ptr->add == ESSENCE_AC)
            return std::make_unique<FuncItemTester>(&object_type::is_armour);
        else
            return std::make_unique<FuncItemTester>(&object_type::is_weapon_armour_ammo);
    };

    auto item_tester = decide_item_tester();

    q = _("どのアイテムを改良しますか？", "Improve which item? ");
    s = _("改良できるアイテムがありません。", "You have nothing to improve.");

    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), *item_tester);
    if (!o_ptr)
        return;

    if ((mode != 10) && (o_ptr->is_artifact() || o_ptr->is_smith())) {
        msg_print(_("そのアイテムはこれ以上改良できない。", "This item can not be improved any further."));
        return;
    }

    describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    use_essence = es_ptr->value;
    if ((o_ptr->tval >= TV_SHOT) && (o_ptr->tval <= TV_BOLT))
        use_essence = (use_essence + 9) / 10;
    if (o_ptr->number > 1) {
        use_essence *= o_ptr->number;
        msg_format(_("%d個あるのでエッセンスは%d必要です。", "For %d items, it will take %d essences."), o_ptr->number, use_essence);
    }

    if (es_ptr->essence != -1) {
        if (creature_ptr->magic_num1[es_ptr->essence] < use_essence) {
            msg_print(_("エッセンスが足りない。", "You don't have enough essences."));
            return;
        }
        if (es_ptr->add < TR_FLAG_MAX && TR_PVAL_FLAG_MASK.has(static_cast<tr_type>(es_ptr->add))) {
            if (o_ptr->pval < 0) {
                msg_print(_("このアイテムの能力修正を強化することはできない。", "You cannot increase magic number of this item."));
                return;
            } else if (es_ptr->add == TR_BLOWS) {
                if (o_ptr->pval > 1) {
                    if (!get_check(_("修正値は1になります。よろしいですか？", "The magic number of this weapon will become 1. Are you sure? ")))
                        return;
                }

                o_ptr->pval = 1;
                msg_format(_("エッセンスを%d個使用します。", "It will take %d essences."), use_essence);
            } else if (o_ptr->pval > 0) {
                use_essence *= o_ptr->pval;
                msg_format(_("エッセンスを%d個使用します。", "It will take %d essences."), use_essence);
            } else {
                char tmp[80];
                char tmp_val[160];
                PARAMETER_VALUE pval;
                PARAMETER_VALUE limit = MIN(5, creature_ptr->magic_num1[es_ptr->essence] / es_ptr->value);

                sprintf(tmp, _("いくつ付加しますか？ (1-%d): ", "Enchant how many? (1-%d): "), limit);
                strcpy(tmp_val, "1");

                if (!get_string(tmp, tmp_val, 1))
                    return;
                pval = (PARAMETER_VALUE)atoi(tmp_val);
                if (pval > limit)
                    pval = limit;
                else if (pval < 1)
                    pval = 1;
                o_ptr->pval += pval;
                use_essence *= pval;
                msg_format(_("エッセンスを%d個使用します。", "It will take %d essences."), use_essence);
            }

            if (creature_ptr->magic_num1[es_ptr->essence] < use_essence) {
                msg_print(_("エッセンスが足りない。", "You don't have enough essences."));
                return;
            }
        } else if (es_ptr->add == ESSENCE_SLAY_GLOVE) {
            char tmp_val[160];
            int val;
            HIT_PROB get_to_h;
            HIT_POINT get_to_d;

            strcpy(tmp_val, "1");
            if (!get_string(format(_("いくつ付加しますか？ (1-%d):", "Enchant how many? (1-%d):"), creature_ptr->lev / 7 + 3), tmp_val, 2))
                return;
            val = atoi(tmp_val);
            if (val > creature_ptr->lev / 7 + 3)
                val = creature_ptr->lev / 7 + 3;
            else if (val < 1)
                val = 1;
            use_essence *= val;
            msg_format(_("エッセンスを%d個使用します。", "It will take %d essences."), use_essence);
            if (creature_ptr->magic_num1[es_ptr->essence] < use_essence) {
                msg_print(_("エッセンスが足りない。", "You don't have enough essences."));
                return;
            }
            get_to_h = ((val + 1) / 2 + randint0(val / 2 + 1));
            get_to_d = ((val + 1) / 2 + randint0(val / 2 + 1));
            o_ptr->xtra4 = (get_to_h << 8) + get_to_d;
            o_ptr->to_h += get_to_h;
            o_ptr->to_d += get_to_d;
        }
        creature_ptr->magic_num1[es_ptr->essence] -= use_essence;
        PlayerEnergy energy(creature_ptr);
        if (es_ptr->add == ESSENCE_ATTACK) {
            if ((o_ptr->to_h >= creature_ptr->lev / 5 + 5) && (o_ptr->to_d >= creature_ptr->lev / 5 + 5)) {
                msg_print(_("改良に失敗した。", "You failed to enchant."));
                energy.set_player_turn_energy(100);
                return;
            } else {
                if (o_ptr->to_h < creature_ptr->lev / 5 + 5)
                    o_ptr->to_h++;
                if (o_ptr->to_d < creature_ptr->lev / 5 + 5)
                    o_ptr->to_d++;
            }
        } else if (es_ptr->add == ESSENCE_AC) {
            if (o_ptr->to_a >= creature_ptr->lev / 5 + 5) {
                msg_print(_("改良に失敗した。", "You failed to enchant."));
                energy.set_player_turn_energy(100);
                return;
            } else {
                if (o_ptr->to_a < creature_ptr->lev / 5 + 5)
                    o_ptr->to_a++;
            }
        } else {
            o_ptr->xtra3 = es_ptr->add + 1;
        }
    } else {
        bool success = true;

        switch (es_ptr->add) {
        case ESSENCE_SH_FIRE:
            if ((creature_ptr->magic_num1[TR_BRAND_FIRE] < use_essence) || (creature_ptr->magic_num1[TR_RES_FIRE] < use_essence)) {
                success = false;
                break;
            }
            creature_ptr->magic_num1[TR_BRAND_FIRE] -= use_essence;
            creature_ptr->magic_num1[TR_RES_FIRE] -= use_essence;
            break;
        case ESSENCE_SH_ELEC:
            if ((creature_ptr->magic_num1[TR_BRAND_ELEC] < use_essence) || (creature_ptr->magic_num1[TR_RES_ELEC] < use_essence)) {
                success = false;
                break;
            }
            creature_ptr->magic_num1[TR_BRAND_ELEC] -= use_essence;
            creature_ptr->magic_num1[TR_RES_ELEC] -= use_essence;
            break;
        case ESSENCE_SH_COLD:
            if ((creature_ptr->magic_num1[TR_BRAND_COLD] < use_essence) || (creature_ptr->magic_num1[TR_RES_COLD] < use_essence)) {
                success = false;
                break;
            }
            creature_ptr->magic_num1[TR_BRAND_COLD] -= use_essence;
            creature_ptr->magic_num1[TR_RES_COLD] -= use_essence;
            break;
        case ESSENCE_RESISTANCE:
        case ESSENCE_SUSTAIN:
            if ((creature_ptr->magic_num1[TR_RES_ACID] < use_essence) || (creature_ptr->magic_num1[TR_RES_ELEC] < use_essence)
                || (creature_ptr->magic_num1[TR_RES_FIRE] < use_essence) || (creature_ptr->magic_num1[TR_RES_COLD] < use_essence)) {
                success = false;
                break;
            }
            creature_ptr->magic_num1[TR_RES_ACID] -= use_essence;
            creature_ptr->magic_num1[TR_RES_ELEC] -= use_essence;
            creature_ptr->magic_num1[TR_RES_FIRE] -= use_essence;
            creature_ptr->magic_num1[TR_RES_COLD] -= use_essence;
            break;
        }
        if (!success) {
            msg_print(_("エッセンスが足りない。", "You don't have enough essences."));
            return;
        }
        if (es_ptr->add == ESSENCE_SUSTAIN) {
            o_ptr->art_flags.set(TR_IGNORE_ACID);
            o_ptr->art_flags.set(TR_IGNORE_ELEC);
            o_ptr->art_flags.set(TR_IGNORE_FIRE);
            o_ptr->art_flags.set(TR_IGNORE_COLD);
        } else {
            o_ptr->xtra3 = es_ptr->add + 1;
        }
    }

    PlayerEnergy(creature_ptr).set_player_turn_energy(100);
    _(msg_format("%sに%sの能力を付加しました。", o_name, es_ptr->add_name), msg_format("You have added ability of %s to %s.", es_ptr->add_name, o_name));
    creature_ptr->update |= (PU_COMBINE | PU_REORDER);
    creature_ptr->window_flags |= (PW_INVEN);
}

/*!
 * @brief エッセンスを消去する
 */
static void erase_essence(player_type *creature_ptr)
{
    OBJECT_IDX item;
    concptr q, s;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];

    q = _("どのアイテムのエッセンスを消去しますか？", "Remove from which item? ");
    s = _("エッセンスを付加したアイテムがありません。", "You have nothing with added essence to remove.");

    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(&object_type::is_smith));
    if (!o_ptr)
        return;

    describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    if (!get_check(format(_("よろしいですか？ [%s]", "Are you sure? [%s]"), o_name)))
        return;

    PlayerEnergy(creature_ptr).set_player_turn_energy(100);

    if (o_ptr->xtra3 == 1 + ESSENCE_SLAY_GLOVE) {
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
    msg_print(_("エッセンスを取り去った。", "You removed all essence you have added."));
    creature_ptr->update |= (PU_COMBINE | PU_REORDER);
    creature_ptr->window_flags |= (PW_INVEN);
}

/*!
 * @brief 鍛冶コマンドのメインルーチン
 * @param only_browse TRUEならばエッセンス一覧の表示のみを行う
 */
void do_cmd_kaji(player_type *creature_ptr, bool only_browse)
{
    COMMAND_CODE mode = 0;
    char choice;

    COMMAND_CODE menu_line = (use_menu ? 1 : 0);

    if (!only_browse) {
        if (cmd_limit_confused(creature_ptr))
            return;
        if (cmd_limit_blind(creature_ptr))
            return;
        if (cmd_limit_image(creature_ptr))
            return;
    }

    if (!(repeat_pull(&mode) && 1 <= mode && mode <= 5)) {
        if (only_browse)
            screen_save();
        do {
            if (!only_browse)
                screen_save();
            if (use_menu) {
                while (!mode) {
#ifdef JP
                    prt(format(" %s エッセンス一覧", (menu_line == 1) ? "》" : "  "), 2, 14);
                    prt(format(" %s エッセンス抽出", (menu_line == 2) ? "》" : "  "), 3, 14);
                    prt(format(" %s エッセンス消去", (menu_line == 3) ? "》" : "  "), 4, 14);
                    prt(format(" %s エッセンス付加", (menu_line == 4) ? "》" : "  "), 5, 14);
                    prt(format(" %s 武器/防具強化", (menu_line == 5) ? "》" : "  "), 6, 14);
                    prt(format("どの種類の技術を%sますか？", only_browse ? "調べ" : "使い"), 0, 0);
#else
                    prt(format(" %s List essences", (menu_line == 1) ? "> " : "  "), 2, 14);
                    prt(format(" %s Extract essence", (menu_line == 2) ? "> " : "  "), 3, 14);
                    prt(format(" %s Remove essence", (menu_line == 3) ? "> " : "  "), 4, 14);
                    prt(format(" %s Add essence", (menu_line == 4) ? "> " : "  "), 5, 14);
                    prt(format(" %s Enchant weapon/armor", (menu_line == 5) ? "> " : "  "), 6, 14);
                    prt(format("Choose command from menu."), 0, 0);
#endif
                    choice = inkey();
                    switch (choice) {
                    case ESCAPE:
                    case 'z':
                    case 'Z':
                        screen_load();
                        return;
                    case '2':
                    case 'j':
                    case 'J':
                        menu_line++;
                        break;
                    case '8':
                    case 'k':
                    case 'K':
                        menu_line += 4;
                        break;
                    case '\r':
                    case '\n':
                    case 'x':
                    case 'X':
                        mode = menu_line;
                        break;
                    }
                    if (menu_line > 5)
                        menu_line -= 5;
                }
            }

            else {
                while (!mode) {
                    prt(_("  a) エッセンス一覧", "  a) List essences"), 2, 14);
                    prt(_("  b) エッセンス抽出", "  b) Extract essence"), 3, 14);
                    prt(_("  c) エッセンス消去", "  c) Remove essence"), 4, 14);
                    prt(_("  d) エッセンス付加", "  d) Add essence"), 5, 14);
                    prt(_("  e) 武器/防具強化", "  e) Enchant weapon/armor"), 6, 14);
#ifdef JP
                    if (!get_com(format("どの能力を%sますか:", only_browse ? "調べ" : "使い"), &choice, true))
#else
                    if (!get_com("Command :", &choice, true))
#endif
                    {
                        screen_load();
                        return;
                    }
                    switch (choice) {
                    case 'A':
                    case 'a':
                        mode = 1;
                        break;
                    case 'B':
                    case 'b':
                        mode = 2;
                        break;
                    case 'C':
                    case 'c':
                        mode = 3;
                        break;
                    case 'D':
                    case 'd':
                        mode = 4;
                        break;
                    case 'E':
                    case 'e':
                        mode = 5;
                        break;
                    }
                }
            }

            if (only_browse) {
                char temp[62 * 5];
                int line, j;

                /* Clear lines, position cursor  (really should use strlen here) */
                term_erase(14, 21, 255);
                term_erase(14, 20, 255);
                term_erase(14, 19, 255);
                term_erase(14, 18, 255);
                term_erase(14, 17, 255);
                term_erase(14, 16, 255);

                shape_buffer(kaji_tips[mode - 1], 62, temp, sizeof(temp));
                for (j = 0, line = 17; temp[j]; j += (1 + strlen(&temp[j]))) {
                    prt(&temp[j], line, 15);
                    line++;
                }
                mode = 0;
            }
            if (!only_browse)
                screen_load();
        } while (only_browse);
        repeat_push(mode);
    }
    switch (mode) {
    case 1:
        display_essence(creature_ptr);
        break;
    case 2:
        drain_essence(creature_ptr);
        break;
    case 3:
        erase_essence(creature_ptr);
        break;
    case 4:
        mode = choose_essence();
        if (mode == 0)
            break;
        add_essence(creature_ptr, mode);
        break;
    case 5:
        add_essence(creature_ptr, 10);
        break;
    }
}
