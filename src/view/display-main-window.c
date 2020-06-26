/*!
 * @brief プレイヤーのステータス処理 / status
 * @date 2018/09/25
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "view/display-main-window.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "cmd-building/cmd-building.h"
#include "core/player-processor.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "game-option/map-screen-options.h"
#include "game-option/option-flags.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/player-inventory.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-processor.h"
#include "io/input-key-requester.h"
#include "io/targeting.h"
#include "main/sound-of-music.h"
#include "market/arena-info-table.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "object/object-flavor.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "player/avatar.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-effects.h"
#include "player/player-race-types.h"
#include "player/player-status.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "spell/spells3.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-lore.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "view/main-window-row-column.h"
#include "view/main-window-util.h"
#include "view/object-describer.h"
#include "view/status-bars-table.h"
#include "world/world.h"

/*
 * Not using graphical tiles for this feature?
 */
#define IS_ASCII_GRAPHICS(A) (!((A)&0x80))

static int feat_priority; /*!< マップ縮小表示時に表示すべき地形の優先度を保管する */
static byte display_autopick; /*!< 自動拾い状態の設定フラグ */
static int match_autopick;
static object_type *autopick_obj; /*!< 各種自動拾い処理時に使うオブジェクトポインタ */

/*
 * Dungeon size info
 */
POSITION panel_row_min, panel_row_max;
POSITION panel_col_min, panel_col_max;
POSITION panel_col_prt, panel_row_prt;

void print_map(player_type *player_ptr);
void display_map(player_type *player_ptr, int *cy, int *cx);
void set_term_color(player_type *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp);

/*!
 * @brief ゲーム時刻を表示する /
 * Print time
 * @return なし
 */
void print_time(player_type *player_ptr)
{
    int day, hour, min;
    c_put_str(TERM_WHITE, "             ", ROW_DAY, COL_DAY);
    extract_day_hour_min(player_ptr, &day, &hour, &min);
    if (day < 1000)
        c_put_str(TERM_WHITE, format(_("%2d日目", "Day%3d"), day), ROW_DAY, COL_DAY);
    else
        c_put_str(TERM_WHITE, _("***日目", "Day***"), ROW_DAY, COL_DAY);

    c_put_str(TERM_WHITE, format("%2d:%02d", hour, min), ROW_DAY, COL_DAY + 7);
}

/*!
 * @brief 現在のマップ名を返す /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return マップ名の文字列参照ポインタ
 */
concptr map_name(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest) && (quest[floor_ptr->inside_quest].flags & QUEST_FLAG_PRESET))
        return _("クエスト", "Quest");
    else if (creature_ptr->wild_mode)
        return _("地上", "Surface");
    else if (creature_ptr->current_floor_ptr->inside_arena)
        return _("アリーナ", "Arena");
    else if (creature_ptr->phase_out)
        return _("闘技場", "Monster Arena");
    else if (!floor_ptr->dun_level && creature_ptr->town_num)
        return town_info[creature_ptr->town_num].name;
    else
        return d_name + d_info[creature_ptr->dungeon_idx].name;
}

/*!
 * @brief 現在のマップ名を描画する / Print dungeon
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void print_dungeon(player_type *creature_ptr)
{
    c_put_str(TERM_WHITE, "             ", ROW_DUNGEON, COL_DUNGEON);
    concptr dungeon_name = map_name(creature_ptr);
    TERM_LEN col = COL_DUNGEON + 6 - strlen(dungeon_name) / 2;
    if (col < 0)
        col = 0;

    c_put_str(TERM_L_UMBER, format("%s", dungeon_name), ROW_DUNGEON, col);
}

/*!
 * @brief プレイヤー能力値を描画する / Print character stat in given row, column
 * @param stat 描画するステータスのID
 * @return なし
 */
static void print_stat(player_type *creature_ptr, int stat)
{
    GAME_TEXT tmp[32];
    if (creature_ptr->stat_cur[stat] < creature_ptr->stat_max[stat]) {
        put_str(stat_names_reduced[stat], ROW_STAT + stat, 0);
        cnv_stat(creature_ptr->stat_use[stat], tmp);
        c_put_str(TERM_YELLOW, tmp, ROW_STAT + stat, COL_STAT + 6);
    } else {
        put_str(stat_names[stat], ROW_STAT + stat, 0);
        cnv_stat(creature_ptr->stat_use[stat], tmp);
        c_put_str(TERM_L_GREEN, tmp, ROW_STAT + stat, COL_STAT + 6);
    }

    if (creature_ptr->stat_max[stat] != creature_ptr->stat_max_max[stat])
        return;

#ifdef JP
    /* 日本語にかぶらないように表示位置を変更 */
    put_str("!", ROW_STAT + stat, 5);
#else
    put_str("!", ROW_STAT + stat, 3);
#endif
}

/*!
 * @brief 32ビット変数配列の指定位置のビットフラグを1にする。
 * @param FLG フラグ位置(ビット)
 * @return なし
 */
#define ADD_BAR_FLAG(FLG) (bar_flags[FLG / 32] |= (1L << (FLG % 32)))

/*!
 * @brief 32ビット変数配列の指定位置のビットフラグが1かどうかを返す。
 * @param FLG フラグ位置(ビット)
 * @return 1ならば0以外を返す
 */
#define IS_BAR_FLAG(FLG) (bar_flags[FLG / 32] & (1L << (FLG % 32)))

/*!
 * @brief 下部に状態表示を行う / Show status bar
 * @return なし
 */
static void print_status(player_type *creature_ptr)
{
    TERM_LEN wid, hgt;
    Term_get_size(&wid, &hgt);
    TERM_LEN row_statbar = hgt + ROW_STATBAR;
    TERM_LEN max_col_statbar = wid + MAX_COL_STATBAR;

    Term_erase(0, row_statbar, max_col_statbar);

    BIT_FLAGS bar_flags[3];
    bar_flags[0] = bar_flags[1] = bar_flags[2] = 0L;

    if (creature_ptr->tsuyoshi)
        ADD_BAR_FLAG(BAR_TSUYOSHI);

    if (creature_ptr->image)
        ADD_BAR_FLAG(BAR_HALLUCINATION);

    if (creature_ptr->blind)
        ADD_BAR_FLAG(BAR_BLINDNESS);

    if (creature_ptr->paralyzed)
        ADD_BAR_FLAG(BAR_PARALYZE);

    if (creature_ptr->confused)
        ADD_BAR_FLAG(BAR_CONFUSE);

    if (creature_ptr->poisoned)
        ADD_BAR_FLAG(BAR_POISONED);

    if (creature_ptr->tim_invis)
        ADD_BAR_FLAG(BAR_SENSEUNSEEN);

    if (is_time_limit_esp(creature_ptr))
        ADD_BAR_FLAG(BAR_TELEPATHY);

    if (creature_ptr->tim_regen)
        ADD_BAR_FLAG(BAR_REGENERATION);

    if (creature_ptr->tim_infra)
        ADD_BAR_FLAG(BAR_INFRAVISION);

    if (creature_ptr->protevil)
        ADD_BAR_FLAG(BAR_PROTEVIL);

    if (is_invuln(creature_ptr))
        ADD_BAR_FLAG(BAR_INVULN);

    if (creature_ptr->wraith_form)
        ADD_BAR_FLAG(BAR_WRAITH);

    if (creature_ptr->kabenuke)
        ADD_BAR_FLAG(BAR_PASSWALL);

    if (creature_ptr->tim_reflect)
        ADD_BAR_FLAG(BAR_REFLECTION);

    if (is_hero(creature_ptr))
        ADD_BAR_FLAG(BAR_HEROISM);

    if (creature_ptr->shero)
        ADD_BAR_FLAG(BAR_BERSERK);

    if (is_blessed(creature_ptr))
        ADD_BAR_FLAG(BAR_BLESSED);

    if (creature_ptr->magicdef)
        ADD_BAR_FLAG(BAR_MAGICDEFENSE);

    if (creature_ptr->tsubureru)
        ADD_BAR_FLAG(BAR_EXPAND);

    if (creature_ptr->shield)
        ADD_BAR_FLAG(BAR_STONESKIN);

    if (creature_ptr->special_defense & NINJA_KAWARIMI)
        ADD_BAR_FLAG(BAR_KAWARIMI);

    if (creature_ptr->special_defense & DEFENSE_ACID)
        ADD_BAR_FLAG(BAR_IMMACID);
    if (is_oppose_acid(creature_ptr))
        ADD_BAR_FLAG(BAR_RESACID);

    if (creature_ptr->special_defense & DEFENSE_ELEC)
        ADD_BAR_FLAG(BAR_IMMELEC);
    if (is_oppose_elec(creature_ptr))
        ADD_BAR_FLAG(BAR_RESELEC);

    if (creature_ptr->special_defense & DEFENSE_FIRE)
        ADD_BAR_FLAG(BAR_IMMFIRE);
    if (is_oppose_fire(creature_ptr))
        ADD_BAR_FLAG(BAR_RESFIRE);

    if (creature_ptr->special_defense & DEFENSE_COLD)
        ADD_BAR_FLAG(BAR_IMMCOLD);
    if (is_oppose_cold(creature_ptr))
        ADD_BAR_FLAG(BAR_RESCOLD);

    if (is_oppose_pois(creature_ptr))
        ADD_BAR_FLAG(BAR_RESPOIS);

    if (creature_ptr->word_recall)
        ADD_BAR_FLAG(BAR_RECALL);

    if (creature_ptr->alter_reality)
        ADD_BAR_FLAG(BAR_ALTER);

    if (creature_ptr->afraid)
        ADD_BAR_FLAG(BAR_AFRAID);

    if (creature_ptr->tim_res_time)
        ADD_BAR_FLAG(BAR_RESTIME);

    if (creature_ptr->multishadow)
        ADD_BAR_FLAG(BAR_MULTISHADOW);

    if (creature_ptr->special_attack & ATTACK_CONFUSE)
        ADD_BAR_FLAG(BAR_ATTKCONF);

    if (creature_ptr->resist_magic)
        ADD_BAR_FLAG(BAR_REGMAGIC);

    if (creature_ptr->ult_res)
        ADD_BAR_FLAG(BAR_ULTIMATE);

    if (creature_ptr->tim_levitation)
        ADD_BAR_FLAG(BAR_LEVITATE);

    if (creature_ptr->tim_res_nether)
        ADD_BAR_FLAG(BAR_RESNETH);

    if (creature_ptr->dustrobe)
        ADD_BAR_FLAG(BAR_DUSTROBE);

    if (creature_ptr->special_attack & ATTACK_FIRE)
        ADD_BAR_FLAG(BAR_ATTKFIRE);
    if (creature_ptr->special_attack & ATTACK_COLD)
        ADD_BAR_FLAG(BAR_ATTKCOLD);
    if (creature_ptr->special_attack & ATTACK_ELEC)
        ADD_BAR_FLAG(BAR_ATTKELEC);
    if (creature_ptr->special_attack & ATTACK_ACID)
        ADD_BAR_FLAG(BAR_ATTKACID);
    if (creature_ptr->special_attack & ATTACK_POIS)
        ADD_BAR_FLAG(BAR_ATTKPOIS);
    if (creature_ptr->special_defense & NINJA_S_STEALTH)
        ADD_BAR_FLAG(BAR_SUPERSTEALTH);

    if (creature_ptr->tim_sh_fire)
        ADD_BAR_FLAG(BAR_SHFIRE);

    if (is_time_limit_stealth(creature_ptr))
        ADD_BAR_FLAG(BAR_STEALTH);

    if (creature_ptr->tim_sh_touki)
        ADD_BAR_FLAG(BAR_TOUKI);

    if (creature_ptr->tim_sh_holy)
        ADD_BAR_FLAG(BAR_SHHOLY);

    if (creature_ptr->tim_eyeeye)
        ADD_BAR_FLAG(BAR_EYEEYE);

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_BLESS))
            ADD_BAR_FLAG(BAR_BLESSED);
        if (hex_spelling(creature_ptr, HEX_DEMON_AURA)) {
            ADD_BAR_FLAG(BAR_SHFIRE);
            ADD_BAR_FLAG(BAR_REGENERATION);
        }
        if (hex_spelling(creature_ptr, HEX_XTRA_MIGHT))
            ADD_BAR_FLAG(BAR_MIGHT);
        if (hex_spelling(creature_ptr, HEX_DETECT_EVIL))
            ADD_BAR_FLAG(BAR_ESP_EVIL);
        if (hex_spelling(creature_ptr, HEX_ICE_ARMOR))
            ADD_BAR_FLAG(BAR_SHCOLD);
        if (hex_spelling(creature_ptr, HEX_RUNESWORD))
            ADD_BAR_FLAG(BAR_RUNESWORD);
        if (hex_spelling(creature_ptr, HEX_BUILDING))
            ADD_BAR_FLAG(BAR_BUILD);
        if (hex_spelling(creature_ptr, HEX_ANTI_TELE))
            ADD_BAR_FLAG(BAR_ANTITELE);
        if (hex_spelling(creature_ptr, HEX_SHOCK_CLOAK))
            ADD_BAR_FLAG(BAR_SHELEC);
        if (hex_spelling(creature_ptr, HEX_SHADOW_CLOAK))
            ADD_BAR_FLAG(BAR_SHSHADOW);
        if (hex_spelling(creature_ptr, HEX_CONFUSION))
            ADD_BAR_FLAG(BAR_ATTKCONF);
        if (hex_spelling(creature_ptr, HEX_EYE_FOR_EYE))
            ADD_BAR_FLAG(BAR_EYEEYE);
        if (hex_spelling(creature_ptr, HEX_ANTI_MULTI))
            ADD_BAR_FLAG(BAR_ANTIMULTI);
        if (hex_spelling(creature_ptr, HEX_VAMP_BLADE))
            ADD_BAR_FLAG(BAR_VAMPILIC);
        if (hex_spelling(creature_ptr, HEX_ANTI_MAGIC))
            ADD_BAR_FLAG(BAR_ANTIMAGIC);
        if (hex_spelling(creature_ptr, HEX_CURE_LIGHT) || hex_spelling(creature_ptr, HEX_CURE_SERIOUS) || hex_spelling(creature_ptr, HEX_CURE_CRITICAL))
            ADD_BAR_FLAG(BAR_CURE);

        if (hex_revenge_turn(creature_ptr)) {
            if (hex_revenge_type(creature_ptr) == 1)
                ADD_BAR_FLAG(BAR_PATIENCE);
            if (hex_revenge_type(creature_ptr) == 2)
                ADD_BAR_FLAG(BAR_REVENGE);
        }
    }

    TERM_LEN col = 0, num = 0;
    for (int i = 0; stat_bars[i].sstr; i++) {
        if (IS_BAR_FLAG(i)) {
            col += strlen(stat_bars[i].lstr) + 1;
            num++;
        }
    }

    int space = 2;
    if (col - 1 > max_col_statbar) {
        space = 0;
        col = 0;

        for (int i = 0; stat_bars[i].sstr; i++) {
            if (IS_BAR_FLAG(i)) {
                col += strlen(stat_bars[i].sstr);
            }
        }

        if (col - 1 <= max_col_statbar - (num - 1)) {
            space = 1;
            col += num - 1;
        }
    }

    col = (max_col_statbar - col) / 2;
    for (int i = 0; stat_bars[i].sstr; i++) {
        if (!IS_BAR_FLAG(i))
            continue;

        concptr str;
        if (space == 2)
            str = stat_bars[i].lstr;
        else
            str = stat_bars[i].sstr;

        c_put_str(stat_bars[i].attr, str, row_statbar, col);
        col += strlen(str);
        if (space > 0)
            col++;
        if (col > max_col_statbar)
            break;
    }
}

