#include "window/main-window-stat-poster.h"
#include "io/input-key-requester.h"
#include "mind/stances-table.h"
#include "monster/monster-status.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/mane-data-type.h"
#include "player-info/monk-data-type.h"
#include "player-info/ninja-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "status/element-resistance.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "view/status-bars-table.h"
#include "window/main-window-row-column.h"
#include "world/world.h"

/*!
 * @brief 32ビット変数配列の指定位置のビットフラグを1にする。
 * @param FLG フラグ位置(ビット)
 */
#define ADD_BAR_FLAG(FLG) (bar_flags[FLG / 32] |= (1UL << (FLG % 32)))

/*!
 * @brief 32ビット変数配列の指定位置のビットフラグが1かどうかを返す。
 * @param FLG フラグ位置(ビット)
 * @return 1ならば0以外を返す
 */
#define IS_BAR_FLAG(FLG) (bar_flags[FLG / 32] & (1UL << (FLG % 32)))

/*!
 * @brief プレイヤー能力値を描画する / Print character stat in given row, column
 * @param stat 描画するステータスのID
 */
void print_stat(PlayerType *player_ptr, int stat)
{
    GAME_TEXT tmp[32];
    if (player_ptr->stat_cur[stat] < player_ptr->stat_max[stat]) {
        put_str(stat_names_reduced[stat], ROW_STAT + stat, 0);
        cnv_stat(player_ptr->stat_use[stat], tmp);
        c_put_str(TERM_YELLOW, tmp, ROW_STAT + stat, COL_STAT + 6);
    } else {
        put_str(stat_names[stat], ROW_STAT + stat, 0);
        cnv_stat(player_ptr->stat_use[stat], tmp);
        c_put_str(TERM_L_GREEN, tmp, ROW_STAT + stat, COL_STAT + 6);
    }

    if (player_ptr->stat_max[stat] != player_ptr->stat_max_max[stat]) {
        return;
    }

#ifdef JP
    /* 日本語にかぶらないように表示位置を変更 */
    put_str("!", ROW_STAT + stat, 5);
#else
    put_str("!", ROW_STAT + stat, 3);
#endif
}

/*!
 * @brief プレイヤーの負傷状態を表示する
 */
void print_cut(PlayerType *player_ptr)
{
    auto player_cut = player_ptr->effects()->cut();
    if (!player_cut->is_cut()) {
        put_str("            ", ROW_CUT, COL_CUT);
        return;
    }

    auto [color, stat] = player_cut->get_expr();
    c_put_str(color, stat.data(), ROW_CUT, COL_CUT);
}

/*!
 * @brief プレイヤーの朦朧状態を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_stun(PlayerType *player_ptr)
{
    auto player_stun = player_ptr->effects()->stun();
    if (!player_stun->is_stunned()) {
        put_str("            ", ROW_STUN, COL_STUN);
        return;
    }

    auto [color, stat] = player_stun->get_expr();
    c_put_str(color, stat.data(), ROW_STUN, COL_STUN);
}

/*!
 * @brief プレイヤーの空腹状態を表示する / Prints status of hunger
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_hunger(PlayerType *player_ptr)
{
    if (w_ptr->wizard && player_ptr->current_floor_ptr->inside_arena) {
        return;
    }

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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Display is always exactly 10 characters wide (see below)
 * This function was a major bottleneck when resting, so a lot of
 * the text formatting code was optimized in place below.
 */
