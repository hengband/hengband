/*!
 * @brief 青魔法の処理実装 / Blue magic
 * @date 2014/01/15
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "mspell/mspells3.h"
#include "cmd-action/cmd-spell.h"
#include "cmd/cmd-basic.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "floor/floor.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/targeting.h"
#include "lore/lore-calculator.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-blue-mage.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags4.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/monster-power-table.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-mask-definitions.h"
#include "mspell/mspell-type.h"
#include "player/avatar.h"
#include "player/player-effects.h"
#include "player/player-status.h"
#include "realm/realm-types.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/spells3.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-main-window.h"
#include "view/display-messages.h"

/*!
 * @brief モンスター魔法をプレイヤーが使用する場合の換算レベル
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param 換算レベル
 */
PLAYER_LEVEL get_pseudo_monstetr_level(player_type *caster_ptr)
{
    PLAYER_LEVEL monster_level = caster_ptr->lev + 40;
    return (monster_level * monster_level - 1550) / 130;
}

/*!
 * @brief 文字列に青魔導師の呪文の攻撃力を加える
 * @param SPELL_NUM 呪文番号
 * @param plev プレイヤーレベル
 * @param msg 表示する文字列
 * @param tmp 返すメッセージを格納する配列
 * @return なし
 */
void set_bluemage_damage(player_type *learner_type, monster_spell_type ms_type, PLAYER_LEVEL plev, concptr msg, char *tmp)
{
    int base_damage = monspell_bluemage_damage(learner_type, ms_type, plev, BASE_DAM);
    int dice_num = monspell_bluemage_damage(learner_type, ms_type, plev, DICE_NUM);
    int dice_side = monspell_bluemage_damage(learner_type, ms_type, plev, DICE_SIDE);
    int dice_mult = monspell_bluemage_damage(learner_type, ms_type, plev, DICE_MULT);
    int dice_div = monspell_bluemage_damage(learner_type, ms_type, plev, DICE_DIV);
    char dmg_str[80];
    dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div, dmg_str);
    sprintf(tmp, " %s %s", msg, dmg_str);
}

/*!
 * @brief 受け取ったモンスター魔法のIDに応じて青魔法の効果情報をまとめたフォーマットを返す
 * @param learner_ptr プレーヤーへの参照ポインタ
 * @param p 情報を返す文字列参照ポインタ
 * @param power モンスター魔法のID
 * @return なし
 */
static void learned_info(player_type *learner_ptr, char *p, int power)
{
    PLAYER_LEVEL plev = get_pseudo_monstetr_level(learner_ptr);

    strcpy(p, "");

    switch (power) {
    case MS_SHRIEK:
    case MS_XXX1:
    case MS_XXX2:
    case MS_XXX3:
    case MS_XXX4:
    case MS_SCARE:
    case MS_BLIND:
    case MS_CONF:
    case MS_SLOW:
    case MS_SLEEP:
    case MS_HAND_DOOM:
    case MS_WORLD:
    case MS_SPECIAL:
    case MS_TELE_TO:
    case MS_TELE_AWAY:
    case MS_TELE_LEVEL:
    case MS_DARKNESS:
    case MS_MAKE_TRAP:
    case MS_FORGET:
    case MS_S_KIN:
    case MS_S_CYBER:
    case MS_S_MONSTER:
    case MS_S_MONSTERS:
    case MS_S_ANT:
    case MS_S_SPIDER:
    case MS_S_HOUND:
    case MS_S_HYDRA:
    case MS_S_ANGEL:
    case MS_S_DEMON:
    case MS_S_UNDEAD:
    case MS_S_DRAGON:
    case MS_S_HI_UNDEAD:
    case MS_S_HI_DRAGON:
    case MS_S_AMBERITE:
    case MS_S_UNIQUE:
        break;
    case MS_BALL_MANA:
    case MS_BALL_DARK:
    case MS_STARBURST:
        set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p);
        break;
    case MS_DISPEL:
        break;
    case MS_ROCKET:
    case MS_SHOOT:
    case MS_BR_ACID:
    case MS_BR_ELEC:
    case MS_BR_FIRE:
    case MS_BR_COLD:
    case MS_BR_POIS:
    case MS_BR_NUKE:
    case MS_BR_NEXUS:
    case MS_BR_TIME:
    case MS_BR_GRAVITY:
    case MS_BR_MANA:
    case MS_BR_NETHER:
    case MS_BR_LITE:
    case MS_BR_DARK:
    case MS_BR_CONF:
    case MS_BR_SOUND:
    case MS_BR_CHAOS:
    case MS_BR_DISEN:
    case MS_BR_SHARDS:
    case MS_BR_PLASMA:
    case MS_BR_INERTIA:
    case MS_BR_FORCE:
    case MS_BR_DISI:
    case MS_BALL_NUKE:
    case MS_BALL_CHAOS:
    case MS_BALL_ACID:
    case MS_BALL_ELEC:
    case MS_BALL_FIRE:
    case MS_BALL_COLD:
    case MS_BALL_POIS:
    case MS_BALL_NETHER:
    case MS_BALL_WATER:
        set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p);
        break;
    case MS_DRAIN_MANA:
        set_bluemage_damage(learner_ptr, power, plev, KWD_HEAL, p);
        break;
    case MS_MIND_BLAST:
    case MS_BRAIN_SMASH:
    case MS_CAUSE_1:
    case MS_CAUSE_2:
    case MS_CAUSE_3:
    case MS_CAUSE_4:
    case MS_BOLT_ACID:
    case MS_BOLT_ELEC:
    case MS_BOLT_FIRE:
    case MS_BOLT_COLD:
    case MS_BOLT_NETHER:
    case MS_BOLT_WATER:
    case MS_BOLT_MANA:
    case MS_BOLT_PLASMA:
    case MS_BOLT_ICE:
    case MS_MAGIC_MISSILE:
        set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p);
        break;
    case MS_SPEED:
        sprintf(p, " %sd%d+%d", KWD_DURATION, 20 + plev, plev);
        break;
    case MS_HEAL:
        set_bluemage_damage(learner_ptr, power, plev, KWD_HEAL, p);
        break;
    case MS_INVULNER:
        sprintf(p, " %sd7+7", KWD_DURATION);
        break;
    case MS_BLINK:
        sprintf(p, " %s10", KWD_SPHERE);
        break;
    case MS_TELEPORT:
        sprintf(p, " %s%d", KWD_SPHERE, plev * 5);
        break;
    case MS_PSY_SPEAR:
        set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p);
        break;
        break;
    case MS_RAISE_DEAD:
        sprintf(p, " %s5", KWD_SPHERE);
        break;
    default:
        break;
    }
}

