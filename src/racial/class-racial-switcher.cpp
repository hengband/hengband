#include "racial/class-racial-switcher.h"
#include "racial/racial-util.h"
#include "realm/realm-names-table.h"
#include "realm/realm-types.h"

void switch_class_racial(player_type *creature_ptr, rc_type *rc_ptr)
{
    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("剣の舞い", "Sword Dancing"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 40;
        rc_ptr->power_desc[rc_ptr->num].cost = 75;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 35;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_HIGH_MAGE:
        if (creature_ptr->realm1 == REALM_HEX) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("詠唱をやめる", "Stop spell casting"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 1;
            rc_ptr->power_desc[rc_ptr->num].cost = 0;
            rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
            rc_ptr->power_desc[rc_ptr->num].fail = 0;
            rc_ptr->power_desc[rc_ptr->num++].number = -3;
            break;
        }
        /* Fall through */
    case CLASS_MAGE:
    case CLASS_SORCERER:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("魔力食い", "Eat Magic"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 25;
        rc_ptr->power_desc[rc_ptr->num].cost = 1;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 25;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_PRIEST:
        if (is_good_realm(creature_ptr->realm1)) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("武器祝福", "Bless Weapon"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 35;
            rc_ptr->power_desc[rc_ptr->num].cost = 70;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 50;
            rc_ptr->power_desc[rc_ptr->num++].number = -3;
        } else {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("召魂", "Evocation"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 42;
            rc_ptr->power_desc[rc_ptr->num].cost = 40;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 35;
            rc_ptr->power_desc[rc_ptr->num++].number = -3;
        }

        break;
    case CLASS_ROGUE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ヒット＆アウェイ", "Hit and Away"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 8;
        rc_ptr->power_desc[rc_ptr->num].cost = 12;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 14;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_RANGER:
    case CLASS_SNIPER:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("モンスター調査", "Probe Monster"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 15;
        rc_ptr->power_desc[rc_ptr->num].cost = 20;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 12;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_PALADIN:
        if (is_good_realm(creature_ptr->realm1)) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ホーリー・ランス", "Holy Lance"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 30;
            rc_ptr->power_desc[rc_ptr->num].cost = 30;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 30;
            rc_ptr->power_desc[rc_ptr->num++].number = -3;
        } else {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ヘル・ランス", "Hell Lance"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 30;
            rc_ptr->power_desc[rc_ptr->num].cost = 30;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 30;
            rc_ptr->power_desc[rc_ptr->num++].number = -3;
        }

        break;
    case CLASS_WARRIOR_MAGE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("変換: ＨＰ→ＭＰ", "Convert HP to SP"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 25;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 10;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;

        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("変換: ＭＰ→ＨＰ", "Convert SP to HP"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 25;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 10;
        rc_ptr->power_desc[rc_ptr->num++].number = -4;
        break;
    case CLASS_CHAOS_WARRIOR:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("幻惑の光", "Confusing Light"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 40;
        rc_ptr->power_desc[rc_ptr->num].cost = 50;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 25;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_MONK:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("構える", "Assume a Stance"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 25;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;

        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("百裂拳", "Double Attack"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 30;
        rc_ptr->power_desc[rc_ptr->num].cost = 30;
        rc_ptr->power_desc[rc_ptr->num].stat = A_STR;
        rc_ptr->power_desc[rc_ptr->num].fail = 20;
        rc_ptr->power_desc[rc_ptr->num++].number = -4;
        break;
    case CLASS_MINDCRAFTER:
    case CLASS_FORCETRAINER:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("明鏡止水", "Clear Mind"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 15;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
        rc_ptr->power_desc[rc_ptr->num].fail = 10;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_TOURIST:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("写真撮影", "Take a Photograph"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 1;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;

        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("真・鑑定", "Identify True"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 25;
        rc_ptr->power_desc[rc_ptr->num].cost = 20;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 20;
        rc_ptr->power_desc[rc_ptr->num++].number = -4;
        break;
    case CLASS_IMITATOR:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("倍返し", "Double Revenge"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 30;
        rc_ptr->power_desc[rc_ptr->num].cost = 100;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 30;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_BEASTMASTER:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("生物支配", "Dominate a Living Thing"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 1;
        rc_ptr->power_desc[rc_ptr->num].cost = (creature_ptr->lev + 3) / 4;
        rc_ptr->power_desc[rc_ptr->num].stat = A_CHR;
        rc_ptr->power_desc[rc_ptr->num].fail = 10;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;

        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("真・生物支配", "Dominate Living Things"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 30;
        rc_ptr->power_desc[rc_ptr->num].cost = (creature_ptr->lev + 20) / 2;
        rc_ptr->power_desc[rc_ptr->num].stat = A_CHR;
        rc_ptr->power_desc[rc_ptr->num].fail = 10;
        rc_ptr->power_desc[rc_ptr->num++].number = -4;
        break;
    case CLASS_ARCHER:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("弾/矢の製造", "Create Ammo"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 1;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_MAGIC_EATER:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("魔力の取り込み", "Absorb Magic"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 1;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;

        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("強力発動", "Powerful Activation"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 10;
        rc_ptr->power_desc[rc_ptr->num].cost = 10 + (rc_ptr->lvl - 10) / 2;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -4;
        break;
    case CLASS_BARD:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("歌を止める", "Stop Singing"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 1;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_CHR;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_RED_MAGE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("連続魔", "Double Magic"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 48;
        rc_ptr->power_desc[rc_ptr->num].cost = 20;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_SAMURAI:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("気合いため", "Concentration"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 1;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;

        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("型", "Assume a Stance"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 25;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -4;
        break;
    case CLASS_BLUE_MAGE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ラーニング", "Learning"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 1;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_CAVALRY:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("荒馬ならし", "Rodeo"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 10;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_STR;
        rc_ptr->power_desc[rc_ptr->num].fail = 10;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_BERSERKER:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("帰還", "Recall"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 10;
        rc_ptr->power_desc[rc_ptr->num].cost = 10;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 20;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_MIRROR_MASTER:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("鏡割り", "Break Mirrors"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 1;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;

        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("静水", "Mirror Concentration"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 30;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 20;
        rc_ptr->power_desc[rc_ptr->num++].number = -4;
        break;
    case CLASS_SMITH:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("目利き", "Judgment"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 5;
        rc_ptr->power_desc[rc_ptr->num].cost = 15;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 20;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    case CLASS_NINJA:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("速駆け", "Quick Walk"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 20;
        rc_ptr->power_desc[rc_ptr->num].cost = 0;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 0;
        rc_ptr->power_desc[rc_ptr->num++].number = -3;
        break;
    default:
        strcpy(rc_ptr->power_desc[0].racial_name, _("(なし)", "(none)"));
        break;
    }
}
