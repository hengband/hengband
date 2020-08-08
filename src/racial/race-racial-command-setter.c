#include "racial/race-racial-command-setter.h"
#include "player/player-race.h"
#include "racial/racial-util.h"

void set_mimic_racial_command(player_type *creature_ptr, rc_type *rc_ptr)
{
    switch (creature_ptr->mimic_form) {
    case MIMIC_DEMON:
    case MIMIC_DEMON_LORD:
        sprintf(rc_ptr->power_desc[rc_ptr->num].racial_name, _("地獄/火炎のブレス (ダメージ %d)", "Nether or Fire Breath (dam %d)"), rc_ptr->lvl * 3);
        rc_ptr->power_desc[rc_ptr->num].min_level = 15;
        rc_ptr->power_desc[rc_ptr->num].cost = 10 + rc_ptr->lvl / 3;
        rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
        rc_ptr->power_desc[rc_ptr->num].fail = 20;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case MIMIC_VAMPIRE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("吸血", "Vampiric Drain"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 2;
        rc_ptr->power_desc[rc_ptr->num].cost = 1 + (rc_ptr->lvl / 3);
        rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
        rc_ptr->power_desc[rc_ptr->num].fail = 9;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    }
}

void set_race_racial_command(player_type *creature_ptr, rc_type *rc_ptr)
{
    switch (creature_ptr->prace) {
    case RACE_DWARF:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ドアと罠 感知", "Detect Doors+Traps"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 5;
        rc_ptr->power_desc[rc_ptr->num].cost = 5;
        rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
        rc_ptr->power_desc[rc_ptr->num].fail = 12;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_NIBELUNG:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ドアと罠 感知", "Detect Doors+Traps"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 10;
        rc_ptr->power_desc[rc_ptr->num].cost = 5;
        rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
        rc_ptr->power_desc[rc_ptr->num].fail = 10;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_HOBBIT:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("食糧生成", "Create Food"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 15;
        rc_ptr->power_desc[rc_ptr->num].cost = 10;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 10;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_GNOME:
        sprintf(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ショート・テレポート", "Blink"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 5;
        rc_ptr->power_desc[rc_ptr->num].cost = 5;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 12;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_HALF_ORC:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("恐怖除去", "Remove Fear"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 3;
        rc_ptr->power_desc[rc_ptr->num].cost = 5;
        rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
        rc_ptr->power_desc[rc_ptr->num].fail = rc_ptr->is_warrior ? 5 : 10;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_HALF_TROLL:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("狂戦士化", "Berserk"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 10;
        rc_ptr->power_desc[rc_ptr->num].cost = 12;
        rc_ptr->power_desc[rc_ptr->num].stat = A_STR;
        rc_ptr->power_desc[rc_ptr->num].fail = rc_ptr->is_warrior ? 6 : 12;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_BARBARIAN:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("狂戦士化", "Berserk"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 8;
        rc_ptr->power_desc[rc_ptr->num].cost = 10;
        rc_ptr->power_desc[rc_ptr->num].stat = A_STR;
        rc_ptr->power_desc[rc_ptr->num].fail = rc_ptr->is_warrior ? 6 : 12;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_AMBERITE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("シャドウ・シフト", "Shadow Shifting"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 30;
        rc_ptr->power_desc[rc_ptr->num].cost = 50;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 50;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;

        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("パターン・ウォーク", "Pattern Mindwalking"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 40;
        rc_ptr->power_desc[rc_ptr->num].cost = 75;
        rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
        rc_ptr->power_desc[rc_ptr->num].fail = 50;
        rc_ptr->power_desc[rc_ptr->num++].number = -2;
        break;
    case RACE_HALF_OGRE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("爆発のルーン", "Explosive Rune"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 25;
        rc_ptr->power_desc[rc_ptr->num].cost = 35;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 15;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_HALF_GIANT:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("岩石溶解", "Stone to Mud"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 20;
        rc_ptr->power_desc[rc_ptr->num].cost = 10;
        rc_ptr->power_desc[rc_ptr->num].stat = A_STR;
        rc_ptr->power_desc[rc_ptr->num].fail = 12;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_HALF_TITAN:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("スキャン・モンスター", "Probing"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 15;
        rc_ptr->power_desc[rc_ptr->num].cost = 10;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 12;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_CYCLOPS:
        sprintf(rc_ptr->power_desc[rc_ptr->num].racial_name, _("岩石投げ（ダメージ %d）", "Throw Boulder (dam %d)"), (3 * rc_ptr->lvl) / 2);
        rc_ptr->power_desc[rc_ptr->num].min_level = 20;
        rc_ptr->power_desc[rc_ptr->num].cost = 15;
        rc_ptr->power_desc[rc_ptr->num].stat = A_STR;
        rc_ptr->power_desc[rc_ptr->num].fail = 12;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_YEEK:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("モンスター恐慌", "Scare Monster"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 15;
        rc_ptr->power_desc[rc_ptr->num].cost = 15;
        rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
        rc_ptr->power_desc[rc_ptr->num].fail = 10;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_SPECTRE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("モンスター恐慌", "Scare Monster"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 4;
        rc_ptr->power_desc[rc_ptr->num].cost = 6;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 3;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_KLACKON:
        sprintf(rc_ptr->power_desc[rc_ptr->num].racial_name, _("酸の唾 (ダメージ %d)", "Spit Acid (dam %d)"), rc_ptr->lvl);
        rc_ptr->power_desc[rc_ptr->num].min_level = 9;
        rc_ptr->power_desc[rc_ptr->num].cost = 9;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 14;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_KOBOLD:
        sprintf(rc_ptr->power_desc[rc_ptr->num].racial_name, _("毒のダーツ (ダメージ %d)", "Poison Dart (dam %d)"), rc_ptr->lvl);
        rc_ptr->power_desc[rc_ptr->num].min_level = 12;
        rc_ptr->power_desc[rc_ptr->num].cost = 8;
        rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
        rc_ptr->power_desc[rc_ptr->num].fail = 14;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_DARK_ELF:
        sprintf(
            rc_ptr->power_desc[rc_ptr->num].racial_name, _("マジック・ミサイル (ダメージ %dd%d)", "Magic Missile (dm %dd%d)"), 3 + ((rc_ptr->lvl - 1) / 5), 4);
        rc_ptr->power_desc[rc_ptr->num].min_level = 2;
        rc_ptr->power_desc[rc_ptr->num].cost = 2;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 9;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_DRACONIAN:
        sprintf(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ブレス (ダメージ %d)", "Breath Weapon (dam %d)"), rc_ptr->lvl * 2);
        rc_ptr->power_desc[rc_ptr->num].min_level = 1;
        rc_ptr->power_desc[rc_ptr->num].cost = rc_ptr->lvl;
        rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
        rc_ptr->power_desc[rc_ptr->num].fail = 12;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_MIND_FLAYER:
        sprintf(rc_ptr->power_desc[rc_ptr->num].racial_name, _("精神攻撃 (ダメージ %d)", "Mind Blast (dam %d)"), rc_ptr->lvl);
        rc_ptr->power_desc[rc_ptr->num].min_level = 15;
        rc_ptr->power_desc[rc_ptr->num].cost = 12;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 14;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_IMP:
        sprintf(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ファイア・ボルト/ボール (ダメージ %d)", "Fire Bolt/Ball (dam %d)"), rc_ptr->lvl);
        rc_ptr->power_desc[rc_ptr->num].min_level = 9;
        rc_ptr->power_desc[rc_ptr->num].cost = 15;
        rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
        rc_ptr->power_desc[rc_ptr->num].fail = 15;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_GOLEM:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("肌石化 (期間 1d20+30)", "Stone Skin (dur 1d20+30)"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 20;
        rc_ptr->power_desc[rc_ptr->num].cost = 15;
        rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
        rc_ptr->power_desc[rc_ptr->num].fail = 8;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_SKELETON:
    case RACE_ZOMBIE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("経験値復活", "Restore Experience"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 30;
        rc_ptr->power_desc[rc_ptr->num].cost = 30;
        rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
        rc_ptr->power_desc[rc_ptr->num].fail = 18;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_VAMPIRE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("吸血", "Vampiric Drain"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 2;
        rc_ptr->power_desc[rc_ptr->num].cost = 1 + (rc_ptr->lvl / 3);
        rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
        rc_ptr->power_desc[rc_ptr->num].fail = 9;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_SPRITE:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("眠り粉", "Sleeping Dust"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 12;
        rc_ptr->power_desc[rc_ptr->num].cost = 12;
        rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
        rc_ptr->power_desc[rc_ptr->num].fail = 15;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_BALROG:
        sprintf(rc_ptr->power_desc[rc_ptr->num].racial_name, _("地獄/火炎のブレス (ダメージ %d)", "Nether or Fire Breath (dam %d)"), rc_ptr->lvl * 3);
        rc_ptr->power_desc[rc_ptr->num].min_level = 15;
        rc_ptr->power_desc[rc_ptr->num].cost = 10 + rc_ptr->lvl / 3;
        rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
        rc_ptr->power_desc[rc_ptr->num].fail = 20;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_KUTAR:
        strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("横に伸びる", "Expand Horizontally (dur 30+1d20)"));
        rc_ptr->power_desc[rc_ptr->num].min_level = 20;
        rc_ptr->power_desc[rc_ptr->num].cost = 15;
        rc_ptr->power_desc[rc_ptr->num].stat = A_CHR;
        rc_ptr->power_desc[rc_ptr->num].fail = 8;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    case RACE_ANDROID:
        if (creature_ptr->lev < 10) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("レイガン", "Ray Gun"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 1;
            rc_ptr->power_desc[rc_ptr->num].cost = 7;
            rc_ptr->power_desc[rc_ptr->num].fail = 8;
        } else if (creature_ptr->lev < 25) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ブラスター", "Blaster"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 10;
            rc_ptr->power_desc[rc_ptr->num].cost = 13;
            rc_ptr->power_desc[rc_ptr->num].fail = 10;
        } else if (creature_ptr->lev < 35) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("バズーカ", "Bazooka"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 25;
            rc_ptr->power_desc[rc_ptr->num].cost = 26;
            rc_ptr->power_desc[rc_ptr->num].fail = 12;
        } else if (creature_ptr->lev < 45) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ビームキャノン", "Beam Cannon"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 35;
            rc_ptr->power_desc[rc_ptr->num].cost = 40;
            rc_ptr->power_desc[rc_ptr->num].fail = 15;
        } else {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ロケット", "Rocket"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 45;
            rc_ptr->power_desc[rc_ptr->num].cost = 60;
            rc_ptr->power_desc[rc_ptr->num].fail = 18;
        }
        rc_ptr->power_desc[rc_ptr->num].stat = A_STR;
        rc_ptr->power_desc[rc_ptr->num++].number = -1;
        break;
    default:
        break;
    }
}