/*!
 * @brief プレイヤーの称号を表示する / Prints "title", including "wizard" or "winner" as needed.
 * @return なし
 */
static void print_title(player_type *creature_ptr)
{
    GAME_TEXT str[14];
    concptr p = "";
    if (current_world_ptr->wizard) {
        p = _("[ウィザード]", "[=-WIZARD-=]");
    } else if (current_world_ptr->total_winner || (creature_ptr->lev > PY_MAX_LEVEL)) {
        if (creature_ptr->arena_number > MAX_ARENA_MONS + 2) {
            p = _("*真・勝利者*", "*TRUEWINNER*");
        } else {
            p = _("***勝利者***", "***WINNER***");
        }
    } else {
        angband_strcpy(str, player_title[creature_ptr->pclass][(creature_ptr->lev - 1) / 5], sizeof(str));
        p = str;
    }

    print_field(p, ROW_TITLE, COL_TITLE);
}

/*!
 * @brief プレイヤーのレベルを表示する / Prints level
 * @return なし
 */
static void print_level(player_type *creature_ptr)
{
    char tmp[32];
    sprintf(tmp, "%5d", creature_ptr->lev);
    if (creature_ptr->lev >= creature_ptr->max_plv) {
        put_str(_("レベル ", "LEVEL "), ROW_LEVEL, 0);
        c_put_str(TERM_L_GREEN, tmp, ROW_LEVEL, COL_LEVEL + 7);
    } else {
        put_str(_("xレベル", "Level "), ROW_LEVEL, 0);
        c_put_str(TERM_YELLOW, tmp, ROW_LEVEL, COL_LEVEL + 7);
    }
}

/*!
 * @brief プレイヤーの経験値を表示する / Display the experience
 * @return なし
 */
static void print_exp(player_type *creature_ptr)
{
    char out_val[32];

    if ((!exp_need) || (creature_ptr->prace == RACE_ANDROID)) {
        (void)sprintf(out_val, "%8ld", (long)creature_ptr->exp);
    } else {
        if (creature_ptr->lev >= PY_MAX_LEVEL) {
            (void)sprintf(out_val, "********");
        } else {
            (void)sprintf(out_val, "%8ld", (long)(player_exp[creature_ptr->lev - 1] * creature_ptr->expfact / 100L) - creature_ptr->exp);
        }
    }

    if (creature_ptr->exp >= creature_ptr->max_exp) {
        if (creature_ptr->prace == RACE_ANDROID)
            put_str(_("強化 ", "Cst "), ROW_EXP, 0);
        else
            put_str(_("経験 ", "EXP "), ROW_EXP, 0);
        c_put_str(TERM_L_GREEN, out_val, ROW_EXP, COL_EXP + 4);
    } else {
        put_str(_("x経験", "Exp "), ROW_EXP, 0);
        c_put_str(TERM_YELLOW, out_val, ROW_EXP, COL_EXP + 4);
    }
}

/*!
 * @brief プレイヤーの所持金を表示する / Prints current gold
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void print_gold(player_type *creature_ptr)
{
    char tmp[32];
    put_str(_("＄ ", "AU "), ROW_GOLD, COL_GOLD);
    sprintf(tmp, "%9ld", (long)creature_ptr->au);
    c_put_str(TERM_L_GREEN, tmp, ROW_GOLD, COL_GOLD + 3);
}

/*!
 * @brief プレイヤーのACを表示する / Prints current AC
 * @return なし
 */
static void print_ac(player_type *creature_ptr)
{
    char tmp[32];

#ifdef JP
    /* AC の表示方式を変更している */
    put_str(" ＡＣ(     )", ROW_AC, COL_AC);
    sprintf(tmp, "%5d", creature_ptr->dis_ac + creature_ptr->dis_to_a);
    c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 6);
#else
    put_str("Cur AC ", ROW_AC, COL_AC);
    sprintf(tmp, "%5d", creature_ptr->dis_ac + creature_ptr->dis_to_a);
    c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 7);
#endif
}

/*!
 * @brief プレイヤーのHPを表示する / Prints Cur/Max hit points
 * @return なし
 */
static void print_hp(player_type *creature_ptr)
{
    char tmp[32];
    put_str("HP", ROW_CURHP, COL_CURHP);
    sprintf(tmp, "%4ld", (long int)creature_ptr->chp);
    TERM_COLOR color;
    if (creature_ptr->chp >= creature_ptr->mhp) {
        color = TERM_L_GREEN;
    } else if (creature_ptr->chp > (creature_ptr->mhp * hitpoint_warn) / 10) {
        color = TERM_YELLOW;
    } else {
        color = TERM_RED;
    }

    c_put_str(color, tmp, ROW_CURHP, COL_CURHP + 3);
    put_str("/", ROW_CURHP, COL_CURHP + 7);
    sprintf(tmp, "%4ld", (long int)creature_ptr->mhp);
    color = TERM_L_GREEN;
    c_put_str(color, tmp, ROW_CURHP, COL_CURHP + 8);
}

/*!
 * @brief プレイヤーのMPを表示する / Prints players max/cur spell points
 * @return なし
 */
static void print_sp(player_type *creature_ptr)
{
    char tmp[32];
    byte color;
    if (!mp_ptr->spell_book)
        return;

    put_str(_("MP", "SP"), ROW_CURSP, COL_CURSP);
    sprintf(tmp, "%4ld", (long int)creature_ptr->csp);
    if (creature_ptr->csp >= creature_ptr->msp) {
        color = TERM_L_GREEN;
    } else if (creature_ptr->csp > (creature_ptr->msp * mana_warn) / 10) {
        color = TERM_YELLOW;
    } else {
        color = TERM_RED;
    }

    c_put_str(color, tmp, ROW_CURSP, COL_CURSP + 3);
    put_str("/", ROW_CURSP, COL_CURSP + 7);
    sprintf(tmp, "%4ld", (long int)creature_ptr->msp);
    color = TERM_L_GREEN;
    c_put_str(color, tmp, ROW_CURSP, COL_CURSP + 8);
}

/*!
 * @brief 現在のフロアの深さを表示する / Prints depth in stat area
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void print_depth(player_type *creature_ptr)
{
    char depths[32];
    TERM_COLOR attr = TERM_WHITE;

    TERM_LEN wid, hgt;
    Term_get_size(&wid, &hgt);
    TERM_LEN col_depth = wid + COL_DEPTH;
    TERM_LEN row_depth = hgt + ROW_DEPTH;

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (!floor_ptr->dun_level) {
        strcpy(depths, _("地上", "Surf."));
        c_prt(attr, format("%7s", depths), row_depth, col_depth);
        return;
    }

    if (floor_ptr->inside_quest && !creature_ptr->dungeon_idx) {
        strcpy(depths, _("地上", "Quest"));
        c_prt(attr, format("%7s", depths), row_depth, col_depth);
        return;
    }

    if (depth_in_feet)
        (void)sprintf(depths, _("%d ft", "%d ft"), (int)floor_ptr->dun_level * 50);
    else
        (void)sprintf(depths, _("%d 階", "Lev %d"), (int)floor_ptr->dun_level);

    switch (creature_ptr->feeling) {
    case 0:
        attr = TERM_SLATE;
        break; /* Unknown */
    case 1:
        attr = TERM_L_BLUE;
        break; /* Special */
    case 2:
        attr = TERM_VIOLET;
        break; /* Horrible visions */
    case 3:
        attr = TERM_RED;
        break; /* Very dangerous */
    case 4:
        attr = TERM_L_RED;
        break; /* Very bad feeling */
    case 5:
        attr = TERM_ORANGE;
        break; /* Bad feeling */
    case 6:
        attr = TERM_YELLOW;
        break; /* Nervous */
    case 7:
        attr = TERM_L_UMBER;
        break; /* Luck is turning */
    case 8:
        attr = TERM_L_WHITE;
        break; /* Don't like */
    case 9:
        attr = TERM_WHITE;
        break; /* Reasonably safe */
    case 10:
        attr = TERM_WHITE;
        break; /* Boring place */
    }

    c_prt(attr, format("%7s", depths), row_depth, col_depth);
}