void print_state(PlayerType *player_ptr)
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
        auto bluemage_data = PlayerClass(player_ptr).get_specific_data<bluemage_data_type>();
        if (bluemage_data->new_magic_learned) {
            attr = TERM_L_RED;
        }
        break;
    }
    case ACTION_FISH: {
        strcpy(text, _("釣り", "fish"));
        break;
    }
    case ACTION_MONK_STANCE: {
        if (auto stance = PlayerClass(player_ptr).get_monk_stance();
            stance != MonkStanceType::NONE) {
            switch (stance) {
            case MonkStanceType::GENBU:
                attr = TERM_GREEN;
                break;
            case MonkStanceType::BYAKKO:
                attr = TERM_WHITE;
                break;
            case MonkStanceType::SEIRYU:
                attr = TERM_L_BLUE;
                break;
            case MonkStanceType::SUZAKU:
                attr = TERM_L_RED;
                break;
            default:
                break;
            }
            strcpy(text, monk_stances[enum2i(stance) - 1].desc);
        }
        break;
    }
    case ACTION_SAMURAI_STANCE: {
        if (auto stance = PlayerClass(player_ptr).get_samurai_stance();
            stance != SamuraiStanceType::NONE) {
            strcpy(text, samurai_stances[enum2i(stance) - 1].desc);
        }
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
 * @brief プレイヤーの行動速度を表示する / Prints the speed_value of a character.			-CJS-
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_speed(PlayerType *player_ptr)
{
    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    TERM_LEN col_speed = wid + COL_SPEED;
    TERM_LEN row_speed = hgt + ROW_SPEED;

    int speed_value = player_ptr->pspeed - 110;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    bool is_player_fast = is_fast(player_ptr);
    char buf[32] = "";
    TERM_COLOR attr = TERM_WHITE;
    if (speed_value > 0) {
        if (player_ptr->riding) {
            auto *m_ptr = &floor_ptr->m_list[player_ptr->riding];
            if (monster_fast_remaining(m_ptr) && !monster_slow_remaining(m_ptr)) {
                attr = TERM_L_BLUE;
            } else if (monster_slow_remaining(m_ptr) && !monster_fast_remaining(m_ptr)) {
                attr = TERM_VIOLET;
            } else {
                attr = TERM_GREEN;
            }
        } else if ((is_player_fast && !player_ptr->slow) || player_ptr->lightspeed) {
            attr = TERM_YELLOW;
        } else if (player_ptr->slow && !is_player_fast) {
            attr = TERM_VIOLET;
        } else {
            attr = TERM_L_GREEN;
        }
        sprintf(buf, "%s(+%d)", (player_ptr->riding ? _("乗馬", "Ride") : _("加速", "Fast")), speed_value);
    } else if (speed_value < 0) {
        if (player_ptr->riding) {
            auto *m_ptr = &floor_ptr->m_list[player_ptr->riding];
            if (monster_fast_remaining(m_ptr) && !monster_slow_remaining(m_ptr)) {
                attr = TERM_L_BLUE;
            } else if (monster_slow_remaining(m_ptr) && !monster_fast_remaining(m_ptr)) {
                attr = TERM_VIOLET;
            } else {
                attr = TERM_RED;
            }
        } else if (is_player_fast && !player_ptr->slow) {
            attr = TERM_YELLOW;
        } else if (player_ptr->slow && !is_player_fast) {
            attr = TERM_VIOLET;
        } else {
            attr = TERM_L_UMBER;
        }
        sprintf(buf, "%s(%d)", (player_ptr->riding ? _("乗馬", "Ride") : _("減速", "Slow")), speed_value);
    } else if (player_ptr->riding) {
        attr = TERM_GREEN;
        strcpy(buf, _("乗馬中", "Riding"));
    }

    c_put_str(attr, format("%-9s", buf), row_speed, col_speed);
}

/*!
 * @brief プレイヤーの呪文学習可能状態を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_study(PlayerType *player_ptr)
{
    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
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
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_imitation(PlayerType *player_ptr)
{
    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    TERM_LEN col_study = wid + COL_STUDY;
    TERM_LEN row_study = hgt + ROW_STUDY;

    PlayerClass pc(player_ptr);
    if (!pc.equals(PlayerClassType::IMITATOR)) {
        return;
    }

    auto mane_data = pc.get_specific_data<mane_data_type>();

    if (mane_data->mane_list.size() == 0) {
        put_str("    ", row_study, col_study);
        return;
    }

    TERM_COLOR attr = mane_data->new_mane ? TERM_L_RED : TERM_WHITE;
    c_put_str(attr, _("まね", "Imit"), row_study, col_study);
}

/*!
 * @brief 画面下部に表示すべき呪術の呪文をリストアップする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @bar_flags 表示可否を決めるためのフラグ群
 */
static void add_hex_status_flags(PlayerType *player_ptr, BIT_FLAGS *bar_flags)
{
    if (player_ptr->realm1 != REALM_HEX) {
        return;
    }

    SpellHex spell_hex(player_ptr);
    if (spell_hex.is_spelling_specific(HEX_BLESS)) {
        ADD_BAR_FLAG(BAR_BLESSED);
    }

    if (spell_hex.is_spelling_specific(HEX_DEMON_AURA)) {
        ADD_BAR_FLAG(BAR_SHFIRE);
        ADD_BAR_FLAG(BAR_REGENERATION);
    }

    if (spell_hex.is_spelling_specific(HEX_XTRA_MIGHT)) {
        ADD_BAR_FLAG(BAR_MIGHT);
    }

    if (spell_hex.is_spelling_specific(HEX_DETECT_EVIL)) {
        ADD_BAR_FLAG(BAR_ESP_EVIL);
    }

    if (spell_hex.is_spelling_specific(HEX_ICE_ARMOR)) {
        ADD_BAR_FLAG(BAR_SHCOLD);
    }

    if (spell_hex.is_spelling_specific(HEX_RUNESWORD)) {
        ADD_BAR_FLAG(BAR_RUNESWORD);
    }

    if (spell_hex.is_spelling_specific(HEX_BUILDING)) {
        ADD_BAR_FLAG(BAR_BUILD);
    }

    if (spell_hex.is_spelling_specific(HEX_ANTI_TELE)) {
        ADD_BAR_FLAG(BAR_ANTITELE);
    }

    if (spell_hex.is_spelling_specific(HEX_SHOCK_CLOAK)) {
        ADD_BAR_FLAG(BAR_SHELEC);
    }

    if (spell_hex.is_spelling_specific(HEX_SHADOW_CLOAK)) {
        ADD_BAR_FLAG(BAR_SHSHADOW);
    }

    if (spell_hex.is_spelling_specific(HEX_CONFUSION)) {
        ADD_BAR_FLAG(BAR_ATTKCONF);
    }

    if (spell_hex.is_spelling_specific(HEX_EYE_FOR_EYE)) {
        ADD_BAR_FLAG(BAR_EYEEYE);
    }

    if (spell_hex.is_spelling_specific(HEX_ANTI_MULTI)) {
        ADD_BAR_FLAG(BAR_ANTIMULTI);
    }

    if (spell_hex.is_spelling_specific(HEX_VAMP_BLADE)) {
        ADD_BAR_FLAG(BAR_VAMPILIC);
    }

    if (spell_hex.is_spelling_specific(HEX_ANTI_MAGIC)) {
        ADD_BAR_FLAG(BAR_ANTIMAGIC);
    }

    if (spell_hex.is_spelling_specific(HEX_CURE_LIGHT) || spell_hex.is_spelling_specific(HEX_CURE_SERIOUS) || spell_hex.is_spelling_specific(HEX_CURE_CRITICAL)) {
        ADD_BAR_FLAG(BAR_CURE);
    }

    if (spell_hex.get_revenge_turn() > 0) {
        auto revenge_type = spell_hex.get_revenge_type();
        if (revenge_type == SpellHexRevengeType::PATIENCE) {
            ADD_BAR_FLAG(BAR_PATIENCE);
        }

        if (revenge_type == SpellHexRevengeType::REVENGE) {
            ADD_BAR_FLAG(BAR_REVENGE);
        }
    }
}

/*!
 * @brief 下部に状態表示を行う / Show status bar
 */
void print_status(PlayerType *player_ptr)
{
    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    TERM_LEN row_statbar = hgt + ROW_STATBAR;
    TERM_LEN max_col_statbar = wid + MAX_COL_STATBAR;

    term_erase(0, row_statbar, max_col_statbar);

    BIT_FLAGS bar_flags[3];
    bar_flags[0] = bar_flags[1] = bar_flags[2] = 0L;

    if (player_ptr->tsuyoshi) {
        ADD_BAR_FLAG(BAR_TSUYOSHI);
    }

    if (player_ptr->hallucinated) {
        ADD_BAR_FLAG(BAR_HALLUCINATION);
    }

    if (player_ptr->blind) {
        ADD_BAR_FLAG(BAR_BLINDNESS);
    }

    if (player_ptr->paralyzed) {
        ADD_BAR_FLAG(BAR_PARALYZE);
    }

    if (player_ptr->effects()->confusion()->is_confused()) {
        ADD_BAR_FLAG(BAR_CONFUSE);
    }

    if (player_ptr->poisoned) {
        ADD_BAR_FLAG(BAR_POISONED);
    }

    if (player_ptr->tim_invis) {
        ADD_BAR_FLAG(BAR_SENSEUNSEEN);
    }

    auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();
    if (sniper_data && (sniper_data->concent >= CONCENT_RADAR_THRESHOLD)) {
        ADD_BAR_FLAG(BAR_SENSEUNSEEN);
        ADD_BAR_FLAG(BAR_NIGHTSIGHT);
    }

    if (is_time_limit_esp(player_ptr)) {
        ADD_BAR_FLAG(BAR_TELEPATHY);
    }

    if (player_ptr->tim_regen) {
        ADD_BAR_FLAG(BAR_REGENERATION);
    }

    if (player_ptr->tim_infra) {
        ADD_BAR_FLAG(BAR_INFRAVISION);
    }

    if (player_ptr->protevil) {
        ADD_BAR_FLAG(BAR_PROTEVIL);
    }

    if (is_invuln(player_ptr)) {
        ADD_BAR_FLAG(BAR_INVULN);
    }

    if (player_ptr->wraith_form) {
        ADD_BAR_FLAG(BAR_WRAITH);
    }

    if (player_ptr->tim_pass_wall) {
        ADD_BAR_FLAG(BAR_PASSWALL);
    }

    if (player_ptr->tim_reflect) {
        ADD_BAR_FLAG(BAR_REFLECTION);
    }

    if (is_hero(player_ptr)) {
        ADD_BAR_FLAG(BAR_HEROISM);
    }

    if (is_shero(player_ptr)) {
        ADD_BAR_FLAG(BAR_BERSERK);
    }

    if (is_blessed(player_ptr)) {
        ADD_BAR_FLAG(BAR_BLESSED);
    }

    if (player_ptr->magicdef) {
        ADD_BAR_FLAG(BAR_MAGICDEFENSE);
    }

    if (player_ptr->tsubureru) {
        ADD_BAR_FLAG(BAR_EXPAND);
    }

    if (player_ptr->shield) {
        ADD_BAR_FLAG(BAR_STONESKIN);
    }

    auto ninja_data = PlayerClass(player_ptr).get_specific_data<ninja_data_type>();
    if (ninja_data && ninja_data->kawarimi) {
        ADD_BAR_FLAG(BAR_KAWARIMI);
    }

    if (player_ptr->special_defense & DEFENSE_ACID) {
        ADD_BAR_FLAG(BAR_IMMACID);
    }

    if (is_oppose_acid(player_ptr)) {
        ADD_BAR_FLAG(BAR_RESACID);
    }

    if (player_ptr->special_defense & DEFENSE_ELEC) {
        ADD_BAR_FLAG(BAR_IMMELEC);
    }

    if (is_oppose_elec(player_ptr)) {
        ADD_BAR_FLAG(BAR_RESELEC);
    }

    if (player_ptr->special_defense & DEFENSE_FIRE) {
        ADD_BAR_FLAG(BAR_IMMFIRE);
    }

    if (is_oppose_fire(player_ptr)) {
        ADD_BAR_FLAG(BAR_RESFIRE);
    }

    if (player_ptr->special_defense & DEFENSE_COLD) {
        ADD_BAR_FLAG(BAR_IMMCOLD);
    }

    if (is_oppose_cold(player_ptr)) {
        ADD_BAR_FLAG(BAR_RESCOLD);
    }

    if (is_oppose_pois(player_ptr)) {
        ADD_BAR_FLAG(BAR_RESPOIS);
    }

    if (player_ptr->word_recall) {
        ADD_BAR_FLAG(BAR_RECALL);
    }

    if (player_ptr->alter_reality) {
        ADD_BAR_FLAG(BAR_ALTER);
    }

    if (player_ptr->afraid) {
        ADD_BAR_FLAG(BAR_AFRAID);
    }

    if (player_ptr->tim_res_time) {
        ADD_BAR_FLAG(BAR_RESTIME);
    }

    if (player_ptr->multishadow) {
        ADD_BAR_FLAG(BAR_MULTISHADOW);
    }

    if (player_ptr->special_attack & ATTACK_CONFUSE) {
        ADD_BAR_FLAG(BAR_ATTKCONF);
    }

    if (player_ptr->resist_magic) {
        ADD_BAR_FLAG(BAR_REGMAGIC);
    }

    if (player_ptr->ult_res) {
        ADD_BAR_FLAG(BAR_ULTIMATE);
    }

    if (player_ptr->tim_levitation) {
        ADD_BAR_FLAG(BAR_LEVITATE);
    }

    if (player_ptr->tim_res_nether) {
        ADD_BAR_FLAG(BAR_RESNETH);
    }

    if (player_ptr->dustrobe) {
        ADD_BAR_FLAG(BAR_DUSTROBE);
    }

    if (player_ptr->special_attack & ATTACK_FIRE) {
        ADD_BAR_FLAG(BAR_ATTKFIRE);
    }

    if (player_ptr->special_attack & ATTACK_COLD) {
        ADD_BAR_FLAG(BAR_ATTKCOLD);
    }

    if (player_ptr->special_attack & ATTACK_ELEC) {
        ADD_BAR_FLAG(BAR_ATTKELEC);
    }

    if (player_ptr->special_attack & ATTACK_ACID) {
        ADD_BAR_FLAG(BAR_ATTKACID);
    }

    if (player_ptr->special_attack & ATTACK_POIS) {
        ADD_BAR_FLAG(BAR_ATTKPOIS);
    }

    if (ninja_data && ninja_data->s_stealth) {
        ADD_BAR_FLAG(BAR_SUPERSTEALTH);
    }

    if (player_ptr->tim_sh_fire) {
        ADD_BAR_FLAG(BAR_SHFIRE);
    }

    if (is_time_limit_stealth(player_ptr)) {
        ADD_BAR_FLAG(BAR_STEALTH);
    }

    if (player_ptr->tim_sh_touki) {
        ADD_BAR_FLAG(BAR_TOUKI);
    }

    if (player_ptr->tim_sh_holy) {
        ADD_BAR_FLAG(BAR_SHHOLY);
    }

    if (player_ptr->tim_eyeeye) {
        ADD_BAR_FLAG(BAR_EYEEYE);
    }

    add_hex_status_flags(player_ptr, bar_flags);
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
        if (!IS_BAR_FLAG(i)) {
            continue;
        }

        concptr str;
        if (space == 2) {
            str = stat_bars[i].lstr;
        } else {
            str = stat_bars[i].sstr;
        }

        c_put_str(stat_bars[i].attr, str, row_statbar, col);
        col += strlen(str);
        if (space > 0) {
            col++;
        }

        if (col > max_col_statbar) {
            break;
        }
    }
}

/*!
 * @brief プレイヤーのステータスを一括表示する（下部分） / Display extra info (mostly below map)
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_frame_extra(PlayerType *player_ptr)
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
