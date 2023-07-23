#include "racial/class-racial-switcher.h"
#include "cmd-action/cmd-spell.h"
#include "mind/mind-elementalist.h"
#include "racial/racial-util.h"
#include "realm/realm-names-table.h"
#include "realm/realm-types.h"
#include "system/player-type-definition.h"

void switch_class_racial(PlayerType *player_ptr, rc_type *rc_ptr)
{
    rpi_type rpi;
    switch (player_ptr->pclass) {
    case PlayerClassType::WARRIOR:
        rpi = rpi_type(_("剣の舞い", "Sword Dancing"));
        rpi.text = _("ランダムな方向に数回攻撃する。", "Attacks some times to random directions.");
        rpi.min_level = 40;
        rpi.cost = 75;
        rpi.stat = A_DEX;
        rpi.fail = 35;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::HIGH_MAGE:
        if (player_ptr->realm1 == REALM_HEX) {
            rpi = rpi_type(_("詠唱をやめる", "Stop spell casting"));
            rpi.text = _("呪術の詠唱を全てやめる。", "Stops all casting hex spells.");
            rpi.min_level = 1;
            rpi.cost = 0;
            rpi.stat = A_INT;
            rpi.fail = 0;
            rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
            break;
        }
        [[fallthrough]];
    case PlayerClassType::MAGE:
    case PlayerClassType::SORCERER:
        rpi = rpi_type(_("魔力食い", "Eat Magic"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl * 2);
        rpi.text = _("魔法道具から魔力を吸収してMPを回復する。", "Absorbs mana from a magic device to heal your SP.");
        rpi.min_level = 25;
        rpi.cost = 1;
        rpi.stat = A_INT;
        rpi.fail = 25;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::PRIEST:
        if (is_good_realm(player_ptr->realm1)) {
            rpi = rpi_type(_("武器祝福", "Bless Weapon"));
            rpi.text = _("武器を祝福する。抵抗されることがある。", "Blesses a weapon. Some weapons can resist it.");
            rpi.min_level = 35;
            rpi.cost = 70;
            rpi.stat = A_WIS;
            rpi.fail = 50;
            rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        } else {
            rpi = rpi_type(_("召魂", "Evocation"));
            rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl * 4);
            rpi.text = _("視界内の全てのモンスターにダメージを与え、恐怖させ、遠くへ飛ばす。", "Deals damage to all monster in your sight, makes them scared and tereports then away.");
            rpi.min_level = 42;
            rpi.cost = 40;
            rpi.stat = A_WIS;
            rpi.fail = 35;
            rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        }

        break;
    case PlayerClassType::ROGUE:
        rpi = rpi_type(_("ヒット＆アウェイ", "Hit and Away"));
        rpi.info = format("%s%d", KWD_SPHERE, 30);
        rpi.text = _("対象のモンスターを攻撃したあと短距離テレポートする。", "Attacks a monster then tereports you a short range.");
        rpi.min_level = 8;
        rpi.cost = 12;
        rpi.stat = A_DEX;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::RANGER:
    case PlayerClassType::SNIPER:
        rpi = rpi_type(_("モンスター調査", "Probe Monster"));
        rpi.text = _("モンスターの属性、残り体力、最大体力、スピード、正体を知る。", "Probes all monsters' alignment, HP, speed and their true character.");
        rpi.min_level = 15;
        rpi.cost = 20;
        rpi.stat = A_INT;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::PALADIN:
        if (is_good_realm(player_ptr->realm1)) {
            rpi = rpi_type(_("ホーリー・ランス", "Holy Lance"));
            rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 3);
            rpi.text = _("聖なる炎のビームを放つ。", "Fires a beam of holy fire.");
            rpi.min_level = 30;
            rpi.cost = 30;
            rpi.stat = A_WIS;
            rpi.fail = 30;
            rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        } else {
            rpi = rpi_type(_("ヘル・ランス", "Hell Lance"));
            rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 3);
            rpi.text = _("地獄の業火のビームを放つ。", "Fires a beam of hell fire.");
            rpi.min_level = 30;
            rpi.cost = 30;
            rpi.stat = A_WIS;
            rpi.fail = 30;
            rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        }

        break;
    case PlayerClassType::WARRIOR_MAGE:
        rpi = rpi_type(_("変換: ＨＰ→ＭＰ", "Convert HP to SP"));
        rpi.text = _("HPを少しMPに変換する。", "Transfers a few HP to SP.");
        rpi.min_level = 25;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rpi_type(_("変換: ＭＰ→ＨＰ", "Convert SP to HP"));
        rpi.text = _("MPを少しHPに変換する。", "Transfers a few SP to HP.");
        rpi.min_level = 25;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case PlayerClassType::CHAOS_WARRIOR:
        rpi = rpi_type(_("幻惑の光", "Confusing Light"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl * 4);
        rpi.text = _("周辺のモンスターを減速・朦朧・混乱・朦朧・恐怖・睡眠させようとする。抵抗されると無効。",
            "Tries to make all monsters in your sight slowed, stunned, confused, scared, sleeped.");
        rpi.min_level = 40;
        rpi.cost = 50;
        rpi.stat = A_INT;
        rpi.fail = 25;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::MONK:
        rpi = rpi_type(_("構える", "Assume a Stance"));
        rpi.text = _("型に構えて特殊な能力を得る。", "Gains extra abilities with posing a 'kata'.");
        rpi.min_level = 25;
        rpi.cost = 0;
        rpi.stat = A_DEX;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rpi_type(_("百裂拳", "Double Attack"));
        rpi.text = _("対象に対して2回の打撃を行う。", "Melee attacks to a target monster two times.");
        rpi.min_level = 30;
        rpi.cost = 30;
        rpi.stat = A_STR;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, -4);
        break;
    case PlayerClassType::MINDCRAFTER:
    case PlayerClassType::FORCETRAINER:
        rpi = rpi_type(_("明鏡止水", "Clear Mind"));
        rpi.info = format("%s%d", KWD_MANA, 3 + rc_ptr->lvl / 20);
        rpi.text = _("精神を集中してMPを少し回復する。", "Concentrates deeply to heal your SP a little.");
        rpi.min_level = 15;
        rpi.cost = 0;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::TOURIST:
        rpi = rpi_type(_("写真撮影", "Take a Photograph"));
        rpi.text = _("対象のモンスター1体の写真を撮影する。", "Takes a picture of a monster.");
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_DEX;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rpi_type(_("真・鑑定", "Identify True"));
        rpi.text = _("アイテムの持つ能力を完全に知る。", "*Identifies* an item.");
        rpi.min_level = 25;
        rpi.cost = 20;
        rpi.stat = A_INT;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, -4);
        break;
    case PlayerClassType::IMITATOR:
        rpi = rpi_type(_("倍返し", "Double Revenge"));
        rpi.text = _("威力を倍にしてものまねを行う。", "Fires an imitation a damage of which you makes doubled.");
        rpi.min_level = 30;
        rpi.cost = 100;
        rpi.stat = A_DEX;
        rpi.fail = 30;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::BEASTMASTER:
        rpi = rpi_type(_("生物支配", "Dominate a Living Thing"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl);
        rpi.text = _("1体のモンスターをペットにする。抵抗されると無効。", "Attempts to charm a monster.");
        rpi.min_level = 1;
        rpi.cost = (player_ptr->lev + 3) / 4;
        rpi.stat = A_CHR;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rpi_type(_("真・生物支配", "Dominate Living Things"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl);
        rpi.text = _("周辺のモンスターをペットにする。抵抗されると無効。", "Attempts to charm a monsters in your sight.");
        rpi.min_level = 30;
        rpi.cost = (player_ptr->lev + 20) / 2;
        rpi.stat = A_CHR;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, -4);
        break;
    case PlayerClassType::ARCHER:
        rpi = rpi_type(_("弾/矢の製造", "Create Ammo"));
        rpi.text = _("弾または矢を製造する。原料となるアイテムが必要。", "Creates ammos from materials.");
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_DEX;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::MAGIC_EATER:
        rpi = rpi_type(_("魔力の取り込み", "Absorb Magic"));
        rpi.text = _("魔法道具を取りこんで魔力とする。取りこんだ魔法道具は取り出せない。",
            "Absorbs a magic device as your mana. Cannot take out it from your mana later.");
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rpi_type(_("強力発動", "Powerful Activation"));
        rpi.text = _("取りこんだ魔法道具を威力を高めて使用する。", "Activates absorbed magic device powerfully.");
        rpi.min_level = 10;
        rpi.cost = 10 + (rc_ptr->lvl - 10) / 2;
        rpi.stat = A_INT;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, -4);
        break;
    case PlayerClassType::BARD:
        rpi = rpi_type(_("歌を止める", "Stop Singing"));
        rpi.text = _("現在詠唱中の歌をやめる。", "Stops singing a song.");
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_CHR;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::RED_MAGE:
        rpi = rpi_type(_("連続魔", "Double Magic"));
        rpi.text = _("1回の行動で2つの呪文を詠唱する。", "Casts two spells in an action.");
        rpi.min_level = 48;
        rpi.cost = 20;
        rpi.stat = A_INT;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::SAMURAI:
        rpi = rpi_type(_("気合いため", "Concentration"));
        rpi.text = _("気合を溜めてMPを増やす。上限値をある程度超えられる。", "Increases SP for Kendo over SP limit.");
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_WIS;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rpi_type(_("型", "Assume a Stance"));
        rpi.text = _("型に構えて特殊な能力を得る。", "Gains extra abilities with posing a 'kata'.");
        rpi.min_level = 25;
        rpi.cost = 0;
        rpi.stat = A_DEX;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, -4);
        break;
    case PlayerClassType::BLUE_MAGE:
        rpi = rpi_type(_("ラーニング", "Learning"));
        rpi.text = _("青魔法の学習を開始または終了する。学習中はMPを消費する。", "Starts or ends to learn blue magics. Pays SP for upkeep costs during it.");
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::CAVALRY:
        rpi = rpi_type(_("荒馬ならし", "Rodeo"));
        rpi.text = _("対象のモンスターの無理やり乗馬しペットにする。", "Rides to target monster and tame it focibly.");
        rpi.min_level = 10;
        rpi.cost = 0;
        rpi.stat = A_STR;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::BERSERKER:
        rpi = rpi_type(_("帰還", "Recall"));
        rpi.text = _("地上にいるときはダンジョンの最深階へ、ダンジョンにいるときは地上へと移動する。",
            "Recalls player from dungeon to town or from town to the deepest level of dungeon.");
        rpi.min_level = 10;
        rpi.cost = 10;
        rpi.stat = A_DEX;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::MIRROR_MASTER:
        rpi = rpi_type(_("鏡割り", "Break Mirrors"));
        rpi.text = _("現在の階に設置した鏡を全て割る。割られた鏡はなくなり、破片属性のボールが発生する。", "Destroys all mirrors located in current level. They are deleted from the level.");
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rpi_type(_("静水", "Mirror Concentration"));
        rpi.info = format("%s%d", KWD_MANA, 5 + rc_ptr->lvl * rc_ptr->lvl / 100);
        rpi.text = _("精神を集中してMPを少し回復する。", "Concentrates deeply to heal your SP a little.");
        rpi.min_level = 30;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, -4);
        break;
    case PlayerClassType::SMITH:
        rpi = rpi_type(_("目利き", "Judgment"));
        rpi.text = _("武器・矢弾・防具を鑑定する。", "Identifies an equipment or an ammo object.");
        rpi.min_level = 5;
        rpi.cost = 15;
        rpi.stat = A_INT;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::NINJA:
        rpi = rpi_type(_("速駆け", "Quick Walk"));
        rpi.text = _("身体を酷使して素早く移動する。", "Moves quickly but cannot regenerate HP naturally.");
        rpi.min_level = 20;
        rpi.cost = 0;
        rpi.stat = A_DEX;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case PlayerClassType::ELEMENTALIST:
        rpi = rpi_type(_("明鏡止水", "Clear Mind"));
        rpi.info = format("%s%d", KWD_MANA, 3 + rc_ptr->lvl / 20);
        rpi.text = _("精神を集中してMPを少し回復する。", "Concentrates deeply to heal your SP a little.");
        rpi.min_level = 15;
        rpi.cost = 0;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        switch_element_racial(player_ptr, rc_ptr);
        break;
    default:
        break;
    }
}