/*!
 * @brief プレイヤーの空腹状態を表示する / Prints status of hunger
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void print_hunger(player_type *player_ptr)
{
    if (current_world_ptr->wizard && player_ptr->current_floor_ptr->inside_arena)
        return;

    if (player_ptr->food < PY_FOOD_FAINT) {
        c_put_str(TERM_RED, _("衰弱  ", "Weak  "), ROW_HUNGRY, COL_HUNGRY);
        return;
    }

    if (player_ptr->food < PY_FOOD_WEAK) {
        c_put_str(TERM_ORANGE, _("衰弱  ", "Weak  "), ROW_HUNGRY, COL_HUNGRY);
        return;
    }

    if (player_ptr->food < PY_FOOD_ALERT) {
        c_put_str(TERM_YELLOW, _("空腹  ", "Hungry"), ROW_HUNGRY, COL_HUNGRY);
        return;
    }

    if (player_ptr->food < PY_FOOD_FULL) {
        c_put_str(TERM_L_GREEN, "      ", ROW_HUNGRY, COL_HUNGRY);
        return;
    }

    if (player_ptr->food < PY_FOOD_MAX) {
        c_put_str(TERM_L_GREEN, _("満腹  ", "Full  "), ROW_HUNGRY, COL_HUNGRY);
        return;
    }

    c_put_str(TERM_GREEN, _("食過ぎ", "Gorged"), ROW_HUNGRY, COL_HUNGRY);
}

/*!
 * @brief プレイヤーの行動状態を表示する / Prints Searching, Resting, Paralysis, or 'count' status
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * Display is always exactly 10 characters wide (see below)
 * This function was a major bottleneck when resting, so a lot of
 * the text formatting code was optimized in place below.
 */
static void print_state(player_type *player_ptr)
{
    TERM_COLOR attr = TERM_WHITE;
    GAME_TEXT text[16];
    if (command_rep) {
        if (command_rep > 999) {
            (void)sprintf(text, "%2d00", command_rep / 100);
        } else {
            (void)sprintf(text, "  %2d", command_rep);
        }

        c_put_str(attr, format("%5.5s", text), ROW_STATE, COL_STATE);
        return;
    }

    switch (player_ptr->action) {
    case ACTION_SEARCH: {
        strcpy(text, _("探索", "Sear"));
        break;
    }
    case ACTION_REST:
        strcpy(text, _("    ", "    "));
        if (player_ptr->resting > 0) {
            sprintf(text, "%4d", player_ptr->resting);
        } else if (player_ptr->resting == COMMAND_ARG_REST_FULL_HEALING) {
            text[0] = text[1] = text[2] = text[3] = '*';
        } else if (player_ptr->resting == COMMAND_ARG_REST_UNTIL_DONE) {
            text[0] = text[1] = text[2] = text[3] = '&';
        }

        break;

    case ACTION_LEARN: {
        strcpy(text, _("学習", "lear"));
        if (player_ptr->new_mane)
            attr = TERM_L_RED;
        break;
    }
    case ACTION_FISH: {
        strcpy(text, _("釣り", "fish"));
        break;
    }
    case ACTION_KAMAE: {
        int i;
        for (i = 0; i < MAX_KAMAE; i++)
            if (player_ptr->special_defense & (KAMAE_GENBU << i))
                break;
        switch (i) {
        case 0:
            attr = TERM_GREEN;
            break;
        case 1:
            attr = TERM_WHITE;
            break;
        case 2:
            attr = TERM_L_BLUE;
            break;
        case 3:
            attr = TERM_L_RED;
            break;
        }

        strcpy(text, kamae_shurui[i].desc);
        break;
    }
    case ACTION_KATA: {
        int i;
        for (i = 0; i < MAX_KATA; i++)
            if (player_ptr->special_defense & (KATA_IAI << i))
                break;

        strcpy(text, kata_shurui[i].desc);
        break;
    }
    case ACTION_SING: {
        strcpy(text, _("歌  ", "Sing"));
        break;
    }
    case ACTION_HAYAGAKE: {
        strcpy(text, _("速駆", "Fast"));
        break;
    }
    case ACTION_SPELL: {
        strcpy(text, _("詠唱", "Spel"));
        break;
    }
    default: {
        strcpy(text, "    ");
        break;
    }
    }

    c_put_str(attr, format("%5.5s", text), ROW_STATE, COL_STATE);
}

/*!
 * @brief プレイヤーの行動速度を表示する / Prints the speed of a character.			-CJS-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void print_speed(player_type *player_ptr)
{
    TERM_LEN wid, hgt;
    Term_get_size(&wid, &hgt);
    TERM_LEN col_speed = wid + COL_SPEED;
    TERM_LEN row_speed = hgt + ROW_SPEED;

    int i = player_ptr->pspeed;
    if (player_ptr->action == ACTION_SEARCH && !player_ptr->lightspeed)
        i += 10;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    bool is_player_fast = is_fast(player_ptr);
    char buf[32] = "";
    TERM_COLOR attr = TERM_WHITE;
    if (i > 110) {
        if (player_ptr->riding) {
            monster_type *m_ptr = &floor_ptr->m_list[player_ptr->riding];
            if (monster_fast_remaining(m_ptr) && !monster_slow_remaining(m_ptr))
                attr = TERM_L_BLUE;
            else if (monster_slow_remaining(m_ptr) && !monster_fast_remaining(m_ptr))
                attr = TERM_VIOLET;
            else
                attr = TERM_GREEN;
        } else if ((is_player_fast && !player_ptr->slow) || player_ptr->lightspeed)
            attr = TERM_YELLOW;
        else if (player_ptr->slow && !is_player_fast)
            attr = TERM_VIOLET;
        else
            attr = TERM_L_GREEN;
        sprintf(buf, "%s(+%d)", (player_ptr->riding ? _("乗馬", "Ride") : _("加速", "Fast")), (i - 110));
    } else if (i < 110) {
        if (player_ptr->riding) {
            monster_type *m_ptr = &floor_ptr->m_list[player_ptr->riding];
            if (monster_fast_remaining(m_ptr) && !monster_slow_remaining(m_ptr))
                attr = TERM_L_BLUE;
            else if (monster_slow_remaining(m_ptr) && !monster_fast_remaining(m_ptr))
                attr = TERM_VIOLET;
            else
                attr = TERM_RED;
        } else if (is_player_fast && !player_ptr->slow)
            attr = TERM_YELLOW;
        else if (player_ptr->slow && !is_player_fast)
            attr = TERM_VIOLET;
        else
            attr = TERM_L_UMBER;
        sprintf(buf, "%s(-%d)", (player_ptr->riding ? _("乗馬", "Ride") : _("減速", "Slow")), (110 - i));
    } else if (player_ptr->riding) {
        attr = TERM_GREEN;
        strcpy(buf, _("乗馬中", "Riding"));
    }

    c_put_str(attr, format("%-9s", buf), row_speed, col_speed);
}

/*!
 * @brief プレイヤーの呪文学習可能状態を表示する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void print_study(player_type *player_ptr)
{
    TERM_LEN wid, hgt;
    Term_get_size(&wid, &hgt);
    TERM_LEN col_study = wid + COL_STUDY;
    TERM_LEN row_study = hgt + ROW_STUDY;

    if (player_ptr->new_spells) {
        put_str(_("学習", "Stud"), row_study, col_study);
    } else {
        put_str("    ", row_study, col_study);
    }
}

/*!
 * @brief プレイヤーのものまね可能状態を表示する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void print_imitation(player_type *player_ptr)
{
    TERM_LEN wid, hgt;
    Term_get_size(&wid, &hgt);
    TERM_LEN col_study = wid + COL_STUDY;
    TERM_LEN row_study = hgt + ROW_STUDY;

    if (player_ptr->pclass != CLASS_IMITATOR)
        return;

    if (player_ptr->mane_num != 0) {
        put_str("    ", row_study, col_study);
        return;
    }

    TERM_COLOR attr;
    if (player_ptr->new_mane)
        attr = TERM_L_RED;
    else
        attr = TERM_WHITE;
    c_put_str(attr, _("まね", "Imit"), row_study, col_study);
}

/*!
 * @brief プレイヤーの負傷状態を表示する
 * @return なし
 */
static void print_cut(player_type *creature_ptr)
{
    int c = creature_ptr->cut;
    if (c > 1000) {
        c_put_str(TERM_L_RED, _("致命傷      ", "Mortal wound"), ROW_CUT, COL_CUT);
        return;
    }

    if (c > 200) {
        c_put_str(TERM_RED, _("ひどい深手  ", "Deep gash   "), ROW_CUT, COL_CUT);
        return;
    }

    if (c > 100) {
        c_put_str(TERM_RED, _("重傷        ", "Severe cut  "), ROW_CUT, COL_CUT);
        return;
    }

    if (c > 50) {
        c_put_str(TERM_ORANGE, _("大変な傷    ", "Nasty cut   "), ROW_CUT, COL_CUT);
        return;
    }

    if (c > 25) {
        c_put_str(TERM_ORANGE, _("ひどい傷    ", "Bad cut     "), ROW_CUT, COL_CUT);
        return;
    }

    if (c > 10) {
        c_put_str(TERM_YELLOW, _("軽傷        ", "Light cut   "), ROW_CUT, COL_CUT);
        return;
    }

    if (c) {
        c_put_str(TERM_YELLOW, _("かすり傷    ", "Graze       "), ROW_CUT, COL_CUT);
        return;
    }

    put_str("            ", ROW_CUT, COL_CUT);
}

/*!
 * @brief プレイヤーの朦朧状態を表示する
 * @return なし
 */
static void print_stun(player_type *creature_ptr)
{
    int s = creature_ptr->stun;
    if (s > 100) {
        c_put_str(TERM_RED, _("意識不明瞭  ", "Knocked out "), ROW_STUN, COL_STUN);
        return;
    }

    if (s > 50) {
        c_put_str(TERM_ORANGE, _("ひどく朦朧  ", "Heavy stun  "), ROW_STUN, COL_STUN);
        return;
    }

    if (s) {
        c_put_str(TERM_ORANGE, _("朦朧        ", "Stun        "), ROW_STUN, COL_STUN);
        return;
    }

    put_str("            ", ROW_STUN, COL_STUN);
}

/*!
 * @brief モンスターの体力ゲージを表示する
 * @param riding TRUEならば騎乗中のモンスターの体力、FALSEならターゲットモンスターの体力を表示する。表示位置は固定。
 * @return なし
 * @details
 * <pre>
 * Redraw the "monster health bar"	-DRS-
 * Rather extensive modifications by	-BEN-
 *
 * The "monster health bar" provides visual feedback on the "health"
 * of the monster currently being "tracked".  There are several ways
 * to "track" a monster, including targetting it, attacking it, and
 * affecting it (and nobody else) with a ranged attack.
 *
 * Display the monster health bar (affectionately known as the
 * "health-o-meter").  Clear health bar if nothing is being tracked.
 * Auto-track current target monster when bored.  Note that the
 * health-bar stops tracking any monster that "disappears".
 * </pre>
 */
