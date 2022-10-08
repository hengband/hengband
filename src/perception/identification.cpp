#include "perception/identification.h"
#include "artifact/fixed-art-types.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/special-options.h"
#include "io/input-key-acceptor.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-indice-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info-definition.h"
#include "system/monster-race-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/buffer-shaper.h"
#include "util/enum-converter.h"

/*!
 * @brief オブジェクトの*鑑定*内容を詳述して表示する /
 * Describe a "fully identified" item
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr *鑑定*情報を取得する元のオブジェクト構造体参照ポインタ
 * @param mode 表示オプション
 * @return 特筆すべき情報が一つでもあった場合TRUE、一つもなく表示がキャンセルされた場合FALSEを返す。
 */
bool screen_object(PlayerType *player_ptr, ObjectType *o_ptr, BIT_FLAGS mode)
{
    char temp[70 * 20];
    concptr info[128];
    GAME_TEXT o_name[MAX_NLEN];
    char desc[256];

    int trivial_info = 0;
    auto flgs = object_flags(o_ptr);

    const auto item_text = o_ptr->is_fixed_artifact() ? a_info.at(o_ptr->fixed_artifact_idx).text.c_str() : k_info[o_ptr->k_idx].text.c_str();
    shape_buffer(item_text, 77 - 15, temp, sizeof(temp));

    int i = 0;
    for (int j = 0; temp[j]; j += 1 + strlen(&temp[j])) {
        info[i] = &temp[j];
        i++;
    }

    if (o_ptr->is_equipment()) {
        trivial_info = i;
    }

    if (flgs.has(TR_ACTIVATE)) {
        info[i++] = _("始動したときの効果...", "It can be activated for...");
        info[i++] = activation_explanation(o_ptr);
        info[i++] = _("...ただし装備していなければならない。", "...if it is being worn.");
    }

    if (o_ptr->tval == ItemKindType::FIGURINE) {
        info[i++] = _("それは投げた時ペットに変化する。", "It will transform into a pet when thrown.");
    }

    if (o_ptr->fixed_artifact_idx == FixedArtifactId::STONEMASK) {
        info[i++] = _("それを装備した者は吸血鬼になる。", "It makes you turn into a vampire permanently.");
    }

    if ((o_ptr->tval == ItemKindType::SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) {
        info[i++] = _("それは相手を一撃で倒すことがある。", "It will attempt to instantly kill a monster.");
    }

    if ((o_ptr->tval == ItemKindType::POLEARM) && (o_ptr->sval == SV_DEATH_SCYTHE)) {
        info[i++] = _("それは自分自身に攻撃が返ってくることがある。", "It causes you to strike yourself sometimes.");
        info[i++] = _("それは無敵のバリアを切り裂く。", "It always penetrates invulnerability barriers.");
    }

    if (flgs.has(TR_EASY2_WEAPON)) {
        info[i++] = _("それは二刀流での命中率を向上させる。", "It affects your ability to hit when you are wielding two weapons.");
    }

    if (flgs.has(TR_INVULN_ARROW)) {
        info[i++] = _("それは視界がある限り物理的な飛び道具の一切をはねのける。", "It repels all physical missiles as long as there is visibility.");
    }

    if (flgs.has(TR_NO_AC)) {
        info[i++] = _("それは物理的防護の一切を奪う。", "It robs you of any physical protection.");
    }

    if (flgs.has(TR_EASY_SPELL)) {
        info[i++] = _("それは魔法の難易度を下げる。", "It affects your ability to cast spells.");
    }

    if (flgs.has(TR_HEAVY_SPELL)) {
        info[i++] = _("それは魔法の難易度を上げる。", "It interferes with casting spells.");
    }

    if (flgs.has(TR_MIGHTY_THROW)) {
        info[i++] = _("それは物を強く投げることを可能にする。", "It provides great strength when you throw an item.");
    }

    if (o_ptr->tval == ItemKindType::STATUE) {
        auto statue_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
        auto *r_ptr = &r_info[statue_r_idx];
        if (statue_r_idx == MonsterRaceId::BULLGATES) {
            info[i++] = _("それは部屋に飾ると恥ずかしい。", "It is shameful.");
        } else if (r_ptr->flags2 & (RF2_ELDRITCH_HORROR)) {
            info[i++] = _("それは部屋に飾ると恐い。", "It is fearful.");
        } else {
            info[i++] = _("それは部屋に飾ると楽しい。", "It is cheerful.");
        }
    }

    if (flgs.has(TR_DARK_SOURCE)) {
        info[i++] = _("それは全く光らない。", "It provides no light.");
    }

    POSITION rad = 0;
    if (flgs.has(TR_LITE_1) && flgs.has_not(TR_DARK_SOURCE)) {
        rad += 1;
    }
    if (flgs.has(TR_LITE_2) && flgs.has_not(TR_DARK_SOURCE)) {
        rad += 2;
    }
    if (flgs.has(TR_LITE_3) && flgs.has_not(TR_DARK_SOURCE)) {
        rad += 3;
    }
    if (flgs.has(TR_LITE_M1)) {
        rad -= 1;
    }
    if (flgs.has(TR_LITE_M2)) {
        rad -= 2;
    }
    if (flgs.has(TR_LITE_M3)) {
        rad -= 3;
    }

    if (o_ptr->ego_idx == EgoType::LITE_SHINE) {
        rad++;
    }

    if (flgs.has(TR_LITE_FUEL) && flgs.has_not(TR_DARK_SOURCE)) {
        if (rad > 0) {
            sprintf(desc, _("それは燃料補給によって明かり(半径 %d)を授ける。", "It provides light (radius %d) when fueled."), (int)rad);
        }
    } else {
        if (rad > 0) {
            sprintf(desc, _("それは永遠なる明かり(半径 %d)を授ける。", "It provides light (radius %d) forever."), (int)rad);
        }
        if (rad < 0) {
            sprintf(desc, _("それは明かりの半径を狭める(半径に-%d)。", "It decreases the radius of your light by %d."), (int)-rad);
        }
    }

    if (rad != 0) {
        info[i++] = desc;
    }

    if (o_ptr->ego_idx == EgoType::LITE_LONG) {
        info[i++] = _("それは長いターン明かりを授ける。", "It provides light for much longer time.");
    }

    if (flgs.has(TR_RIDING)) {
        if (o_ptr->is_lance()) {
            info[i++] = _("それは乗馬中は非常に使いやすい。", "It is made for use while riding.");
        } else {
            info[i++] = _("それは乗馬中でも使いやすい。", "It is suitable for use while riding.");
            trivial_info++;
        }
    }

    if (flgs.has(TR_SUPPORTIVE)) {
        info[i++] = _("それは武器の補助として扱いやすい。", "It is easy to treat it as assistance to weapon.");
    }

    if (flgs.has(TR_STR)) {
        info[i++] = _("それは腕力に影響を及ぼす。", "It affects your strength.");
    }

    if (flgs.has(TR_INT)) {
        info[i++] = _("それは知能に影響を及ぼす。", "It affects your intelligence.");
    }

    if (flgs.has(TR_WIS)) {
        info[i++] = _("それは賢さに影響を及ぼす。", "It affects your wisdom.");
    }

    if (flgs.has(TR_DEX)) {
        info[i++] = _("それは器用さに影響を及ぼす。", "It affects your dexterity.");
    }

    if (flgs.has(TR_CON)) {
        info[i++] = _("それは耐久力に影響を及ぼす。", "It affects your constitution.");
    }

    if (flgs.has(TR_CHR)) {
        info[i++] = _("それは魅力に影響を及ぼす。", "It affects your charisma.");
    }

    if (flgs.has(TR_MAGIC_MASTERY)) {
        info[i++] = _("それは魔法道具使用能力に影響を及ぼす。", "It affects your ability to use magic devices.");
    }

    if (flgs.has(TR_STEALTH)) {
        info[i++] = _("それは隠密行動能力に影響を及ぼす。", "It affects your stealth.");
    }

    if (flgs.has(TR_SEARCH)) {
        info[i++] = _("それは探索能力に影響を及ぼす。", "It affects your searching.");
    }

    if (flgs.has(TR_INFRA)) {
        info[i++] = _("それは赤外線視力に影響を及ぼす。", "It affects your infravision.");
    }

    if (flgs.has(TR_TUNNEL)) {
        info[i++] = _("それは採掘能力に影響を及ぼす。", "It affects your ability to tunnel.");
    }

    if (flgs.has(TR_SPEED)) {
        info[i++] = _("それはスピードに影響を及ぼす。", "It affects your speed.");
    }

    if (flgs.has(TR_BLOWS)) {
        info[i++] = _("それは打撃回数に影響を及ぼす。", "It affects your attack speed.");
    }

    if (flgs.has(TR_BRAND_ACID)) {
        info[i++] = _("それは酸によって大きなダメージを与える。", "It does extra damage from acid.");
    }

    if (flgs.has(TR_BRAND_ELEC)) {
        info[i++] = _("それは電撃によって大きなダメージを与える。", "It does extra damage from electricity.");
    }

    if (flgs.has(TR_BRAND_FIRE)) {
        info[i++] = _("それは火炎によって大きなダメージを与える。", "It does extra damage from fire.");
    }

    if (flgs.has(TR_BRAND_COLD)) {
        info[i++] = _("それは冷気によって大きなダメージを与える。", "It does extra damage from frost.");
    }

    if (flgs.has(TR_BRAND_POIS)) {
        info[i++] = _("それは敵を毒する。", "It poisons your foes.");
    }

    if (flgs.has(TR_CHAOTIC)) {
        info[i++] = _("それはカオス的な効果を及ぼす。", "It produces chaotic effects.");
    }

    if (flgs.has(TR_BRAND_MAGIC)) {
        info[i++] = _("それは魔術的な効果を及ぼす。", "It produces magical effects.");
    }

    if (flgs.has(TR_VAMPIRIC)) {
        info[i++] = _("それは敵から生命力を吸収する。", "It drains life from your foes.");
    }

    if (flgs.has(TR_EARTHQUAKE)) {
        info[i++] = _("それは地震を起こすことができる。", "It can cause earthquakes.");
    }

    if (flgs.has(TR_VORPAL)) {
        info[i++] = _("それは非常に切れ味が鋭く敵を切断することができる。", "It is very sharp and can cut your foes.");
    }

    if (flgs.has(TR_IMPACT)) {
        info[i++] = _("それは非常に強く敵を攻撃することができる。", "It can hit your foes strongly.");
    }

    if (flgs.has(TR_KILL_DRAGON)) {
        info[i++] = _("それはドラゴンにとっての天敵である。", "It is a great bane of dragons.");
    } else if (flgs.has(TR_SLAY_DRAGON)) {
        info[i++] = _("それはドラゴンに対して特に恐るべき力を発揮する。", "It is especially deadly against dragons.");
    }

    if (flgs.has(TR_KILL_ORC)) {
        info[i++] = _("それはオークにとっての天敵である。", "It is a great bane of orcs.");
    }

    if (flgs.has(TR_SLAY_ORC)) {
        info[i++] = _("それはオークに対して特に恐るべき力を発揮する。", "It is especially deadly against orcs.");
    }

    if (flgs.has(TR_KILL_TROLL)) {
        info[i++] = _("それはトロルにとっての天敵である。", "It is a great bane of trolls.");
    }

    if (flgs.has(TR_SLAY_TROLL)) {
        info[i++] = _("それはトロルに対して特に恐るべき力を発揮する。", "It is especially deadly against trolls.");
    }

    if (flgs.has(TR_KILL_GIANT)) {
        info[i++] = _("それは巨人にとっての天敵である。", "It is a great bane of giants.");
    } else if (flgs.has(TR_SLAY_GIANT)) {
        info[i++] = _("それは巨人に対して特に恐るべき力を発揮する。", "It is especially deadly against giants.");
    }

    if (flgs.has(TR_KILL_DEMON)) {
        info[i++] = _("それはデーモンにとっての天敵である。", "It is a great bane of demons.");
    }

    if (flgs.has(TR_SLAY_DEMON)) {
        info[i++] = _("それはデーモンに対して聖なる力を発揮する。", "It strikes at demons with holy wrath.");
    }

    if (flgs.has(TR_KILL_UNDEAD)) {
        info[i++] = _("それはアンデッドにとっての天敵である。", "It is a great bane of undead.");
    }

    if (flgs.has(TR_SLAY_UNDEAD)) {
        info[i++] = _("それはアンデッドに対して聖なる力を発揮する。", "It strikes at undead with holy wrath.");
    }

    if (flgs.has(TR_KILL_EVIL)) {
        info[i++] = _("それは邪悪なる存在にとっての天敵である。", "It is a great bane of evil monsters.");
    }

    if (flgs.has(TR_SLAY_EVIL)) {
        info[i++] = _("それは邪悪なる存在に対して聖なる力で攻撃する。", "It fights against evil with holy fury.");
    }

    if (flgs.has(TR_KILL_GOOD)) {
        info[i++] = _("それは善良なる存在にとっての天敵である。", "It is a great bane of good monsters.");
    }

    if (flgs.has(TR_SLAY_GOOD)) {
        info[i++] = _("それは善良なる存在に対して邪悪なる力で攻撃する。", "It fights against good with evil fury.");
    }

    if (flgs.has(TR_KILL_ANIMAL)) {
        info[i++] = _("それは自然界の動物にとっての天敵である。", "It is a great bane of natural creatures.");
    }

    if (flgs.has(TR_SLAY_ANIMAL)) {
        info[i++] = _("それは自然界の動物に対して特に恐るべき力を発揮する。", "It is especially deadly against natural creatures.");
    }

    if (flgs.has(TR_KILL_HUMAN)) {
        info[i++] = _("それは人間にとっての天敵である。", "It is a great bane of humans.");
    }

    if (flgs.has(TR_SLAY_HUMAN)) {
        info[i++] = _("それは人間に対して特に恐るべき力を発揮する。", "It is especially deadly against humans.");
    }

    if (flgs.has(TR_FORCE_WEAPON)) {
        info[i++] = _("それは使用者の魔力を使って攻撃する。", "It powerfully strikes at a monster using your mana.");
    }

    if (flgs.has(TR_DEC_MANA)) {
        info[i++] = _("それは魔力の消費を押さえる。", "It decreases your mana consumption.");
    }

    if (flgs.has(TR_SUST_STR)) {
        info[i++] = _("それはあなたの腕力を維持する。", "It sustains your strength.");
    }

    if (flgs.has(TR_SUST_INT)) {
        info[i++] = _("それはあなたの知能を維持する。", "It sustains your intelligence.");
    }

    if (flgs.has(TR_SUST_WIS)) {
        info[i++] = _("それはあなたの賢さを維持する。", "It sustains your wisdom.");
    }

    if (flgs.has(TR_SUST_DEX)) {
        info[i++] = _("それはあなたの器用さを維持する。", "It sustains your dexterity.");
    }

    if (flgs.has(TR_SUST_CON)) {
        info[i++] = _("それはあなたの耐久力を維持する。", "It sustains your constitution.");
    }

    if (flgs.has(TR_SUST_CHR)) {
        info[i++] = _("それはあなたの魅力を維持する。", "It sustains your charisma.");
    }

    if (flgs.has(TR_IM_ACID)) {
        info[i++] = _("それは酸に対する完全な免疫を授ける。", "It provides immunity to acid.");
    } else if (flgs.has(TR_VUL_ACID)) {
        info[i++] = _("それは酸に対する弱点を授ける。", "It provides vulnerability to acid.");
    }

    if (flgs.has(TR_IM_ELEC)) {
        info[i++] = _("それは電撃に対する完全な免疫を授ける。", "It provides immunity to electricity.");
    } else if (flgs.has(TR_VUL_ELEC)) {
        info[i++] = _("それは電撃に対する弱点を授ける。", "It provides vulnerability to electricity.");
    }

    if (flgs.has(TR_IM_FIRE)) {
        info[i++] = _("それは火に対する完全な免疫を授ける。", "It provides immunity to fire.");
    } else if (flgs.has(TR_VUL_FIRE)) {
        info[i++] = _("それは火に対する弱点を授ける。", "It provides vulnerability to fire.");
    }

    if (flgs.has(TR_IM_COLD)) {
        info[i++] = _("それは寒さに対する完全な免疫を授ける。", "It provides immunity to cold.");
    } else if (flgs.has(TR_VUL_COLD)) {
        info[i++] = _("それは寒さに対する弱点を授ける。", "It provides vulnerability to cold.");
    }

    if (flgs.has(TR_IM_DARK)) {
        info[i++] = _("それは暗黒に対する完全な免疫を授ける。", "It provides immunity to dark.");
    }

    if (flgs.has(TR_VUL_LITE)) {
        info[i++] = _("それは閃光に対する弱点を授ける。", "It provides vulnerability to cold.");
    }

    if (flgs.has(TR_THROW)) {
        info[i++] = _("それは敵に投げて大きなダメージを与えることができる。", "It is perfectly balanced for throwing.");
    }

    if (flgs.has(TR_FREE_ACT)) {
        info[i++] = _("それは麻痺に対する完全な免疫を授ける。", "It provides immunity to paralysis.");
    }

    if (flgs.has(TR_HOLD_EXP)) {
        info[i++] = _("それは経験値吸収に対する耐性を授ける。", "It provides resistance to experience draining.");
    }

    if (flgs.has(TR_RES_FEAR)) {
        info[i++] = _("それは恐怖への完全な耐性を授ける。", "It makes you completely fearless.");
    }

    if (flgs.has(TR_RES_ACID)) {
        info[i++] = _("それは酸への耐性を授ける。", "It provides resistance to acid.");
    }

    if (flgs.has(TR_RES_ELEC)) {
        info[i++] = _("それは電撃への耐性を授ける。", "It provides resistance to electricity.");
    }

    if (flgs.has(TR_RES_FIRE)) {
        info[i++] = _("それは火への耐性を授ける。", "It provides resistance to fire.");
    }

    if (flgs.has(TR_RES_COLD)) {
        info[i++] = _("それは寒さへの耐性を授ける。", "It provides resistance to cold.");
    }

    if (flgs.has(TR_RES_POIS)) {
        info[i++] = _("それは毒への耐性を授ける。", "It provides resistance to poison.");
    }

    if (flgs.has(TR_RES_LITE)) {
        info[i++] = _("それは閃光への耐性を授ける。", "It provides resistance to light.");
    }

    if (flgs.has(TR_RES_DARK)) {
        info[i++] = _("それは暗黒への耐性を授ける。", "It provides resistance to dark.");
    }

    if (flgs.has(TR_RES_BLIND)) {
        info[i++] = _("それは盲目への耐性を授ける。", "It provides resistance to blindness.");
    }

    if (flgs.has(TR_RES_CONF)) {
        info[i++] = _("それは混乱への耐性を授ける。", "It provides resistance to confusion.");
    }

    if (flgs.has(TR_RES_SOUND)) {
        info[i++] = _("それは轟音への耐性を授ける。", "It provides resistance to sound.");
    }

    if (flgs.has(TR_RES_SHARDS)) {
        info[i++] = _("それは破片への耐性を授ける。", "It provides resistance to shards.");
    }

    if (flgs.has(TR_RES_NETHER)) {
        info[i++] = _("それは地獄への耐性を授ける。", "It provides resistance to nether.");
    }

    if (flgs.has(TR_RES_NEXUS)) {
        info[i++] = _("それは因果混乱への耐性を授ける。", "It provides resistance to nexus.");
    }

    if (flgs.has(TR_RES_CHAOS)) {
        info[i++] = _("それはカオスへの耐性を授ける。", "It provides resistance to chaos.");
    }

    if (flgs.has(TR_RES_TIME)) {
        info[i++] = _("それは時間逆転への耐性を授ける。", "It provides resistance to time stream.");
    }

    if (flgs.has(TR_RES_WATER)) {
        info[i++] = _("それは水流への耐性を授ける。", "It provides resistance to water stream.");
    }

    if (flgs.has(TR_RES_DISEN)) {
        info[i++] = _("それは劣化への耐性を授ける。", "It provides resistance to disenchantment.");
    }

    if (flgs.has(TR_LEVITATION)) {
        info[i++] = _("それは宙に浮くことを可能にする。", "It allows you to levitate.");
    }

    if (flgs.has(TR_SEE_INVIS)) {
        info[i++] = _("それは透明なモンスターを見ることを可能にする。", "It allows you to see invisible monsters.");
    }

    if (flgs.has(TR_TELEPATHY)) {
        info[i++] = _("それはテレパシー能力を授ける。", "It gives telepathic powers.");
    }

    if (flgs.has(TR_ESP_ANIMAL)) {
        info[i++] = _("それは自然界の生物を感知する。", "It senses natural creatures.");
    }

    if (flgs.has(TR_ESP_UNDEAD)) {
        info[i++] = _("それはアンデッドを感知する。", "It senses undead.");
    }

    if (flgs.has(TR_ESP_DEMON)) {
        info[i++] = _("それは悪魔を感知する。", "It senses demons.");
    }

    if (flgs.has(TR_ESP_ORC)) {
        info[i++] = _("それはオークを感知する。", "It senses orcs.");
    }

    if (flgs.has(TR_ESP_TROLL)) {
        info[i++] = _("それはトロルを感知する。", "It senses trolls.");
    }

    if (flgs.has(TR_ESP_GIANT)) {
        info[i++] = _("それは巨人を感知する。", "It senses giants.");
    }

    if (flgs.has(TR_ESP_DRAGON)) {
        info[i++] = _("それはドラゴンを感知する。", "It senses dragons.");
    }

    if (flgs.has(TR_ESP_HUMAN)) {
        info[i++] = _("それは人間を感知する。", "It senses humans.");
    }

    if (flgs.has(TR_ESP_EVIL)) {
        info[i++] = _("それは邪悪な存在を感知する。", "It senses evil creatures.");
    }

    if (flgs.has(TR_ESP_GOOD)) {
        info[i++] = _("それは善良な存在を感知する。", "It senses good creatures.");
    }

    if (flgs.has(TR_ESP_NONLIVING)) {
        info[i++] = _("それは活動する無生物体を感知する。", "It senses non-living creatures.");
    }

    if (flgs.has(TR_ESP_UNIQUE)) {
        info[i++] = _("それは特別な強敵を感知する。", "It senses unique monsters.");
    }

    if (flgs.has(TR_SLOW_DIGEST)) {
        info[i++] = _("それはあなたの新陳代謝を遅くする。", "It slows your metabolism.");
    }

    if (flgs.has(TR_REGEN)) {
        info[i++] = _("それは体力回復力を強化する。", "It speeds your regenerative powers.");
    }

    if (flgs.has(TR_WARNING)) {
        info[i++] = _("それは危険に対して警告を発する。", "It warns you of danger");
    }

    if (flgs.has(TR_REFLECT)) {
        info[i++] = _("それは矢の呪文を反射する。", "It reflects bolt spells.");
    }

    if (flgs.has(TR_RES_CURSE)) {
        info[i++] = _("それは呪いへの抵抗力を高める。", "It increases your resistance to curses.");
    }

    if (flgs.has(TR_SH_FIRE)) {
        info[i++] = _("それは炎のバリアを張る。", "It produces a fiery sheath.");
    }

    if (flgs.has(TR_SH_ELEC)) {
        info[i++] = _("それは電気のバリアを張る。", "It produces an electric sheath.");
    }

    if (flgs.has(TR_SH_COLD)) {
        info[i++] = _("それは冷気のバリアを張る。", "It produces a sheath of coldness.");
    }

    if (flgs.has(TR_SELF_FIRE)) {
        info[i++] = _("それはあなたを燃やす。", "It burns you.");
    }

    if (flgs.has(TR_SELF_ELEC)) {
        info[i++] = _("それはあなたを電撃で包む。", "It electrocutes you.");
    }

    if (flgs.has(TR_SELF_COLD)) {
        info[i++] = _("それはあなたを凍らせる。", "It freezes you.");
    }

    if (flgs.has(TR_NO_MAGIC)) {
        info[i++] = _("それは反魔法バリアを張る。", "It produces an anti-magic shell.");
    }

    if (flgs.has(TR_NO_TELE)) {
        info[i++] = _("それはテレポートを邪魔する。", "It prevents teleportation.");
    }

    if (flgs.has(TR_XTRA_MIGHT)) {
        info[i++] = _("それは矢／ボルト／弾をより強力に発射することができる。", "It fires missiles with extra might.");
    }

    if (flgs.has(TR_XTRA_SHOTS)) {
        info[i++] = _("それは矢／ボルト／弾を非常に早く発射することができる。", "It fires missiles excessively fast.");
    }

    if (flgs.has(TR_BLESSED)) {
        info[i++] = _("それは神に祝福されている。", "It has been blessed by the gods.");
    }

    if (o_ptr->is_cursed()) {
        if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
            info[i++] = _("それは永遠の呪いがかけられている。", "It is permanently cursed.");
        } else if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
            info[i++] = _("それは強力な呪いがかけられている。", "It is heavily cursed.");
        } else {
            info[i++] = _("それは呪われている。", "It is cursed.");

            /*
             * It's a trivial infomation since there is
             * fake inscription {cursed}
             */
            trivial_info++;
        }
    }

    if ((flgs.has(TR_TY_CURSE)) || o_ptr->curse_flags.has(CurseTraitType::TY_CURSE)) {
        info[i++] = _("それは太古の禍々しい怨念が宿っている。", "It carries an ancient foul curse.");
    }

    if ((flgs.has(TR_AGGRAVATE)) || o_ptr->curse_flags.has(CurseTraitType::AGGRAVATE)) {
        info[i++] = _("それは付近のモンスターを怒らせる。", "It aggravates nearby creatures.");
    }

    if ((flgs.has(TR_DRAIN_EXP)) || o_ptr->curse_flags.has(CurseTraitType::DRAIN_EXP)) {
        info[i++] = _("それは経験値を吸い取る。", "It drains experience.");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::SLOW_REGEN)) {
        info[i++] = _("それは回復力を弱める。", "It slows your regenerative powers.");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::ADD_L_CURSE) || flgs.has(TR_ADD_L_CURSE)) {
        info[i++] = _("それは弱い呪いを増やす。", "It adds weak curses.");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::ADD_H_CURSE) || flgs.has(TR_ADD_H_CURSE)) {
        info[i++] = _("それは強力な呪いを増やす。", "It adds heavy curses.");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::PERSISTENT_CURSE) || flgs.has(TR_PERSISTENT_CURSE)) {
        info[i++] = _("それは頻繁に呪いをかけなおす。", "It curses itself persistently.");
    }

    if ((flgs.has(TR_CALL_ANIMAL)) || o_ptr->curse_flags.has(CurseTraitType::CALL_ANIMAL)) {
        info[i++] = _("それは動物を呼び寄せる。", "It attracts animals.");
    }

    if ((flgs.has(TR_CALL_DEMON)) || o_ptr->curse_flags.has(CurseTraitType::CALL_DEMON)) {
        info[i++] = _("それは悪魔を呼び寄せる。", "It attracts demons.");
    }

    if ((flgs.has(TR_CALL_DRAGON)) || o_ptr->curse_flags.has(CurseTraitType::CALL_DRAGON)) {
        info[i++] = _("それはドラゴンを呼び寄せる。", "It attracts dragons.");
    }

    if ((flgs.has(TR_CALL_UNDEAD)) || o_ptr->curse_flags.has(CurseTraitType::CALL_UNDEAD)) {
        info[i++] = _("それは死霊を呼び寄せる。", "It attracts undead.");
    }

    if ((flgs.has(TR_COWARDICE)) || o_ptr->curse_flags.has(CurseTraitType::COWARDICE)) {
        info[i++] = _("それは恐怖感を引き起こす。", "It makes you subject to cowardice.");
    }

    if (flgs.has(TR_BERS_RAGE) || o_ptr->curse_flags.has(CurseTraitType::BERS_RAGE)) {
        info[i++] = _("それは狂戦士化の発作を引き起こす。", "It makes you subject to berserker fits.");
    }

    if ((flgs.has(TR_TELEPORT)) || o_ptr->curse_flags.has(CurseTraitType::TELEPORT)) {
        info[i++] = _("それはランダムなテレポートを引き起こす。", "It induces random teleportation.");
    }

    if ((flgs.has(TR_LOW_MELEE)) || o_ptr->curse_flags.has(CurseTraitType::LOW_MELEE)) {
        info[i++] = _("それは攻撃を外しやすい。", "It causes you to miss blows.");
    }

    if ((flgs.has(TR_LOW_AC)) || o_ptr->curse_flags.has(CurseTraitType::LOW_AC)) {
        info[i++] = _("それは攻撃を受けやすい。", "It helps your enemies' blows.");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::VUL_CURSE) || flgs.has(TR_VUL_CURSE)) {
        info[i++] = _("それは呪いへの抵抗力を下げる。", "It decreases your resistance to curses.");
    }

    if (flgs.has(TR_DOWN_SAVING)) {
        info[i++] = _("それは魔法抵抗力を半減させる。", "It halves your magic resistance.");
    }

    if ((flgs.has(TR_HARD_SPELL)) || o_ptr->curse_flags.has(CurseTraitType::HARD_SPELL)) {
        info[i++] = _("それは魔法を唱えにくくする。", "It encumbers you while spellcasting.");
    }

    if ((flgs.has(TR_FAST_DIGEST)) || o_ptr->curse_flags.has(CurseTraitType::FAST_DIGEST)) {
        info[i++] = _("それはあなたの新陳代謝を速くする。", "It speeds your metabolism.");
    }

    if ((flgs.has(TR_DRAIN_HP)) || o_ptr->curse_flags.has(CurseTraitType::DRAIN_HP)) {
        info[i++] = _("それはあなたの体力を吸い取る。", "It drains you.");
    }

    if ((flgs.has(TR_DRAIN_MANA)) || o_ptr->curse_flags.has(CurseTraitType::DRAIN_MANA)) {
        info[i++] = _("それはあなたの魔力を吸い取る。", "It drains your mana.");
    }

    if (mode & SCROBJ_FAKE_OBJECT) {
        switch (o_ptr->tval) {
        case ItemKindType::RING:
            switch (o_ptr->sval) {
            case SV_RING_LORDLY:
                info[i++] = _("それは幾つかのランダムな耐性を授ける。", "It provides some random resistances.");
                break;
            case SV_RING_WARNING:
                info[i++] = _("それはひとつの低級なESPを授ける事がある。", "It may provide a low rank ESP.");
                break;
            }

            break;

        case ItemKindType::AMULET:
            switch (o_ptr->sval) {
            case SV_AMULET_RESISTANCE:
                info[i++] = _("それは毒への耐性を授ける事がある。", "It may provides resistance to poison.");
                info[i++] = _("それはランダムな耐性を授ける事がある。", "It may provide a random resistances.");
                break;
            case SV_AMULET_THE_MAGI:
                info[i++] = _("それは最大で３つまでの低級なESPを授ける。", "It provides up to three low rank ESPs.");
                break;
            }

            break;

        default:
            break;
        }
    }

    if (flgs.has(TR_IGNORE_ACID) && flgs.has(TR_IGNORE_ELEC) && flgs.has(TR_IGNORE_FIRE) && flgs.has(TR_IGNORE_COLD)) {
        info[i++] = _("それは酸・電撃・火炎・冷気では傷つかない。", "It cannot be harmed by the elements.");
    } else {
        if (flgs.has(TR_IGNORE_ACID)) {
            info[i++] = _("それは酸では傷つかない。", "It cannot be harmed by acid.");
        }

        if (flgs.has(TR_IGNORE_ELEC)) {
            info[i++] = _("それは電撃では傷つかない。", "It cannot be harmed by electricity.");
        }

        if (flgs.has(TR_IGNORE_FIRE)) {
            info[i++] = _("それは火炎では傷つかない。", "It cannot be harmed by fire.");
        }

        if (flgs.has(TR_IGNORE_COLD)) {
            info[i++] = _("それは冷気では傷つかない。", "It cannot be harmed by cold.");
        }
    }

    if (mode & SCROBJ_FORCE_DETAIL) {
        trivial_info = 0;
    }

    if (i <= trivial_info) {
        return false;
    }

    screen_save();
    int wid, hgt;
    term_get_size(&wid, &hgt);

    if (!(mode & SCROBJ_FAKE_OBJECT)) {
        describe_flavor(player_ptr, o_name, o_ptr, 0);
    } else {
        describe_flavor(player_ptr, o_name, o_ptr, (OD_NAME_ONLY | OD_STORE));
    }

    prt(o_name, 0, 0);
    for (int k = 1; k < hgt; k++) {
        prt("", k, 13);
    }

    if ((o_ptr->tval == ItemKindType::STATUE) && (o_ptr->sval == SV_PHOTO)) {
        auto statue_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
        auto *r_ptr = &r_info[statue_r_idx];
        int namelen = strlen(r_ptr->name.c_str());
        prt(format("%s: '", r_ptr->name.c_str()), 1, 15);
        term_queue_bigchar(18 + namelen, 1, r_ptr->x_attr, r_ptr->x_char, 0, 0);
        prt("'", 1, (use_bigtile ? 20 : 19) + namelen);
    } else {
        prt(_("     アイテムの能力:", "     Item Attributes:"), 1, 15);
    }

    int k = 2;
    for (int j = 0; j < i; j++) {
        prt(info[j], k++, 15);
        if ((k == hgt - 2) && (j + 1 < i)) {
            prt(_("-- 続く --", "-- more --"), k, 15);
            inkey();
            for (; k > 2; k--) {
                prt("", k, 15);
            }
        }
    }

    prt(_("[何かキーを押すとゲームに戻ります]", "[Press any key to continue]"), k, 15);
    inkey();
    screen_load();
    return true;
}