/*!
 * @brief 使用可能な青魔法を選択する /
 * Allow user to choose a imitation.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param sn 選択したモンスター攻撃ID、キャンセルの場合-1、不正な選択の場合-2を返す
 * @return 発動可能な魔法を選択した場合TRUE、キャンセル処理か不正な選択が行われた場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE\n
 * If the user hits escape, returns FALSE, and set '*sn' to -1\n
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2\n
 *\n
 * The "prompt" should be "cast", "recite", or "study"\n
 * The "known" should be TRUE for cast/pray, FALSE for study\n
 *\n
 * nb: This function has a (trivial) display bug which will be obvious\n
 * when you run it. It's probably easy to fix but I haven't tried,\n
 * sorry.\n
 */
static bool get_learned_power(player_type *caster_ptr, SPELL_IDX *sn)
{
    int i = 0;
    int num = 0;
    TERM_LEN y = 1;
    TERM_LEN x = 18;
    PERCENTAGE minfail = 0;
    PLAYER_LEVEL plev = caster_ptr->lev;
    PERCENTAGE chance = 0;
    int ask = TRUE, mode = 0;
    int spellnum[MAX_MONSPELLS];
    char ch;
    char choice;
    char out_val[160];
    char comment[80];
    BIT_FLAGS f4 = 0L, f5 = 0L, f6 = 0L;
    concptr p = _("魔法", "magic");
    COMMAND_CODE code;
    monster_power spell;
    bool flag, redraw;
    int menu_line = (use_menu ? 1 : 0);

    *sn = (-1);

    flag = FALSE;
    redraw = FALSE;

    if (repeat_pull(&code)) {
        *sn = (SPELL_IDX)code;
        return TRUE;
    }

    if (use_menu) {
        screen_save();
        while (!mode) {
            prt(format(_(" %s ボルト", " %s bolt"), (menu_line == 1) ? _("》", "> ") : "  "), 2, 14);
            prt(format(_(" %s ボール", " %s ball"), (menu_line == 2) ? _("》", "> ") : "  "), 3, 14);
            prt(format(_(" %s ブレス", " %s breath"), (menu_line == 3) ? _("》", "> ") : "  "), 4, 14);
            prt(format(_(" %s 召喚", " %s sommoning"), (menu_line == 4) ? _("》", "> ") : "  "), 5, 14);
            prt(format(_(" %s その他", " %s others"), (menu_line == 5) ? _("》", "> ") : "  "), 6, 14);
            prt(_("どの種類の魔法を使いますか？", "use which type of magic? "), 0, 0);

            choice = inkey();
            switch (choice) {
            case ESCAPE:
            case 'z':
            case 'Z':
                screen_load();
                return FALSE;
            case '2':
            case 'j':
            case 'J':
                menu_line++;
                break;
            case '8':
            case 'k':
            case 'K':
                menu_line += 4;
                break;
            case '\r':
            case 'x':
            case 'X':
                mode = menu_line;
                break;
            }
            if (menu_line > 5)
                menu_line -= 5;
        }
        screen_load();
    } else {
        sprintf(comment, _("[A]ボルト, [B]ボール, [C]ブレス, [D]召喚, [E]その他:", "[A] bolt, [B] ball, [C] breath, [D] summoning, [E] others:"));
        while (TRUE) {
            if (!get_com(comment, &ch, TRUE)) {
                return FALSE;
            }
            if (ch == 'A' || ch == 'a') {
                mode = 1;
                break;
            }
            if (ch == 'B' || ch == 'b') {
                mode = 2;
                break;
            }
            if (ch == 'C' || ch == 'c') {
                mode = 3;
                break;
            }
            if (ch == 'D' || ch == 'd') {
                mode = 4;
                break;
            }
            if (ch == 'E' || ch == 'e') {
                mode = 5;
                break;
            }
        }
    }

    set_rf_masks(&f4, &f5, &f6, mode);

    for (i = 0, num = 0; i < 32; i++) {
        if ((0x00000001 << i) & f4)
            spellnum[num++] = i;
    }
    for (; i < 64; i++) {
        if ((0x00000001 << (i - 32)) & f5)
            spellnum[num++] = i;
    }
    for (; i < 96; i++) {
        if ((0x00000001 << (i - 64)) & f6)
            spellnum[num++] = i;
    }
    for (i = 0; i < num; i++) {
        if (caster_ptr->magic_num2[spellnum[i]]) {
            if (use_menu)
                menu_line = i + 1;
            break;
        }
    }
    if (i == num) {
        msg_print(_("その種類の魔法は覚えていない！", "You don't know any spell of this type."));
        return FALSE;
    }

    (void)strnfmt(out_val, 78, _("(%c-%c, '*'で一覧, ESC) どの%sを唱えますか？", "(%c-%c, *=List, ESC=exit) Use which %s? "), I2A(0), I2A(num - 1), p);

    if (use_menu)
        screen_save();

    choice = (always_show_list || use_menu) ? ESCAPE : 1;
    while (!flag) {
        if (choice == ESCAPE)
            choice = ' ';
        else if (!get_com(out_val, &choice, TRUE))
            break;

        if (use_menu && choice != ' ') {
            switch (choice) {
            case '0': {
                screen_load();
                return FALSE;
            }

            case '8':
            case 'k':
            case 'K': {
                do {
                    menu_line += (num - 1);
                    if (menu_line > num)
                        menu_line -= num;
                } while (!caster_ptr->magic_num2[spellnum[menu_line - 1]]);
                break;
            }

            case '2':
            case 'j':
            case 'J': {
                do {
                    menu_line++;
                    if (menu_line > num)
                        menu_line -= num;
                } while (!caster_ptr->magic_num2[spellnum[menu_line - 1]]);
                break;
            }

            case '6':
            case 'l':
            case 'L': {
                menu_line = num;
                while (!caster_ptr->magic_num2[spellnum[menu_line - 1]])
                    menu_line--;
                break;
            }

            case '4':
            case 'h':
            case 'H': {
                menu_line = 1;
                while (!caster_ptr->magic_num2[spellnum[menu_line - 1]])
                    menu_line++;
                break;
            }

            case 'x':
            case 'X':
            case '\r': {
                i = menu_line - 1;
                ask = FALSE;
                break;
            }
            }
        }

        if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask)) {
            if (!redraw || use_menu) {
                char psi_desc[80];
                redraw = TRUE;
                if (!use_menu)
                    screen_save();

                prt("", y, x);
                put_str(_("名前", "Name"), y, x + 5);
                put_str(_("MP 失率 効果", "SP Fail Info"), y, x + 33);

                for (i = 0; i < num; i++) {
                    int need_mana;
                    prt("", y + i + 1, x);
                    if (!caster_ptr->magic_num2[spellnum[i]])
                        continue;

                    spell = monster_powers[spellnum[i]];
                    chance = spell.fail;
                    if (plev > spell.level)
                        chance -= 3 * (plev - spell.level);
                    else
                        chance += (spell.level - plev);

                    chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[A_INT]] - 1);
                    chance = mod_spell_chance_1(caster_ptr, chance);
                    need_mana = mod_need_mana(caster_ptr, monster_powers[spellnum[i]].smana, 0, REALM_NONE);
                    if (need_mana > caster_ptr->csp) {
                        chance += 5 * (need_mana - caster_ptr->csp);
                    }

                    minfail = adj_mag_fail[caster_ptr->stat_ind[A_INT]];
                    if (chance < minfail)
                        chance = minfail;

                    if (caster_ptr->stun > 50)
                        chance += 25;
                    else if (caster_ptr->stun)
                        chance += 15;

                    if (chance > 95)
                        chance = 95;

                    chance = mod_spell_chance_2(caster_ptr, chance);
                    learned_info(caster_ptr, comment, spellnum[i]);
                    if (use_menu) {
                        if (i == (menu_line - 1))
                            strcpy(psi_desc, _("  》", "  > "));
                        else
                            strcpy(psi_desc, "    ");
                    } else
                        sprintf(psi_desc, "  %c)", I2A(i));

                    strcat(psi_desc, format(" %-26s %3d %3d%%%s", spell.name, need_mana, chance, comment));
                    prt(psi_desc, y + i + 1, x);
                }

                if (y < 22)
                    prt("", y + i + 1, x);
            } else {
                redraw = FALSE;
                screen_load();
            }

            continue;
        }

        if (!use_menu) {
            ask = isupper(choice);
            if (ask)
                choice = (char)tolower(choice);

            i = (islower(choice) ? A2I(choice) : -1);
        }

        if ((i < 0) || (i >= num) || !caster_ptr->magic_num2[spellnum[i]]) {
            bell();
            continue;
        }

        spell = monster_powers[spellnum[i]];
        if (ask) {
            char tmp_val[160];
            (void)strnfmt(tmp_val, 78, _("%sの魔法を唱えますか？", "Use %s? "), monster_powers[spellnum[i]].name);
            if (!get_check(tmp_val))
                continue;
        }

        flag = TRUE;
    }

    if (redraw)
        screen_load();

    caster_ptr->window |= (PW_SPELL);
    handle_stuff(caster_ptr);

    if (!flag)
        return FALSE;

    (*sn) = spellnum[i];
    return TRUE;
}

