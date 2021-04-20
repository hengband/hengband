#include "racial/class-racial-switcher.h"
#include "cmd-action/cmd-spell.h"
#include "mind/mind-elementalist.h"
#include "racial/racial-util.h"
#include "realm/realm-names-table.h"
#include "realm/realm-types.h"

void switch_class_racial(player_type *creature_ptr, rc_type *rc_ptr)
{
    rpi_type rpi;
    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
        rpi = rc_ptr->make_power(_("剣の舞い", "Sword Dancing"));
        rpi.min_level = 40;
        rpi.cost = 75;
        rpi.stat = A_DEX;
        rpi.fail = 35;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_HIGH_MAGE:
        if (creature_ptr->realm1 == REALM_HEX) {
            rpi = rc_ptr->make_power(_("詠唱をやめる", "Stop spell casting"));
            rpi.min_level = 1;
            rpi.cost = 0;
            rpi.stat = A_INT;
            rpi.fail = 0;
            rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
            break;
        }
        /* Fall through */
    case CLASS_MAGE:
    case CLASS_SORCERER:
        rpi = rc_ptr->make_power(_("魔力食い", "Eat Magic"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl * 2);
        rpi.min_level = 25;
        rpi.cost = 1;
        rpi.stat = A_INT;
        rpi.fail = 25;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_PRIEST:
        if (is_good_realm(creature_ptr->realm1)) {
            rpi = rc_ptr->make_power(_("武器祝福", "Bless Weapon"));
            rpi.min_level = 35;
            rpi.cost = 70;
            rpi.stat = A_WIS;
            rpi.fail = 50;
            rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        } else {
            rpi = rc_ptr->make_power(_("召魂", "Evocation"));
            rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl * 4);
            rpi.min_level = 42;
            rpi.cost = 40;
            rpi.stat = A_WIS;
            rpi.fail = 35;
            rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        }

        break;
    case CLASS_ROGUE:
        rpi = rc_ptr->make_power(_("ヒット＆アウェイ", "Hit and Away"));
        rpi.info = format("%s%d", KWD_SPHERE, 30);
        rpi.min_level = 8;
        rpi.cost = 12;
        rpi.stat = A_DEX;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_RANGER:
    case CLASS_SNIPER:
        rpi = rc_ptr->make_power(_("モンスター調査", "Probe Monster"));
        rpi.min_level = 15;
        rpi.cost = 20;
        rpi.stat = A_INT;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_PALADIN:
        if (is_good_realm(creature_ptr->realm1)) {
            rpi = rc_ptr->make_power(_("ホーリー・ランス", "Holy Lance"));
            rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 3);
            rpi.min_level = 30;
            rpi.cost = 30;
            rpi.stat = A_WIS;
            rpi.fail = 30;
            rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        } else {
            rpi = rc_ptr->make_power(_("ヘル・ランス", "Hell Lance"));
            rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 3);
            rpi.min_level = 30;
            rpi.cost = 30;
            rpi.stat = A_WIS;
            rpi.fail = 30;
            rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        }

        break;
    case CLASS_WARRIOR_MAGE:
        rpi = rc_ptr->make_power(_("変換: ＨＰ→ＭＰ", "Convert HP to SP"));
        rpi.min_level = 25;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rc_ptr->make_power(_("変換: ＭＰ→ＨＰ", "Convert SP to HP"));
        rpi.min_level = 25;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case CLASS_CHAOS_WARRIOR:
        rpi = rc_ptr->make_power(_("幻惑の光", "Confusing Light"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl * 4);
        rpi.min_level = 40;
        rpi.cost = 50;
        rpi.stat = A_INT;
        rpi.fail = 25;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_MONK:
        rpi = rc_ptr->make_power(_("構える", "Assume a Stance"));
        rpi.min_level = 25;
        rpi.cost = 0;
        rpi.stat = A_DEX;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rc_ptr->make_power(_("百裂拳", "Double Attack"));
        rpi.min_level = 30;
        rpi.cost = 30;
        rpi.stat = A_STR;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, -4);
        break;
    case CLASS_MINDCRAFTER:
    case CLASS_FORCETRAINER:
        rpi = rc_ptr->make_power(_("明鏡止水", "Clear Mind"));
        rpi.info = format("%s%d", KWD_MANA, 3 + rc_ptr->lvl / 20);
        rpi.min_level = 15;
        rpi.cost = 0;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_TOURIST:
        rpi = rc_ptr->make_power(_("写真撮影", "Take a Photograph"));
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_DEX;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rc_ptr->make_power(_("真・鑑定", "Identify True"));
        rpi.min_level = 25;
        rpi.cost = 20;
        rpi.stat = A_INT;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, -4);
        break;
    case CLASS_IMITATOR:
        rpi = rc_ptr->make_power(_("倍返し", "Double Revenge"));
        rpi.min_level = 30;
        rpi.cost = 100;
        rpi.stat = A_DEX;
        rpi.fail = 30;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_BEASTMASTER:
        rpi = rc_ptr->make_power(_("生物支配", "Dominate a Living Thing"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl);
        rpi.min_level = 1;
        rpi.cost = (creature_ptr->lev + 3) / 4;
        rpi.stat = A_CHR;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rc_ptr->make_power(_("真・生物支配", "Dominate Living Things"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl);
        rpi.min_level = 30;
        rpi.cost = (creature_ptr->lev + 20) / 2;
        rpi.stat = A_CHR;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, -4);
        break;
    case CLASS_ARCHER:
        rpi = rc_ptr->make_power(_("弾/矢の製造", "Create Ammo"));
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_DEX;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_MAGIC_EATER:
        rpi = rc_ptr->make_power(_("魔力の取り込み", "Absorb Magic"));
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rc_ptr->make_power(_("強力発動", "Powerful Activation"));
        rpi.min_level = 10;
        rpi.cost = 10 + (rc_ptr->lvl - 10) / 2;
        rpi.stat = A_INT;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, -4);
        break;
    case CLASS_BARD:
        rpi = rc_ptr->make_power(_("歌を止める", "Stop Singing"));
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_CHR;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_RED_MAGE:
        rpi = rc_ptr->make_power(_("連続魔", "Double Magic"));
        rpi.min_level = 48;
        rpi.cost = 20;
        rpi.stat = A_INT;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_SAMURAI:
        rpi = rc_ptr->make_power(_("気合いため", "Concentration"));
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_WIS;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rc_ptr->make_power(_("型", "Assume a Stance"));
        rpi.min_level = 25;
        rpi.cost = 0;
        rpi.stat = A_DEX;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, -4);
        break;
    case CLASS_BLUE_MAGE:
        rpi = rc_ptr->make_power(_("ラーニング", "Learning"));
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_CAVALRY:
        rpi = rc_ptr->make_power(_("荒馬ならし", "Rodeo"));
        rpi.min_level = 10;
        rpi.cost = 0;
        rpi.stat = A_STR;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_BERSERKER:
        rpi = rc_ptr->make_power(_("帰還", "Recall"));
        rpi.min_level = 10;
        rpi.cost = 10;
        rpi.stat = A_DEX;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_MIRROR_MASTER:
        rpi = rc_ptr->make_power(_("鏡割り", "Break Mirrors"));
        rpi.min_level = 1;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        rpi = rc_ptr->make_power(_("静水", "Mirror Concentration"));
        rpi.info = format("%s%d", KWD_MANA, 3 + rc_ptr->lvl / 20);
        rpi.min_level = 30;
        rpi.cost = 0;
        rpi.stat = A_INT;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, -4);
        break;
    case CLASS_SMITH:
        rpi = rc_ptr->make_power(_("目利き", "Judgment"));
        rpi.min_level = 5;
        rpi.cost = 15;
        rpi.stat = A_INT;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_NINJA:
        rpi = rc_ptr->make_power(_("速駆け", "Quick Walk"));
        rpi.min_level = 20;
        rpi.cost = 0;
        rpi.stat = A_DEX;
        rpi.fail = 0;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);
        break;
    case CLASS_ELEMENTALIST:
        rpi = rc_ptr->make_power(_("明鏡止水", "Clear Mind"));
        rpi.info = format("%s%d", KWD_MANA, 3 + rc_ptr->lvl / 20);
        rpi.min_level = 15;
        rpi.cost = 0;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_0);

        switch_element_racial(creature_ptr, rc_ptr);
        break;
    default:
        break;
    }
}
