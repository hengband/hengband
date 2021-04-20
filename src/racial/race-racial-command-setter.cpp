#include "racial/race-racial-command-setter.h"
#include "player/player-race.h"
#include "racial/racial-util.h"

void set_mimic_racial_command(player_type *creature_ptr, rc_type *rc_ptr)
{
    rpi_type rpi;
    switch (creature_ptr->mimic_form) {
    case MIMIC_DEMON:
    case MIMIC_DEMON_LORD:
        rpi = rc_ptr->make_power(format(_("地獄/火炎のブレス (ダメージ %d)", "Nether or Fire Breath (dam %d)"), rc_ptr->lvl * 3));
        rpi.min_level = 15;
        rpi.cost = 10 + rc_ptr->lvl / 3;
        rpi.stat = A_CON;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case MIMIC_VAMPIRE:
        rpi = rc_ptr->make_power(_("吸血", "Vampiric Drain"));
        rpi.min_level = 2;
        rpi.cost = 1 + (rc_ptr->lvl / 3);
        rpi.stat = A_CON;
        rpi.fail = 9;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    }
}

void set_race_racial_command(player_type *creature_ptr, rc_type *rc_ptr)
{
    rpi_type rpi;
    switch (creature_ptr->prace) {
    case RACE_DWARF:
        rpi = rc_ptr->make_power(_("ドアと罠 感知", "Detect Doors+Traps"));
        rpi.min_level = 5;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_NIBELUNG:
        rpi = rc_ptr->make_power(_("ドアと罠 感知", "Detect Doors+Traps"));
        rpi.min_level = 10;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_HOBBIT:
        rpi = rc_ptr->make_power(_("食糧生成", "Create Food"));
        rpi.min_level = 15;
        rpi.cost = 10;
        rpi.stat = A_INT;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_GNOME:
        rpi = rc_ptr->make_power(_("ショート・テレポート", "Blink"));
        rpi.min_level = 5;
        rpi.cost = 5;
        rpi.stat = A_INT;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_HALF_ORC:
        rpi = rc_ptr->make_power(_("恐怖除去", "Remove Fear"));
        rpi.min_level = 3;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = rc_ptr->is_warrior ? 5 : 10;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_HALF_TROLL:
        rpi = rc_ptr->make_power(_("狂戦士化", "Berserk"));
        rpi.min_level = 10;
        rpi.cost = 12;
        rpi.stat = A_STR;
        rpi.fail = rc_ptr->is_warrior ? 6 : 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_BARBARIAN:
        rpi = rc_ptr->make_power(_("狂戦士化", "Berserk"));
        rpi.min_level = 8;
        rpi.cost = 10;
        rpi.stat = A_STR;
        rpi.fail = rc_ptr->is_warrior ? 6 : 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_AMBERITE:
        rpi = rc_ptr->make_power(_("シャドウ・シフト", "Shadow Shifting"));
        rpi.min_level = 30;
        rpi.cost = 50;
        rpi.stat = A_INT;
        rpi.fail = 50;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);

        rpi = rc_ptr->make_power(_("パターン・ウォーク", "Pattern Mindwalking"));
        rpi.min_level = 40;
        rpi.cost = 75;
        rpi.stat = A_WIS;
        rpi.fail = 50;
        rc_ptr->add_power(rpi, RC_IDX_RACE_1);
        break;
    case RACE_HALF_OGRE:
        rpi = rc_ptr->make_power(_("爆発のルーン", "Explosive Rune"));
        rpi.min_level = 25;
        rpi.cost = 35;
        rpi.stat = A_INT;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_HALF_GIANT:
        rpi = rc_ptr->make_power(_("岩石溶解", "Stone to Mud"));
        rpi.min_level = 20;
        rpi.cost = 10;
        rpi.stat = A_STR;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_HALF_TITAN:
        rpi = rc_ptr->make_power(_("スキャン・モンスター", "Probing"));
        rpi.min_level = 15;
        rpi.cost = 10;
        rpi.stat = A_INT;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_CYCLOPS:
        rpi = rc_ptr->make_power(format(_("岩石投げ（ダメージ %d）", "Throw Boulder (dam %d)"), (3 * rc_ptr->lvl) / 2));
        rpi.min_level = 20;
        rpi.cost = 15;
        rpi.stat = A_STR;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_YEEK:
        rpi = rc_ptr->make_power(_("モンスター恐慌", "Scare Monster"));
        rpi.min_level = 15;
        rpi.cost = 15;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_SPECTRE:
        rpi = rc_ptr->make_power(_("モンスター恐慌", "Scare Monster"));
        rpi.min_level = 4;
        rpi.cost = 6;
        rpi.stat = A_INT;
        rpi.fail = 3;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_KLACKON:
        rpi = rc_ptr->make_power(format(_("酸の唾 (ダメージ %d)", "Spit Acid (dam %d)"), rc_ptr->lvl));
        rpi.min_level = 9;
        rpi.cost = 9;
        rpi.stat = A_DEX;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_KOBOLD:
        rpi = rc_ptr->make_power(format(_("毒のダーツ (ダメージ %d)", "Poison Dart (dam %d)"), rc_ptr->lvl));
        rpi.min_level = 12;
        rpi.cost = 8;
        rpi.stat = A_DEX;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_DARK_ELF:
        rpi = rc_ptr->make_power(format(_("マジック・ミサイル (ダメージ %dd%d)", "Magic Missile (dm %dd%d)"), 3 + ((rc_ptr->lvl - 1) / 5), 4));
        rpi.min_level = 2;
        rpi.cost = 2;
        rpi.stat = A_INT;
        rpi.fail = 9;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_DRACONIAN:
        rpi = rc_ptr->make_power(format(_("ブレス (ダメージ %d)", "Breath Weapon (dam %d)"), rc_ptr->lvl * 2));
        rpi.min_level = 1;
        rpi.cost = rc_ptr->lvl;
        rpi.stat = A_CON;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_MIND_FLAYER:
        rpi = rc_ptr->make_power(format(_("精神攻撃 (ダメージ %d)", "Mind Blast (dam %d)"), rc_ptr->lvl));
        rpi.min_level = 15;
        rpi.cost = 12;
        rpi.stat = A_INT;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_IMP:
        rpi = rc_ptr->make_power(format(_("ファイア・ボルト/ボール (ダメージ %d)", "Fire Bolt/Ball (dam %d)"), rc_ptr->lvl));
        rpi.min_level = 9;
        rpi.cost = 15;
        rpi.stat = A_WIS;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_GOLEM:
        rpi = rc_ptr->make_power(_("肌石化 (期間 1d20+30)", "Stone Skin (dur 1d20+30)"));
        rpi.min_level = 20;
        rpi.cost = 15;
        rpi.stat = A_CON;
        rpi.fail = 8;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_SKELETON:
    case RACE_ZOMBIE:
        rpi = rc_ptr->make_power(_("経験値復活", "Restore Experience"));
        rpi.min_level = 30;
        rpi.cost = 30;
        rpi.stat = A_WIS;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_VAMPIRE:
        rpi = rc_ptr->make_power(_("吸血", "Vampiric Drain"));
        rpi.min_level = 2;
        rpi.cost = 1 + (rc_ptr->lvl / 3);
        rpi.stat = A_CON;
        rpi.fail = 9;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_SPRITE:
        rpi = rc_ptr->make_power(_("眠り粉", "Sleeping Dust"));
        rpi.min_level = 12;
        rpi.cost = 12;
        rpi.stat = A_INT;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_BALROG:
        rpi = rc_ptr->make_power(format(_("地獄/火炎のブレス (ダメージ %d)", "Nether or Fire Breath (dam %d)"), rc_ptr->lvl * 3));
        rpi.min_level = 15;
        rpi.cost = 10 + rc_ptr->lvl / 3;
        rpi.stat = A_CON;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_KUTAR:
        rpi = rc_ptr->make_power(_("横に伸びる", "Expand Horizontally (dur 30+1d20)"));
        rpi.min_level = 20;
        rpi.cost = 15;
        rpi.stat = A_CHR;
        rpi.fail = 8;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case RACE_ANDROID:
        if (creature_ptr->lev < 10) {
            rpi = rc_ptr->make_power(_("レイガン", "Ray Gun"));
            rpi.min_level = 1;
            rpi.cost = 7;
            rpi.fail = 8;
        } else if (creature_ptr->lev < 25) {
            rpi = rc_ptr->make_power(_("ブラスター", "Blaster"));
            rpi.min_level = 10;
            rpi.cost = 13;
            rpi.fail = 10;
        } else if (creature_ptr->lev < 35) {
            rpi = rc_ptr->make_power(_("バズーカ", "Bazooka"));
            rpi.min_level = 25;
            rpi.cost = 26;
            rpi.fail = 12;
        } else if (creature_ptr->lev < 45) {
            rpi = rc_ptr->make_power(_("ビームキャノン", "Beam Cannon"));
            rpi.min_level = 35;
            rpi.cost = 40;
            rpi.fail = 15;
        } else {
            rpi = rc_ptr->make_power(_("ロケット", "Rocket"));
            rpi.min_level = 45;
            rpi.cost = 60;
            rpi.fail = 18;
        }
        rpi.stat = A_STR;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    default:
        break;
    }
}