/*!
 * @brief 青魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'blue-mage'.
 * @param spell 発動するモンスター攻撃のID
 * @param success TRUEは成功時、FALSEは失敗時の処理を行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_learned_spell(player_type *caster_ptr, int spell, bool success)
{
    DIRECTION dir;
    PLAYER_LEVEL plev = get_pseudo_monstetr_level(caster_ptr);
    PLAYER_LEVEL summon_lev = caster_ptr->lev * 2 / 3 + randint1(caster_ptr->lev / 2);
    HIT_POINT damage = 0;
    bool pet = success;
    bool no_trump = FALSE;
    BIT_FLAGS p_mode, u_mode = 0L, g_mode;

    if (pet) {
        p_mode = PM_FORCE_PET;
        g_mode = 0;
    } else {
        p_mode = PM_NO_PET;
        g_mode = PM_ALLOW_GROUP;
    }

    if (!success || (randint1(50 + plev) < plev / 10))
        u_mode = PM_ALLOW_UNIQUE;

    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    switch (spell) {
    case MS_SHRIEK:
        msg_print(_("かん高い金切り声をあげた。", "You make a high pitched shriek."));
        aggravate_monsters(caster_ptr, 0);
        break;
    case MS_XXX1:
        break;
    case MS_DISPEL: {
        MONSTER_IDX m_idx;

        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        m_idx = floor_ptr->grid_array[target_row][target_col].m_idx;
        if (!m_idx)
            break;
        if (!player_has_los_bold(caster_ptr, target_row, target_col))
            break;
        if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
            break;
        dispel_monster_status(caster_ptr, m_idx);
        break;
    }
    case MS_ROCKET:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("ロケットを発射した。", "You fire a rocket."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_ROCKET), plev, DAM_ROLL);
        fire_rocket(caster_ptr, GF_ROCKET, dir, damage, 2);
        break;
    case MS_SHOOT: {
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("矢を放った。", "You fire an arrow."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_SHOOT), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ARROW, dir, damage);
        break;
    }
    case MS_XXX2:
        break;
    case MS_XXX3:
        break;
    case MS_XXX4:
        break;
    case MS_BR_ACID:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("酸のブレスを吐いた。", "You breathe acid."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_ACID), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_ACID, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_ELEC:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("稲妻のブレスを吐いた。", "You breathe lightning."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_ELEC), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_ELEC, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_FIRE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("火炎のブレスを吐いた。", "You breathe fire."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_FIRE), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_FIRE, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_COLD:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("冷気のブレスを吐いた。", "You breathe frost."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_COLD), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_COLD, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_POIS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("ガスのブレスを吐いた。", "You breathe gas."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_POIS), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_POIS, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_NETHER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("地獄のブレスを吐いた。", "You breathe nether."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NETHER), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_NETHER, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_LITE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("閃光のブレスを吐いた。", "You breathe light."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_LITE), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_LITE, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_DARK:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("暗黒のブレスを吐いた。", "You breathe darkness."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DARK), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_DARK, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_CONF:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("混乱のブレスを吐いた。", "You breathe confusion."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_CONF), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_CONFUSION, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_SOUND:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("轟音のブレスを吐いた。", "You breathe sound."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_SOUND), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_SOUND, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_CHAOS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("カオスのブレスを吐いた。", "You breathe chaos."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_CHAOS), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_CHAOS, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_DISEN:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("劣化のブレスを吐いた。", "You breathe disenchantment."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DISEN), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_DISENCHANT, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_NEXUS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("因果混乱のブレスを吐いた。", "You breathe nexus."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NEXUS), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_NEXUS, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_TIME:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("時間逆転のブレスを吐いた。", "You breathe time."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_TIME), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_TIME, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_INERTIA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("遅鈍のブレスを吐いた。", "You breathe inertia."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_INERTIA), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_INERTIAL, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_GRAVITY:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("重力のブレスを吐いた。", "You breathe gravity."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_GRAVITY), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_GRAVITY, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_SHARDS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("破片のブレスを吐いた。", "You breathe shards."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_SHARDS), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_SHARDS, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_PLASMA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("プラズマのブレスを吐いた。", "You breathe plasma."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_PLASMA), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_PLASMA, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_FORCE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("フォースのブレスを吐いた。", "You breathe force."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_FORCE), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_FORCE, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("魔力のブレスを吐いた。", "You breathe mana."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_MANA), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_MANA, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BALL_NUKE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("放射能球を放った。", "You cast a ball of radiation."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_NUKE), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_NUKE, dir, damage, 2);
        break;
    case MS_BR_NUKE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NUKE), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_NUKE, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BALL_CHAOS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("純ログルスを放った。", "You invoke a raw Logrus."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_CHAOS), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_CHAOS, dir, damage, 4);
        break;
    case MS_BR_DISI:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("分解のブレスを吐いた。", "You breathe disintegration."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DISI), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_DISINTEGRATE, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BALL_ACID:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("アシッド・ボールの呪文を唱えた。", "You cast an acid ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_ACID), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_ACID, dir, damage, 2);
        break;
    case MS_BALL_ELEC:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_ELEC), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_ELEC, dir, damage, 2);
        break;
    case MS_BALL_FIRE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("ファイア・ボールの呪文を唱えた。", "You cast a fire ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_FIRE), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_FIRE, dir, damage, 2);
        break;
    case MS_BALL_COLD:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("アイス・ボールの呪文を唱えた。", "You cast a frost ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_COLD), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_COLD, dir, damage, 2);
        break;
    case MS_BALL_POIS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("悪臭雲の呪文を唱えた。", "You cast a stinking cloud."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_POIS), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_POIS, dir, damage, 2);
        break;
    case MS_BALL_NETHER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("地獄球の呪文を唱えた。", "You cast a nether ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_NETHER), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_NETHER, dir, damage, 2);
        break;
    case MS_BALL_WATER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("流れるような身振りをした。", "You gesture fluidly."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_WATER), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_WATER, dir, damage, 4);
        break;
    case MS_BALL_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("魔力の嵐の呪文を念じた。", "You invoke a mana storm."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_MANA), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_MANA, dir, damage, 4);
        break;
    case MS_BALL_DARK:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_DARK), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_DARK, dir, damage, 4);
        break;
    case MS_DRAIN_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_DRAIN_MANA), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_DRAIN_MANA, dir, damage, 0);
        break;
    case MS_MIND_BLAST:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_MIND_BLAST), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_MIND_BLAST, dir, damage, 0);
        break;
    case MS_BRAIN_SMASH:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_BRAIN_SMASH), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_BRAIN_SMASH, dir, damage, 0);
        break;
    case MS_CAUSE_1:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_1), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_1, dir, damage, 0);
        break;
    case MS_CAUSE_2:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_2), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_2, dir, damage, 0);
        break;
    case MS_CAUSE_3:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_3), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_3, dir, damage, 0);
        break;
    case MS_CAUSE_4:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_4), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_4, dir, damage, 0);
        break;
    case MS_BOLT_ACID:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ACID), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ACID, dir, damage);
        break;
    case MS_BOLT_ELEC:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ELEC), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ELEC, dir, damage);
        break;
    case MS_BOLT_FIRE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_FIRE), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_FIRE, dir, damage);
        break;
    case MS_BOLT_COLD:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_COLD), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_COLD, dir, damage);
        break;
    case MS_STARBURST:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("スターバーストの呪文を念じた。", "You invoke a starburst."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_STARBURST), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_LITE, dir, damage, 4);
        break;
    case MS_BOLT_NETHER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("地獄の矢の呪文を唱えた。", "You cast a nether bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_NETHER), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_NETHER, dir, damage);
        break;
    case MS_BOLT_WATER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_WATER), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_WATER, dir, damage);
        break;
    case MS_BOLT_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("魔力の矢の呪文を唱えた。", "You cast a mana bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_MANA), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_MANA, dir, damage);
        break;
    case MS_BOLT_PLASMA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_PLASMA), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_PLASMA, dir, damage);
        break;
    case MS_BOLT_ICE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("極寒の矢の呪文を唱えた。", "You cast a ice bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ICE), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ICE, dir, damage);
        break;
    case MS_MAGIC_MISSILE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_MAGIC_MISSILE), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_MISSILE, dir, damage);
        break;
    case MS_SCARE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("恐ろしげな幻覚を作り出した。", "You cast a fearful illusion."));
        fear_monster(caster_ptr, dir, plev + 10);
        break;
    case MS_BLIND:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        confuse_monster(caster_ptr, dir, plev * 2);
        break;
    case MS_CONF:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("誘惑的な幻覚をつくり出した。", "You cast a mesmerizing illusion."));
        confuse_monster(caster_ptr, dir, plev * 2);
        break;
    case MS_SLOW:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        slow_monster(caster_ptr, dir, plev);
        break;
    case MS_SLEEP:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        sleep_monster(caster_ptr, dir, plev);
        break;
    case MS_SPEED:
        (void)set_fast(caster_ptr, randint1(20 + plev) + plev, FALSE);
        break;
    case MS_HAND_DOOM: {
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
        fire_ball_hide(caster_ptr, GF_HAND_DOOM, dir, plev * 3, 0);
        break;
    }
    case MS_HEAL:
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
        (void)hp_player(caster_ptr, plev * 4);
        (void)set_stun(caster_ptr, 0);
        (void)set_cut(caster_ptr, 0);
        break;
    case MS_INVULNER:
        msg_print(_("無傷の球の呪文を唱えた。", "You cast a Globe of Invulnerability."));
        (void)set_invuln(caster_ptr, randint1(4) + 4, FALSE);
        break;
    case MS_BLINK:
        teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case MS_TELEPORT:
        teleport_player(caster_ptr, plev * 5, TELEPORT_SPONTANEOUS);
        break;
    case MS_WORLD:
        (void)time_walk(caster_ptr);
        break;
    case MS_SPECIAL:
        break;
    case MS_TELE_TO: {
        monster_type *m_ptr;
        monster_race *r_ptr;
        GAME_TEXT m_name[MAX_NLEN];

        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        if (!floor_ptr->grid_array[target_row][target_col].m_idx)
            break;
        if (!player_has_los_bold(caster_ptr, target_row, target_col))
            break;
        if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
            break;
        m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[target_row][target_col].m_idx];
        r_ptr = &r_info[m_ptr->r_idx];
        monster_desc(caster_ptr, m_name, m_ptr, 0);
        if (r_ptr->flagsr & RFR_RES_TELE) {
            if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flagsr & RFR_RES_ALL)) {
                if (is_original_ap_and_seen(caster_ptr, m_ptr))
                    r_ptr->r_flagsr |= RFR_RES_TELE;
                msg_format(_("%sには効果がなかった！", "%s is unaffected!"), m_name);
                break;
            } else if (r_ptr->level > randint1(100)) {
                if (is_original_ap_and_seen(caster_ptr, m_ptr))
                    r_ptr->r_flagsr |= RFR_RES_TELE;
                msg_format(_("%sには耐性がある！", "%s resists!"), m_name);
                break;
            }
        }

        msg_format(_("%sを引き戻した。", "You command %s to return."), m_name);
        teleport_monster_to(caster_ptr, floor_ptr->grid_array[target_row][target_col].m_idx, caster_ptr->y, caster_ptr->x, 100, TELEPORT_PASSIVE);
        break;
    }
    case MS_TELE_AWAY:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        (void)fire_beam(caster_ptr, GF_AWAY_ALL, dir, 100);
        break;

    case MS_TELE_LEVEL:
        return teleport_level_other(caster_ptr);
        break;

    case MS_PSY_SPEAR:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_PSY_SPEAR), plev, DAM_ROLL);
        (void)fire_beam(caster_ptr, GF_PSY_SPEAR, dir, damage);
        break;
    case MS_DARKNESS:

        msg_print(_("暗闇の中で手を振った。", "You gesture in shadow."));
        (void)unlite_area(caster_ptr, 10, 3);
        break;
    case MS_MAKE_TRAP:
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;

        msg_print(_("呪文を唱えて邪悪に微笑んだ。", "You cast a spell and cackle evilly."));
        trap_creation(caster_ptr, target_row, target_col);
        break;
    case MS_FORGET:
        msg_print(_("しかし何も起きなかった。", "Nothing happen."));
        break;
    case MS_RAISE_DEAD:
        msg_print(_("死者復活の呪文を唱えた。", "You animate the dead."));
        (void)animate_dead(caster_ptr, 0, caster_ptr->y, caster_ptr->x);
        break;
    case MS_S_KIN: {
        msg_print(_("援軍を召喚した。", "You summon one of your kin."));
        for (int k = 0; k < 1; k++) {
            if (summon_kin_player(caster_ptr, summon_lev, caster_ptr->y, caster_ptr->x, (pet ? PM_FORCE_PET : 0L))) {
                if (!pet)
                    msg_print(_("召喚された仲間は怒っている！", "The summoned companion is angry!"));
            } else {
                no_trump = TRUE;
            }
        }

        break;
    }
    case MS_S_CYBER: {
        msg_print(_("サイバーデーモンを召喚した！", "You summon a Cyberdemon!"));
        for (int k = 0; k < 1; k++) {
            if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_CYBER, p_mode)) {
                if (!pet)
                    msg_print(_("召喚されたサイバーデーモンは怒っている！", "The summoned Cyberdemon are angry!"));
            } else {
                no_trump = TRUE;
            }
        }
        break;
    }
    case MS_S_MONSTER: {
        msg_print(_("仲間を召喚した。", "You summon help."));
        for (int k = 0; k < 1; k++) {
            if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, 0, p_mode)) {
                if (!pet)
                    msg_print(_("召喚されたモンスターは怒っている！", "The summoned monster is angry!"));
            } else {
                no_trump = TRUE;
            }
        }

        break;
    }
    case MS_S_MONSTERS: {
        msg_print(_("モンスターを召喚した！", "You summon monsters!"));
        for (int k = 0; k < plev / 15 + 2; k++) {
            if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, 0, (p_mode | u_mode))) {
                if (!pet)
                    msg_print(_("召喚されたモンスターは怒っている！", "The summoned monsters are angry!"));
            } else {
                no_trump = TRUE;
            }
        }

        break;
    }
    case MS_S_ANT: {
        msg_print(_("アリを召喚した。", "You summon ants."));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_ANT, (PM_ALLOW_GROUP | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたアリは怒っている！", "The summoned ants are angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_SPIDER: {
        msg_print(_("蜘蛛を召喚した。", "You summon spiders."));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_SPIDER, (PM_ALLOW_GROUP | p_mode))) {
            if (!pet)
                msg_print(_("召喚された蜘蛛は怒っている！", "Summoned spiders are angry!"));
        } else {
            no_trump = TRUE;
        }

        break;
    }
    case MS_S_HOUND: {
        msg_print(_("ハウンドを召喚した。", "You summon hounds."));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HOUND, (PM_ALLOW_GROUP | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたハウンドは怒っている！", "Summoned hounds are angry!"));
        } else {
            no_trump = TRUE;
        }

        break;
    }
    case MS_S_HYDRA: {
        msg_print(_("ヒドラを召喚した。", "You summon a hydras."));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HYDRA, (g_mode | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたヒドラは怒っている！", "Summoned hydras are angry!"));
        } else {
            no_trump = TRUE;
        }

        break;
    }
    case MS_S_ANGEL: {
        msg_print(_("天使を召喚した！", "You summon an angel!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_ANGEL, (g_mode | p_mode))) {
            if (!pet)
                msg_print(_("召喚された天使は怒っている！", "The summoned angel is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_DEMON: {
        msg_print(_("混沌の宮廷から悪魔を召喚した！", "You summon a demon from the Courts of Chaos!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_DEMON, (g_mode | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたデーモンは怒っている！", "The summoned demon is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_UNDEAD: {
        msg_print(_("アンデッドの強敵を召喚した！", "You summon an undead adversary!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_UNDEAD, (g_mode | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたアンデッドは怒っている！", "The summoned undead is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_DRAGON: {
        msg_print(_("ドラゴンを召喚した！", "You summon a dragon!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_DRAGON, (g_mode | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたドラゴンは怒っている！", "The summoned dragon is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_HI_UNDEAD: {
        msg_print(_("強力なアンデッドを召喚した！", "You summon a greater undead!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HI_UNDEAD, (g_mode | p_mode | u_mode))) {
            if (!pet)
                msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_HI_DRAGON: {
        msg_print(_("古代ドラゴンを召喚した！", "You summon an ancient dragon!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HI_DRAGON, (g_mode | p_mode | u_mode))) {
            if (!pet)
                msg_print(_("召喚された古代ドラゴンは怒っている！", "The summoned ancient dragon is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_AMBERITE: {
        msg_print(_("アンバーの王族を召喚した！", "You summon a Lord of Amber!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_AMBERITES, (g_mode | p_mode | u_mode))) {
            if (!pet)
                msg_print(_("召喚されたアンバーの王族は怒っている！", "The summoned Lord of Amber is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_UNIQUE: {
        int k, count = 0;
        msg_print(_("特別な強敵を召喚した！", "You summon a special opponent!"));
        for (k = 0; k < 1; k++) {
            if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_UNIQUE, (g_mode | p_mode | PM_ALLOW_UNIQUE))) {
                count++;
                if (!pet)
                    msg_print(_("召喚されたユニーク・モンスターは怒っている！", "The summoned special opponent is angry!"));
            }
        }

        for (k = count; k < 1; k++) {
            if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HI_UNDEAD, (g_mode | p_mode | PM_ALLOW_UNIQUE))) {
                count++;
                if (!pet)
                    msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead is angry!"));
            }
        }

        if (!count) {
            no_trump = TRUE;
        }

        break;
    }
    default:
        msg_print("hoge?");
    }

    if (no_trump) {
        msg_print(_("何も現れなかった。", "No one appeared."));
    }

    return TRUE;
}

/*!
 * @brief 青魔法コマンドのメインルーチン /
 * do_cmd_cast calls this function if the player's class is 'Blue-Mage'.
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool do_cmd_cast_learned(player_type *caster_ptr)
{
    SPELL_IDX n = 0;
    PERCENTAGE chance;
    PERCENTAGE minfail = 0;
    PLAYER_LEVEL plev = caster_ptr->lev;
    monster_power spell;
    bool cast;
    MANA_POINT need_mana;

    if (cmd_limit_confused(caster_ptr))
        return FALSE;

    if (!get_learned_power(caster_ptr, &n))
        return FALSE;

    spell = monster_powers[n];
    need_mana = mod_need_mana(caster_ptr, spell.smana, 0, REALM_NONE);
    if (need_mana > caster_ptr->csp) {
        msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));
        if (!over_exert)
            return FALSE;

        if (!get_check(_("それでも挑戦しますか? ", "Attempt it anyway? ")))
            return FALSE;
    }

    chance = spell.fail;
    if (plev > spell.level)
        chance -= 3 * (plev - spell.level);
    else
        chance += (spell.level - plev);

    chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[A_INT]] - 1);
    chance = mod_spell_chance_1(caster_ptr, chance);
    if (need_mana > caster_ptr->csp) {
        chance += 5 * (need_mana - caster_ptr->csp);
    }

    minfail = adj_mag_fail[caster_ptr->stat_ind[A_INT]];
    if (chance < minfail)
        chance = minfail;

    if (caster_ptr->stun > 50)
        chance += 25;
    else if (caster_ptr->stun)
        chance += 15;

    if (chance > 95)
        chance = 95;

    chance = mod_spell_chance_2(caster_ptr, chance);
    if (randint0(100) < chance) {
        if (flush_failure)
            flush();
        msg_print(_("魔法をうまく唱えられなかった。", "You failed to concentrate hard enough!"));
        sound(SOUND_FAIL);
        if (n >= MS_S_KIN)
            cast = cast_learned_spell(caster_ptr, n, FALSE);
    } else {
        sound(SOUND_ZAP);
        cast = cast_learned_spell(caster_ptr, n, TRUE);
        if (!cast)
            return FALSE;
    }

    if (need_mana <= caster_ptr->csp) {
        caster_ptr->csp -= need_mana;
    } else {
        int oops = need_mana;
        caster_ptr->csp = 0;
        caster_ptr->csp_frac = 0;
        msg_print(_("精神を集中しすぎて気を失ってしまった！", "You faint from the effort!"));
        (void)set_paralyzed(caster_ptr, caster_ptr->paralyzed + randint1(5 * oops + 1));
        chg_virtue(caster_ptr, V_KNOWLEDGE, -10);
        if (randint0(100) < 50) {
            bool perm = (randint0(100) < 25);
            msg_print(_("体を悪くしてしまった！", "You have damaged your health!"));
            (void)dec_stat(caster_ptr, A_CON, 15 + randint1(10), perm);
        }
    }

    take_turn(caster_ptr, 100);
    caster_ptr->redraw |= (PR_MANA);
    caster_ptr->window |= (PW_PLAYER | PW_SPELL);
    return TRUE;
}

/*!
 * @brief 青魔法のラーニング判定と成功した場合のラーニング処理
 * @param monspell ラーニングを試みるモンスター攻撃のID
 * @return なし
 */
