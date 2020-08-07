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

#define MAX_RACIAL_POWERS 36

// Racial Command.
typedef struct rc_type {
    rpi_type power_desc[MAX_RACIAL_POWERS];
    int num;
    COMMAND_CODE command_code;
    int ask;
    PLAYER_LEVEL lvl;
    bool flag;
    bool redraw;
    bool cast;
    bool is_warrior;
    char choice;
    char out_val[160];
    int menu_line;
} rc_type;

rc_type *initialize_rc_type(player_type *creature_ptr, rc_type *rc_ptr)
{
    rc_ptr->num = 0;
    rc_ptr->command_code = 0;
    rc_ptr->ask = TRUE;
    rc_ptr->lvl = creature_ptr->lev;
    rc_ptr->cast = FALSE;
    rc_ptr->is_warrior = (creature_ptr->pclass == CLASS_WARRIOR || creature_ptr->pclass == CLASS_BERSERKER);
    rc_ptr->menu_line = use_menu ? 1 : 0;
    for (int i = 0; i < MAX_RACIAL_POWERS; i++) {
        strcpy(rc_ptr->power_desc[i].racial_name, "");
        rc_ptr->power_desc[i].number = 0;
    }
}

/*!
 * @brief レイシャル・パワーコマンドのメインルーチン / Allow user to choose a power (racial / mutation) to activate
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_racial_power(player_type *creature_ptr)
{
    if (creature_ptr->wild_mode)
        return;

    if (cmd_limit_confused(creature_ptr)) {
        free_turn(creature_ptr);
        return;
    }

    if (creature_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
        set_action(creature_ptr, ACTION_NONE);

    rc_type tmp_rc;
    rc_type *rc_ptr = initialize_rc_type(creature_ptr, &tmp_rc);
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

    if (creature_ptr->mimic_form) {
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
    } else {
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
            sprintf(rc_ptr->power_desc[rc_ptr->num].racial_name, _("マジック・ミサイル (ダメージ %dd%d)", "Magic Missile (dm %dd%d)"),
                3 + ((rc_ptr->lvl - 1) / 5), 4);
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
        default: {
            break;
        }
        }
    }

    if (creature_ptr->muta1) {
        if (creature_ptr->muta1 & MUT1_SPIT_ACID) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("酸の唾", "Spit Acid"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 9;
            rc_ptr->power_desc[rc_ptr->num].cost = 9;
            rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
            rc_ptr->power_desc[rc_ptr->num].fail = 15;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_SPIT_ACID;
        }

        if (creature_ptr->muta1 & MUT1_BR_FIRE) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("炎のブレス", "Fire Breath"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 20;
            rc_ptr->power_desc[rc_ptr->num].cost = rc_ptr->lvl;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
            rc_ptr->power_desc[rc_ptr->num].fail = 18;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_BR_FIRE;
        }

        if (creature_ptr->muta1 & MUT1_HYPN_GAZE) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("催眠睨み", "Hypnotic Gaze"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 12;
            rc_ptr->power_desc[rc_ptr->num].cost = 12;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CHR;
            rc_ptr->power_desc[rc_ptr->num].fail = 18;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_HYPN_GAZE;
        }

        if (creature_ptr->muta1 & MUT1_TELEKINES) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("念動力", "Telekinesis"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 9;
            rc_ptr->power_desc[rc_ptr->num].cost = 9;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 14;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_TELEKINES;
        }

        if (creature_ptr->muta1 & MUT1_VTELEPORT) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("テレポート", "Teleport"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 7;
            rc_ptr->power_desc[rc_ptr->num].cost = 7;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 15;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_VTELEPORT;
        }

        if (creature_ptr->muta1 & MUT1_MIND_BLST) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("精神攻撃", "Mind Blast"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 5;
            rc_ptr->power_desc[rc_ptr->num].cost = 3;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 15;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_MIND_BLST;
        }

        if (creature_ptr->muta1 & MUT1_RADIATION) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("放射能", "Emit Radiation"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 15;
            rc_ptr->power_desc[rc_ptr->num].cost = 15;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
            rc_ptr->power_desc[rc_ptr->num].fail = 14;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_RADIATION;
        }

        if (creature_ptr->muta1 & MUT1_VAMPIRISM) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("吸血", "Vampiric Drain"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 2;
            rc_ptr->power_desc[rc_ptr->num].cost = (1 + (rc_ptr->lvl / 3));
            rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
            rc_ptr->power_desc[rc_ptr->num].fail = 9;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_VAMPIRISM;
        }

        if (creature_ptr->muta1 & MUT1_SMELL_MET) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("金属嗅覚", "Smell Metal"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 3;
            rc_ptr->power_desc[rc_ptr->num].cost = 2;
            rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
            rc_ptr->power_desc[rc_ptr->num].fail = 12;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_SMELL_MET;
        }

        if (creature_ptr->muta1 & MUT1_SMELL_MON) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("敵臭嗅覚", "Smell Monsters"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 5;
            rc_ptr->power_desc[rc_ptr->num].cost = 4;
            rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
            rc_ptr->power_desc[rc_ptr->num].fail = 15;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_SMELL_MON;
        }

        if (creature_ptr->muta1 & MUT1_BLINK) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ショート・テレポート", "Blink"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 3;
            rc_ptr->power_desc[rc_ptr->num].cost = 3;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 12;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_BLINK;
        }

        if (creature_ptr->muta1 & MUT1_EAT_ROCK) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("岩食い", "Eat Rock"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 8;
            rc_ptr->power_desc[rc_ptr->num].cost = 12;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
            rc_ptr->power_desc[rc_ptr->num].fail = 18;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_EAT_ROCK;
        }

        if (creature_ptr->muta1 & MUT1_SWAP_POS) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("位置交換", "Swap Position"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 15;
            rc_ptr->power_desc[rc_ptr->num].cost = 12;
            rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
            rc_ptr->power_desc[rc_ptr->num].fail = 16;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_SWAP_POS;
        }

        if (creature_ptr->muta1 & MUT1_SHRIEK) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("叫び", "Shriek"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 20;
            rc_ptr->power_desc[rc_ptr->num].cost = 14;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
            rc_ptr->power_desc[rc_ptr->num].fail = 16;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_SHRIEK;
        }

        if (creature_ptr->muta1 & MUT1_ILLUMINE) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("照明", "Illuminate"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 3;
            rc_ptr->power_desc[rc_ptr->num].cost = 2;
            rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
            rc_ptr->power_desc[rc_ptr->num].fail = 10;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_ILLUMINE;
        }

        if (creature_ptr->muta1 & MUT1_DET_CURSE) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("呪い感知", "Detect Curses"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 7;
            rc_ptr->power_desc[rc_ptr->num].cost = 14;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 14;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_DET_CURSE;
        }

        if (creature_ptr->muta1 & MUT1_BERSERK) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("狂戦士化", "Berserk"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 8;
            rc_ptr->power_desc[rc_ptr->num].cost = 8;
            rc_ptr->power_desc[rc_ptr->num].stat = A_STR;
            rc_ptr->power_desc[rc_ptr->num].fail = 14;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_BERSERK;
        }

        if (creature_ptr->muta1 & MUT1_POLYMORPH) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("変身", "Polymorph"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 18;
            rc_ptr->power_desc[rc_ptr->num].cost = 20;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
            rc_ptr->power_desc[rc_ptr->num].fail = 18;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_POLYMORPH;
        }

        if (creature_ptr->muta1 & MUT1_MIDAS_TCH) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ミダスの手", "Midas Touch"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 10;
            rc_ptr->power_desc[rc_ptr->num].cost = 5;
            rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
            rc_ptr->power_desc[rc_ptr->num].fail = 12;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_MIDAS_TCH;
        }

        if (creature_ptr->muta1 & MUT1_GROW_MOLD) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("カビ発生", "Grow Mold"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 1;
            rc_ptr->power_desc[rc_ptr->num].cost = 6;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
            rc_ptr->power_desc[rc_ptr->num].fail = 14;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_GROW_MOLD;
        }

        if (creature_ptr->muta1 & MUT1_RESIST) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("エレメント耐性", "Resist Elements"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 10;
            rc_ptr->power_desc[rc_ptr->num].cost = 12;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
            rc_ptr->power_desc[rc_ptr->num].fail = 12;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_RESIST;
        }

        if (creature_ptr->muta1 & MUT1_EARTHQUAKE) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("地震", "Earthquake"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 12;
            rc_ptr->power_desc[rc_ptr->num].cost = 12;
            rc_ptr->power_desc[rc_ptr->num].stat = A_STR;
            rc_ptr->power_desc[rc_ptr->num].fail = 16;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_EARTHQUAKE;
        }

        if (creature_ptr->muta1 & MUT1_EAT_MAGIC) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("魔力食い", "Eat Magic"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 17;
            rc_ptr->power_desc[rc_ptr->num].cost = 1;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 15;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_EAT_MAGIC;
        }

        if (creature_ptr->muta1 & MUT1_WEIGH_MAG) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("魔力感知", "Weigh Magic"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 6;
            rc_ptr->power_desc[rc_ptr->num].cost = 6;
            rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
            rc_ptr->power_desc[rc_ptr->num].fail = 10;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_WEIGH_MAG;
        }

        if (creature_ptr->muta1 & MUT1_STERILITY) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("増殖阻止", "Sterilize"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 12;
            rc_ptr->power_desc[rc_ptr->num].cost = 23;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CHR;
            rc_ptr->power_desc[rc_ptr->num].fail = 15;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_STERILITY;
        }

        if (creature_ptr->muta1 & MUT1_HIT_AND_AWAY) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("ヒット＆アウェイ", "Panic Hit"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 10;
            rc_ptr->power_desc[rc_ptr->num].cost = 12;
            rc_ptr->power_desc[rc_ptr->num].stat = A_DEX;
            rc_ptr->power_desc[rc_ptr->num].fail = 14;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_HIT_AND_AWAY;
        }

        if (creature_ptr->muta1 & MUT1_DAZZLE) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("眩惑", "Dazzle"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 7;
            rc_ptr->power_desc[rc_ptr->num].cost = 15;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CHR;
            rc_ptr->power_desc[rc_ptr->num].fail = 8;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_DAZZLE;
        }

        if (creature_ptr->muta1 & MUT1_LASER_EYE) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("レーザー・アイ", "Laser Eye"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 7;
            rc_ptr->power_desc[rc_ptr->num].cost = 10;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 9;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_LASER_EYE;
        }

        if (creature_ptr->muta1 & MUT1_RECALL) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("帰還", "Recall"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 17;
            rc_ptr->power_desc[rc_ptr->num].cost = 50;
            rc_ptr->power_desc[rc_ptr->num].stat = A_INT;
            rc_ptr->power_desc[rc_ptr->num].fail = 16;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_RECALL;
        }

        if (creature_ptr->muta1 & MUT1_BANISH) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("邪悪消滅", "Banish Evil"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 25;
            rc_ptr->power_desc[rc_ptr->num].cost = 25;
            rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
            rc_ptr->power_desc[rc_ptr->num].fail = 18;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_BANISH;
        }

        if (creature_ptr->muta1 & MUT1_COLD_TOUCH) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("凍結の手", "Cold Touch"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 2;
            rc_ptr->power_desc[rc_ptr->num].cost = 2;
            rc_ptr->power_desc[rc_ptr->num].stat = A_CON;
            rc_ptr->power_desc[rc_ptr->num].fail = 11;
            rc_ptr->power_desc[rc_ptr->num++].number = MUT1_COLD_TOUCH;
        }

        if (creature_ptr->muta1 & MUT1_LAUNCHER) {
            strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("アイテム投げ", "Throw Object"));
            rc_ptr->power_desc[rc_ptr->num].min_level = 1;
            rc_ptr->power_desc[rc_ptr->num].cost = rc_ptr->lvl;
            rc_ptr->power_desc[rc_ptr->num].stat = A_STR;
            rc_ptr->power_desc[rc_ptr->num].fail = 6;
            rc_ptr->power_desc[rc_ptr->num++].number = 3;
        }
    }

    rc_ptr->flag = FALSE;
    rc_ptr->redraw = FALSE;

    (void)strnfmt(rc_ptr->out_val, 78,
        _("(特殊能力 %c-%c, *'で一覧, ESCで中断) どの特殊能力を使いますか？", "(Powers %c-%c, *=List, ESC=exit) Use which power? "),
        I2A(0), (rc_ptr->num <= 26) ? I2A(rc_ptr->num - 1) : '0' + rc_ptr->num - 27);

    if (!repeat_pull(&rc_ptr->command_code) || rc_ptr->command_code < 0 || rc_ptr->command_code >= rc_ptr->num) {
        if (use_menu)
            screen_save();

        rc_ptr->choice = (always_show_list || use_menu) ? ESCAPE : 1;
        while (!rc_ptr->flag) {
            if (rc_ptr->choice == ESCAPE)
                rc_ptr->choice = ' ';
            else if (!get_com(rc_ptr->out_val, &rc_ptr->choice, FALSE))
                break;

            if (use_menu && rc_ptr->choice != ' ') {
                switch (rc_ptr->choice) {
                case '0': {
                    screen_load();
                    free_turn(creature_ptr);
                    return;
                }

                case '8':
                case 'k':
                case 'K': {
                    rc_ptr->menu_line += (rc_ptr->num - 1);
                    break;
                }

                case '2':
                case 'j':
                case 'J': {
                    rc_ptr->menu_line++;
                    break;
                }

                case '6':
                case 'l':
                case 'L':
                case '4':
                case 'h':
                case 'H': {
                    if (rc_ptr->menu_line > 18)
                        rc_ptr->menu_line -= 18;
                    else if (rc_ptr->menu_line + 18 <= rc_ptr->num)
                        rc_ptr->menu_line += 18;
                    break;
                }

                case 'x':
                case 'X':
                case '\r': {
                    rc_ptr->command_code = rc_ptr->menu_line - 1;
                    rc_ptr->ask = FALSE;
                    break;
                }
                }
                if (rc_ptr->menu_line > rc_ptr->num)
                    rc_ptr->menu_line -= rc_ptr->num;
            }

            if ((rc_ptr->choice == ' ') || (rc_ptr->choice == '*') || (rc_ptr->choice == '?') || (use_menu && rc_ptr->ask)) {
                if (!rc_ptr->redraw || use_menu) {
                    byte y = 1, x = 0;
                    int ctr = 0;
                    char dummy[80];
                    char letter;
                    TERM_LEN x1, y1;
                    strcpy(dummy, "");
                    rc_ptr->redraw = TRUE;
                    if (!use_menu)
                        screen_save();

                    if (rc_ptr->num < 18)
                        prt(_("                            Lv   MP 失率", "                            Lv Cost Fail"), y++, x);
                    else
                        prt(_("                            Lv   MP 失率                            Lv   MP 失率",
                                "                            Lv Cost Fail                            Lv Cost Fail"),
                            y++, x);

                    while (ctr < rc_ptr->num) {
                        x1 = ((ctr < 18) ? x : x + 40);
                        y1 = ((ctr < 18) ? y + ctr : y + ctr - 18);
                        if (use_menu) {
                            if (ctr == (rc_ptr->menu_line - 1))
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
                            format("%-23.23s %2d %4d %3d%%", rc_ptr->power_desc[ctr].racial_name, rc_ptr->power_desc[ctr].min_level, rc_ptr->power_desc[ctr].cost,
                                100 - racial_chance(creature_ptr, &rc_ptr->power_desc[ctr])));
                        prt(dummy, y1, x1);
                        ctr++;
                    }
                } else {
                    rc_ptr->redraw = FALSE;
                    screen_load();
                }

                continue;
            }

            if (!use_menu) {
                if (rc_ptr->choice == '\r' && rc_ptr->num == 1)
                    rc_ptr->choice = 'a';

                if (isalpha(rc_ptr->choice)) {
                    rc_ptr->ask = (isupper(rc_ptr->choice));
                    if (rc_ptr->ask)
                        rc_ptr->choice = (char)tolower(rc_ptr->choice);

                    rc_ptr->command_code = (islower(rc_ptr->choice) ? A2I(rc_ptr->choice) : -1);
                } else {
                    rc_ptr->ask = FALSE;
                    rc_ptr->command_code = rc_ptr->choice - '0' + 26;
                }
            }

            if ((rc_ptr->command_code < 0) || (rc_ptr->command_code >= rc_ptr->num)) {
                bell();
                continue;
            }

            if (rc_ptr->ask) {
                char tmp_val[160];
                (void)strnfmt(tmp_val, 78, _("%sを使いますか？ ", "Use %s? "), rc_ptr->power_desc[rc_ptr->command_code].racial_name);
                if (!get_check(tmp_val))
                    continue;
            }

            rc_ptr->flag = TRUE;
        }

        if (rc_ptr->redraw)
            screen_load();

        if (!rc_ptr->flag) {
            free_turn(creature_ptr);
            return;
        }

        repeat_push(rc_ptr->command_code);
    }

    switch (racial_aux(creature_ptr, &rc_ptr->power_desc[rc_ptr->command_code])) {
    case 1:
        if (rc_ptr->power_desc[rc_ptr->command_code].number < 0)
            rc_ptr->cast = exe_racial_power(creature_ptr, rc_ptr->power_desc[rc_ptr->command_code].number);
        else
            rc_ptr->cast = exe_mutation_power(creature_ptr, rc_ptr->power_desc[rc_ptr->command_code].number);
        break;
    case 0:
        rc_ptr->cast = FALSE;
        break;
    case -1:
        rc_ptr->cast = TRUE;
        break;
    }

    if (!rc_ptr->cast) {
        free_turn(creature_ptr);
        return;
    }

    int racial_cost = rc_ptr->power_desc[rc_ptr->command_code].racial_cost;
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