static void health_redraw(player_type *creature_ptr, bool riding)
{
    s16b health_who;
    int row, col;

    if (riding) {
        health_who = creature_ptr->riding;
        row = ROW_RIDING_INFO;
        col = COL_RIDING_INFO;
    } else {
        health_who = creature_ptr->health_who;
        row = ROW_INFO;
        col = COL_INFO;
    }

    monster_type *m_ptr;
    m_ptr = &creature_ptr->current_floor_ptr->m_list[health_who];

    if (current_world_ptr->wizard && creature_ptr->phase_out) {
        row = ROW_INFO - 2;
        col = COL_INFO + 2;

        Term_putstr(col - 2, row, 12, TERM_WHITE, "      /     ");
        Term_putstr(col - 2, row + 1, 12, TERM_WHITE, "      /     ");
        Term_putstr(col - 2, row + 2, 12, TERM_WHITE, "      /     ");
        Term_putstr(col - 2, row + 3, 12, TERM_WHITE, "      /     ");

        if (creature_ptr->current_floor_ptr->m_list[1].r_idx) {
            Term_putstr(col - 2, row, 2, r_info[creature_ptr->current_floor_ptr->m_list[1].r_idx].x_attr,
                format("%c", r_info[creature_ptr->current_floor_ptr->m_list[1].r_idx].x_char));
            Term_putstr(col - 1, row, 5, TERM_WHITE, format("%5d", creature_ptr->current_floor_ptr->m_list[1].hp));
            Term_putstr(col + 5, row, 6, TERM_WHITE, format("%5d", creature_ptr->current_floor_ptr->m_list[1].max_maxhp));
        }

        if (creature_ptr->current_floor_ptr->m_list[2].r_idx) {
            Term_putstr(col - 2, row + 1, 2, r_info[creature_ptr->current_floor_ptr->m_list[2].r_idx].x_attr,
                format("%c", r_info[creature_ptr->current_floor_ptr->m_list[2].r_idx].x_char));
            Term_putstr(col - 1, row + 1, 5, TERM_WHITE, format("%5d", creature_ptr->current_floor_ptr->m_list[2].hp));
            Term_putstr(col + 5, row + 1, 6, TERM_WHITE, format("%5d", creature_ptr->current_floor_ptr->m_list[2].max_maxhp));
        }

        if (creature_ptr->current_floor_ptr->m_list[3].r_idx) {
            Term_putstr(col - 2, row + 2, 2, r_info[creature_ptr->current_floor_ptr->m_list[3].r_idx].x_attr,
                format("%c", r_info[creature_ptr->current_floor_ptr->m_list[3].r_idx].x_char));
            Term_putstr(col - 1, row + 2, 5, TERM_WHITE, format("%5d", creature_ptr->current_floor_ptr->m_list[3].hp));
            Term_putstr(col + 5, row + 2, 6, TERM_WHITE, format("%5d", creature_ptr->current_floor_ptr->m_list[3].max_maxhp));
        }

        if (creature_ptr->current_floor_ptr->m_list[4].r_idx) {
            Term_putstr(col - 2, row + 3, 2, r_info[creature_ptr->current_floor_ptr->m_list[4].r_idx].x_attr,
                format("%c", r_info[creature_ptr->current_floor_ptr->m_list[4].r_idx].x_char));
            Term_putstr(col - 1, row + 3, 5, TERM_WHITE, format("%5d", creature_ptr->current_floor_ptr->m_list[4].hp));
            Term_putstr(col + 5, row + 3, 6, TERM_WHITE, format("%5d", creature_ptr->current_floor_ptr->m_list[4].max_maxhp));
        }

        return;
    }

    if (!health_who) {
        Term_erase(col, row, 12);
        return;
    }

    if (!m_ptr->ml) {
        Term_putstr(col, row, 12, TERM_WHITE, "[----------]");
        return;
    }

    if (creature_ptr->image) {
        Term_putstr(col, row, 12, TERM_WHITE, "[----------]");
        return;
    }

    if (m_ptr->hp < 0) {
        Term_putstr(col, row, 12, TERM_WHITE, "[----------]");
        return;
    }

    int pct = m_ptr->maxhp > 0 ? 100L * m_ptr->hp / m_ptr->maxhp : 0;
    int pct2 = m_ptr->maxhp > 0 ? 100L * m_ptr->hp / m_ptr->max_maxhp : 0;
    int len = (pct2 < 10) ? 1 : (pct2 < 90) ? (pct2 / 10 + 1) : 10;
    TERM_COLOR attr = TERM_RED;
    if (monster_invulner_remaining(m_ptr))
        attr = TERM_WHITE;
    else if (monster_csleep_remaining(m_ptr))
        attr = TERM_BLUE;
    else if (monster_fear_remaining(m_ptr))
        attr = TERM_VIOLET;
    else if (pct >= 100)
        attr = TERM_L_GREEN;
    else if (pct >= 60)
        attr = TERM_YELLOW;
    else if (pct >= 25)
        attr = TERM_ORANGE;
    else if (pct >= 10)
        attr = TERM_L_RED;

    Term_putstr(col, row, 12, TERM_WHITE, "[----------]");
    Term_putstr(col + 1, row, len, attr, "**********");
}

/*!
 * @brief プレイヤーのステータスを一括表示する（左側部分） / Display basic info (mostly left of map)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void print_frame_basic(player_type *creature_ptr)
{
    if (creature_ptr->mimic_form) {
        print_field(mimic_info[creature_ptr->mimic_form].title, ROW_RACE, COL_RACE);
    } else {
        char str[14];
        angband_strcpy(str, rp_ptr->title, sizeof(str));
        print_field(str, ROW_RACE, COL_RACE);
    }

    print_title(creature_ptr);
    print_level(creature_ptr);
    print_exp(creature_ptr);
    for (int i = 0; i < A_MAX; i++)
        print_stat(creature_ptr, i);

    print_ac(creature_ptr);
    print_hp(creature_ptr);
    print_sp(creature_ptr);
    print_gold(creature_ptr);
    print_depth(creature_ptr);
    health_redraw(creature_ptr, FALSE);
    health_redraw(creature_ptr, TRUE);
}

/*!
 * @brief プレイヤーのステータスを一括表示する（下部分） / Display extra info (mostly below map)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void print_frame_extra(player_type *player_ptr)
{
    print_cut(player_ptr);
    print_stun(player_ptr);
    print_hunger(player_ptr);
    print_state(player_ptr);
    print_speed(player_ptr);
    print_study(player_ptr);
    print_imitation(player_ptr);
    print_status(player_ptr);
}

/*!
 * @brief サブウィンドウに所持品一覧を表示する / Hack -- display inventory in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void fix_inventory(player_type *player_ptr, tval_type item_tester_tval)
{
    for (int j = 0; j < 8; j++) {
        term *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_INVEN)))
            continue;

        Term_activate(angband_term[j]);
        display_inventory(player_ptr, item_tester_tval);
        Term_fresh();
        Term_activate(old);
    }
}

/*!
 * @brief モンスターの現在数を一行で表現する / Print monster info in line
 * @param x 表示列
 * @param y 表示行
 * @param m_ptr 思い出を表示するモンスター情報の参照ポインタ
 * @param n_same モンスターの数の現在数
 * @details
 * <pre>
 * nnn X LV name
 *  nnn : number or unique(U) or wanted unique(W)
 *  X   : symbol of monster
 *  LV  : monster lv if known
 *  name: name of monster
 * @return なし
 * </pre>
 */
static void print_monster_line(TERM_LEN x, TERM_LEN y, monster_type *m_ptr, int n_same)
{
    char buf[256];
    MONRACE_IDX r_idx = m_ptr->ap_r_idx;
    monster_race *r_ptr = &r_info[r_idx];

    Term_gotoxy(x, y);
    if (!r_ptr)
        return;
    if (r_ptr->flags1 & RF1_UNIQUE) {
        bool is_bounty = FALSE;
        for (int i = 0; i < MAX_BOUNTY; i++) {
            if (current_world_ptr->bounty_r_idx[i] == r_idx) {
                is_bounty = TRUE;
                break;
            }
        }

        Term_addstr(-1, TERM_WHITE, is_bounty ? "  W" : "  U");
    } else {
        sprintf(buf, "%3d", n_same);
        Term_addstr(-1, TERM_WHITE, buf);
    }

    Term_addstr(-1, TERM_WHITE, " ");
    Term_add_bigch(r_ptr->x_attr, r_ptr->x_char);

    if (r_ptr->r_tkills && !(m_ptr->mflag2 & MFLAG2_KAGE)) {
        sprintf(buf, " %2d", (int)r_ptr->level);
    } else {
        strcpy(buf, " ??");
    }

    Term_addstr(-1, TERM_WHITE, buf);

    sprintf(buf, " %s ", r_name + r_ptr->name);
    Term_addstr(-1, TERM_WHITE, buf);
}

/*!
 * @brief モンスターの出現リストを表示する / Print monster info in line
 * @param x 表示列
 * @param y 表示行
 * @param max_lines 最大何行描画するか
 */
void print_monster_list(floor_type *floor_ptr, TERM_LEN x, TERM_LEN y, TERM_LEN max_lines)
{
    TERM_LEN line = y;
    monster_type *last_mons = NULL;
    monster_type *m_ptr = NULL;
    int n_same = 0;
    int i;
    for (i = 0; i < tmp_pos.n; i++) {
        grid_type *g_ptr = &floor_ptr->grid_array[tmp_pos.y[i]][tmp_pos.x[i]];
        if (!g_ptr->m_idx || !floor_ptr->m_list[g_ptr->m_idx].ml)
            continue;
        m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
        if (is_pet(m_ptr))
            continue; // pet
        if (!m_ptr->r_idx)
            continue; // dead?

        //ソート済みなので同じモンスターは連続する．これを利用して同じモンスターをカウント，まとめて表示する．
        //先頭モンスター
        if (!last_mons) {
            last_mons = m_ptr;
            n_same = 1;
            continue;
        }

        // same race?
        if (last_mons->ap_r_idx == m_ptr->ap_r_idx) {
            n_same++;
            continue; //表示処理を次に回す
        }

        // print last mons info
        print_monster_line(x, line++, last_mons, n_same);
        n_same = 1;
        last_mons = m_ptr;
        if (line - y - 1 == max_lines)
            break;
    }

    if (line - y - 1 == max_lines && i != tmp_pos.n) {
        Term_gotoxy(x, line);
        Term_addstr(-1, TERM_WHITE, "-- and more --");
    } else {
        if (last_mons)
            print_monster_line(x, line++, last_mons, n_same);
    }
}

/*!
 * @brief 出現中モンスターのリストをサブウィンドウに表示する / Hack -- display monster list in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void fix_monster_list(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term *old = Term;
        if (!angband_term[j])
            continue;
        if (!(window_flag[j] & (PW_MONSTER_LIST)))
            continue;

        Term_activate(angband_term[j]);
        int w, h;
        Term_get_size(&w, &h);
        Term_clear();
        target_set_prepare_look(player_ptr);
        print_monster_list(player_ptr->current_floor_ptr, 0, 0, h);
        Term_fresh();
        Term_activate(old);
    }
}

/*!
 * @brief 現在の装備品をサブウィンドウに表示する /
 * Hack -- display equipment in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void fix_equip(player_type *player_ptr, tval_type item_tester_tval)
{
    for (int j = 0; j < 8; j++) {
        term *old = Term;
        if (!angband_term[j])
            continue;
        if (!(window_flag[j] & (PW_EQUIP)))
            continue;

        Term_activate(angband_term[j]);
        display_equipment(player_ptr, item_tester_tval);
        Term_fresh();
        Term_activate(old);
    }
}

/*!
 * @brief 現在の習得済魔法をサブウィンドウに表示する /
 * @param player_ptr プレーヤーへの参照ポインタ
 * Hack -- display spells in sub-windows
 * @return なし
 */
