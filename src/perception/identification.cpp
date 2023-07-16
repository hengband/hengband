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
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/buffer-shaper.h"
#include "util/enum-converter.h"
#include <algorithm>
#include <array>
#include <string>

/*!
 * @brief オブジェクトの*鑑定*内容を詳述して表示する /
 * Describe a "fully identified" item
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr *鑑定*情報を取得する元のオブジェクト構造体参照ポインタ
 * @param mode 表示オプション
 * @return 特筆すべき情報が一つでもあった場合TRUE、一つもなく表示がキャンセルされた場合FALSEを返す。
 */
bool screen_object(PlayerType *player_ptr, ItemEntity *o_ptr, BIT_FLAGS mode)
{
    std::array<std::string, 128> info{};
    int trivial_info = 0;
    auto flags = object_flags(o_ptr);

    const auto item_text = o_ptr->is_fixed_artifact() ? o_ptr->get_fixed_artifact().text.data() : o_ptr->get_baseitem().text.data();
    const auto item_text_lines = shape_buffer(item_text, 77 - 15);

    int i = 0;
    std::transform(item_text_lines.begin(), item_text_lines.end(), &info[i], [](const auto &line) { return line.data(); });
    i += item_text_lines.size();

    if (o_ptr->is_equipment()) {
        trivial_info = i;
    }

    if (flags.has(TR_ACTIVATE)) {
        info[i++] = _("始動したときの効果...", "It can be activated for...");
        info[i++] = activation_explanation(o_ptr);
        info[i++] = _("...ただし装備していなければならない。", "...if it is being worn.");
    }

    const auto &bi_key = o_ptr->bi_key;
    if (bi_key.tval() == ItemKindType::FIGURINE) {
        info[i++] = _("それは投げた時ペットに変化する。", "It will transform into a pet when thrown.");
    }

    if (o_ptr->is_specific_artifact(FixedArtifactId::STONEMASK)) {
        info[i++] = _("それを装備した者は吸血鬼になる。", "It makes you turn into a vampire permanently.");
    }

    if (bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
        info[i++] = _("それは相手を一撃で倒すことがある。", "It will attempt to instantly kill a monster.");
    }

    if (bi_key == BaseitemKey(ItemKindType::POLEARM, SV_DEATH_SCYTHE)) {
        info[i++] = _("それは自分自身に攻撃が返ってくることがある。", "It causes you to strike yourself sometimes.");
        info[i++] = _("それは無敵のバリアを切り裂く。", "It always penetrates invulnerability barriers.");
    }

    if (flags.has(TR_EASY2_WEAPON)) {
        info[i++] = _("それは二刀流での命中率を向上させる。", "It affects your ability to hit when you are wielding two weapons.");
    }

    if (flags.has(TR_INVULN_ARROW)) {
        info[i++] = _("それは視界がある限り物理的な飛び道具の一切をはねのける。", "It repels all physical missiles as long as there is visibility.");
    }

    if (flags.has(TR_NO_AC)) {
        info[i++] = _("それは物理的防護の一切を奪う。", "It robs you of any physical protection.");
    }

    if (flags.has(TR_EASY_SPELL)) {
        info[i++] = _("それは魔法の難易度を下げる。", "It affects your ability to cast spells.");
    }

    if (flags.has(TR_HEAVY_SPELL)) {
        info[i++] = _("それは魔法の難易度を上げる。", "It interferes with casting spells.");
    }

    if (flags.has(TR_MIGHTY_THROW)) {
        info[i++] = _("それは物を強く投げることを可能にする。", "It provides great strength when you throw an item.");
    }

    if (bi_key.tval() == ItemKindType::STATUE) {
        auto statue_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
        auto *r_ptr = &monraces_info[statue_r_idx];
        if (statue_r_idx == MonsterRaceId::BULLGATES) {
            info[i++] = _("それは部屋に飾ると恥ずかしい。", "It is shameful.");
        } else if (r_ptr->flags2 & (RF2_ELDRITCH_HORROR)) {
            info[i++] = _("それは部屋に飾ると恐い。", "It is fearful.");
        } else {
            info[i++] = _("それは部屋に飾ると楽しい。", "It is cheerful.");
        }
    }

    if (flags.has(TR_DARK_SOURCE)) {
        info[i++] = _("それは全く光らない。", "It provides no light.");
    }

    POSITION rad = 0;
    if (flags.has(TR_LITE_1) && flags.has_not(TR_DARK_SOURCE)) {
        rad += 1;
    }
    if (flags.has(TR_LITE_2) && flags.has_not(TR_DARK_SOURCE)) {
        rad += 2;
    }
    if (flags.has(TR_LITE_3) && flags.has_not(TR_DARK_SOURCE)) {
        rad += 3;
    }
    if (flags.has(TR_LITE_M1)) {
        rad -= 1;
    }
    if (flags.has(TR_LITE_M2)) {
        rad -= 2;
    }
    if (flags.has(TR_LITE_M3)) {
        rad -= 3;
    }

    if (o_ptr->ego_idx == EgoType::LITE_SHINE) {
        rad++;
    }

    std::string desc;
    if (flags.has(TR_LITE_FUEL) && flags.has_not(TR_DARK_SOURCE)) {
        if (rad > 0) {
            desc = _("それは燃料補給によって明かり(半径 ", "It provides light (radius ");
            desc.append(std::to_string((int)rad)).append(_(")を授ける。", ") when fueled."));
        }
    } else {
        if (rad > 0) {
            desc = _("それは永遠なる明かり(半径 ", "It provides light (radius ");
            desc.append(std::to_string((int)rad)).append(_(")を授ける。", ") forever."));
        }
        if (rad < 0) {
            desc = _("それは明かりの半径を狭める(半径に-", "It decreases the radius of your light by");
            desc.append(std::to_string((int)-rad)).append(_(")。", "."));
        }
    }

    if (rad != 0) {
        info[i++] = desc.data();
    }

    if (o_ptr->ego_idx == EgoType::LITE_LONG) {
        info[i++] = _("それは長いターン明かりを授ける。", "It provides light for much longer time.");
    }

    if (flags.has(TR_RIDING)) {
        if (o_ptr->is_lance()) {
            info[i++] = _("それは乗馬中は非常に使いやすい。", "It is made for use while riding.");
        } else {
            info[i++] = _("それは乗馬中でも使いやすい。", "It is suitable for use while riding.");
            trivial_info++;
        }
    }

    if (flags.has(TR_SUPPORTIVE)) {
        info[i++] = _("それは武器の補助として扱いやすい。", "It is easy to treat it as assistance to weapon.");
    }

    if (flags.has(TR_STR)) {
        info[i++] = _("それは腕力に影響を及ぼす。", "It affects your strength.");
    }

    if (flags.has(TR_INT)) {
        info[i++] = _("それは知能に影響を及ぼす。", "It affects your intelligence.");
    }

    if (flags.has(TR_WIS)) {
        info[i++] = _("それは賢さに影響を及ぼす。", "It affects your wisdom.");
    }

    if (flags.has(TR_DEX)) {
        info[i++] = _("それは器用さに影響を及ぼす。", "It affects your dexterity.");
    }

    if (flags.has(TR_CON)) {
        info[i++] = _("それは耐久力に影響を及ぼす。", "It affects your constitution.");
    }

    if (flags.has(TR_CHR)) {
        info[i++] = _("それは魅力に影響を及ぼす。", "It affects your charisma.");
    }

    if (flags.has(TR_MAGIC_MASTERY)) {
        info[i++] = _("それは魔法道具使用能力に影響を及ぼす。", "It affects your ability to use magic devices.");
    }

    if (flags.has(TR_STEALTH)) {
        info[i++] = _("それは隠密行動能力に影響を及ぼす。", "It affects your stealth.");
    }

    if (flags.has(TR_SEARCH)) {
        info[i++] = _("それは探索能力に影響を及ぼす。", "It affects your searching.");
    }

    if (flags.has(TR_INFRA)) {
        info[i++] = _("それは赤外線視力に影響を及ぼす。", "It affects your infravision.");
    }

    if (flags.has(TR_TUNNEL)) {
        info[i++] = _("それは採掘能力に影響を及ぼす。", "It affects your ability to tunnel.");
    }

    if (flags.has(TR_SPEED)) {
        info[i++] = _("それはスピードに影響を及ぼす。", "It affects your speed.");
    }

    if (flags.has(TR_BLOWS)) {
        info[i++] = _("それは打撃回数に影響を及ぼす。", "It affects your attack speed.");
    }

    if (flags.has(TR_BRAND_ACID)) {
        info[i++] = _("それは酸によって大きなダメージを与える。", "It does extra damage from acid.");
    }

    if (flags.has(TR_BRAND_ELEC)) {
        info[i++] = _("それは電撃によって大きなダメージを与える。", "It does extra damage from electricity.");
    }

    if (flags.has(TR_BRAND_FIRE)) {
        info[i++] = _("それは火炎によって大きなダメージを与える。", "It does extra damage from fire.");
    }

    if (flags.has(TR_BRAND_COLD)) {
        info[i++] = _("それは冷気によって大きなダメージを与える。", "It does extra damage from frost.");
    }

    if (flags.has(TR_BRAND_POIS)) {
        info[i++] = _("それは敵を毒する。", "It poisons your foes.");
    }

    if (flags.has(TR_CHAOTIC)) {
        info[i++] = _("それはカオス的な効果を及ぼす。", "It produces chaotic effects.");
    }

    if (flags.has(TR_BRAND_MAGIC)) {
        info[i++] = _("それは魔術的な効果を及ぼす。", "It produces magical effects.");
    }

    if (flags.has(TR_VAMPIRIC)) {
        info[i++] = _("それは敵から生命力を吸収する。", "It drains life from your foes.");
    }

    if (flags.has(TR_EARTHQUAKE)) {
        info[i++] = _("それは地震を起こすことができる。", "It can cause earthquakes.");
    }

    if (flags.has(TR_VORPAL)) {
        info[i++] = _("それは非常に切れ味が鋭く敵を切断することができる。", "It is very sharp and can cut your foes.");
    }

    if (flags.has(TR_IMPACT)) {
        info[i++] = _("それは非常に強く敵を攻撃することができる。", "It can hit your foes strongly.");
    }

    if (flags.has(TR_KILL_DRAGON)) {
        info[i++] = _("それはドラゴンにとっての天敵である。", "It is a great bane of dragons.");
    } else if (flags.has(TR_SLAY_DRAGON)) {
        info[i++] = _("それはドラゴンに対して特に恐るべき力を発揮する。", "It is especially deadly against dragons.");
    }

    if (flags.has(TR_KILL_ORC)) {
        info[i++] = _("それはオークにとっての天敵である。", "It is a great bane of orcs.");
    }

    if (flags.has(TR_SLAY_ORC)) {
        info[i++] = _("それはオークに対して特に恐るべき力を発揮する。", "It is especially deadly against orcs.");
    }

    if (flags.has(TR_KILL_TROLL)) {
        info[i++] = _("それはトロルにとっての天敵である。", "It is a great bane of trolls.");
    }

    if (flags.has(TR_SLAY_TROLL)) {
        info[i++] = _("それはトロルに対して特に恐るべき力を発揮する。", "It is especially deadly against trolls.");
    }

    if (flags.has(TR_KILL_GIANT)) {
        info[i++] = _("それは巨人にとっての天敵である。", "It is a great bane of giants.");
    } else if (flags.has(TR_SLAY_GIANT)) {
        info[i++] = _("それは巨人に対して特に恐るべき力を発揮する。", "It is especially deadly against giants.");
    }

    if (flags.has(TR_KILL_DEMON)) {
        info[i++] = _("それはデーモンにとっての天敵である。", "It is a great bane of demons.");
    }

    if (flags.has(TR_SLAY_DEMON)) {
        info[i++] = _("それはデーモンに対して聖なる力を発揮する。", "It strikes at demons with holy wrath.");
    }

    if (flags.has(TR_KILL_UNDEAD)) {
        info[i++] = _("それはアンデッドにとっての天敵である。", "It is a great bane of undead.");
    }

    if (flags.has(TR_SLAY_UNDEAD)) {
        info[i++] = _("それはアンデッドに対して聖なる力を発揮する。", "It strikes at undead with holy wrath.");
    }

    if (flags.has(TR_KILL_EVIL)) {
        info[i++] = _("それは邪悪なる存在にとっての天敵である。", "It is a great bane of evil monsters.");
    }

    if (flags.has(TR_SLAY_EVIL)) {
        info[i++] = _("それは邪悪なる存在に対して聖なる力で攻撃する。", "It fights against evil with holy fury.");
    }

    if (flags.has(TR_KILL_GOOD)) {
        info[i++] = _("それは善良なる存在にとっての天敵である。", "It is a great bane of good monsters.");
    }

    if (flags.has(TR_SLAY_GOOD)) {
        info[i++] = _("それは善良なる存在に対して邪悪なる力で攻撃する。", "It fights against good with evil fury.");
    }

    if (flags.has(TR_KILL_ANIMAL)) {
        info[i++] = _("それは自然界の動物にとっての天敵である。", "It is a great bane of natural creatures.");
    }

    if (flags.has(TR_SLAY_ANIMAL)) {
        info[i++] = _("それは自然界の動物に対して特に恐るべき力を発揮する。", "It is especially deadly against natural creatures.");
    }

    if (flags.has(TR_KILL_HUMAN)) {
        info[i++] = _("それは人間にとっての天敵である。", "It is a great bane of humans.");
    }

    if (flags.has(TR_SLAY_HUMAN)) {
        info[i++] = _("それは人間に対して特に恐るべき力を発揮する。", "It is especially deadly against humans.");
    }

    if (flags.has(TR_FORCE_WEAPON)) {
        info[i++] = _("それは使用者の魔力を使って攻撃する。", "It powerfully strikes at a monster using your mana.");
    }

    if (flags.has(TR_DEC_MANA)) {
        info[i++] = _("それは魔力の消費を押さえる。", "It decreases your mana consumption.");
    }

    if (flags.has(TR_SUST_STR)) {
        info[i++] = _("それはあなたの腕力を維持する。", "It sustains your strength.");
    }

    if (flags.has(TR_SUST_INT)) {
        info[i++] = _("それはあなたの知能を維持する。", "It sustains your intelligence.");
    }

    if (flags.has(TR_SUST_WIS)) {
        info[i++] = _("それはあなたの賢さを維持する。", "It sustains your wisdom.");
    }

    if (flags.has(TR_SUST_DEX)) {
        info[i++] = _("それはあなたの器用さを維持する。", "It sustains your dexterity.");
    }

    if (flags.has(TR_SUST_CON)) {
        info[i++] = _("それはあなたの耐久力を維持する。", "It sustains your constitution.");
    }

    if (flags.has(TR_SUST_CHR)) {
        info[i++] = _("それはあなたの魅力を維持する。", "It sustains your charisma.");
    }

    if (flags.has(TR_IM_ACID)) {
        info[i++] = _("それは酸に対する完全な免疫を授ける。", "It provides immunity to acid.");
    } else if (flags.has(TR_VUL_ACID)) {
        info[i++] = _("それは酸に対する弱点を授ける。", "It provides vulnerability to acid.");
    }

    if (flags.has(TR_IM_ELEC)) {
        info[i++] = _("それは電撃に対する完全な免疫を授ける。", "It provides immunity to electricity.");
    } else if (flags.has(TR_VUL_ELEC)) {
        info[i++] = _("それは電撃に対する弱点を授ける。", "It provides vulnerability to electricity.");
    }

    if (flags.has(TR_IM_FIRE)) {
        info[i++] = _("それは火に対する完全な免疫を授ける。", "It provides immunity to fire.");
    } else if (flags.has(TR_VUL_FIRE)) {
        info[i++] = _("それは火に対する弱点を授ける。", "It provides vulnerability to fire.");
    }

    if (flags.has(TR_IM_COLD)) {
        info[i++] = _("それは寒さに対する完全な免疫を授ける。", "It provides immunity to cold.");
    } else if (flags.has(TR_VUL_COLD)) {
        info[i++] = _("それは寒さに対する弱点を授ける。", "It provides vulnerability to cold.");
    }

    if (flags.has(TR_IM_DARK)) {
        info[i++] = _("それは暗黒に対する完全な免疫を授ける。", "It provides immunity to dark.");
    }

    if (flags.has(TR_VUL_LITE)) {
        info[i++] = _("それは閃光に対する弱点を授ける。", "It provides vulnerability to cold.");
    }

    if (flags.has(TR_THROW)) {
        info[i++] = _("それは敵に投げて大きなダメージを与えることができる。", "It is perfectly balanced for throwing.");
    }

    if (flags.has(TR_FREE_ACT)) {
        info[i++] = _("それは麻痺に対する完全な免疫を授ける。", "It provides immunity to paralysis.");
    }

    if (flags.has(TR_HOLD_EXP)) {
        info[i++] = _("それは経験値吸収に対する耐性を授ける。", "It provides resistance to experience draining.");
    }

    if (flags.has(TR_RES_FEAR)) {
        info[i++] = _("それは恐怖への完全な耐性を授ける。", "It makes you completely fearless.");
    }

    if (flags.has(TR_RES_ACID)) {
        info[i++] = _("それは酸への耐性を授ける。", "It provides resistance to acid.");
    }

    if (flags.has(TR_RES_ELEC)) {
        info[i++] = _("それは電撃への耐性を授ける。", "It provides resistance to electricity.");
    }

    if (flags.has(TR_RES_FIRE)) {
        info[i++] = _("それは火への耐性を授ける。", "It provides resistance to fire.");
    }

    if (flags.has(TR_RES_COLD)) {
        info[i++] = _("それは寒さへの耐性を授ける。", "It provides resistance to cold.");
    }

    if (flags.has(TR_RES_POIS)) {
        info[i++] = _("それは毒への耐性を授ける。", "It provides resistance to poison.");
    }

    if (flags.has(TR_RES_LITE)) {
        info[i++] = _("それは閃光への耐性を授ける。", "It provides resistance to light.");
    }

    if (flags.has(TR_RES_DARK)) {
        info[i++] = _("それは暗黒への耐性を授ける。", "It provides resistance to dark.");
    }

    if (flags.has(TR_RES_BLIND)) {
        info[i++] = _("それは盲目への耐性を授ける。", "It provides resistance to blindness.");
    }

    if (flags.has(TR_RES_CONF)) {
        info[i++] = _("それは混乱への耐性を授ける。", "It provides resistance to confusion.");
    }

    if (flags.has(TR_RES_SOUND)) {
        info[i++] = _("それは轟音への耐性を授ける。", "It provides resistance to sound.");
    }

    if (flags.has(TR_RES_SHARDS)) {
        info[i++] = _("それは破片への耐性を授ける。", "It provides resistance to shards.");
    }

    if (flags.has(TR_RES_NETHER)) {
        info[i++] = _("それは地獄への耐性を授ける。", "It provides resistance to nether.");
    }

    if (flags.has(TR_RES_NEXUS)) {
        info[i++] = _("それは因果混乱への耐性を授ける。", "It provides resistance to nexus.");
    }

    if (flags.has(TR_RES_CHAOS)) {
        info[i++] = _("それはカオスへの耐性を授ける。", "It provides resistance to chaos.");
    }

    if (flags.has(TR_RES_TIME)) {
        info[i++] = _("それは時間逆転への耐性を授ける。", "It provides resistance to time stream.");
    }

    if (flags.has(TR_RES_WATER)) {
        info[i++] = _("それは水流への耐性を授ける。", "It provides resistance to water stream.");
    }

    if (flags.has(TR_RES_DISEN)) {
        info[i++] = _("それは劣化への耐性を授ける。", "It provides resistance to disenchantment.");
    }

    if (flags.has(TR_LEVITATION)) {
        info[i++] = _("それは宙に浮くことを可能にする。", "It allows you to levitate.");
    }

    if (flags.has(TR_SEE_INVIS)) {
        info[i++] = _("それは透明なモンスターを見ることを可能にする。", "It allows you to see invisible monsters.");
    }

    if (flags.has(TR_TELEPATHY)) {
        info[i++] = _("それはテレパシー能力を授ける。", "It gives telepathic powers.");
    }

    if (flags.has(TR_ESP_ANIMAL)) {
        info[i++] = _("それは自然界の生物を感知する。", "It senses natural creatures.");
    }

    if (flags.has(TR_ESP_UNDEAD)) {
        info[i++] = _("それはアンデッドを感知する。", "It senses undead.");
    }

    if (flags.has(TR_ESP_DEMON)) {
        info[i++] = _("それは悪魔を感知する。", "It senses demons.");
    }

    if (flags.has(TR_ESP_ORC)) {
        info[i++] = _("それはオークを感知する。", "It senses orcs.");
    }

    if (flags.has(TR_ESP_TROLL)) {
        info[i++] = _("それはトロルを感知する。", "It senses trolls.");
    }

    if (flags.has(TR_ESP_GIANT)) {
        info[i++] = _("それは巨人を感知する。", "It senses giants.");
    }

    if (flags.has(TR_ESP_DRAGON)) {
        info[i++] = _("それはドラゴンを感知する。", "It senses dragons.");
    }

    if (flags.has(TR_ESP_HUMAN)) {
        info[i++] = _("それは人間を感知する。", "It senses humans.");
    }

    if (flags.has(TR_ESP_EVIL)) {
        info[i++] = _("それは邪悪な存在を感知する。", "It senses evil creatures.");
    }

    if (flags.has(TR_ESP_GOOD)) {
        info[i++] = _("それは善良な存在を感知する。", "It senses good creatures.");
    }

    if (flags.has(TR_ESP_NONLIVING)) {
        info[i++] = _("それは活動する無生物体を感知する。", "It senses non-living creatures.");
    }

    if (flags.has(TR_ESP_UNIQUE)) {
        info[i++] = _("それは特別な強敵を感知する。", "It senses unique monsters.");
    }

    if (flags.has(TR_SLOW_DIGEST)) {
        info[i++] = _("それはあなたの新陳代謝を遅くする。", "It slows your metabolism.");
    }

    if (flags.has(TR_REGEN)) {
        info[i++] = _("それは体力回復力を強化する。", "It speeds your regenerative powers.");
    }

    if (flags.has(TR_WARNING)) {
        info[i++] = _("それは危険に対して警告を発する。", "It warns you of danger");
    }

    if (flags.has(TR_REFLECT)) {
        info[i++] = _("それは矢の呪文を反射する。", "It reflects bolt spells.");
    }

    if (flags.has(TR_RES_CURSE)) {
        info[i++] = _("それは呪いへの抵抗力を高める。", "It increases your resistance to curses.");
    }

    if (flags.has(TR_SH_FIRE)) {
        info[i++] = _("それは炎のバリアを張る。", "It produces a fiery sheath.");
    }

    if (flags.has(TR_SH_ELEC)) {
        info[i++] = _("それは電気のバリアを張る。", "It produces an electric sheath.");
    }

    if (flags.has(TR_SH_COLD)) {
        info[i++] = _("それは冷気のバリアを張る。", "It produces a sheath of coldness.");
    }

    if (flags.has(TR_SELF_FIRE)) {
        info[i++] = _("それはあなたを燃やす。", "It burns you.");
    }

    if (flags.has(TR_SELF_ELEC)) {
        info[i++] = _("それはあなたを電撃で包む。", "It electrocutes you.");
    }

    if (flags.has(TR_SELF_COLD)) {
        info[i++] = _("それはあなたを凍らせる。", "It freezes you.");
    }

    if (flags.has(TR_NO_MAGIC)) {
        info[i++] = _("それは反魔法バリアを張る。", "It produces an anti-magic shell.");
    }

    if (flags.has(TR_NO_TELE)) {
        info[i++] = _("それはテレポートを邪魔する。", "It prevents teleportation.");
    }

    if (flags.has(TR_XTRA_MIGHT)) {
        info[i++] = _("それは矢／ボルト／弾をより強力に発射することができる。", "It fires missiles with extra might.");
    }

    if (flags.has(TR_XTRA_SHOTS)) {
        info[i++] = _("それは矢／ボルト／弾を非常に早く発射することができる。", "It fires missiles excessively fast.");
    }

    if (flags.has(TR_BLESSED)) {
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

    if ((flags.has(TR_TY_CURSE)) || o_ptr->curse_flags.has(CurseTraitType::TY_CURSE)) {
        info[i++] = _("それは太古の禍々しい怨念が宿っている。", "It carries an ancient foul curse.");
    }

    if ((flags.has(TR_AGGRAVATE)) || o_ptr->curse_flags.has(CurseTraitType::AGGRAVATE)) {
        info[i++] = _("それは付近のモンスターを怒らせる。", "It aggravates nearby creatures.");
    }

    if ((flags.has(TR_DRAIN_EXP)) || o_ptr->curse_flags.has(CurseTraitType::DRAIN_EXP)) {
        info[i++] = _("それは経験値を吸い取る。", "It drains experience.");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::SLOW_REGEN)) {
        info[i++] = _("それは回復力を弱める。", "It slows your regenerative powers.");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::ADD_L_CURSE) || flags.has(TR_ADD_L_CURSE)) {
        info[i++] = _("それは弱い呪いを増やす。", "It adds weak curses.");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::ADD_H_CURSE) || flags.has(TR_ADD_H_CURSE)) {
        info[i++] = _("それは強力な呪いを増やす。", "It adds heavy curses.");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::PERSISTENT_CURSE) || flags.has(TR_PERSISTENT_CURSE)) {
        info[i++] = _("それは頻繁に呪いをかけなおす。", "It curses itself persistently.");
    }

    if ((flags.has(TR_CALL_ANIMAL)) || o_ptr->curse_flags.has(CurseTraitType::CALL_ANIMAL)) {
        info[i++] = _("それは動物を呼び寄せる。", "It attracts animals.");
    }

    if ((flags.has(TR_CALL_DEMON)) || o_ptr->curse_flags.has(CurseTraitType::CALL_DEMON)) {
        info[i++] = _("それは悪魔を呼び寄せる。", "It attracts demons.");
    }

    if ((flags.has(TR_CALL_DRAGON)) || o_ptr->curse_flags.has(CurseTraitType::CALL_DRAGON)) {
        info[i++] = _("それはドラゴンを呼び寄せる。", "It attracts dragons.");
    }

    if ((flags.has(TR_CALL_UNDEAD)) || o_ptr->curse_flags.has(CurseTraitType::CALL_UNDEAD)) {
        info[i++] = _("それは死霊を呼び寄せる。", "It attracts undead.");
    }

    if ((flags.has(TR_COWARDICE)) || o_ptr->curse_flags.has(CurseTraitType::COWARDICE)) {
        info[i++] = _("それは恐怖感を引き起こす。", "It makes you subject to cowardice.");
    }

    if (flags.has(TR_BERS_RAGE) || o_ptr->curse_flags.has(CurseTraitType::BERS_RAGE)) {
        info[i++] = _("それは狂戦士化の発作を引き起こす。", "It makes you subject to berserker fits.");
    }

    if ((flags.has(TR_TELEPORT)) || o_ptr->curse_flags.has(CurseTraitType::TELEPORT)) {
        info[i++] = _("それはランダムなテレポートを引き起こす。", "It induces random teleportation.");
    }

    if ((flags.has(TR_LOW_MELEE)) || o_ptr->curse_flags.has(CurseTraitType::LOW_MELEE)) {
        info[i++] = _("それは攻撃を外しやすい。", "It causes you to miss blows.");
    }

    if ((flags.has(TR_LOW_AC)) || o_ptr->curse_flags.has(CurseTraitType::LOW_AC)) {
        info[i++] = _("それは攻撃を受けやすい。", "It helps your enemies' blows.");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::VUL_CURSE) || flags.has(TR_VUL_CURSE)) {
        info[i++] = _("それは呪いへの抵抗力を下げる。", "It decreases your resistance to curses.");
    }

    if (flags.has(TR_DOWN_SAVING)) {
        info[i++] = _("それは魔法抵抗力を半減させる。", "It halves your magic resistance.");
    }

    if ((flags.has(TR_HARD_SPELL)) || o_ptr->curse_flags.has(CurseTraitType::HARD_SPELL)) {
        info[i++] = _("それは魔法を唱えにくくする。", "It encumbers you while spellcasting.");
    }

    if ((flags.has(TR_FAST_DIGEST)) || o_ptr->curse_flags.has(CurseTraitType::FAST_DIGEST)) {
        info[i++] = _("それはあなたの新陳代謝を速くする。", "It speeds your metabolism.");
    }

    if ((flags.has(TR_DRAIN_HP)) || o_ptr->curse_flags.has(CurseTraitType::DRAIN_HP)) {
        info[i++] = _("それはあなたの体力を吸い取る。", "It drains you.");
    }

    if ((flags.has(TR_DRAIN_MANA)) || o_ptr->curse_flags.has(CurseTraitType::DRAIN_MANA)) {
        info[i++] = _("それはあなたの魔力を吸い取る。", "It drains your mana.");
    }

    if (mode & SCROBJ_FAKE_OBJECT) {
        const auto sval = o_ptr->bi_key.sval().value();
        switch (o_ptr->bi_key.tval()) {
        case ItemKindType::RING:
            switch (sval) {
            case SV_RING_LORDLY:
                info[i++] = _("それは幾つかのランダムな耐性を授ける。", "It provides some random resistances.");
                break;
            case SV_RING_WARNING:
                info[i++] = _("それはひとつの低級なESPを授ける事がある。", "It may provide a low rank ESP.");
                break;
            }

            break;

        case ItemKindType::AMULET:
            switch (sval) {
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

    if (flags.has(TR_IGNORE_ACID) && flags.has(TR_IGNORE_ELEC) && flags.has(TR_IGNORE_FIRE) && flags.has(TR_IGNORE_COLD)) {
        info[i++] = _("それは酸・電撃・火炎・冷気では傷つかない。", "It cannot be harmed by the elements.");
    } else {
        if (flags.has(TR_IGNORE_ACID)) {
            info[i++] = _("それは酸では傷つかない。", "It cannot be harmed by acid.");
        }

        if (flags.has(TR_IGNORE_ELEC)) {
            info[i++] = _("それは電撃では傷つかない。", "It cannot be harmed by electricity.");
        }

        if (flags.has(TR_IGNORE_FIRE)) {
            info[i++] = _("それは火炎では傷つかない。", "It cannot be harmed by fire.");
        }

        if (flags.has(TR_IGNORE_COLD)) {
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
    const auto [wid, hgt] = term_get_size();
    std::string item_name;
    if (!(mode & SCROBJ_FAKE_OBJECT)) {
        item_name = describe_flavor(player_ptr, o_ptr, 0);
    } else {
        item_name = describe_flavor(player_ptr, o_ptr, (OD_NAME_ONLY | OD_STORE));
    }

    prt(item_name, 0, 0);
    for (int k = 1; k < hgt; k++) {
        prt("", k, 13);
    }

    if (bi_key == BaseitemKey(ItemKindType::STATUE, SV_PHOTO)) {
        auto statue_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
        auto *r_ptr = &monraces_info[statue_r_idx];
        int namelen = strlen(r_ptr->name.data());
        prt(format("%s: '", r_ptr->name.data()), 1, 15);
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