void learn_spell(player_type *learner_ptr, int monspell)
{
    if (learner_ptr->action != ACTION_LEARN)
        return;
    if (monspell < 0)
        return;
    if (learner_ptr->magic_num2[monspell])
        return;
    if (learner_ptr->confused || learner_ptr->blind || learner_ptr->image || learner_ptr->stun || learner_ptr->paralyzed)
        return;
    if (randint1(learner_ptr->lev + 70) > monster_powers[monspell].level + 40) {
        learner_ptr->magic_num2[monspell] = 1;
        msg_format(_("%sを学習した！", "You have learned %s!"), monster_powers[monspell].name);
        gain_exp(learner_ptr, monster_powers[monspell].level * monster_powers[monspell].smana);

        sound(SOUND_STUDY);

        learner_ptr->new_mane = TRUE;
        learner_ptr->redraw |= (PR_STATE);
    }
}

/*!
 * todo f4, f5, f6を構造体にまとめ直す
 * @brief モンスター特殊能力のフラグ配列から特定条件の魔法だけを抜き出す処理
 * Extract monster spells mask for the given mode
 * @param f4 モンスター特殊能力の4番目のフラグ配列
 * @param f5 モンスター特殊能力の5番目のフラグ配列
 * @param f6 モンスター特殊能力の6番目のフラグ配列
 * @param mode 抜き出したい条件
 * @return なし
 */
