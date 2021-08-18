#include "racial/race-racial-command-setter.h"
#include "cmd-action/cmd-spell.h"
#include "player/player-race.h"
#include "racial/racial-util.h"
#include "system/player-type-definition.h"

void set_mimic_racial_command(player_type *creature_ptr, rc_type *rc_ptr)
{
    rpi_type rpi;
    switch (creature_ptr->mimic_form) {
    case MIMIC_DEMON:
    case MIMIC_DEMON_LORD:
        rpi = rpi_type(_("地獄/火炎のブレス", "Nether or Fire Breath"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 3);
        rpi.text = _("火炎または地獄のブレスを放つ。", "Fires a breath of fire or nether.");
        rpi.min_level = 15;
        rpi.cost = 10 + rc_ptr->lvl / 3;
        rpi.stat = A_CON;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case MIMIC_VAMPIRE:
        rpi = rpi_type(_("吸血", "Vampiric Drain"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.text = _("隣接したモンスター1体から生命力を吸い取る。吸い取った生命力によって満腹度があがる。",
            "Drains and transfers HP from a monster near by you. You will also gain nutritional sustenance from this.");
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
    case player_race_type::DWARF:
        rpi = rpi_type(_("ドアと罠 感知", "Detect Doors+Traps"));
        rpi.text = _("近くの全ての扉と罠、階段を感知する。", "Detects traps, doors, and stairs in your vicinity.");
        rpi.min_level = 5;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::NIBELUNG:
        rpi = rpi_type(_("ドアと罠 感知", "Detect Doors+Traps"));
        rpi.text = _("近くの全ての扉と罠、階段を感知する。", "Detects traps, doors, and stairs in your vicinity.");
        rpi.min_level = 10;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::HOBBIT:
        rpi = rpi_type(_("食糧生成", "Create Food"));
        rpi.text = _("食料を一つ作り出す。", "Produces a Ration of Food.");
        rpi.min_level = 15;
        rpi.cost = 10;
        rpi.stat = A_INT;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::GNOME:
        rpi = rpi_type(_("ショート・テレポート", "Blink"));
        rpi.info = format("%s%d", KWD_SPHERE, 10);
        rpi.text = _("近距離のテレポートをする。", "Teleports you a short distance.");
        rpi.min_level = 5;
        rpi.cost = 5;
        rpi.stat = A_INT;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::HALF_ORC:
        rpi = rpi_type(_("恐怖除去", "Remove Fear"));
        rpi.text = _("恐怖を取り除く。", "Removes fear.");
        rpi.min_level = 3;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = rc_ptr->is_warrior ? 5 : 10;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::HALF_TROLL:
        rpi = rpi_type(_("狂戦士化", "Berserk"));
        rpi.info = format("%s%d+d%d", KWD_DURATION, 10, rc_ptr->lvl);
        rpi.text = _("狂戦士化し、恐怖を除去する。防御力が少し低下する。", "Gives a bonus to hit and HP, immunity to fear for a while. But decreases AC.");
        rpi.min_level = 10;
        rpi.cost = 12;
        rpi.stat = A_STR;
        rpi.fail = rc_ptr->is_warrior ? 6 : 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::BARBARIAN:
        rpi = rpi_type(_("狂戦士化", "Berserk"));
        rpi.info = format("%s%d+d%d", KWD_DURATION, 10, rc_ptr->lvl);
        rpi.text = _("狂戦士化し、恐怖を除去する。防御力が少し低下する。", "Gives a bonus to hit and HP, immunity to fear for a while. But decreases AC.");
        rpi.min_level = 8;
        rpi.cost = 10;
        rpi.stat = A_STR;
        rpi.fail = rc_ptr->is_warrior ? 6 : 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::AMBERITE:
        rpi = rpi_type(_("シャドウ・シフト", "Shadow Shifting"));
        rpi.text = _("現在の階を再構成する。", "Recreates current dungeon level.");
        rpi.min_level = 30;
        rpi.cost = 50;
        rpi.stat = A_INT;
        rpi.fail = 50;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);

        rpi = rpi_type(_("パターン・ウォーク", "Pattern Mindwalking"));
        rpi.text = _("すべてのステータスと経験値を回復する。", "Restores all stats and experience.");
        rpi.min_level = 40;
        rpi.cost = 75;
        rpi.stat = A_WIS;
        rpi.fail = 50;
        rc_ptr->add_power(rpi, RC_IDX_RACE_1);
        break;
    case player_race_type::HALF_OGRE:
        rpi = rpi_type(_("爆発のルーン", "Explosive Rune"));
        rpi.text = _("自分のいる床の上に、モンスターが上を通ろうとすると爆発するルーンを描く。",
            "Sets a rune on the floor beneath you which exprodes if a monster through upon it. Monsters can try to disarm it.");
        rpi.min_level = 25;
        rpi.cost = 35;
        rpi.stat = A_INT;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::HALF_GIANT:
        rpi = rpi_type(_("岩石溶解", "Stone to Mud"));
        rpi.text = _("壁を溶かして床にする。", "Turns one rock square to mud.");
        rpi.min_level = 20;
        rpi.cost = 10;
        rpi.stat = A_STR;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::HALF_TITAN:
        rpi = rpi_type(_("スキャン・モンスター", "Probing"));
        rpi.text = _("モンスターの属性、残り体力、最大体力、スピード、正体を知る。", "Probes all monsters' alignment, HP, speed and their true character.");
        rpi.min_level = 15;
        rpi.cost = 10;
        rpi.stat = A_INT;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::CYCLOPS:
        rpi = rpi_type(_("岩石投げ", "Throw Boulder"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 3 / 2);
        rpi.text = _("弱い魔法のボールを放つ", "Fires a weak boll of magic.");
        rpi.min_level = 20;
        rpi.cost = 15;
        rpi.stat = A_STR;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::YEEK:
        rpi = rpi_type(_("モンスター恐慌", "Scare Monster"));
        rpi.text = _("モンスター1体を恐怖させる。抵抗されると無効。", "Attempts to scare a monster.");
        rpi.min_level = 15;
        rpi.cost = 15;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::SPECTRE:
        rpi = rpi_type(_("モンスター恐慌", "Scare Monster"));
        rpi.text = _("モンスター1体を恐怖させる。抵抗されると無効。", "Attempts to scare a monster.");
        rpi.min_level = 4;
        rpi.cost = 6;
        rpi.stat = A_INT;
        rpi.fail = 3;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::KLACKON:
        rpi = rpi_type(_("酸の唾", "Spit Acid"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl);
        if (rc_ptr->lvl >= 25)
            rpi.text = _("酸のボールを放つ", "Fires a boll of acid.");
        else
            rpi.text = _("酸の矢を放つ", "Fires a bolt of acid.");
        rpi.min_level = 9;
        rpi.cost = 9;
        rpi.stat = A_DEX;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::KOBOLD:
        rpi = rpi_type(_("毒のダーツ", "Poison Dart"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl);
        rpi.text = _("毒の矢を放つ", "Fires a bolt of poison.");
        rpi.min_level = 12;
        rpi.cost = 8;
        rpi.stat = A_DEX;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::DARK_ELF:
        rpi = rpi_type(_("マジック・ミサイル", "Magic Missile"));
        rpi.info = format("%s%dd%d", KWD_DAM, 3 + ((rc_ptr->lvl - 1) / 5), 4);
        rpi.text = _("弱い魔法の矢を放つ。", "Fires a weak bolt of magic.");
        rpi.min_level = 2;
        rpi.cost = 2;
        rpi.stat = A_INT;
        rpi.fail = 9;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::DRACONIAN:
        rpi = rpi_type(_("ブレス", "Breath Weapon"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.text = _("元素のブレスを放つ", "Fires a breath of an element.");
        rpi.min_level = 1;
        rpi.cost = rc_ptr->lvl;
        rpi.stat = A_CON;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::MIND_FLAYER:
        rpi = rpi_type(_("精神攻撃", "Mind Blast"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl);
        rpi.text = _("モンスター1体に精神攻撃を行う。", "Deals a PSI damage to a monster.");
        rpi.min_level = 15;
        rpi.cost = 12;
        rpi.stat = A_INT;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::IMP:
        if (rc_ptr->lvl > 30) {
            rpi = rpi_type(_("ファイア・ボール", "Fire Ball"));
            rpi.text = _("火炎のボールを放つ。", "Fires a ball of fire.");
        } else {
            rpi = rpi_type(_("ファイア・ボルト", "Fire Bolt"));
            rpi.text = _("火炎の矢を放つ。", "Fires a bolt of fire.");
        }
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl);
        rpi.min_level = 9;
        rpi.cost = 15;
        rpi.stat = A_WIS;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::GOLEM:
        rpi = rpi_type(_("肌石化", "Stone Skin"));
        rpi.info = format("%s%d+d%d", KWD_DURATION, 30, 20);
        rpi.text = _("一定期間防御力を高める。", "Increases your AC temporary");
        rpi.min_level = 20;
        rpi.cost = 15;
        rpi.stat = A_CON;
        rpi.fail = 8;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::SKELETON:
    case player_race_type::ZOMBIE:
        rpi = rpi_type(_("経験値復活", "Restore Experience"));
        rpi.text = _("経験値を回復する。", "Restores experience.");
        rpi.min_level = 30;
        rpi.cost = 30;
        rpi.stat = A_WIS;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::VAMPIRE:
        rpi = rpi_type(_("吸血", "Vampiric Drain"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.text = _("隣接したモンスター1体から生命力を吸い取る。吸い取った生命力によって満腹度があがる。",
            "Drains and transfers HP from a monster near by you. You will also gain nutritional sustenance from this.");
        rpi.min_level = 2;
        rpi.cost = 1 + (rc_ptr->lvl / 3);
        rpi.stat = A_CON;
        rpi.fail = 9;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::SPRITE:
        rpi = rpi_type(_("眠り粉", "Sleeping Dust"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl);
        rpi.text = _("モンスター1体を眠らせる。抵抗されると無効。", "Attempts to put a monster to sleep.");
        rpi.min_level = 12;
        rpi.cost = 12;
        rpi.stat = A_INT;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::BALROG:
        rpi = rpi_type(_("地獄/火炎のブレス", "Nether or Fire Breath"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 3);
        rpi.text = _("火炎または地獄のブレスを放つ。", "Fires a breath of fire or nether.");
        rpi.min_level = 15;
        rpi.cost = 10 + rc_ptr->lvl / 3;
        rpi.stat = A_CON;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::KUTAR:
        rpi = rpi_type(_("横に伸びる", "Expand Horizontally"));
        rpi.info = format("%s%d+d%d", KWD_DURATION, 30, 20);
        rpi.text = _("横に伸びて防御力を高める。魔法防御力は低下する。",
            "Expands your body horizontally then increases AC, but decreases your magic resistance for curses.");
        rpi.min_level = 20;
        rpi.cost = 15;
        rpi.stat = A_CHR;
        rpi.fail = 8;
        rc_ptr->add_power(rpi, RC_IDX_RACE_0);
        break;
    case player_race_type::ANDROID:
        if (creature_ptr->lev < 10) {
            rpi = rpi_type(_("レイガン", "Ray Gun"));
            rpi.info = format("%s%d", KWD_DAM, (rc_ptr->lvl + 1) / 2);
            rpi.text = _("弱い魔法の矢を放つ。", "Fires a weak bolt of magic.");
            rpi.min_level = 1;
            rpi.cost = 7;
            rpi.fail = 8;
        } else if (creature_ptr->lev < 25) {
            rpi = rpi_type(_("ブラスター", "Blaster"));
            rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl);
            rpi.text = _("弱い魔法の矢を放つ。", "Fires a weak bolt of magic.");
            rpi.min_level = 10;
            rpi.cost = 13;
            rpi.fail = 10;
        } else if (creature_ptr->lev < 35) {
            rpi = rpi_type(_("バズーカ", "Bazooka"));
            rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
            rpi.text = _("弱い魔法のボールを放つ。", "Fires a weak ball of magic.");
            rpi.min_level = 25;
            rpi.cost = 26;
            rpi.fail = 12;
        } else if (creature_ptr->lev < 45) {
            rpi = rpi_type(_("ビームキャノン", "Beam Cannon"));
            rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
            rpi.text = _("弱い魔法のビームを放つ。", "Fires a beam bolt of magic.");
            rpi.min_level = 35;
            rpi.cost = 40;
            rpi.fail = 15;
        } else {
            rpi = rpi_type(_("ロケット", "Rocket"));
            rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 5);
            rpi.text = _("ロケットを放つ。", "Fires a magic rocket.");
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