static void fix_spell(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_SPELL)))
            continue;

        Term_activate(angband_term[j]);
        display_spell_list(player_ptr);
        Term_fresh();
        Term_activate(old);
    }
}

/*!
 * @brief 現在のプレイヤーステータスをサブウィンドウに表示する /
 * @param player_ptr プレーヤーへの参照ポインタ
 * Hack -- display character in sub-windows
 * @return なし
 */
static void fix_player(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_PLAYER)))
            continue;

        Term_activate(angband_term[j]);
        update_playtime();
        display_player(player_ptr, 0, map_name);
        Term_fresh();
        Term_activate(old);
    }
}

/*!
 * @brief ゲームメッセージ履歴をサブウィンドウに表示する /
 * Hack -- display recent messages in sub-windows
 * Adjust for width and split messages
 * @return なし
 */
static void fix_message(void)
{
    for (int j = 0; j < 8; j++) {
        term *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_MESSAGE)))
            continue;

        Term_activate(angband_term[j]);
        TERM_LEN w, h;
        Term_get_size(&w, &h);
        for (int i = 0; i < h; i++) {
            Term_putstr(0, (h - 1) - i, -1, (byte)((i < now_message) ? TERM_WHITE : TERM_SLATE), message_str((s16b)i));
            TERM_LEN x, y;
            Term_locate(&x, &y);
            Term_erase(x, y, 255);
        }

        Term_fresh();
        Term_activate(old);
    }
}

/*!
 * @brief 簡易マップをサブウィンドウに表示する /
 * Hack -- display overhead view in sub-windows
 * Adjust for width and split messages
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * Note that the "player" symbol does NOT appear on the map.
 */
static void fix_overhead(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term *old = Term;
        TERM_LEN wid, hgt;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_OVERHEAD)))
            continue;

        Term_activate(angband_term[j]);
        Term_get_size(&wid, &hgt);
        if (wid > COL_MAP + 2 && hgt > ROW_MAP + 2) {
            int cy, cx;
            display_map(player_ptr, &cy, &cx);
            Term_fresh();
        }

        Term_activate(old);
    }
}

static void display_dungeon(player_type *player_ptr)
{
    TERM_COLOR ta = 0;
    SYMBOL_CODE tc = '\0';

    for (TERM_LEN x = player_ptr->x - Term->wid / 2 + 1; x <= player_ptr->x + Term->wid / 2; x++) {
        for (TERM_LEN y = player_ptr->y - Term->hgt / 2 + 1; y <= player_ptr->y + Term->hgt / 2; y++) {
            TERM_COLOR a;
            SYMBOL_CODE c;
            if (!in_bounds2(player_ptr->current_floor_ptr, y, x)) {
                feature_type *f_ptr = &f_info[feat_none];
                a = f_ptr->x_attr[F_LIT_STANDARD];
                c = f_ptr->x_char[F_LIT_STANDARD];
                Term_queue_char(x - player_ptr->x + Term->wid / 2 - 1, y - player_ptr->y + Term->hgt / 2 - 1, a, c, ta, tc);
                continue;
            }

            map_info(player_ptr, y, x, &a, &c, &ta, &tc);

            if (!use_graphics) {
                if (current_world_ptr->timewalk_m_idx)
                    a = TERM_DARK;
                else if (is_invuln(player_ptr) || player_ptr->timewalk)
                    a = TERM_WHITE;
                else if (player_ptr->wraith_form)
                    a = TERM_L_DARK;
            }

            Term_queue_char(x - player_ptr->x + Term->wid / 2 - 1, y - player_ptr->y + Term->hgt / 2 - 1, a, c, ta, tc);
        }
    }
}

/*!
 * @brief ダンジョンの地形をサブウィンドウに表示する /
 * Hack -- display dungeon view in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void fix_dungeon(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_DUNGEON)))
            continue;

        Term_activate(angband_term[j]);
        display_dungeon(player_ptr);
        Term_fresh();
        Term_activate(old);
    }
}

/*!
 * @brief モンスターの思い出をサブウィンドウに表示する /
 * Hack -- display dungeon view in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void fix_monster(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_MONSTER)))
            continue;

        Term_activate(angband_term[j]);
        if (player_ptr->monster_race_idx)
            display_roff(player_ptr);

        Term_fresh();
        Term_activate(old);
    }
}

/*!
 * @brief ベースアイテム情報をサブウィンドウに表示する /
 * Hack -- display object recall in sub-windows
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void fix_object(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_OBJECT)))
            continue;

        Term_activate(angband_term[j]);
        if (player_ptr->object_kind_idx)
            display_koff(player_ptr, player_ptr->object_kind_idx);

        Term_fresh();
        Term_activate(old);
    }
}

/*!
 * @brief 射撃武器がプレイヤーにとって重すぎるかどうかの判定 /
 * @param o_ptr 判定する射撃武器のアイテム情報参照ポインタ
 * @return 重すぎるならばTRUE
 */
bool is_heavy_shoot(player_type *creature_ptr, object_type *o_ptr)
{
    int hold = adj_str_hold[creature_ptr->stat_ind[A_STR]];
    return (hold < o_ptr->weight / 10);
}

/*!
 * @brief redraw のフラグに応じた更新をまとめて行う / Handle "redraw"
 * @return なし
 * @details 更新処理の対象はゲーム中の全描画処理
 */
void redraw_stuff(player_type *creature_ptr)
{
    if (!creature_ptr->redraw)
        return;

    if (!current_world_ptr->character_generated)
        return;

    if (current_world_ptr->character_icky)
        return;

    if (creature_ptr->redraw & (PR_WIPE)) {
        creature_ptr->redraw &= ~(PR_WIPE);
        msg_print(NULL);
        Term_clear();
    }

    if (creature_ptr->redraw & (PR_MAP)) {
        creature_ptr->redraw &= ~(PR_MAP);
        print_map(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_BASIC)) {
        creature_ptr->redraw &= ~(PR_BASIC);
        creature_ptr->redraw &= ~(PR_MISC | PR_TITLE | PR_STATS);
        creature_ptr->redraw &= ~(PR_LEV | PR_EXP | PR_GOLD);
        creature_ptr->redraw &= ~(PR_ARMOR | PR_HP | PR_MANA);
        creature_ptr->redraw &= ~(PR_DEPTH | PR_HEALTH | PR_UHEALTH);
        print_frame_basic(creature_ptr);
        print_time(creature_ptr);
        print_dungeon(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_EQUIPPY)) {
        creature_ptr->redraw &= ~(PR_EQUIPPY);
        display_player_equippy(creature_ptr, ROW_EQUIPPY, COL_EQUIPPY, 0);
    }

    if (creature_ptr->redraw & (PR_MISC)) {
        creature_ptr->redraw &= ~(PR_MISC);
        print_field(rp_ptr->title, ROW_RACE, COL_RACE);
    }

    if (creature_ptr->redraw & (PR_TITLE)) {
        creature_ptr->redraw &= ~(PR_TITLE);
        print_title(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_LEV)) {
        creature_ptr->redraw &= ~(PR_LEV);
        print_level(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_EXP)) {
        creature_ptr->redraw &= ~(PR_EXP);
        print_exp(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_STATS)) {
        creature_ptr->redraw &= ~(PR_STATS);
        print_stat(creature_ptr, A_STR);
        print_stat(creature_ptr, A_INT);
        print_stat(creature_ptr, A_WIS);
        print_stat(creature_ptr, A_DEX);
        print_stat(creature_ptr, A_CON);
        print_stat(creature_ptr, A_CHR);
    }

    if (creature_ptr->redraw & (PR_STATUS)) {
        creature_ptr->redraw &= ~(PR_STATUS);
        print_status(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_ARMOR)) {
        creature_ptr->redraw &= ~(PR_ARMOR);
        print_ac(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_HP)) {
        creature_ptr->redraw &= ~(PR_HP);
        print_hp(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_MANA)) {
        creature_ptr->redraw &= ~(PR_MANA);
        print_sp(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_GOLD)) {
        creature_ptr->redraw &= ~(PR_GOLD);
        print_gold(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_DEPTH)) {
        creature_ptr->redraw &= ~(PR_DEPTH);
        print_depth(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_HEALTH)) {
        creature_ptr->redraw &= ~(PR_HEALTH);
        health_redraw(creature_ptr, FALSE);
    }

    if (creature_ptr->redraw & (PR_UHEALTH)) {
        creature_ptr->redraw &= ~(PR_UHEALTH);
        health_redraw(creature_ptr, TRUE);
    }

    if (creature_ptr->redraw & (PR_EXTRA)) {
        creature_ptr->redraw &= ~(PR_EXTRA);
        creature_ptr->redraw &= ~(PR_CUT | PR_STUN);
        creature_ptr->redraw &= ~(PR_HUNGER);
        creature_ptr->redraw &= ~(PR_STATE | PR_SPEED | PR_STUDY | PR_IMITATION | PR_STATUS);
        print_frame_extra(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_CUT)) {
        creature_ptr->redraw &= ~(PR_CUT);
        print_cut(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_STUN)) {
        creature_ptr->redraw &= ~(PR_STUN);
        print_stun(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_HUNGER)) {
        creature_ptr->redraw &= ~(PR_HUNGER);
        print_hunger(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_STATE)) {
        creature_ptr->redraw &= ~(PR_STATE);
        print_state(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_SPEED)) {
        creature_ptr->redraw &= ~(PR_SPEED);
        print_speed(creature_ptr);
    }

    if (creature_ptr->pclass == CLASS_IMITATOR) {
        if (creature_ptr->redraw & (PR_IMITATION)) {
            creature_ptr->redraw &= ~(PR_IMITATION);
            print_imitation(creature_ptr);
        }

        return;
    }

    if (creature_ptr->redraw & (PR_STUDY)) {
        creature_ptr->redraw &= ~(PR_STUDY);
        print_study(creature_ptr);
    }
}

/*!
 * @brief player_ptr->window のフラグに応じた更新をまとめて行う / Handle "player_ptr->window"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details 更新処理の対象はサブウィンドウ全般
 */
