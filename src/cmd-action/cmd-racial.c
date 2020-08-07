#include "cmd-action/cmd-racial.h"
#include "action/action-limited.h"
#include "action/mutation-execution.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "game-option/text-display-options.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "mutation/mutation-flag-types.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/player-race.h"
#include "player/special-defense-types.h"
#include "racial/racial-switcher.h"
#include "realm/realm-names-table.h"
#include "realm/realm-types.h"
#include "status/action-setter.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"

/*!
 * @brief レイシャル・パワーコマンドのメインルーチン / Allow user to choose a power (racial / mutation) to activate
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_racial_power(player_type *creature_ptr)
{
    int num;
    COMMAND_CODE command_code = 0;
    int ask = TRUE;
    PLAYER_LEVEL lvl = creature_ptr->lev;
    bool flag;
    bool redraw;
    bool cast = FALSE;
    bool is_warrior = ((creature_ptr->pclass == CLASS_WARRIOR || creature_ptr->pclass == CLASS_BERSERKER) ? TRUE : FALSE);
    char choice;
    char out_val[160];
    int menu_line = use_menu ? 1 : 0;

    if (creature_ptr->wild_mode)
        return;

    rpi_type power_desc[36];
    for (int i = 0; i < 36; i++) {
        strcpy(power_desc[i].racial_name, "");
        power_desc[i].number = 0;
    }

    num = 0;

    if (cmd_limit_confused(creature_ptr)) {
        free_turn(creature_ptr);
        return;
    }

    if (creature_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
        set_action(creature_ptr, ACTION_NONE);

    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR: {
        strcpy(power_desc[num].racial_name, _("剣の舞い", "Sword Dancing"));
        power_desc[num].min_level = 40;
        power_desc[num].cost = 75;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 35;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_HIGH_MAGE:
        if (creature_ptr->realm1 == REALM_HEX) {
            strcpy(power_desc[num].racial_name, _("詠唱をやめる", "Stop spell casting"));
            power_desc[num].min_level = 1;
            power_desc[num].cost = 0;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 0;
            power_desc[num++].number = -3;
            break;
        }
        /* Fall through */
    case CLASS_MAGE:
    case CLASS_SORCERER: {
        strcpy(power_desc[num].racial_name, _("魔力食い", "Eat Magic"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 1;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 25;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_PRIEST: {
        if (is_good_realm(creature_ptr->realm1)) {
            strcpy(power_desc[num].racial_name, _("武器祝福", "Bless Weapon"));
            power_desc[num].min_level = 35;
            power_desc[num].cost = 70;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 50;
            power_desc[num++].number = -3;
        } else {
            strcpy(power_desc[num].racial_name, _("召魂", "Evocation"));
            power_desc[num].min_level = 42;
            power_desc[num].cost = 40;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 35;
            power_desc[num++].number = -3;
        }
        break;
    }
    case CLASS_ROGUE: {
        strcpy(power_desc[num].racial_name, _("ヒット＆アウェイ", "Hit and Away"));
        power_desc[num].min_level = 8;
        power_desc[num].cost = 12;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 14;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_RANGER:
    case CLASS_SNIPER: {
        strcpy(power_desc[num].racial_name, _("モンスター調査", "Probe Monster"));
        power_desc[num].min_level = 15;
        power_desc[num].cost = 20;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 12;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_PALADIN: {
        if (is_good_realm(creature_ptr->realm1)) {
            strcpy(power_desc[num].racial_name, _("ホーリー・ランス", "Holy Lance"));
            power_desc[num].min_level = 30;
            power_desc[num].cost = 30;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 30;
            power_desc[num++].number = -3;
        } else {
            strcpy(power_desc[num].racial_name, _("ヘル・ランス", "Hell Lance"));
            power_desc[num].min_level = 30;
            power_desc[num].cost = 30;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 30;
            power_desc[num++].number = -3;
        }
        break;
    }
    case CLASS_WARRIOR_MAGE: {
        strcpy(power_desc[num].racial_name, _("変換: ＨＰ→ＭＰ", "Convert HP to SP"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 10;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("変換: ＭＰ→ＨＰ", "Convert SP to HP"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 10;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_CHAOS_WARRIOR: {
        strcpy(power_desc[num].racial_name, _("幻惑の光", "Confusing Light"));
        power_desc[num].min_level = 40;
        power_desc[num].cost = 50;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 25;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_MONK: {
        strcpy(power_desc[num].racial_name, _("構える", "Assume a Stance"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("百裂拳", "Double Attack"));
        power_desc[num].min_level = 30;
        power_desc[num].cost = 30;
        power_desc[num].stat = A_STR;
        power_desc[num].fail = 20;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_MINDCRAFTER:
    case CLASS_FORCETRAINER: {
        strcpy(power_desc[num].racial_name, _("明鏡止水", "Clear Mind"));
        power_desc[num].min_level = 15;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_WIS;
        power_desc[num].fail = 10;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_TOURIST: {
        strcpy(power_desc[num].racial_name, _("写真撮影", "Take a Photograph"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("真・鑑定", "Identify True"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 20;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 20;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_IMITATOR: {
        strcpy(power_desc[num].racial_name, _("倍返し", "Double Revenge"));
        power_desc[num].min_level = 30;
        power_desc[num].cost = 100;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 30;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_BEASTMASTER: {
        strcpy(power_desc[num].racial_name, _("生物支配", "Dominate a Living Thing"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = (creature_ptr->lev + 3) / 4;
        power_desc[num].stat = A_CHR;
        power_desc[num].fail = 10;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("真・生物支配", "Dominate Living Things"));
        power_desc[num].min_level = 30;
        power_desc[num].cost = (creature_ptr->lev + 20) / 2;
        power_desc[num].stat = A_CHR;
        power_desc[num].fail = 10;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_ARCHER: {
        strcpy(power_desc[num].racial_name, _("弾/矢の製造", "Create Ammo"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_MAGIC_EATER: {
        strcpy(power_desc[num].racial_name, _("魔力の取り込み", "Absorb Magic"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("強力発動", "Powerful Activation"));
        power_desc[num].min_level = 10;
        power_desc[num].cost = 10 + (lvl - 10) / 2;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 0;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_BARD: {
        strcpy(power_desc[num].racial_name, _("歌を止める", "Stop Singing"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_CHR;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_RED_MAGE: {
        strcpy(power_desc[num].racial_name, _("連続魔", "Double Magic"));
        power_desc[num].min_level = 48;
        power_desc[num].cost = 20;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_SAMURAI: {
        strcpy(power_desc[num].racial_name, _("気合いため", "Concentration"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_WIS;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("型", "Assume a Stance"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 0;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_BLUE_MAGE: {
        strcpy(power_desc[num].racial_name, _("ラーニング", "Learning"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_CAVALRY: {
        strcpy(power_desc[num].racial_name, _("荒馬ならし", "Rodeo"));
        power_desc[num].min_level = 10;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_STR;
        power_desc[num].fail = 10;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_BERSERKER: {
        strcpy(power_desc[num].racial_name, _("帰還", "Recall"));
        power_desc[num].min_level = 10;
        power_desc[num].cost = 10;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 20;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_MIRROR_MASTER: {
        strcpy(power_desc[num].racial_name, _("鏡割り", "Break Mirrors"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("静水", "Mirror Concentration"));
        power_desc[num].min_level = 30;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 20;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_SMITH: {
        strcpy(power_desc[num].racial_name, _("目利き", "Judgment"));
        power_desc[num].min_level = 5;
        power_desc[num].cost = 15;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 20;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_NINJA: {
        strcpy(power_desc[num].racial_name, _("速駆け", "Quick Walk"));
        power_desc[num].min_level = 20;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;
        break;
    }
    default:
        strcpy(power_desc[0].racial_name, _("(なし)", "(none)"));
    }

    if (creature_ptr->mimic_form) {
        switch (creature_ptr->mimic_form) {
        case MIMIC_DEMON:
        case MIMIC_DEMON_LORD:
            sprintf(power_desc[num].racial_name, _("地獄/火炎のブレス (ダメージ %d)", "Nether or Fire Breath (dam %d)"), lvl * 3);
            power_desc[num].min_level = 15;
            power_desc[num].cost = 10 + lvl / 3;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 20;
            power_desc[num++].number = -1;
            break;
        case MIMIC_VAMPIRE:
            strcpy(power_desc[num].racial_name, _("吸血", "Vampiric Drain"));
            power_desc[num].min_level = 2;
            power_desc[num].cost = 1 + (lvl / 3);
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 9;
            power_desc[num++].number = -1;
            break;
        }
    } else {
        switch (creature_ptr->prace) {
        case RACE_DWARF:
            strcpy(power_desc[num].racial_name, _("ドアと罠 感知", "Detect Doors+Traps"));
            power_desc[num].min_level = 5;
            power_desc[num].cost = 5;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_NIBELUNG:
            strcpy(power_desc[num].racial_name, _("ドアと罠 感知", "Detect Doors+Traps"));
            power_desc[num].min_level = 10;
            power_desc[num].cost = 5;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 10;
            power_desc[num++].number = -1;
            break;
        case RACE_HOBBIT:
            strcpy(power_desc[num].racial_name, _("食糧生成", "Create Food"));
            power_desc[num].min_level = 15;
            power_desc[num].cost = 10;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 10;
            power_desc[num++].number = -1;
            break;
        case RACE_GNOME:
            sprintf(power_desc[num].racial_name, _("ショート・テレポート", "Blink"));
            power_desc[num].min_level = 5;
            power_desc[num].cost = 5;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_HALF_ORC:
            strcpy(power_desc[num].racial_name, _("恐怖除去", "Remove Fear"));
            power_desc[num].min_level = 3;
            power_desc[num].cost = 5;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = is_warrior ? 5 : 10;
            power_desc[num++].number = -1;
            break;
        case RACE_HALF_TROLL:
            strcpy(power_desc[num].racial_name, _("狂戦士化", "Berserk"));
            power_desc[num].min_level = 10;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = is_warrior ? 6 : 12;
            power_desc[num++].number = -1;
            break;
        case RACE_BARBARIAN:
            strcpy(power_desc[num].racial_name, _("狂戦士化", "Berserk"));
            power_desc[num].min_level = 8;
            power_desc[num].cost = 10;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = is_warrior ? 6 : 12;
            power_desc[num++].number = -1;
            break;
        case RACE_AMBERITE:
            strcpy(power_desc[num].racial_name, _("シャドウ・シフト", "Shadow Shifting"));
            power_desc[num].min_level = 30;
            power_desc[num].cost = 50;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 50;
            power_desc[num++].number = -1;

            strcpy(power_desc[num].racial_name, _("パターン・ウォーク", "Pattern Mindwalking"));
            power_desc[num].min_level = 40;
            power_desc[num].cost = 75;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 50;
            power_desc[num++].number = -2;
            break;
        case RACE_HALF_OGRE:
            strcpy(power_desc[num].racial_name, _("爆発のルーン", "Explosive Rune"));
            power_desc[num].min_level = 25;
            power_desc[num].cost = 35;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 15;
            power_desc[num++].number = -1;
            break;
        case RACE_HALF_GIANT:
            strcpy(power_desc[num].racial_name, _("岩石溶解", "Stone to Mud"));
            power_desc[num].min_level = 20;
            power_desc[num].cost = 10;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_HALF_TITAN:
            strcpy(power_desc[num].racial_name, _("スキャン・モンスター", "Probing"));
            power_desc[num].min_level = 15;
            power_desc[num].cost = 10;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_CYCLOPS:
            sprintf(power_desc[num].racial_name, _("岩石投げ（ダメージ %d）", "Throw Boulder (dam %d)"), (3 * lvl) / 2);
            power_desc[num].min_level = 20;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_YEEK:
            strcpy(power_desc[num].racial_name, _("モンスター恐慌", "Scare Monster"));
            power_desc[num].min_level = 15;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 10;
            power_desc[num++].number = -1;
            break;
        case RACE_SPECTRE:
            strcpy(power_desc[num].racial_name, _("モンスター恐慌", "Scare Monster"));
            power_desc[num].min_level = 4;
            power_desc[num].cost = 6;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 3;
            power_desc[num++].number = -1;
            break;
        case RACE_KLACKON:
            sprintf(power_desc[num].racial_name, _("酸の唾 (ダメージ %d)", "Spit Acid (dam %d)"), lvl);
            power_desc[num].min_level = 9;
            power_desc[num].cost = 9;
            power_desc[num].stat = A_DEX;
            power_desc[num].fail = 14;
            power_desc[num++].number = -1;
            break;
        case RACE_KOBOLD:
            sprintf(power_desc[num].racial_name, _("毒のダーツ (ダメージ %d)", "Poison Dart (dam %d)"), lvl);
            power_desc[num].min_level = 12;
            power_desc[num].cost = 8;
            power_desc[num].stat = A_DEX;
            power_desc[num].fail = 14;
            power_desc[num++].number = -1;
            break;
        case RACE_DARK_ELF:
            sprintf(power_desc[num].racial_name, _("マジック・ミサイル (ダメージ %dd%d)", "Magic Missile (dm %dd%d)"), 3 + ((lvl - 1) / 5), 4);
            power_desc[num].min_level = 2;
            power_desc[num].cost = 2;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 9;
            power_desc[num++].number = -1;
            break;
        case RACE_DRACONIAN:
            sprintf(power_desc[num].racial_name, _("ブレス (ダメージ %d)", "Breath Weapon (dam %d)"), lvl * 2);
            power_desc[num].min_level = 1;
            power_desc[num].cost = lvl;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_MIND_FLAYER:
            sprintf(power_desc[num].racial_name, _("精神攻撃 (ダメージ %d)", "Mind Blast (dam %d)"), lvl);
            power_desc[num].min_level = 15;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 14;
            power_desc[num++].number = -1;
            break;
        case RACE_IMP:
            sprintf(power_desc[num].racial_name, _("ファイア・ボルト/ボール (ダメージ %d)", "Fire Bolt/Ball (dam %d)"), lvl);
            power_desc[num].min_level = 9;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 15;
            power_desc[num++].number = -1;
            break;
        case RACE_GOLEM:
            strcpy(power_desc[num].racial_name, _("肌石化 (期間 1d20+30)", "Stone Skin (dur 1d20+30)"));
            power_desc[num].min_level = 20;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 8;
            power_desc[num++].number = -1;
            break;
        case RACE_SKELETON:
        case RACE_ZOMBIE:
            strcpy(power_desc[num].racial_name, _("経験値復活", "Restore Experience"));
            power_desc[num].min_level = 30;
            power_desc[num].cost = 30;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 18;
            power_desc[num++].number = -1;
            break;
        case RACE_VAMPIRE:
            strcpy(power_desc[num].racial_name, _("吸血", "Vampiric Drain"));
            power_desc[num].min_level = 2;
            power_desc[num].cost = 1 + (lvl / 3);
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 9;
            power_desc[num++].number = -1;
            break;
        case RACE_SPRITE:
            strcpy(power_desc[num].racial_name, _("眠り粉", "Sleeping Dust"));
            power_desc[num].min_level = 12;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 15;
            power_desc[num++].number = -1;
            break;
        case RACE_BALROG:
            sprintf(power_desc[num].racial_name, _("地獄/火炎のブレス (ダメージ %d)", "Nether or Fire Breath (dam %d)"), lvl * 3);
            power_desc[num].min_level = 15;
            power_desc[num].cost = 10 + lvl / 3;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 20;
            power_desc[num++].number = -1;
            break;
        case RACE_KUTAR:
            strcpy(power_desc[num].racial_name, _("横に伸びる", "Expand Horizontally (dur 30+1d20)"));
            power_desc[num].min_level = 20;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_CHR;
            power_desc[num].fail = 8;
            power_desc[num++].number = -1;
            break;
        case RACE_ANDROID:
            if (creature_ptr->lev < 10) {
                strcpy(power_desc[num].racial_name, _("レイガン", "Ray Gun"));
                power_desc[num].min_level = 1;
                power_desc[num].cost = 7;
                power_desc[num].fail = 8;
            } else if (creature_ptr->lev < 25) {
                strcpy(power_desc[num].racial_name, _("ブラスター", "Blaster"));
                power_desc[num].min_level = 10;
                power_desc[num].cost = 13;
                power_desc[num].fail = 10;
            } else if (creature_ptr->lev < 35) {
                strcpy(power_desc[num].racial_name, _("バズーカ", "Bazooka"));
                power_desc[num].min_level = 25;
                power_desc[num].cost = 26;
                power_desc[num].fail = 12;
            } else if (creature_ptr->lev < 45) {
                strcpy(power_desc[num].racial_name, _("ビームキャノン", "Beam Cannon"));
                power_desc[num].min_level = 35;
                power_desc[num].cost = 40;
                power_desc[num].fail = 15;
            } else {
                strcpy(power_desc[num].racial_name, _("ロケット", "Rocket"));
                power_desc[num].min_level = 45;
                power_desc[num].cost = 60;
                power_desc[num].fail = 18;
            }
            power_desc[num].stat = A_STR;
            power_desc[num++].number = -1;
            break;
        default: {
            break;
        }
        }
    }

    if (creature_ptr->muta1) {
        if (creature_ptr->muta1 & MUT1_SPIT_ACID) {
            strcpy(power_desc[num].racial_name, _("酸の唾", "Spit Acid"));
            power_desc[num].min_level = 9;
            power_desc[num].cost = 9;
            power_desc[num].stat = A_DEX;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_SPIT_ACID;
        }

        if (creature_ptr->muta1 & MUT1_BR_FIRE) {
            strcpy(power_desc[num].racial_name, _("炎のブレス", "Fire Breath"));
            power_desc[num].min_level = 20;
            power_desc[num].cost = lvl;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 18;
            power_desc[num++].number = MUT1_BR_FIRE;
        }

        if (creature_ptr->muta1 & MUT1_HYPN_GAZE) {
            strcpy(power_desc[num].racial_name, _("催眠睨み", "Hypnotic Gaze"));
            power_desc[num].min_level = 12;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_CHR;
            power_desc[num].fail = 18;
            power_desc[num++].number = MUT1_HYPN_GAZE;
        }

        if (creature_ptr->muta1 & MUT1_TELEKINES) {
            strcpy(power_desc[num].racial_name, _("念動力", "Telekinesis"));
            power_desc[num].min_level = 9;
            power_desc[num].cost = 9;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_TELEKINES;
        }

        if (creature_ptr->muta1 & MUT1_VTELEPORT) {
            strcpy(power_desc[num].racial_name, _("テレポート", "Teleport"));
            power_desc[num].min_level = 7;
            power_desc[num].cost = 7;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_VTELEPORT;
        }

        if (creature_ptr->muta1 & MUT1_MIND_BLST) {
            strcpy(power_desc[num].racial_name, _("精神攻撃", "Mind Blast"));
            power_desc[num].min_level = 5;
            power_desc[num].cost = 3;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_MIND_BLST;
        }

        if (creature_ptr->muta1 & MUT1_RADIATION) {
            strcpy(power_desc[num].racial_name, _("放射能", "Emit Radiation"));
            power_desc[num].min_level = 15;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_RADIATION;
        }

        if (creature_ptr->muta1 & MUT1_VAMPIRISM) {
            strcpy(power_desc[num].racial_name, _("吸血", "Vampiric Drain"));
            power_desc[num].min_level = 2;
            power_desc[num].cost = (1 + (lvl / 3));
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 9;
            power_desc[num++].number = MUT1_VAMPIRISM;
        }

        if (creature_ptr->muta1 & MUT1_SMELL_MET) {
            strcpy(power_desc[num].racial_name, _("金属嗅覚", "Smell Metal"));
            power_desc[num].min_level = 3;
            power_desc[num].cost = 2;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 12;
            power_desc[num++].number = MUT1_SMELL_MET;
        }

        if (creature_ptr->muta1 & MUT1_SMELL_MON) {
            strcpy(power_desc[num].racial_name, _("敵臭嗅覚", "Smell Monsters"));
            power_desc[num].min_level = 5;
            power_desc[num].cost = 4;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_SMELL_MON;
        }

        if (creature_ptr->muta1 & MUT1_BLINK) {
            strcpy(power_desc[num].racial_name, _("ショート・テレポート", "Blink"));
            power_desc[num].min_level = 3;
            power_desc[num].cost = 3;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 12;
            power_desc[num++].number = MUT1_BLINK;
        }

        if (creature_ptr->muta1 & MUT1_EAT_ROCK) {
            strcpy(power_desc[num].racial_name, _("岩食い", "Eat Rock"));
            power_desc[num].min_level = 8;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 18;
            power_desc[num++].number = MUT1_EAT_ROCK;
        }

        if (creature_ptr->muta1 & MUT1_SWAP_POS) {
            strcpy(power_desc[num].racial_name, _("位置交換", "Swap Position"));
            power_desc[num].min_level = 15;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_DEX;
            power_desc[num].fail = 16;
            power_desc[num++].number = MUT1_SWAP_POS;
        }

        if (creature_ptr->muta1 & MUT1_SHRIEK) {
            strcpy(power_desc[num].racial_name, _("叫び", "Shriek"));
            power_desc[num].min_level = 20;
            power_desc[num].cost = 14;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 16;
            power_desc[num++].number = MUT1_SHRIEK;
        }

        if (creature_ptr->muta1 & MUT1_ILLUMINE) {
            strcpy(power_desc[num].racial_name, _("照明", "Illuminate"));
            power_desc[num].min_level = 3;
            power_desc[num].cost = 2;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 10;
            power_desc[num++].number = MUT1_ILLUMINE;
        }

        if (creature_ptr->muta1 & MUT1_DET_CURSE) {
            strcpy(power_desc[num].racial_name, _("呪い感知", "Detect Curses"));
            power_desc[num].min_level = 7;
            power_desc[num].cost = 14;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_DET_CURSE;
        }

        if (creature_ptr->muta1 & MUT1_BERSERK) {
            strcpy(power_desc[num].racial_name, _("狂戦士化", "Berserk"));
            power_desc[num].min_level = 8;
            power_desc[num].cost = 8;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_BERSERK;
        }

        if (creature_ptr->muta1 & MUT1_POLYMORPH) {
            strcpy(power_desc[num].racial_name, _("変身", "Polymorph"));
            power_desc[num].min_level = 18;
            power_desc[num].cost = 20;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 18;
            power_desc[num++].number = MUT1_POLYMORPH;
        }

        if (creature_ptr->muta1 & MUT1_MIDAS_TCH) {
            strcpy(power_desc[num].racial_name, _("ミダスの手", "Midas Touch"));
            power_desc[num].min_level = 10;
            power_desc[num].cost = 5;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 12;
            power_desc[num++].number = MUT1_MIDAS_TCH;
        }

        if (creature_ptr->muta1 & MUT1_GROW_MOLD) {
            strcpy(power_desc[num].racial_name, _("カビ発生", "Grow Mold"));
            power_desc[num].min_level = 1;
            power_desc[num].cost = 6;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_GROW_MOLD;
        }

        if (creature_ptr->muta1 & MUT1_RESIST) {
            strcpy(power_desc[num].racial_name, _("エレメント耐性", "Resist Elements"));
            power_desc[num].min_level = 10;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 12;
            power_desc[num++].number = MUT1_RESIST;
        }

        if (creature_ptr->muta1 & MUT1_EARTHQUAKE) {
            strcpy(power_desc[num].racial_name, _("地震", "Earthquake"));
            power_desc[num].min_level = 12;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = 16;
            power_desc[num++].number = MUT1_EARTHQUAKE;
        }

        if (creature_ptr->muta1 & MUT1_EAT_MAGIC) {
            strcpy(power_desc[num].racial_name, _("魔力食い", "Eat Magic"));
            power_desc[num].min_level = 17;
            power_desc[num].cost = 1;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_EAT_MAGIC;
        }

        if (creature_ptr->muta1 & MUT1_WEIGH_MAG) {
            strcpy(power_desc[num].racial_name, _("魔力感知", "Weigh Magic"));
            power_desc[num].min_level = 6;
            power_desc[num].cost = 6;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 10;
            power_desc[num++].number = MUT1_WEIGH_MAG;
        }

        if (creature_ptr->muta1 & MUT1_STERILITY) {
            strcpy(power_desc[num].racial_name, _("増殖阻止", "Sterilize"));
            power_desc[num].min_level = 12;
            power_desc[num].cost = 23;
            power_desc[num].stat = A_CHR;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_STERILITY;
        }

        if (creature_ptr->muta1 & MUT1_HIT_AND_AWAY) {
            strcpy(power_desc[num].racial_name, _("ヒット＆アウェイ", "Panic Hit"));
            power_desc[num].min_level = 10;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_DEX;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_HIT_AND_AWAY;
        }

        if (creature_ptr->muta1 & MUT1_DAZZLE) {
            strcpy(power_desc[num].racial_name, _("眩惑", "Dazzle"));
            power_desc[num].min_level = 7;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_CHR;
            power_desc[num].fail = 8;
            power_desc[num++].number = MUT1_DAZZLE;
        }

        if (creature_ptr->muta1 & MUT1_LASER_EYE) {
            strcpy(power_desc[num].racial_name, _("レーザー・アイ", "Laser Eye"));
            power_desc[num].min_level = 7;
            power_desc[num].cost = 10;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 9;
            power_desc[num++].number = MUT1_LASER_EYE;
        }

        if (creature_ptr->muta1 & MUT1_RECALL) {
            strcpy(power_desc[num].racial_name, _("帰還", "Recall"));
            power_desc[num].min_level = 17;
            power_desc[num].cost = 50;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 16;
            power_desc[num++].number = MUT1_RECALL;
        }

        if (creature_ptr->muta1 & MUT1_BANISH) {
            strcpy(power_desc[num].racial_name, _("邪悪消滅", "Banish Evil"));
            power_desc[num].min_level = 25;
            power_desc[num].cost = 25;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 18;
            power_desc[num++].number = MUT1_BANISH;
        }

        if (creature_ptr->muta1 & MUT1_COLD_TOUCH) {
            strcpy(power_desc[num].racial_name, _("凍結の手", "Cold Touch"));
            power_desc[num].min_level = 2;
            power_desc[num].cost = 2;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 11;
            power_desc[num++].number = MUT1_COLD_TOUCH;
        }

        if (creature_ptr->muta1 & MUT1_LAUNCHER) {
            strcpy(power_desc[num].racial_name, _("アイテム投げ", "Throw Object"));
            power_desc[num].min_level = 1;
            power_desc[num].cost = lvl;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = 6;
            power_desc[num++].number = 3;
        }
    }

    flag = FALSE;
    redraw = FALSE;

    (void)strnfmt(out_val, 78, _("(特殊能力 %c-%c, *'で一覧, ESCで中断) どの特殊能力を使いますか？", "(Powers %c-%c, *=List, ESC=exit) Use which power? "),
        I2A(0), (num <= 26) ? I2A(num - 1) : '0' + num - 27);

    if (!repeat_pull(&command_code) || command_code < 0 || command_code >= num) {
        if (use_menu)
            screen_save();

        choice = (always_show_list || use_menu) ? ESCAPE : 1;
        while (!flag) {
            if (choice == ESCAPE)
                choice = ' ';
            else if (!get_com(out_val, &choice, FALSE))
                break;

            if (use_menu && choice != ' ') {
                switch (choice) {
                case '0': {
                    screen_load();
                    free_turn(creature_ptr);
                    return;
                }

                case '8':
                case 'k':
                case 'K': {
                    menu_line += (num - 1);
                    break;
                }

                case '2':
                case 'j':
                case 'J': {
                    menu_line++;
                    break;
                }

                case '6':
                case 'l':
                case 'L':
                case '4':
                case 'h':
                case 'H': {
                    if (menu_line > 18)
                        menu_line -= 18;
                    else if (menu_line + 18 <= num)
                        menu_line += 18;
                    break;
                }

                case 'x':
                case 'X':
                case '\r': {
                    command_code = menu_line - 1;
                    ask = FALSE;
                    break;
                }
                }
                if (menu_line > num)
                    menu_line -= num;
            }

            if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask)) {
                if (!redraw || use_menu) {
                    byte y = 1, x = 0;
                    int ctr = 0;
                    char dummy[80];
                    char letter;
                    TERM_LEN x1, y1;
                    strcpy(dummy, "");
                    redraw = TRUE;
                    if (!use_menu)
                        screen_save();

                    if (num < 18)
                        prt(_("                            Lv   MP 失率", "                            Lv Cost Fail"), y++, x);
                    else
                        prt(_("                            Lv   MP 失率                            Lv   MP 失率",
                                "                            Lv Cost Fail                            Lv Cost Fail"),
                            y++, x);

                    while (ctr < num) {
                        x1 = ((ctr < 18) ? x : x + 40);
                        y1 = ((ctr < 18) ? y + ctr : y + ctr - 18);
                        if (use_menu) {
                            if (ctr == (menu_line - 1))
                                strcpy(dummy, _(" 》 ", " >  "));
                            else
                                strcpy(dummy, "    ");
                        } else {
                            if (ctr < 26)
                                letter = I2A(ctr);
                            else
                                letter = '0' + ctr - 26;
                            sprintf(dummy, " %c) ", letter);
                        }

                        strcat(dummy,
                            format("%-23.23s %2d %4d %3d%%", power_desc[ctr].racial_name, power_desc[ctr].min_level, power_desc[ctr].cost,
                                100 - racial_chance(creature_ptr, &power_desc[ctr])));
                        prt(dummy, y1, x1);
                        ctr++;
                    }
                } else {
                    redraw = FALSE;
                    screen_load();
                }

                continue;
            }

            if (!use_menu) {
                if (choice == '\r' && num == 1)
                    choice = 'a';

                if (isalpha(choice)) {
                    ask = (isupper(choice));
                    if (ask)
                        choice = (char)tolower(choice);

                    command_code = (islower(choice) ? A2I(choice) : -1);
                } else {
                    ask = FALSE;
                    command_code = choice - '0' + 26;
                }
            }

            if ((command_code < 0) || (command_code >= num)) {
                bell();
                continue;
            }

            if (ask) {
                char tmp_val[160];
                (void)strnfmt(tmp_val, 78, _("%sを使いますか？ ", "Use %s? "), power_desc[command_code].racial_name);
                if (!get_check(tmp_val))
                    continue;
            }

            flag = TRUE;
        }

        if (redraw)
            screen_load();

        if (!flag) {
            free_turn(creature_ptr);
            return;
        }

        repeat_push(command_code);
    }

    switch (racial_aux(creature_ptr, &power_desc[command_code])) {
    case 1:
        if (power_desc[command_code].number < 0)
            cast = exe_racial_power(creature_ptr, power_desc[command_code].number);
        else
            cast = exe_mutation_power(creature_ptr, power_desc[command_code].number);
        break;
    case 0:
        cast = FALSE;
        break;
    case -1:
        cast = TRUE;
        break;
    }

    if (!cast) {
        free_turn(creature_ptr);
        return;
    }

    int racial_cost = power_desc[command_code].racial_cost;
    if (racial_cost == 0)
        return;

    int actual_racial_cost = racial_cost / 2 + randint1(racial_cost / 2);
    if (creature_ptr->csp < actual_racial_cost) {
        actual_racial_cost -= creature_ptr->csp;
        creature_ptr->csp = 0;
        take_hit(creature_ptr, DAMAGE_USELIFE, actual_racial_cost, _("過度の集中", "concentrating too hard"), -1);
    } else
        creature_ptr->csp -= actual_racial_cost;

    creature_ptr->redraw |= PR_HP | PR_MANA;
    creature_ptr->window |= PW_PLAYER | PW_SPELL;
}