/*
 */
void set_rf_masks(BIT_FLAGS *f4, BIT_FLAGS *f5, BIT_FLAGS *f6, BIT_FLAGS mode)
{
    switch (mode) {
    case MONSPELL_TYPE_BOLT:
        *f4 = ((RF4_BOLT_MASK | RF4_BEAM_MASK) & ~(RF4_ROCKET));
        *f5 = RF5_BOLT_MASK | RF5_BEAM_MASK;
        *f6 = RF6_BOLT_MASK | RF6_BEAM_MASK;
        break;

    case MONSPELL_TYPE_BALL:
        *f4 = (RF4_BALL_MASK & ~(RF4_BREATH_MASK));
        *f5 = (RF5_BALL_MASK & ~(RF5_BREATH_MASK));
        *f6 = (RF6_BALL_MASK & ~(RF6_BREATH_MASK));
        break;

    case MONSPELL_TYPE_BREATH:
        *f4 = (BIT_FLAGS)RF4_BREATH_MASK;
        *f5 = RF5_BREATH_MASK;
        *f6 = RF6_BREATH_MASK;
        break;

    case MONSPELL_TYPE_SUMMON:
        *f4 = RF4_SUMMON_MASK;
        *f5 = RF5_SUMMON_MASK;
        *f6 = (BIT_FLAGS)RF6_SUMMON_MASK;
        break;

    case MONSPELL_TYPE_OTHER:
        *f4 = RF4_ATTACK_MASK & ~(RF4_BOLT_MASK | RF4_BEAM_MASK | RF4_BALL_MASK | RF4_INDIRECT_MASK);
        *f5 = RF5_ATTACK_MASK & ~(RF5_BOLT_MASK | RF5_BEAM_MASK | RF5_BALL_MASK | RF5_INDIRECT_MASK);
        *f6 = RF6_ATTACK_MASK & ~(RF6_BOLT_MASK | RF6_BEAM_MASK | RF6_BALL_MASK | RF6_INDIRECT_MASK);
        break;
    }
}