void window_stuff(player_type *player_ptr)
{
    if (!player_ptr->window)
        return;

    BIT_FLAGS mask = 0L;
    for (int j = 0; j < 8; j++) {
        if (angband_term[j])
            mask |= window_flag[j];
    }

    player_ptr->window &= mask;

    if (!player_ptr->window)
        return;

    if (player_ptr->window & (PW_INVEN)) {
        player_ptr->window &= ~(PW_INVEN);
        fix_inventory(player_ptr, 0); // TODO:2.2.2 まともなtval参照手段を確保
    }

    if (player_ptr->window & (PW_EQUIP)) {
        player_ptr->window &= ~(PW_EQUIP);
        fix_equip(player_ptr, 0); // TODO:2.2.2 まともなtval参照手段を確保
    }

    if (player_ptr->window & (PW_SPELL)) {
        player_ptr->window &= ~(PW_SPELL);
        fix_spell(player_ptr);
    }

    if (player_ptr->window & (PW_PLAYER)) {
        player_ptr->window &= ~(PW_PLAYER);
        fix_player(player_ptr);
    }

    if (player_ptr->window & (PW_MONSTER_LIST)) {
        player_ptr->window &= ~(PW_MONSTER_LIST);
        fix_monster_list(player_ptr);
    }

    if (player_ptr->window & (PW_MESSAGE)) {
        player_ptr->window &= ~(PW_MESSAGE);
        fix_message();
    }

    if (player_ptr->window & (PW_OVERHEAD)) {
        player_ptr->window &= ~(PW_OVERHEAD);
        fix_overhead(player_ptr);
    }

    if (player_ptr->window & (PW_DUNGEON)) {
        player_ptr->window &= ~(PW_DUNGEON);
        fix_dungeon(player_ptr);
    }

    if (player_ptr->window & (PW_MONSTER)) {
        player_ptr->window &= ~(PW_MONSTER);
        fix_monster(player_ptr);
    }

    if (player_ptr->window & (PW_OBJECT)) {
        player_ptr->window &= ~(PW_OBJECT);
        fix_object(player_ptr);
    }
}

/*!
 * todo ここにplayer_type を追加するとz-termに影響が行くので保留
 * @brief コンソールのリサイズに合わせてマップを再描画する /
 * Map resizing whenever the main term changes size
 * @return なし
 */
void resize_map()
{
    if (!current_world_ptr->character_dungeon)
        return;

    panel_row_max = 0;
    panel_col_max = 0;
    panel_row_min = p_ptr->current_floor_ptr->height;
    panel_col_min = p_ptr->current_floor_ptr->width;
    verify_panel(p_ptr);

    p_ptr->update |= (PU_TORCH | PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
    p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);
    p_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
    p_ptr->update |= (PU_MONSTERS);
    p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

    handle_stuff(p_ptr);
    Term_redraw();

    if (can_save)
        move_cursor_relative(p_ptr->y, p_ptr->x);

    Term_fresh();
}

/*!
 * todo ここにplayer_type を追加するとz-termに影響が行くので保留
 * @brief コンソールを再描画する /
 * Redraw a term when it is resized
 * @return なし
 */
void redraw_window(void)
{
    if (!current_world_ptr->character_dungeon)
        return;

    p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
    p_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);

    handle_stuff(p_ptr);
    Term_redraw();
}

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する（サブルーチン）
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param dy 変更先のフロアY座標
 * @param dx 変更先のフロアX座標
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
bool change_panel(player_type *player_ptr, POSITION dy, POSITION dx)
{
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);

    POSITION y = panel_row_min + dy * hgt / 2;
    POSITION x = panel_col_min + dx * wid / 2;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (y > floor_ptr->height - hgt)
        y = floor_ptr->height - hgt;
    if (y < 0)
        y = 0;

    if (x > floor_ptr->width - wid)
        x = floor_ptr->width - wid;
    if (x < 0)
        x = 0;

    if ((y == panel_row_min) && (x == panel_col_min))
        return FALSE;

    panel_row_min = y;
    panel_col_min = x;
    panel_bounds_center();

    player_ptr->update |= (PU_MONSTERS);
    player_ptr->redraw |= (PR_MAP);
    handle_stuff(player_ptr);
    return TRUE;
}

/*!
 * @brief 現在のコンソール表示の縦横を返す。 /
 * Get term size and calculate screen size
 * @param wid_p コンソールの表示幅文字数を返す
 * @param hgt_p コンソールの表示行数を返す
 * @return なし
 */
void get_screen_size(TERM_LEN *wid_p, TERM_LEN *hgt_p)
{
    Term_get_size(wid_p, hgt_p);
    *hgt_p -= ROW_MAP + 2;
    *wid_p -= COL_MAP + 2;
    if (use_bigtile)
        *wid_p /= 2;
}

/*
 * Calculate panel colum of a location in the map
 */
int panel_col_of(int col)
{
    col -= panel_col_min;
    if (use_bigtile)
        col *= 2;
    return col + 13;
}

/*
 * Prints the map of the dungeon
 *
 * Note that, for efficiency, we contain an "optimized" version
 * of both "lite_spot()" and "print_rel()", and that we use the
 * "lite_spot()" function to display the player grid, if needed.
 */
void print_map(player_type *player_ptr)
{
    TERM_LEN wid, hgt;
    Term_get_size(&wid, &hgt);

    wid -= COL_MAP + 2;
    hgt -= ROW_MAP + 2;

    int v;
    (void)Term_get_cursor(&v);

    (void)Term_set_cursor(0);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    POSITION xmin = (0 < panel_col_min) ? panel_col_min : 0;
    POSITION xmax = (floor_ptr->width - 1 > panel_col_max) ? panel_col_max : floor_ptr->width - 1;
    POSITION ymin = (0 < panel_row_min) ? panel_row_min : 0;
    POSITION ymax = (floor_ptr->height - 1 > panel_row_max) ? panel_row_max : floor_ptr->height - 1;

    for (POSITION y = 1; y <= ymin - panel_row_prt; y++) {
        Term_erase(COL_MAP, y, wid);
    }

    for (POSITION y = ymax - panel_row_prt; y <= hgt; y++) {
        Term_erase(COL_MAP, y, wid);
    }

    for (POSITION y = ymin; y <= ymax; y++) {
        for (POSITION x = xmin; x <= xmax; x++) {
            TERM_COLOR a;
            SYMBOL_CODE c;
            TERM_COLOR ta;
            SYMBOL_CODE tc;
            map_info(player_ptr, y, x, &a, &c, &ta, &tc);
            if (!use_graphics) {
                if (current_world_ptr->timewalk_m_idx)
                    a = TERM_DARK;
                else if (is_invuln(player_ptr) || player_ptr->timewalk)
                    a = TERM_WHITE;
                else if (player_ptr->wraith_form)
                    a = TERM_L_DARK;
            }

            Term_queue_bigchar(panel_col_of(x), y - panel_row_prt, a, c, ta, tc);
        }
    }

    lite_spot(player_ptr, player_ptr->y, player_ptr->x);
    (void)Term_set_cursor(v);
}

/* 一般的にモンスターシンボルとして扱われる記号を定義する(幻覚処理向け) / Hack -- Legal monster codes */
static char image_monster_hack[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* 一般的にオブジェクトシンボルとして扱われる記号を定義する(幻覚処理向け) /  Hack -- Legal object codes */
static char image_object_hack[] = "?/|\\\"!$()_-=[]{},~";

/*!
 * @brief モンスターの表示を幻覚状態に差し替える / Mega-Hack -- Hallucinatory monster
 * @param ap 本来の色
 * @param cp 本来のシンボル
 * @return なし
 */
static void image_monster(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
    if (use_graphics) {
        monster_race *r_ptr = &r_info[randint1(max_r_idx - 1)];
        *cp = r_ptr->x_char;
        *ap = r_ptr->x_attr;
        return;
    }

    *cp = (one_in_(25) ? image_object_hack[randint0(sizeof(image_object_hack) - 1)] : image_monster_hack[randint0(sizeof(image_monster_hack) - 1)]);
    *ap = randint1(15);
}

/*!
 * @brief オブジェクトの表示を幻覚状態に差し替える / Hallucinatory object
 * @param ap 本来の色
 * @param cp 本来のシンボル
 * @return なし
 */
static void image_object(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
    if (use_graphics) {
        object_kind *k_ptr = &k_info[randint1(max_k_idx - 1)];
        *cp = k_ptr->x_char;
        *ap = k_ptr->x_attr;
        return;
    }

    int n = sizeof(image_object_hack) - 1;
    *cp = image_object_hack[randint0(n)];
    *ap = randint1(15);
}

/*!
 * @brief オブジェクト＆モンスターの表示を幻覚状態に差し替える / Hack -- Random hallucination
 * @param ap 本来の色
 * @param cp 本来のシンボル
 * @return なし
 */
static void image_random(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
    if (randint0(100) < 75) {
        image_monster(ap, cp);
    } else {
        image_object(ap, cp);
    }
}

/*!
 * 照明の表現を行うための色合いの関係を{暗闇時, 照明時} で定義する /
 * This array lists the effects of "brightness" on various "base" colours.\n
 *\n
 * This is used to do dynamic lighting effects in ascii :-)\n
 * At the moment, only the various "floor" tiles are affected.\n
 *\n
 * The layout of the array is [x][0] = light and [x][1] = dark.\n
 */
static TERM_COLOR lighting_colours[16][2] = {
    /* TERM_DARK */
    { TERM_L_DARK, TERM_DARK },

    /* TERM_WHITE */
    { TERM_YELLOW, TERM_SLATE },

    /* TERM_SLATE */
    { TERM_WHITE, TERM_L_DARK },

    /* TERM_ORANGE */
    { TERM_L_UMBER, TERM_UMBER },

    /* TERM_RED */
    { TERM_RED, TERM_RED },

    /* TERM_GREEN */
    { TERM_L_GREEN, TERM_GREEN },

    /* TERM_BLUE */
    { TERM_BLUE, TERM_BLUE },

    /* TERM_UMBER */
    { TERM_L_UMBER, TERM_RED },

    /* TERM_L_DARK */
    { TERM_SLATE, TERM_L_DARK },

    /* TERM_L_WHITE */
    { TERM_WHITE, TERM_SLATE },

    /* TERM_VIOLET */
    { TERM_L_RED, TERM_BLUE },

    /* TERM_YELLOW */
    { TERM_YELLOW, TERM_ORANGE },

    /* TERM_L_RED */
    { TERM_L_RED, TERM_L_RED },

    /* TERM_L_GREEN */
    { TERM_L_GREEN, TERM_GREEN },

    /* TERM_L_BLUE */
    { TERM_L_BLUE, TERM_L_BLUE },

    /* TERM_L_UMBER */
    { TERM_L_UMBER, TERM_UMBER }
};

/*!
 * @brief 調査中
 * @todo コメントを付加すること
 */
void apply_default_feat_lighting(TERM_COLOR f_attr[F_LIT_MAX], SYMBOL_CODE f_char[F_LIT_MAX])
{
    TERM_COLOR s_attr = f_attr[F_LIT_STANDARD];
    SYMBOL_CODE s_char = f_char[F_LIT_STANDARD];

    if (IS_ASCII_GRAPHICS(s_attr)) /* For ASCII */
    {
        f_attr[F_LIT_LITE] = lighting_colours[s_attr & 0x0f][0];
        f_attr[F_LIT_DARK] = lighting_colours[s_attr & 0x0f][1];
        for (int i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++)
            f_char[i] = s_char;
    } else /* For tile graphics */
    {
        for (int i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++)
            f_attr[i] = s_attr;
        f_char[F_LIT_LITE] = s_char + 2;
        f_char[F_LIT_DARK] = s_char + 1;
    }
}

/*!
 * @brief Mコマンドによる縮小マップの表示を行う / Extract the attr/char to display at the given (legal) map location
 */
void map_info(player_type *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp, TERM_COLOR *tap, SYMBOL_CODE *tcp)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    OBJECT_IDX this_o_idx, next_o_idx = 0;
    FEAT_IDX feat = get_feat_mimic(g_ptr);
    feature_type *f_ptr = &f_info[feat];
    TERM_COLOR a;
    SYMBOL_CODE c;
    if (!have_flag(f_ptr->flags, FF_REMEMBER)) {
        if (!player_ptr->blind
            && ((g_ptr->info & (CAVE_MARK | CAVE_LITE | CAVE_MNLT))
                || ((g_ptr->info & CAVE_VIEW) && (((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) || player_ptr->see_nocto)))) {
            a = f_ptr->x_attr[F_LIT_STANDARD];
            c = f_ptr->x_char[F_LIT_STANDARD];
            if (player_ptr->wild_mode) {
                if (view_special_lite && !is_daytime()) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                }
            } else if (darkened_grid(player_ptr, g_ptr)) {
                feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;
                f_ptr = &f_info[feat];
                a = f_ptr->x_attr[F_LIT_STANDARD];
                c = f_ptr->x_char[F_LIT_STANDARD];
            } else if (view_special_lite) {
                if (g_ptr->info & (CAVE_LITE | CAVE_MNLT)) {
                    if (view_yellow_lite) {
                        a = f_ptr->x_attr[F_LIT_LITE];
                        c = f_ptr->x_char[F_LIT_LITE];
                    }
                } else if ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                } else if (!(g_ptr->info & CAVE_VIEW)) {
                    if (view_bright_lite) {
                        a = f_ptr->x_attr[F_LIT_DARK];
                        c = f_ptr->x_char[F_LIT_DARK];
                    }
                }
            }
        } else {
            feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;
            f_ptr = &f_info[feat];
            a = f_ptr->x_attr[F_LIT_STANDARD];
            c = f_ptr->x_char[F_LIT_STANDARD];
        }
    } else {
        if (g_ptr->info & CAVE_MARK) {
            a = f_ptr->x_attr[F_LIT_STANDARD];
            c = f_ptr->x_char[F_LIT_STANDARD];
            if (player_ptr->wild_mode) {
                if (view_granite_lite && (player_ptr->blind || !is_daytime())) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                }
            } else if (darkened_grid(player_ptr, g_ptr) && !player_ptr->blind) {
                if (have_flag(f_ptr->flags, FF_LOS) && have_flag(f_ptr->flags, FF_PROJECT)) {
                    feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;
                    f_ptr = &f_info[feat];
                    a = f_ptr->x_attr[F_LIT_STANDARD];
                    c = f_ptr->x_char[F_LIT_STANDARD];
                } else if (view_granite_lite && view_bright_lite) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                }
            } else if (view_granite_lite) {
                if (player_ptr->blind) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                } else if (g_ptr->info & (CAVE_LITE | CAVE_MNLT)) {
                    if (view_yellow_lite) {
                        a = f_ptr->x_attr[F_LIT_LITE];
                        c = f_ptr->x_char[F_LIT_LITE];
                    }
                } else if (view_bright_lite) {
                    if (!(g_ptr->info & CAVE_VIEW)) {
                        a = f_ptr->x_attr[F_LIT_DARK];
                        c = f_ptr->x_char[F_LIT_DARK];
                    } else if ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) {
                        a = f_ptr->x_attr[F_LIT_DARK];
                        c = f_ptr->x_char[F_LIT_DARK];
                    } else if (!have_flag(f_ptr->flags, FF_LOS) && !check_local_illumination(player_ptr, y, x)) {
                        a = f_ptr->x_attr[F_LIT_DARK];
                        c = f_ptr->x_char[F_LIT_DARK];
                    }
                }
            }
        } else {
            feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;
            f_ptr = &f_info[feat];
            a = f_ptr->x_attr[F_LIT_STANDARD];
            c = f_ptr->x_char[F_LIT_STANDARD];
        }
    }

    if (feat_priority == -1)
        feat_priority = f_ptr->priority;

    (*tap) = a;
    (*tcp) = c;
    (*ap) = a;
    (*cp) = c;

    if (player_ptr->image && one_in_(256))
        image_random(ap, cp);

    for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if (!(o_ptr->marked & OM_FOUND))
            continue;

        if (display_autopick) {
            byte act;

            match_autopick = find_autopick_list(player_ptr, o_ptr);
            if (match_autopick == -1)
                continue;

            act = autopick_list[match_autopick].action;

            if ((act & DO_DISPLAY) && (act & display_autopick)) {
                autopick_obj = o_ptr;
            } else {
                match_autopick = -1;
                continue;
            }
        }

        (*cp) = object_char(o_ptr);
        (*ap) = object_attr(o_ptr);
        feat_priority = 20;
        if (player_ptr->image)
            image_object(ap, cp);

        break;
    }

    if (g_ptr->m_idx && display_autopick != 0) {
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    if (!m_ptr->ml) {
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    monster_race *r_ptr = &r_info[m_ptr->ap_r_idx];
    feat_priority = 30;
    if (player_ptr->image) {
        if ((r_ptr->flags1 & (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)) == (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)) {
            /* Do nothing */
        } else {
            image_monster(ap, cp);
        }

        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    a = r_ptr->x_attr;
    c = r_ptr->x_char;
    if (!(r_ptr->flags1 & (RF1_CHAR_CLEAR | RF1_SHAPECHANGER | RF1_ATTR_CLEAR | RF1_ATTR_MULTI | RF1_ATTR_SEMIRAND))) {
        *ap = a;
        *cp = c;
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    if ((r_ptr->flags1 & (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)) == (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)) {
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    if ((r_ptr->flags1 & RF1_ATTR_CLEAR) && (*ap != TERM_DARK) && !use_graphics) {
        /* Do nothing */
    } else if ((r_ptr->flags1 & RF1_ATTR_MULTI) && !use_graphics) {
        if (r_ptr->flags2 & RF2_ATTR_ANY)
            *ap = randint1(15);
        else
            switch (randint1(7)) {
            case 1:
                *ap = TERM_RED;
                break;
            case 2:
                *ap = TERM_L_RED;
                break;
            case 3:
                *ap = TERM_WHITE;
                break;
            case 4:
                *ap = TERM_L_GREEN;
                break;
            case 5:
                *ap = TERM_BLUE;
                break;
            case 6:
                *ap = TERM_L_DARK;
                break;
            case 7:
                *ap = TERM_GREEN;
                break;
            }
    } else if ((r_ptr->flags1 & RF1_ATTR_SEMIRAND) && !use_graphics) {
        *ap = g_ptr->m_idx % 15 + 1;
    } else {
        *ap = a;
    }

    if ((r_ptr->flags1 & RF1_CHAR_CLEAR) && (*cp != ' ') && !use_graphics) {
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    if (r_ptr->flags1 & RF1_SHAPECHANGER) {
        if (use_graphics) {
            monster_race *tmp_r_ptr = &r_info[randint1(max_r_idx - 1)];
            *cp = tmp_r_ptr->x_char;
            *ap = tmp_r_ptr->x_attr;
        } else {
            *cp = (one_in_(25) ? image_object_hack[randint0(sizeof(image_object_hack) - 1)] : image_monster_hack[randint0(sizeof(image_monster_hack) - 1)]);
        }

        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    *cp = c;
    set_term_color(player_ptr, y, x, ap, cp);
}

void set_term_color(player_type *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp)
{
    if (!player_bold(player_ptr, y, x))
        return;

    monster_race *r_ptr = &r_info[0];
    *ap = r_ptr->x_attr;
    *cp = r_ptr->x_char;
    feat_priority = 31;
}

static concptr simplify_list[][2] = {
#ifdef JP
    { "の魔法書", "" }, { NULL, NULL }
#else
    { "^Ring of ", "=" }, { "^Amulet of ", "\"" }, { "^Scroll of ", "?" }, { "^Scroll titled ", "?" }, { "^Wand of ", "-" }, { "^Rod of ", "-" },
    { "^Staff of ", "_" }, { "^Potion of ", "!" }, { " Spellbook ", "" }, { "^Book of ", "" }, { " Magic [", "[" }, { " Book [", "[" }, { " Arts [", "[" },
    { "^Set of ", "" }, { "^Pair of ", "" }, { NULL, NULL }
#endif
};

static void display_shortened_item_name(player_type *player_ptr, object_type *o_ptr, int y)
{
    char buf[MAX_NLEN];
    object_desc(player_ptr, buf, o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NAME_ONLY));
    TERM_COLOR attr = tval_to_attr[o_ptr->tval % 128];

    if (player_ptr->image) {
        attr = TERM_WHITE;
        strcpy(buf, _("何か奇妙な物", "something strange"));
    }

    char *c = buf;
    for (c = buf; *c; c++) {
        for (int i = 0; simplify_list[i][1]; i++) {
            concptr org_w = simplify_list[i][0];

            if (*org_w == '^') {
                if (c == buf)
                    org_w++;
                else
                    continue;
            }

            if (strncmp(c, org_w, strlen(org_w)))
                continue;

            char *s = c;
            concptr tmp = simplify_list[i][1];
            while (*tmp)
                *s++ = *tmp++;
            tmp = c + strlen(org_w);
            while (*tmp)
                *s++ = *tmp++;
            *s = '\0';
        }
    }

    c = buf;
    int len = 0;
    /* 半角 12 文字分で切る */
    while (*c) {
#ifdef JP
        if (iskanji(*c)) {
            if (len + 2 > 12)
                break;
            c += 2;
            len += 2;
        } else
#endif
        {
            if (len + 1 > 12)
                break;
            c++;
            len++;
        }
    }

    *c = '\0';
    Term_putstr(0, y, 12, attr, buf);
}

/*
 * Display a "small-scale" map of the dungeon in the active Term
 */
void display_map(player_type *player_ptr, int *cy, int *cx)
{
    int i, j, x, y;

    TERM_COLOR ta;
    SYMBOL_CODE tc;

    byte tp;

    TERM_COLOR **bigma;
    SYMBOL_CODE **bigmc;
    byte **bigmp;

    TERM_COLOR **ma;
    SYMBOL_CODE **mc;
    byte **mp;

    bool old_view_special_lite = view_special_lite;
    bool old_view_granite_lite = view_granite_lite;
    TERM_LEN hgt, wid, yrat, xrat;
    int **match_autopick_yx;
    object_type ***object_autopick_yx;
    Term_get_size(&wid, &hgt);
    hgt -= 2;
    wid -= 14;
    if (use_bigtile)
        wid /= 2;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    yrat = (floor_ptr->height + hgt - 1) / hgt;
    xrat = (floor_ptr->width + wid - 1) / wid;
    view_special_lite = FALSE;
    view_granite_lite = FALSE;

    C_MAKE(ma, (hgt + 2), TERM_COLOR *);
    C_MAKE(mc, (hgt + 2), char_ptr);
    C_MAKE(mp, (hgt + 2), byte_ptr);
    C_MAKE(match_autopick_yx, (hgt + 2), int *);
    C_MAKE(object_autopick_yx, (hgt + 2), object_type **);
    for (y = 0; y < (hgt + 2); y++) {
        C_MAKE(ma[y], (wid + 2), TERM_COLOR);
        C_MAKE(mc[y], (wid + 2), char);
        C_MAKE(mp[y], (wid + 2), byte);
        C_MAKE(match_autopick_yx[y], (wid + 2), int);
        C_MAKE(object_autopick_yx[y], (wid + 2), object_type *);
        for (x = 0; x < wid + 2; ++x) {
            match_autopick_yx[y][x] = -1;
            object_autopick_yx[y][x] = NULL;
            ma[y][x] = TERM_WHITE;
            mc[y][x] = ' ';
            mp[y][x] = 0;
        }
    }

    C_MAKE(bigma, (floor_ptr->height + 2), TERM_COLOR *);
    C_MAKE(bigmc, (floor_ptr->height + 2), char_ptr);
    C_MAKE(bigmp, (floor_ptr->height + 2), byte_ptr);
    for (y = 0; y < (floor_ptr->height + 2); y++) {
        C_MAKE(bigma[y], (floor_ptr->width + 2), TERM_COLOR);
        C_MAKE(bigmc[y], (floor_ptr->width + 2), char);
        C_MAKE(bigmp[y], (floor_ptr->width + 2), byte);
        for (x = 0; x < floor_ptr->width + 2; ++x) {
            bigma[y][x] = TERM_WHITE;
            bigmc[y][x] = ' ';
            bigmp[y][x] = 0;
        }
    }

    for (i = 0; i < floor_ptr->width; ++i) {
        for (j = 0; j < floor_ptr->height; ++j) {
            x = i / xrat + 1;
            y = j / yrat + 1;

            match_autopick = -1;
            autopick_obj = NULL;
            feat_priority = -1;
            map_info(player_ptr, j, i, &ta, &tc, &ta, &tc);
            tp = (byte)feat_priority;
            if (match_autopick != -1 && (match_autopick_yx[y][x] == -1 || match_autopick_yx[y][x] > match_autopick)) {
                match_autopick_yx[y][x] = match_autopick;
                object_autopick_yx[y][x] = autopick_obj;
                tp = 0x7f;
            }

            bigmc[j + 1][i + 1] = tc;
            bigma[j + 1][i + 1] = ta;
            bigmp[j + 1][i + 1] = tp;
        }
    }

    for (j = 0; j < floor_ptr->height; ++j) {
        for (i = 0; i < floor_ptr->width; ++i) {
            x = i / xrat + 1;
            y = j / yrat + 1;

            tc = bigmc[j + 1][i + 1];
            ta = bigma[j + 1][i + 1];
            tp = bigmp[j + 1][i + 1];
            if (mp[y][x] == tp) {
                int t;
                int cnt = 0;

                for (t = 0; t < 8; t++) {
                    if (tc == bigmc[j + 1 + ddy_cdd[t]][i + 1 + ddx_cdd[t]] && ta == bigma[j + 1 + ddy_cdd[t]][i + 1 + ddx_cdd[t]])
                        cnt++;
                }
                if (cnt <= 4)
                    tp++;
            }

            if (mp[y][x] < tp) {
                mc[y][x] = tc;
                ma[y][x] = ta;
                mp[y][x] = tp;
            }
        }
    }

    x = wid + 1;
    y = hgt + 1;

    mc[0][0] = mc[0][x] = mc[y][0] = mc[y][x] = '+';
    for (x = 1; x <= wid; x++)
        mc[0][x] = mc[y][x] = '-';

    for (y = 1; y <= hgt; y++)
        mc[y][0] = mc[y][x] = '|';

    for (y = 0; y < hgt + 2; ++y) {
        Term_gotoxy(COL_MAP, y);
        for (x = 0; x < wid + 2; ++x) {
            ta = ma[y][x];
            tc = mc[y][x];
            if (!use_graphics) {
                if (current_world_ptr->timewalk_m_idx)
                    ta = TERM_DARK;
                else if (is_invuln(player_ptr) || player_ptr->timewalk)
                    ta = TERM_WHITE;
                else if (player_ptr->wraith_form)
                    ta = TERM_L_DARK;
            }

            Term_add_bigch(ta, tc);
        }
    }

    for (y = 1; y < hgt + 1; ++y) {
        match_autopick = -1;
        for (x = 1; x <= wid; x++) {
            if (match_autopick_yx[y][x] != -1 && (match_autopick > match_autopick_yx[y][x] || match_autopick == -1)) {
                match_autopick = match_autopick_yx[y][x];
                autopick_obj = object_autopick_yx[y][x];
            }
        }

        Term_putstr(0, y, 12, 0, "            ");
        if (match_autopick != -1)
            display_shortened_item_name(player_ptr, autopick_obj, y);
    }

    (*cy) = player_ptr->y / yrat + 1 + ROW_MAP;
    if (!use_bigtile)
        (*cx) = player_ptr->x / xrat + 1 + COL_MAP;
    else
        (*cx) = (player_ptr->x / xrat + 1) * 2 + COL_MAP;

    view_special_lite = old_view_special_lite;
    view_granite_lite = old_view_granite_lite;

    for (y = 0; y < (hgt + 2); y++) {
        C_KILL(ma[y], (wid + 2), TERM_COLOR);
        C_KILL(mc[y], (wid + 2), SYMBOL_CODE);
        C_KILL(mp[y], (wid + 2), byte);
        C_KILL(match_autopick_yx[y], (wid + 2), int);
        C_KILL(object_autopick_yx[y], (wid + 2), object_type *);
    }

    C_KILL(ma, (hgt + 2), TERM_COLOR *);
    C_KILL(mc, (hgt + 2), char_ptr);
    C_KILL(mp, (hgt + 2), byte_ptr);
    C_KILL(match_autopick_yx, (hgt + 2), int *);
    C_KILL(object_autopick_yx, (hgt + 2), object_type **);
    for (y = 0; y < (floor_ptr->height + 2); y++) {
        C_KILL(bigma[y], (floor_ptr->width + 2), TERM_COLOR);
        C_KILL(bigmc[y], (floor_ptr->width + 2), SYMBOL_CODE);
        C_KILL(bigmp[y], (floor_ptr->width + 2), byte);
    }

    C_KILL(bigma, (floor_ptr->height + 2), TERM_COLOR *);
    C_KILL(bigmc, (floor_ptr->height + 2), char_ptr);
    C_KILL(bigmp, (floor_ptr->height + 2), byte_ptr);
}

/*
 * Display a "small-scale" map of the dungeon for the player
 *
 * Currently, the "player" is displayed on the map.
 */
void do_cmd_view_map(player_type *player_ptr)
{
    screen_save();
    prt(_("お待ち下さい...", "Please wait..."), 0, 0);
    Term_fresh();
    Term_clear();
    display_autopick = 0;

    int cy, cx;
    display_map(player_ptr, &cy, &cx);
    if ((max_autopick == 0) || player_ptr->wild_mode) {
        put_str(_("何かキーを押すとゲームに戻ります", "Hit any key to continue"), 23, 30);
        move_cursor(cy, cx);
        inkey();
        screen_load();
        return;
    }

    display_autopick = ITEM_DISPLAY;
    while (TRUE) {
        int wid, hgt;
        Term_get_size(&wid, &hgt);
        int row_message = hgt - 1;
        put_str(_("何かキーを押してください('M':拾う 'N':放置 'D':M+N 'K':壊すアイテムを表示)",
                    " Hit M, N(for ~), K(for !), or D(same as M+N) to display auto-picker items."),
            row_message, 1);
        move_cursor(cy, cx);
        int i = inkey();
        byte flag;
        if ('M' == i)
            flag = (DO_AUTOPICK | DO_QUERY_AUTOPICK);
        else if ('N' == i)
            flag = DONT_AUTOPICK;
        else if ('K' == i)
            flag = DO_AUTODESTROY;
        else if ('D' == i)
            flag = (DO_AUTOPICK | DO_QUERY_AUTOPICK | DONT_AUTOPICK);
        else
            break;

        Term_fresh();
        if (~display_autopick & flag)
            display_autopick |= flag;
        else
            display_autopick &= ~flag;

        display_map(player_ptr, &cy, &cx);
    }

    display_autopick = 0;
    screen_load();
}

/*
 * Track a new monster
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx トラッキング対象のモンスターID。0の時キャンセル
 * @param なし
 */
void health_track(player_type *player_ptr, MONSTER_IDX m_idx)
{
    if (m_idx && m_idx == player_ptr->riding)
        return;

    player_ptr->health_who = m_idx;
    player_ptr->redraw |= (PR_HEALTH);
}

/*
 * Moves the cursor to a given MAP (y,x) location
 */
void move_cursor_relative(int row, int col)
{
    row -= panel_row_prt;
    Term_gotoxy(panel_col_of(col), row);
}

/*
 * print project path
 */
void print_path(player_type *player_ptr, POSITION y, POSITION x)
{
    int path_n;
    u16b path_g[512];
    byte default_color = TERM_SLATE;

    if (!display_path)
        return;
    if (project_length == -1)
        return;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    path_n = project_path(player_ptr, path_g, (project_length ? project_length : MAX_RANGE), player_ptr->y, player_ptr->x, y, x, PROJECT_PATH | PROJECT_THRU);
    player_ptr->redraw |= (PR_MAP);
    handle_stuff(player_ptr);
    for (int i = 0; i < path_n; i++) {
        POSITION ny = GRID_Y(path_g[i]);
        POSITION nx = GRID_X(path_g[i]);
        grid_type *g_ptr = &floor_ptr->grid_array[ny][nx];
        if (panel_contains(ny, nx)) {
            TERM_COLOR a = default_color;
            SYMBOL_CODE c;

            TERM_COLOR ta = default_color;
            SYMBOL_CODE tc = '*';

            if (g_ptr->m_idx && floor_ptr->m_list[g_ptr->m_idx].ml) {
                map_info(player_ptr, ny, nx, &a, &c, &ta, &tc);

                if (!IS_ASCII_GRAPHICS(a))
                    a = default_color;
                else if (c == '.' && (a == TERM_WHITE || a == TERM_L_WHITE))
                    a = default_color;
                else if (a == default_color)
                    a = TERM_WHITE;
            }

            if (!use_graphics) {
                if (current_world_ptr->timewalk_m_idx)
                    a = TERM_DARK;
                else if (is_invuln(player_ptr) || player_ptr->timewalk)
                    a = TERM_WHITE;
                else if (player_ptr->wraith_form)
                    a = TERM_L_DARK;
            }

            c = '*';
            Term_queue_bigchar(panel_col_of(nx), ny - panel_row_prt, a, c, ta, tc);
        }

        if ((g_ptr->info & CAVE_MARK) && !cave_have_flag_grid(g_ptr, FF_PROJECT))
            break;

        if (nx == x && ny == y)
            default_color = TERM_L_DARK;
    }
}

/*
 * Track the given monster race
 */
void monster_race_track(player_type *player_ptr, MONRACE_IDX r_idx)
{
    player_ptr->monster_race_idx = r_idx;
    player_ptr->window |= (PW_MONSTER);
}

/*
 * Track the given object kind
 */
void object_kind_track(player_type *player_ptr, KIND_OBJECT_IDX k_idx)
{
    player_ptr->object_kind_idx = k_idx;
    player_ptr->window |= (PW_OBJECT);
}

/*!
 * @brief 実ゲームプレイ時間を更新する
 */
void update_playtime(void)
{
    if (current_world_ptr->start_time != 0) {
        u32b tmp = (u32b)time(NULL);
        current_world_ptr->play_time += (tmp - current_world_ptr->start_time);
        current_world_ptr->start_time = tmp;
    }
}

bool panel_contains(POSITION y, POSITION x) { return (y >= panel_row_min) && (y <= panel_row_max) && (x >= panel_col_min) && (x <= panel_col_max); }

/*
 * Delayed visual update
 * Only used if update_view(), update_lite() or update_mon_lite() was called
 */
void delayed_visual_update(player_type *player_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = 0; i < floor_ptr->redraw_n; i++) {
        POSITION y = floor_ptr->redraw_y[i];
        POSITION x = floor_ptr->redraw_x[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        if (!(g_ptr->info & CAVE_REDRAW))
            continue;

        if (g_ptr->info & CAVE_NOTE)
            note_spot(player_ptr, y, x);

        lite_spot(player_ptr, y, x);
        if (g_ptr->m_idx)
            update_monster(player_ptr, g_ptr->m_idx, FALSE);

        g_ptr->info &= ~(CAVE_NOTE | CAVE_REDRAW);
    }

    floor_ptr->redraw_n = 0;
}
