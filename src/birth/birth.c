/*!
 * @file birth.c
 * @brief プレイヤーの作成を行う / Create a player character
 * @date 2013/12/28
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2013 Deskull Doxygen向けのコメント整理\n
 */

#include "system/angband.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "io/read-pref-file.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "market/building.h"
#include "util/util.h"

#include "object/artifact.h"
#include "autopick/autopick.h"
#include "player/avatar.h"
#include "birth/birth.h"
#include "birth/birth-explanations-table.h"
#include "cmd-pet.h"
#include "cmd/cmd-dump.h"
#include "cmd/cmd-help.h"
#include "dungeon/dungeon-file.h"
#include "dungeon/dungeon.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "birth/history.h"
#include "io/write-diary.h"
#include "locale/japanese.h"
#include "market/store.h"
#include "monster/monster.h"
#include "monster/monsterrace-hook.h"
#include "monster/monster-race.h"
#include "object/object-ego.h"
#include "object/object-kind.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "player/process-name.h"
#include "player/race-info-table.h"
#include "dungeon/quest.h"
#include "realm/realm.h"
#include "io/save.h"
#include "spell/spells-util.h"
#include "spell/spells-status.h"
#include "view/display-main-window.h" // 暫定。後で消す.
#include "view/display-player.h" // 暫定。後で消す.
#include "floor/wild.h"
#include "world/world.h"
#include "birth/birth-util.h"
#include "birth/birth-select-realm.h"
#include "birth/quick-start.h"

/*!
 * オートローラーの内容を描画する間隔 /
 * How often the autoroller will update the display and pause
 * to check for user interuptions.
 * Bigger values will make the autoroller faster, but slower
 * system may have problems because the user can't stop the
 * autoroller for this number of rolls.
 */
#define AUTOROLLER_STEP 5431L

/*!
  * ランダムクエストのモンスターを確定するために試行する回数 / Maximum number of tries for selection of a proper quest monster
  */
#define MAX_TRIES 100

/* 選択可能な職業の最大数 */
#define MAX_CLASS_CHOICE MAX_CLASS

/*! オートローラの能力値的要求水準 / Autoroll limit */
static s16b stat_limit[6];

/*! オートローラの年齢、身長、体重、社会的地位の要求水準 */
static struct {
    s16b agemin, agemax;
    s16b htmin, htmax;
    s16b wtmin, wtmax;
    s16b scmin, scmax;
} chara_limit;

/*! オートローラ中、各能力値が水準を超えた回数 / Autoroll matches */
static s32b stat_match[6];

/*! オートローラの試行回数 / Autoroll round */
static s32b auto_round;

/*!
 * @brief プレイヤーの能力値表現に基づいて加減算を行う。
 * @param value 現在の能力値
 * @param amount 加減算する値
 * @return 加減算の結果
 */
static int adjust_stat(int value, int amount)
{
    int i;

    /* Negative amounts */
    if (amount < 0) {
        /* Apply penalty */
        for (i = 0; i < (0 - amount); i++) {
            if (value >= 18 + 10) {
                value -= 10;
            } else if (value > 18) {
                value = 18;
            } else if (value > 3) {
                value--;
            }
        }
    }

    /* Positive amounts */
    else if (amount > 0) {
        /* Apply reward */
        for (i = 0; i < amount; i++) {
            if (value < 18) {
                value++;
            } else {
                value += 10;
            }
        }
    }

    /* Return the result */
    return (value);
}

/*!
 * @brief プレイヤーの能力値を一通りロールする。 / Roll for a characters stats
 * @details
 * calc_bonuses()による、独立ステータスからの副次ステータス算出も行っている。
 * For efficiency, we include a chunk of "calc_bonuses()".\n
 * @return なし
 */
static void get_stats(player_type* creature_ptr)
{
    /* Roll and verify some stats */
    while (TRUE) {
        int i;
        int sum = 0;

        /* Roll some dice */
        for (i = 0; i < 2; i++) {
            s32b tmp = randint0(60 * 60 * 60);
            BASE_STATUS val;

            /* Extract 5 + 1d3 + 1d4 + 1d5 */
            val = 5 + 3;
            val += tmp % 3;
            tmp /= 3;
            val += tmp % 4;
            tmp /= 4;
            val += tmp % 5;
            tmp /= 5;

            /* Save that value */
            sum += val;
            creature_ptr->stat_cur[3 * i] = creature_ptr->stat_max[3 * i] = val;

            /* Extract 5 + 1d3 + 1d4 + 1d5 */
            val = 5 + 3;
            val += tmp % 3;
            tmp /= 3;
            val += tmp % 4;
            tmp /= 4;
            val += tmp % 5;
            tmp /= 5;

            /* Save that value */
            sum += val;
            creature_ptr->stat_cur[3 * i + 1] = creature_ptr->stat_max[3 * i + 1] = val;

            /* Extract 5 + 1d3 + 1d4 + 1d5 */
            val = 5 + 3;
            val += tmp % 3;
            tmp /= 3;
            val += tmp % 4;
            tmp /= 4;
            val += (BASE_STATUS)tmp;

            /* Save that value */
            sum += val;
            creature_ptr->stat_cur[3 * i + 2] = creature_ptr->stat_max[3 * i + 2] = val;
        }

        /* Verify totals */
        if ((sum > 42 + 5 * 6) && (sum < 57 + 5 * 6))
            break;
        /* 57 was 54... I hate 'magic numbers' :< TY */
    }
}

/*!
 * @brief プレイヤーの限界ステータスを決める。
 * @return なし
 */
void get_max_stats(player_type* creature_ptr)
{
    int i, j;
    int dice[6];

    /* Roll and verify some stats */
    while (TRUE) {
        /* Roll some dice */
        for (j = i = 0; i < A_MAX; i++) {
            /* Roll the dice */
            dice[i] = randint1(7);

            /* Collect the maximum */
            j += dice[i];
        }

        /* Verify totals */
        if (j == 24)
            break;
    }

    /* Acquire the stats */
    for (i = 0; i < A_MAX; i++) {
        BASE_STATUS max_max = 18 + 60 + dice[i] * 10;

        /* Save that value */
        creature_ptr->stat_max_max[i] = max_max;
        if (creature_ptr->stat_max[i] > max_max)
            creature_ptr->stat_max[i] = max_max;
        if (creature_ptr->stat_cur[i] > max_max)
            creature_ptr->stat_cur[i] = max_max;
    }
    creature_ptr->knowledge &= ~(KNOW_STAT);
    creature_ptr->redraw |= (PR_STATS);
}

/*!
 * @brief その他「オートローラ中は算出の対象にしない」副次ステータスを処理する / Roll for some info that the auto-roller ignores
 * @return なし
 */
static void get_extra(player_type* creature_ptr, bool roll_hitdie)
{
    int i, j;

    /* Experience factor */
    if (creature_ptr->prace == RACE_ANDROID)
        creature_ptr->expfact = rp_ptr->r_exp;
    else
        creature_ptr->expfact = rp_ptr->r_exp + cp_ptr->c_exp;

    if (((creature_ptr->pclass == CLASS_MONK) ||
        (creature_ptr->pclass == CLASS_FORCETRAINER) ||
        (creature_ptr->pclass == CLASS_NINJA)) && ((creature_ptr->prace == RACE_KLACKON) ||
            (creature_ptr->prace == RACE_SPRITE)))
        creature_ptr->expfact -= 15;

    /* Reset record of race/realm changes */
    creature_ptr->start_race = creature_ptr->prace;
    creature_ptr->old_race1 = 0L;
    creature_ptr->old_race2 = 0L;
    creature_ptr->old_realm = 0;

    for (i = 0; i < 64; i++) {
        if (creature_ptr->pclass == CLASS_SORCERER)
            creature_ptr->spell_exp[i] = SPELL_EXP_MASTER;
        else if (creature_ptr->pclass == CLASS_RED_MAGE)
            creature_ptr->spell_exp[i] = SPELL_EXP_SKILLED;
        else
            creature_ptr->spell_exp[i] = SPELL_EXP_UNSKILLED;
    }

    for (i = 0; i < 5; i++)
        for (j = 0; j < 64; j++)
            creature_ptr->weapon_exp[i][j] = s_info[creature_ptr->pclass].w_start[i][j];
    if ((creature_ptr->pseikaku == SEIKAKU_SEXY) && (creature_ptr->weapon_exp[TV_HAFTED - TV_WEAPON_BEGIN][SV_WHIP] < WEAPON_EXP_BEGINNER)) {
        creature_ptr->weapon_exp[TV_HAFTED - TV_WEAPON_BEGIN][SV_WHIP] = WEAPON_EXP_BEGINNER;
    }

    for (i = 0; i < GINOU_MAX; i++)
        creature_ptr->skill_exp[i] = s_info[creature_ptr->pclass].s_start[i];

    /* Hitdice */
    if (creature_ptr->pclass == CLASS_SORCERER)
        creature_ptr->hitdie = rp_ptr->r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
    else
        creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;

    /* Roll for hit point unless quick-start */
    if (roll_hitdie)
        roll_hitdice(creature_ptr, SPOP_NO_UPDATE);

    /* Initial hitpoints */
    creature_ptr->mhp = creature_ptr->player_hp[0];
}

/*!
 * @brief プレイヤーの生い立ちの自動生成を行う。 / Get the racial history, and social class, using the "history charts".
 * @return なし
 */
static void get_history(player_type* creature_ptr)
{
    int i, n, chart, roll, social_class;

    char *s, *t;

    char buf[240];

    /* Clear the previous history strings */
    for (i = 0; i < 4; i++)
        creature_ptr->history[i][0] = '\0';

    /* Clear the history text */
    buf[0] = '\0';

    /* Initial social class */
    social_class = randint1(4);

    /* Starting place */
    switch (creature_ptr->prace) {
    case RACE_AMBERITE: {
        chart = 67;
        break;
    }
    case RACE_HUMAN:
    case RACE_BARBARIAN:
    case RACE_DUNADAN: {
        chart = 1;
        break;
    }
    case RACE_HALF_ELF: {
        chart = 4;
        break;
    }
    case RACE_ELF:
    case RACE_HIGH_ELF: {
        chart = 7;
        break;
    }
    case RACE_HOBBIT: {
        chart = 10;
        break;
    }
    case RACE_GNOME: {
        chart = 13;
        break;
    }
    case RACE_DWARF: {
        chart = 16;
        break;
    }
    case RACE_HALF_ORC: {
        chart = 19;
        break;
    }
    case RACE_HALF_TROLL: {
        chart = 22;
        break;
    }
    case RACE_DARK_ELF: {
        chart = 69;
        break;
    }
    case RACE_HALF_OGRE: {
        chart = 74;
        break;
    }
    case RACE_HALF_GIANT: {
        chart = 75;
        break;
    }
    case RACE_HALF_TITAN: {
        chart = 76;
        break;
    }
    case RACE_CYCLOPS: {
        chart = 77;
        break;
    }
    case RACE_YEEK: {
        chart = 78;
        break;
    }
    case RACE_KOBOLD: {
        chart = 82;
        break;
    }
    case RACE_KLACKON: {
        chart = 84;
        break;
    }
    case RACE_NIBELUNG: {
        chart = 87;
        break;
    }
    case RACE_DRACONIAN: {
        chart = 89;
        break;
    }
    case RACE_MIND_FLAYER: {
        chart = 92;
        break;
    }
    case RACE_IMP: {
        chart = 94;
        break;
    }
    case RACE_GOLEM: {
        chart = 98;
        break;
    }
    case RACE_SKELETON: {
        chart = 102;
        break;
    }
    case RACE_ZOMBIE: {
        chart = 107;
        break;
    }
    case RACE_VAMPIRE: {
        chart = 113;
        break;
    }
    case RACE_SPECTRE: {
        chart = 118;
        break;
    }
    case RACE_SPRITE: {
        chart = 124;
        break;
    }
    case RACE_BEASTMAN: {
        chart = 129;
        break;
    }
    case RACE_ENT: {
        chart = 137;
        break;
    }
    case RACE_ANGEL: {
        chart = 142;
        break;
    }
    case RACE_DEMON: {
        chart = 145;
        break;
    }
    case RACE_S_FAIRY: {
        chart = 148;
        break;
    }
    case RACE_KUTAR: {
        chart = 154;
        break;
    }
    case RACE_ANDROID: {
        chart = 155;
        break;
    }
    case RACE_MERFOLK: {
        chart = 170;
        break;
    }
    default: {
        chart = 0;
        break;
    }
    }

    /* Process the history */
    while (chart) {
        /* Start over */
        i = 0;

        /* Roll for nobility */
        roll = randint1(100);

        /* Access the proper entry in the table */
        while ((chart != bg[i].chart) || (roll > bg[i].roll)) {
            i++;
        }

        /* Acquire the textual history */
        (void)strcat(buf, bg[i].info);

        /* Add in the social class */
        social_class += (int)(bg[i].bonus) - 50;

        /* Enter the next chart */
        chart = bg[i].next;
    }

    /* Verify social class */
    if (social_class > 100)
        social_class = 100;
    else if (social_class < 1)
        social_class = 1;

    /* Save the social class */
    creature_ptr->sc = (s16b)social_class;

    /* Skip leading spaces */
    for (s = buf; *s == ' '; s++) /* loop */
        ;

    /* Get apparent length */
    n = strlen(s);

    /* Kill trailing spaces */

    while ((n > 0) && (s[n - 1] == ' '))
        s[--n] = '\0';

    {
        char temp[64 * 4];
        roff_to_buf(s, 60, temp, sizeof(temp));
        t = temp;
        for (i = 0; i < 4; i++) {
            if (t[0] == 0)
                break;
            else {
                strcpy(creature_ptr->history[i], t);
                t += strlen(t) + 1;
            }
        }
    }
}

/*!
 * @brief プレイヤーの身長体重を決める / Get character's height and weight
 * @return なし
 */
void get_height_weight(player_type* creature_ptr)
{
    int h_percent; /* 身長が平均にくらべてどのくらい違うか. */

    /* Calculate the height/weight for males */
    if (creature_ptr->psex == SEX_MALE) {
        creature_ptr->ht = randnor(rp_ptr->m_b_ht, rp_ptr->m_m_ht);
        h_percent = (int)(creature_ptr->ht) * 100 / (int)(rp_ptr->m_b_ht);
        creature_ptr->wt = randnor((int)(rp_ptr->m_b_wt) * h_percent / 100, (int)(rp_ptr->m_m_wt) * h_percent / 300);
    }

    /* Calculate the height/weight for females */
    else if (creature_ptr->psex == SEX_FEMALE) {
        creature_ptr->ht = randnor(rp_ptr->f_b_ht, rp_ptr->f_m_ht);
        h_percent = (int)(creature_ptr->ht) * 100 / (int)(rp_ptr->f_b_ht);
        creature_ptr->wt = randnor((int)(rp_ptr->f_b_wt) * h_percent / 100, (int)(rp_ptr->f_m_wt) * h_percent / 300);
    }
}

/*!
 * @brief プレイヤーの年齢を決める。 / Computes character's age, height, and weight by henkma
 * @details 内部でget_height_weight()も呼び出している。
 * @return なし
 */
static void get_ahw(player_type* creature_ptr)
{
    /* Get character's age */
    creature_ptr->age = rp_ptr->b_age + randint1(rp_ptr->m_age);

    /* Get character's height and weight */
    get_height_weight(creature_ptr);
}

/*!
 * @brief プレイヤーの初期所持金を決める。 / Get the player's starting money
 * @return なし
 */
static void get_money(player_type* creature_ptr)
{
    int i, gold;

    /* Social Class determines starting gold */
    gold = (creature_ptr->sc * 6) + randint1(100) + 300;
    if (creature_ptr->pclass == CLASS_TOURIST)
        gold += 2000;

    /* Process the stats */
    for (i = 0; i < A_MAX; i++) {
        /* Mega-Hack -- reduce gold for high stats */
        if (creature_ptr->stat_max[i] >= 18 + 50)
            gold -= 300;
        else if (creature_ptr->stat_max[i] >= 18 + 20)
            gold -= 200;
        else if (creature_ptr->stat_max[i] > 18)
            gold -= 150;
        else
            gold -= (creature_ptr->stat_max[i] - 8) * 10;
    }

    /* Minimum 100 gold */
    if (gold < 100)
        gold = 100;

    if (creature_ptr->pseikaku == SEIKAKU_NAMAKE)
        gold /= 2;
    else if (creature_ptr->pseikaku == SEIKAKU_MUNCHKIN)
        gold = 10000000;
    if (creature_ptr->prace == RACE_ANDROID)
        gold /= 5;

    /* Save the gold */
    creature_ptr->au = gold;
}

/*!
 * @brief put_stats()のサブルーチンとして、オートロール中のステータスを表示する / Display stat values, subset of "put_stats()"
 * @details See 'display_player(p_ptr, )' for screen layout constraints.
 * @return なし
 */
static void birth_put_stats(player_type* creature_ptr)
{
    int i, j, m, p;
    int col;
    TERM_COLOR attr;
    char buf[80];

    if (autoroller) {
        col = 42;
        /* Put the stats (and percents) */
        for (i = 0; i < A_MAX; i++) {
            /* Race/Class bonus */
            j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];

            /* Obtain the current stat */
            m = adjust_stat(creature_ptr->stat_max[i], j);

            /* Put the stat */
            cnv_stat(m, buf);
            c_put_str(TERM_L_GREEN, buf, 3 + i, col + 24);

            /* Put the percent */
            if (stat_match[i]) {
                if (stat_match[i] > 1000000L) {
                    /* Prevent overflow */
                    p = stat_match[i] / (auto_round / 1000L);
                } else {
                    p = 1000L * stat_match[i] / auto_round;
                }

                attr = (p < 100) ? TERM_YELLOW : TERM_L_GREEN;
                sprintf(buf, "%3d.%d%%", p / 10, p % 10);
                c_put_str(attr, buf, 3 + i, col + 13);
            }

            /* Never happened */
            else {
                c_put_str(TERM_RED, _("(なし)", "(NONE)"), 3 + i, col + 13);
            }
        }
    }
}

/*!
 * @brief ベースアイテム構造体の鑑定済みフラグをリセットする。
 * @return なし
 */
static void k_info_reset(void)
{
    int i;

    /* Reset the "objects" */
    for (i = 1; i < max_k_idx; i++) {
        object_kind* k_ptr = &k_info[i];

        /* Reset "tried" */
        k_ptr->tried = FALSE;

        /* Reset "aware" */
        k_ptr->aware = FALSE;
    }
}

/*!
 * @brief プレイヤー構造体の内容を初期値で消去する(名前を除く) / Clear all the global "character" data (without name)
 * @return なし
 */
static void player_wipe_without_name(player_type* creature_ptr)
{
    int i;
    player_type tmp;

    /* Temporary copy for migration - written back later */
    COPY(&tmp, creature_ptr, player_type);

    /* Hack -- free the "last message" string */
    if (creature_ptr->last_message)
        string_free(creature_ptr->last_message);

    if (creature_ptr->inventory_list != NULL)
        C_WIPE(creature_ptr->inventory_list, INVEN_TOTAL, object_type);

    /* Hack -- zero the struct */
    (void)WIPE(creature_ptr, player_type);

    //TODO: キャラ作成からゲーム開始までに  current_floor_ptr を参照しなければならない処理は今後整理して外す。
    creature_ptr->current_floor_ptr = &floor_info;

    C_MAKE(creature_ptr->inventory_list, INVEN_TOTAL, object_type);

    /* Wipe the history */
    for (i = 0; i < 4; i++) {
        strcpy(creature_ptr->history[i], "");
    }

    /* Wipe the quests */
    for (i = 0; i < max_q_idx; i++) {
        quest_type* const q_ptr = &quest[i];

        q_ptr->status = QUEST_STATUS_UNTAKEN;

        q_ptr->cur_num = 0;
        q_ptr->max_num = 0;
        q_ptr->type = 0;
        q_ptr->level = 0;
        q_ptr->r_idx = 0;
        q_ptr->complev = 0;
        q_ptr->comptime = 0;
    }

    /* No weight */
    creature_ptr->total_weight = 0;

    /* No items */
    creature_ptr->inven_cnt = 0;
    creature_ptr->equip_cnt = 0;

    /* Clear the inventory */
    for (i = 0; i < INVEN_TOTAL; i++) {
        object_wipe(&creature_ptr->inventory_list[i]);
    }

    /* Start with no artifacts made yet */
    for (i = 0; i < max_a_idx; i++) {
        artifact_type* a_ptr = &a_info[i];
        a_ptr->cur_num = 0;
    }

    /* Reset the objects */
    k_info_reset();

    /* Reset the "monsters" */
    for (i = 1; i < max_r_idx; i++) {
        monster_race* r_ptr = &r_info[i];

        /* Hack -- Reset the counter */
        r_ptr->cur_num = 0;

        /* Hack -- Reset the max counter */
        r_ptr->max_num = 100;

        /* Hack -- Reset the max counter */
        if (r_ptr->flags1 & RF1_UNIQUE)
            r_ptr->max_num = 1;

        /* Hack -- Non-unique Nazguls are semi-unique */
        else if (r_ptr->flags7 & RF7_NAZGUL)
            r_ptr->max_num = MAX_NAZGUL_NUM;

        /* Clear visible kills in this life */
        r_ptr->r_pkills = 0;

        /* Clear all kills in this life */
        r_ptr->r_akills = 0;
    }

    /* Hack -- Well fed player */
    creature_ptr->food = PY_FOOD_FULL - 1;

    /* Wipe the spells */
    if (creature_ptr->pclass == CLASS_SORCERER) {
        creature_ptr->spell_learned1 = creature_ptr->spell_learned2 = 0xffffffffL;
        creature_ptr->spell_worked1 = creature_ptr->spell_worked2 = 0xffffffffL;
    } else {
        creature_ptr->spell_learned1 = creature_ptr->spell_learned2 = 0L;
        creature_ptr->spell_worked1 = creature_ptr->spell_worked2 = 0L;
    }
    creature_ptr->spell_forgotten1 = creature_ptr->spell_forgotten2 = 0L;
    for (i = 0; i < 64; i++)
        creature_ptr->spell_order[i] = 99;
    creature_ptr->learned_spells = 0;
    creature_ptr->add_spells = 0;
    creature_ptr->knowledge = 0;

    /* Clean the mutation count */
    creature_ptr->mutant_regenerate_mod = 100;

    /* Clear "cheat" options */
    cheat_peek = FALSE;
    cheat_hear = FALSE;
    cheat_room = FALSE;
    cheat_xtra = FALSE;
    cheat_know = FALSE;
    cheat_live = FALSE;
    cheat_save = FALSE;
    cheat_diary_output = FALSE;
    cheat_turn = FALSE;

    /* Assume no winning game */
    current_world_ptr->total_winner = FALSE;

    creature_ptr->timewalk = FALSE;

    /* Assume no panic save */
    creature_ptr->panic_save = 0;

    /* Assume no cheating */
    current_world_ptr->noscore = 0;
    current_world_ptr->wizard = FALSE;

    /* Not waiting to report score */
    creature_ptr->wait_report_score = FALSE;

    /* Default pet command settings */
    creature_ptr->pet_follow_distance = PET_FOLLOW_DIST;
    creature_ptr->pet_extra_flags = (PF_TELEPORT | PF_ATTACK_SPELL | PF_SUMMON_SPELL);

    /* Wipe the recall depths */
    for (i = 0; i < current_world_ptr->max_d_idx; i++) {
        max_dlv[i] = 0;
    }

    creature_ptr->visit = 1;

    /* Reset wild_mode to FALSE */
    creature_ptr->wild_mode = FALSE;

    for (i = 0; i < 108; i++) {
        creature_ptr->magic_num1[i] = 0;
        creature_ptr->magic_num2[i] = 0;
    }

    /* Level one */
    creature_ptr->max_plv = creature_ptr->lev = 1;

    /* Initialize arena and rewards information -KMW- */
    creature_ptr->arena_number = 0;
    creature_ptr->current_floor_ptr->inside_arena = FALSE;
    creature_ptr->current_floor_ptr->inside_quest = 0;
    for (i = 0; i < MAX_MANE; i++) {
        creature_ptr->mane_spell[i] = -1;
        creature_ptr->mane_dam[i] = 0;
    }

    creature_ptr->mane_num = 0;
    creature_ptr->exit_bldg = TRUE; /* only used for arena now -KMW- */

    /* Bounty */
    creature_ptr->today_mon = 0;

    /* Reset monster arena */
    update_gambling_monsters(creature_ptr);

    /* Reset mutations */
    creature_ptr->muta1 = 0;
    creature_ptr->muta2 = 0;
    creature_ptr->muta3 = 0;

    /* Reset virtues */
    for (i = 0; i < 8; i++)
        creature_ptr->virtues[i] = 0;

    creature_ptr->dungeon_idx = 0;

    /* Set the recall dungeon accordingly */
    if (vanilla_town || ironman_downward) {
        creature_ptr->recall_dungeon = DUNGEON_ANGBAND;
    } else {
        creature_ptr->recall_dungeon = DUNGEON_GALGALS;
    }

    /* Data migration */
    memcpy(creature_ptr->name, tmp.name, sizeof(tmp.name));
}

/*!
 * @brief ダンジョン内部のクエストを初期化する / Initialize random quests and final quests
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void init_dungeon_quests(player_type* creature_ptr)
{
    int number_of_quests = MAX_RANDOM_QUEST - MIN_RANDOM_QUEST + 1;
    int i;

    /* Init the random quests */
    init_flags = INIT_ASSIGN;
    floor_type* floor_ptr = creature_ptr->current_floor_ptr;
    floor_ptr->inside_quest = MIN_RANDOM_QUEST;

    process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);

    floor_ptr->inside_quest = 0;

    /* Generate quests */
    for (i = MIN_RANDOM_QUEST + number_of_quests - 1; i >= MIN_RANDOM_QUEST; i--) {
        quest_type* q_ptr = &quest[i];
        monster_race* quest_r_ptr;

        q_ptr->status = QUEST_STATUS_TAKEN;
        determine_random_questor(creature_ptr, q_ptr);

        /* Mark uniques */
        quest_r_ptr = &r_info[q_ptr->r_idx];
        quest_r_ptr->flags1 |= RF1_QUESTOR;

        q_ptr->max_num = 1;
    }

    /* Init the two main quests (Oberon + Serpent) */
    init_flags = INIT_ASSIGN;
    floor_ptr->inside_quest = QUEST_OBERON;

    process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);

    quest[QUEST_OBERON].status = QUEST_STATUS_TAKEN;

    floor_ptr->inside_quest = QUEST_SERPENT;

    process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);

    quest[QUEST_SERPENT].status = QUEST_STATUS_TAKEN;
    floor_ptr->inside_quest = 0;
}

/*!
 * @brief ゲームターンを初期化する / Reset turn
 * @details アンデッド系種族は開始時刻を夜からにする。
 * @return なし
 */
static void init_turn(player_type* creature_ptr)
{
    if ((creature_ptr->prace == RACE_VAMPIRE) || (creature_ptr->prace == RACE_SKELETON) || (creature_ptr->prace == RACE_ZOMBIE) || (creature_ptr->prace == RACE_SPECTRE)) {
        /* Undead start just after midnight */
        current_world_ptr->game_turn = (TURNS_PER_TICK * 3 * TOWN_DAWN) / 4 + 1;
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    } else {
        current_world_ptr->game_turn = 1;
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    }

    current_world_ptr->dungeon_turn = 1;
    current_world_ptr->dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
}

/*!
 * @brief 所持状態にあるアイテムの中から一部枠の装備可能なものを装備させる。
 * @return なし
 */
static void wield_all(player_type* creature_ptr)
{
    object_type* o_ptr;
    object_type* i_ptr;
    object_type object_type_body;

    int slot;
    INVENTORY_IDX item;

    /* Scan through the slots backwards */
    for (item = INVEN_PACK - 1; item >= 0; item--) {
        o_ptr = &creature_ptr->inventory_list[item];

        /* Skip non-objects */
        if (!o_ptr->k_idx)
            continue;

        /* Make sure we can wield it and that there's nothing else in that slot */
        slot = wield_slot(creature_ptr, o_ptr);
        if (slot < INVEN_RARM)
            continue;
        if (slot == INVEN_LITE)
            continue; /* Does not wield toaches because buys a lantern soon */
        if (creature_ptr->inventory_list[slot].k_idx)
            continue;

        i_ptr = &object_type_body;
        object_copy(i_ptr, o_ptr);
        i_ptr->number = 1;

        /* Decrease the item (from the pack) */
        if (item >= 0) {
            inven_item_increase(creature_ptr, item, -1);
            inven_item_optimize(creature_ptr, item);
        }

        /* Decrease the item (from the floor) */
        else {
            floor_item_increase(creature_ptr->current_floor_ptr, 0 - item, -1);
            floor_item_optimize(creature_ptr, 0 - item);
        }

        o_ptr = &creature_ptr->inventory_list[slot];
        object_copy(o_ptr, i_ptr);
        creature_ptr->total_weight += i_ptr->weight;

        /* Increment the equip counter by hand */
        creature_ptr->equip_cnt++;
    }
    return;
}

/*!
 * プレイヤーの職業毎の初期装備テーブル。/\n
 * Each player starts out with a few items, given as tval/sval pairs.\n
 * In addition, he always has some food and a few torches.\n
 */
static byte player_init[MAX_CLASS][3][2] = {
    { /* Warrior */
        { TV_RING, SV_RING_RES_FEAR }, /* Warriors need it! */
        { TV_HARD_ARMOR, SV_CHAIN_MAIL },
        { TV_SWORD, SV_BROAD_SWORD } },

    { /* Mage */
        { TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
        { TV_DEATH_BOOK, 0 }, /* Hack: for realm2 book */
        { TV_SWORD, SV_DAGGER } },

    { /* Priest */
        { TV_SORCERY_BOOK, 0 }, /* Hack: for Life / Death book */
        { TV_DEATH_BOOK, 0 }, /* Hack: for realm2 book */
        { TV_HAFTED, SV_MACE } },

    { /* Rogue */
        { TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_DAGGER } },

    { /* Ranger */
        { TV_NATURE_BOOK, 0 },
        { TV_DEATH_BOOK, 0 }, /* Hack: for realm2 book */
        { TV_SWORD, SV_DAGGER } },

    { /* Paladin */
        { TV_SORCERY_BOOK, 0 },
        { TV_SCROLL, SV_SCROLL_PROTECTION_FROM_EVIL },
        { TV_SWORD, SV_BROAD_SWORD } },

    { /* Warrior-Mage */
        { TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
        { TV_DEATH_BOOK, 0 }, /* Hack: for realm2 book */
        { TV_SWORD, SV_SHORT_SWORD } },

    { /* Chaos Warrior */
        { TV_SORCERY_BOOK, 0 }, /* Hack: For realm1 book */
        { TV_HARD_ARMOR, SV_METAL_SCALE_MAIL },
        { TV_SWORD, SV_BROAD_SWORD } },

    { /* Monk */
        { TV_SORCERY_BOOK, 0 },
        { TV_POTION, SV_POTION_SPEED },
        { TV_POTION, SV_POTION_HEROISM } },

    { /* Mindcrafter */
        { TV_POTION, SV_POTION_SPEED },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_SMALL_SWORD } },

    { /* High Mage */
        { TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
        { TV_RING, SV_RING_SUSTAIN_INT },
        { TV_SWORD, SV_DAGGER } },

    { /* Tourist */
        { TV_FOOD, SV_FOOD_JERKY },
        { TV_SCROLL, SV_SCROLL_MAPPING },
        { TV_BOW, SV_SLING } },

    { /* Imitator */
        { TV_POTION, SV_POTION_SPEED },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_SHORT_SWORD } },

    { /* Beastmaster */
        { TV_TRUMP_BOOK, 0 },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_POLEARM, SV_SPEAR } },

    { /* Sorcerer */
        { TV_HAFTED, SV_WIZSTAFF }, /* Hack: for realm1 book */
        { TV_RING, SV_RING_SUSTAIN_INT },
        { TV_WAND, SV_WAND_MAGIC_MISSILE } },

    {
        /* Archer */
        { TV_BOW, SV_SHORT_BOW },
        { TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
        { TV_SWORD, SV_SHORT_SWORD },
    },

    {
        /* Magic eater */
        { TV_WAND, SV_WAND_MAGIC_MISSILE },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_SHORT_SWORD },
    },

    {
        /* Bard */
        { TV_MUSIC_BOOK, 0 },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_SHORT_SWORD },
    },

    {
        /* Red Mage */
        { TV_ARCANE_BOOK, 0 },
        { TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
        { TV_SWORD, SV_SHORT_SWORD },
    },

    { /* Samurai */
        { TV_HISSATSU_BOOK, 0 },
        { TV_HARD_ARMOR, SV_CHAIN_MAIL },
        { TV_SWORD, SV_BROAD_SWORD } },

    { /* ForceTrainer */
        { TV_SORCERY_BOOK, 0 },
        { TV_POTION, SV_POTION_SPEED },
        { TV_POTION, SV_POTION_RESTORE_MANA } },

    { /* Blue Mage */
        { TV_SOFT_ARMOR, SV_ROBE },
        { TV_WAND, SV_WAND_MAGIC_MISSILE },
        { TV_SWORD, SV_DAGGER } },

    { /* Cavalry */
        { TV_BOW, SV_SHORT_BOW },
        { TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
        { TV_POLEARM, SV_BROAD_SPEAR } },

    { /* Berserker */
        { TV_POTION, SV_POTION_HEALING },
        { TV_HARD_ARMOR, SV_AUGMENTED_CHAIN_MAIL },
        { TV_POLEARM, SV_BROAD_AXE } },

    { /* Weaponsmith */
        { TV_RING, SV_RING_RES_FEAR }, /* Warriors need it! */
        { TV_HARD_ARMOR, SV_CHAIN_MAIL },
        { TV_POLEARM, SV_BROAD_AXE } },
    { /* Mirror-Master */
        { TV_POTION, SV_POTION_SPEED },
        { TV_RING, SV_RING_SUSTAIN_INT },
        { TV_SWORD, SV_DAGGER } },
    { /* Ninja */
        { TV_POTION, SV_POTION_SPEED },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_DAGGER } },
    { /* Sniper */
        { TV_BOW, SV_LIGHT_XBOW },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_DAGGER } },
};

/*!
 * @brief 初期所持アイテムの処理 / Add an outfit object
 * @details アイテムを既知のものとした上でwield_all()関数により装備させる。
 * @param o_ptr 処理したいオブジェクト構造体の参照ポインタ
 * @return なし
 */
static void add_outfit(player_type* creature_ptr, object_type* o_ptr)
{
    s16b slot;

    object_aware(creature_ptr, o_ptr);
    object_known(o_ptr);
    slot = inven_carry(creature_ptr, o_ptr);

    /* Auto-inscription */
    autopick_alter_item(creature_ptr, slot, FALSE);

    /* Now try wielding everything */
    wield_all(creature_ptr);
}

/*!
 * @brief 種族/職業/性格などに基づき初期所持アイテムを設定するメインセット関数。 / Init players with some belongings
 * @details Having an item makes the player "aware" of its purpose.
 * @return なし
 */
void player_outfit(player_type* creature_ptr)
{
    int i;
    OBJECT_TYPE_VALUE tv;
    OBJECT_SUBTYPE_VALUE sv;

    object_type forge;
    object_type* q_ptr;

    q_ptr = &forge;

    /* Give the player some food */
    switch (creature_ptr->prace) {
    case RACE_VAMPIRE:
        /* Nothing! */
        /* Vampires can drain blood of creatures */
        break;

    case RACE_DEMON:
        /* Demon can drain vitality from humanoid corpse */
        get_mon_num_prep(creature_ptr, monster_hook_human, NULL);

        for (i = rand_range(3, 4); i > 0; i--) {
            object_prep(q_ptr, lookup_kind(TV_CORPSE, SV_CORPSE));
            q_ptr->pval = get_mon_num(creature_ptr, 2, 0);
            if (q_ptr->pval) {
                q_ptr->number = 1;
                add_outfit(creature_ptr, q_ptr);
            }
        }
        break;

    case RACE_SKELETON:
    case RACE_GOLEM:
    case RACE_ZOMBIE:
    case RACE_SPECTRE:
        /* Staff (of Nothing) */
        object_prep(q_ptr, lookup_kind(TV_STAFF, SV_STAFF_NOTHING));
        q_ptr->number = 1;

        add_outfit(creature_ptr, q_ptr);
        break;

    case RACE_ENT:
        /* Potions of Water */
        object_prep(q_ptr, lookup_kind(TV_POTION, SV_POTION_WATER));
        q_ptr->number = (ITEM_NUMBER)rand_range(15, 23);
        add_outfit(creature_ptr, q_ptr);

        break;

    case RACE_ANDROID:
        /* Flasks of oil */
        object_prep(q_ptr, lookup_kind(TV_FLASK, SV_ANY));

        /* Fuel with oil (move pval to xtra4) */
        apply_magic(creature_ptr, q_ptr, 1, AM_NO_FIXED_ART);

        q_ptr->number = (ITEM_NUMBER)rand_range(7, 12);
        add_outfit(creature_ptr, q_ptr);

        break;

    default:
        /* Food rations */
        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_RATION));
        q_ptr->number = (ITEM_NUMBER)rand_range(3, 7);

        add_outfit(creature_ptr, q_ptr);
    }
    q_ptr = &forge;

    if ((creature_ptr->prace == RACE_VAMPIRE) && (creature_ptr->pclass != CLASS_NINJA)) {
        /* Hack -- Give the player scrolls of DARKNESS! */
        object_prep(q_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_DARKNESS));

        q_ptr->number = (ITEM_NUMBER)rand_range(2, 5);

        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass != CLASS_NINJA) {
        /* Hack -- Give the player some torches */
        object_prep(q_ptr, lookup_kind(TV_LITE, SV_LITE_TORCH));
        q_ptr->number = (ITEM_NUMBER)rand_range(3, 7);
        q_ptr->xtra4 = rand_range(3, 7) * 500;

        add_outfit(creature_ptr, q_ptr);
    }
    q_ptr = &forge;

    if (creature_ptr->prace == RACE_MERFOLK) {
        object_prep(q_ptr, lookup_kind(TV_RING, SV_RING_LEVITATION_FALL));
        q_ptr->number = 1;
        add_outfit(creature_ptr, q_ptr);
    }

    if ((creature_ptr->pclass == CLASS_RANGER) || (creature_ptr->pclass == CLASS_CAVALRY)) {
        /* Hack -- Give the player some arrows */
        object_prep(q_ptr, lookup_kind(TV_ARROW, SV_AMMO_NORMAL));
        q_ptr->number = (byte)rand_range(15, 20);

        add_outfit(creature_ptr, q_ptr);
    }
    if (creature_ptr->pclass == CLASS_RANGER) {
        /* Hack -- Give the player some arrows */
        object_prep(q_ptr, lookup_kind(TV_BOW, SV_SHORT_BOW));

        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_ARCHER) {
        /* Hack -- Give the player some arrows */
        object_prep(q_ptr, lookup_kind(TV_ARROW, SV_AMMO_NORMAL));
        q_ptr->number = (ITEM_NUMBER)rand_range(15, 20);

        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_HIGH_MAGE) {
        /* Hack -- Give the player some arrows */
        object_prep(q_ptr, lookup_kind(TV_WAND, SV_WAND_MAGIC_MISSILE));
        q_ptr->number = 1;
        q_ptr->pval = (PARAMETER_VALUE)rand_range(25, 30);

        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_SORCERER) {
        OBJECT_TYPE_VALUE book_tval;
        for (book_tval = TV_LIFE_BOOK; book_tval <= TV_LIFE_BOOK + MAX_MAGIC - 1; book_tval++) {
            /* Hack -- Give the player some arrows */
            object_prep(q_ptr, lookup_kind(book_tval, 0));
            q_ptr->number = 1;

            add_outfit(creature_ptr, q_ptr);
        }
    } else if (creature_ptr->pclass == CLASS_TOURIST) {
        if (creature_ptr->pseikaku != SEIKAKU_SEXY) {
            /* Hack -- Give the player some arrows */
            object_prep(q_ptr, lookup_kind(TV_SHOT, SV_AMMO_LIGHT));
            q_ptr->number = rand_range(15, 20);

            add_outfit(creature_ptr, q_ptr);
        }

        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_BISCUIT));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);

        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_WAYBREAD));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);

        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_JERKY));
        q_ptr->number = rand_range(1, 3);

        add_outfit(creature_ptr, q_ptr);

        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_ALE));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);

        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_WINE));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_NINJA) {
        /* Hack -- Give the player some arrows */
        object_prep(q_ptr, lookup_kind(TV_SPIKE, 0));
        q_ptr->number = rand_range(15, 20);

        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_SNIPER) {
        /* Hack -- Give the player some bolts */
        object_prep(q_ptr, lookup_kind(TV_BOLT, SV_AMMO_NORMAL));
        q_ptr->number = rand_range(15, 20);

        add_outfit(creature_ptr, q_ptr);
    }

    if (creature_ptr->pseikaku == SEIKAKU_SEXY) {
        player_init[creature_ptr->pclass][2][0] = TV_HAFTED;
        player_init[creature_ptr->pclass][2][1] = SV_WHIP;
    }

    /* Hack -- Give the player three useful objects */
    for (i = 0; i < 3; i++) {
        /* Look up standard equipment */
        tv = player_init[creature_ptr->pclass][i][0];
        sv = player_init[creature_ptr->pclass][i][1];

        if ((creature_ptr->prace == RACE_ANDROID) && ((tv == TV_SOFT_ARMOR) || (tv == TV_HARD_ARMOR)))
            continue;
        /* Hack to initialize spellbooks */
        if (tv == TV_SORCERY_BOOK)
            tv = TV_LIFE_BOOK + creature_ptr->realm1 - 1;
        else if (tv == TV_DEATH_BOOK)
            tv = TV_LIFE_BOOK + creature_ptr->realm2 - 1;

        else if (tv == TV_RING && sv == SV_RING_RES_FEAR && creature_ptr->prace == RACE_BARBARIAN)
            /* Barbarians do not need a ring of resist fear */
            sv = SV_RING_SUSTAIN_STR;

        else if (tv == TV_RING && sv == SV_RING_SUSTAIN_INT && creature_ptr->prace == RACE_MIND_FLAYER) {
            tv = TV_POTION;
            sv = SV_POTION_RESTORE_MANA;
        }
        q_ptr = &forge;

        /* Hack -- Give the player an object */
        object_prep(q_ptr, lookup_kind(tv, sv));

        /* Assassins begin the game with a poisoned dagger */
        if ((tv == TV_SWORD || tv == TV_HAFTED) && (creature_ptr->pclass == CLASS_ROGUE && creature_ptr->realm1 == REALM_DEATH)) /* Only assassins get a poisoned weapon */
        {
            q_ptr->name2 = EGO_BRAND_POIS;
        }

        add_outfit(creature_ptr, q_ptr);
    }

    /* Hack -- make aware of the water */
    k_info[lookup_kind(TV_POTION, SV_POTION_WATER)].aware = TRUE;
}

/*!
 * @brief プレイヤーの種族選択を行う / Player race
 * @return なし
 */
static bool get_player_race(player_type* creature_ptr)
{
    int k, n, cs, os;
    concptr str;
    char c;
    char sym[MAX_RACES];
    char p2 = ')';
    char buf[80], cur[80];

    /* Extra info */
    clear_from(10);
    put_str(_("注意：《種族》によってキャラクターの先天的な資質やボーナスが変化します。",
                "Note: Your 'race' determines various intrinsic factors and bonuses."),
        23, 5);

    /* Dump races */
    for (n = 0; n < MAX_RACES; n++) {
        /* Analyze */
        rp_ptr = &race_info[n];
        str = rp_ptr->title;

        /* Display */
        if (n < 26)
            sym[n] = I2A(n);
        else
            sym[n] = ('A' + n - 26);
        sprintf(buf, "%c%c%s", sym[n], p2, str);
        put_str(buf, 12 + (n / 5), 1 + 16 * (n % 5));
    }

    sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));

    /* Choose */
    k = -1;
    cs = creature_ptr->prace;
    os = MAX_RACES;
    while (TRUE) {
        /* Move Cursol */
        if (cs != os) {
            c_put_str(TERM_WHITE, cur, 12 + (os / 5), 1 + 16 * (os % 5));
            put_str("                                   ", 3, 40);
            if (cs == MAX_RACES) {
                sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
                put_str("                                   ", 4, 40);
                put_str("                                   ", 5, 40);
            } else {
                rp_ptr = &race_info[cs];
                str = rp_ptr->title;
                sprintf(cur, "%c%c%s", sym[cs], p2, str);
                c_put_str(TERM_L_BLUE, rp_ptr->title, 3, 40);
                put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
                put_str(_("の種族修正", ": Race modification"), 3, 40 + strlen(rp_ptr->title));

                sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ",
                    rp_ptr->r_adj[0], rp_ptr->r_adj[1], rp_ptr->r_adj[2], rp_ptr->r_adj[3],
                    rp_ptr->r_adj[4], rp_ptr->r_adj[5], (rp_ptr->r_exp - 100));
                c_put_str(TERM_L_BLUE, buf, 5, 40);
            }
            c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 1 + 16 * (cs % 5));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("種族を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a race (%c-%c) ('=' for options): "), sym[0], sym[MAX_RACES - 1]);

        put_str(buf, 10, 10);
        c = inkey();
        if (c == 'Q')
            birth_quit();
        if (c == 'S')
            return FALSE;
        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_RACES) {
                k = randint0(MAX_RACES);
                cs = k;
                continue;
            } else {
                k = cs;
                break;
            }
        }
        if (c == '*') {
            k = randint0(MAX_RACES);
            cs = k;
            continue;
        }
        if (c == '8') {
            if (cs >= 5)
                cs -= 5;
        }
        if (c == '4') {
            if (cs > 0)
                cs--;
        }
        if (c == '6') {
            if (cs < MAX_RACES)
                cs++;
        }
        if (c == '2') {
            if ((cs + 5) <= MAX_RACES)
                cs += 5;
        }
        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_RACES)) {
            cs = k;
            continue;
        }
        k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((k >= 26) && (k < MAX_RACES)) {
            cs = k;
            continue;
        } else
            k = -1;
        if (c == '?') {
#ifdef JP
            show_help(creature_ptr, "jraceclas.txt#TheRaces");
#else
            show_help(creature_ptr, "raceclas.txt#TheRaces");
#endif
        } else if (c == '=') {
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
        } else if (c != '2' && c != '4' && c != '6' && c != '8')
            bell();
    }

    /* Set race */
    creature_ptr->prace = (byte)k;

    rp_ptr = &race_info[creature_ptr->prace];

    /* Display */
    c_put_str(TERM_L_BLUE, rp_ptr->title, 4, 15);

    /* Success */
    return TRUE;
}

/*!
 * @brief プレイヤーの職業選択を行う / Player class
 * @return なし
 */
static bool get_player_class(player_type* creature_ptr)
{
    int k, n, cs, os;
    char c;
    char sym[MAX_CLASS_CHOICE];
    char p2 = ')';
    char buf[80], cur[80];
    concptr str;

    /* Extra info */
    clear_from(10);
    put_str(_("注意：《職業》によってキャラクターの先天的な能力やボーナスが変化します。",
                "Note: Your 'class' determines various intrinsic abilities and bonuses."),
        23, 5);

    put_str(_("()で囲まれた選択肢はこの種族には似合わない職業です。",
                "Any entries in parentheses should only be used by advanced players."),
        11, 5);

    /* Dump classes */
    for (n = 0; n < MAX_CLASS_CHOICE; n++) {
        /* Analyze */
        cp_ptr = &class_info[n];
        mp_ptr = &m_info[n];
        str = cp_ptr->title;
        if (n < 26)
            sym[n] = I2A(n);
        else
            sym[n] = ('A' + n - 26);

        /* Display */
        if (!(rp_ptr->choice & (1L << n)))
            sprintf(buf, "%c%c(%s)", sym[n], p2, str);
        else
            sprintf(buf, "%c%c%s", sym[n], p2, str);

        put_str(buf, 13 + (n / 4), 2 + 19 * (n % 4));
    }

    sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));

    /* Get a class */
    k = -1;
    cs = creature_ptr->pclass;
    os = MAX_CLASS_CHOICE;
    while (TRUE) {
        /* Move Cursol */
        if (cs != os) {
            c_put_str(TERM_WHITE, cur, 13 + (os / 4), 2 + 19 * (os % 4));
            put_str("                                   ", 3, 40);
            if (cs == MAX_CLASS_CHOICE) {
                sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
                put_str("                                   ", 4, 40);
                put_str("                                   ", 5, 40);
            } else {
                cp_ptr = &class_info[cs];
                mp_ptr = &m_info[cs];
                str = cp_ptr->title;
                if (!(rp_ptr->choice & (1L << cs)))
                    sprintf(cur, "%c%c(%s)", sym[cs], p2, str);
                else
                    sprintf(cur, "%c%c%s", sym[cs], p2, str);

                c_put_str(TERM_L_BLUE, cp_ptr->title, 3, 40);
                put_str(_("の職業修正", ": Class modification"), 3, 40 + strlen(cp_ptr->title));
                put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
                sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ",
                    cp_ptr->c_adj[0], cp_ptr->c_adj[1], cp_ptr->c_adj[2], cp_ptr->c_adj[3],
                    cp_ptr->c_adj[4], cp_ptr->c_adj[5], cp_ptr->c_exp);
                c_put_str(TERM_L_BLUE, buf, 5, 40);
            }
            c_put_str(TERM_YELLOW, cur, 13 + (cs / 4), 2 + 19 * (cs % 4));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("職業を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a class (%c-%c) ('=' for options): "), sym[0], sym[MAX_CLASS_CHOICE - 1]);

        put_str(buf, 10, 10);
        c = inkey();
        if (c == 'Q')
            birth_quit();
        if (c == 'S')
            return FALSE;
        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_CLASS_CHOICE) {
                k = randint0(MAX_CLASS_CHOICE);
                cs = k;
                continue;
            } else {
                k = cs;
                break;
            }
        }
        if (c == '*') {
            k = randint0(MAX_CLASS_CHOICE);
            cs = k;
            continue;
        }
        if (c == '8') {
            if (cs >= 4)
                cs -= 4;
        }
        if (c == '4') {
            if (cs > 0)
                cs--;
        }
        if (c == '6') {
            if (cs < MAX_CLASS_CHOICE)
                cs++;
        }
        if (c == '2') {
            if ((cs + 4) <= MAX_CLASS_CHOICE)
                cs += 4;
        }
        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_CLASS_CHOICE)) {
            cs = k;
            continue;
        }
        k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((k >= 26) && (k < MAX_CLASS_CHOICE)) {
            cs = k;
            continue;
        } else
            k = -1;
        if (c == '?') {
#ifdef JP
            show_help(creature_ptr, "jraceclas.txt#TheClasses");
#else
            show_help(creature_ptr, "raceclas.txt#TheClasses");
#endif
        } else if (c == '=') {
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
        } else if (c != '2' && c != '4' && c != '6' && c != '8')
            bell();
    }

    /* Set class */
    creature_ptr->pclass = (byte)k;
    cp_ptr = &class_info[creature_ptr->pclass];
    mp_ptr = &m_info[creature_ptr->pclass];

    /* Display */
    c_put_str(TERM_L_BLUE, cp_ptr->title, 5, 15);

    return TRUE;
}

/*!
 * @brief プレイヤーの性格選択を行う / Player Player seikaku
 * @return なし
 */
static bool get_player_seikaku(player_type* creature_ptr)
{
    int k;
    int n, os, cs;
    char c;
    char sym[MAX_SEIKAKU];
    char p2 = ')';
    char buf[80], cur[80];
    char tmp[64];
    concptr str;

    /* Extra info */
    clear_from(10);
    put_str(_("注意：《性格》によってキャラクターの能力やボーナスが変化します。", "Note: Your personality determines various intrinsic abilities and bonuses."), 23, 5);

    /* Dump seikakus */
    for (n = 0; n < MAX_SEIKAKU; n++) {
        if (seikaku_info[n].sex && (seikaku_info[n].sex != (creature_ptr->psex + 1)))
            continue;

        /* Analyze */
        ap_ptr = &seikaku_info[n];
        str = ap_ptr->title;
        if (n < 26)
            sym[n] = I2A(n);
        else
            sym[n] = ('A' + n - 26);

        /* Display */
        sprintf(buf, "%c%c%s", I2A(n), p2, str);
        put_str(buf, 12 + (n / 4), 2 + 18 * (n % 4));
    }

    sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));

    /* Get a seikaku */
    k = -1;
    cs = creature_ptr->pseikaku;
    os = MAX_SEIKAKU;
    while (TRUE) {
        /* Move Cursol */
        if (cs != os) {
            c_put_str(TERM_WHITE, cur, 12 + (os / 4), 2 + 18 * (os % 4));
            put_str("                                   ", 3, 40);
            if (cs == MAX_SEIKAKU) {
                sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
                put_str("                                   ", 4, 40);
                put_str("                                   ", 5, 40);
            } else {
                ap_ptr = &seikaku_info[cs];
                str = ap_ptr->title;
                sprintf(cur, "%c%c%s", sym[cs], p2, str);
                c_put_str(TERM_L_BLUE, ap_ptr->title, 3, 40);
                put_str(_("の性格修正", ": Personality modification"), 3, 40 + strlen(ap_ptr->title));
                put_str(_("腕力 知能 賢さ 器用 耐久 魅力      ", "Str  Int  Wis  Dex  Con  Chr       "), 4, 40);
                sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d       ",
                    ap_ptr->a_adj[0], ap_ptr->a_adj[1], ap_ptr->a_adj[2], ap_ptr->a_adj[3],
                    ap_ptr->a_adj[4], ap_ptr->a_adj[5]);
                c_put_str(TERM_L_BLUE, buf, 5, 40);
            }
            c_put_str(TERM_YELLOW, cur, 12 + (cs / 4), 2 + 18 * (cs % 4));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("性格を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a personality (%c-%c) ('=' for options): "), sym[0], sym[MAX_SEIKAKU - 1]);

        put_str(buf, 10, 10);
        c = inkey();
        if (c == 'Q')
            birth_quit();
        if (c == 'S')
            return FALSE;
        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_SEIKAKU) {
                do {
                    k = randint0(MAX_SEIKAKU);
                } while (seikaku_info[k].sex && (seikaku_info[k].sex != (creature_ptr->psex + 1)));
                cs = k;
                continue;
            } else {
                k = cs;
                break;
            }
        }
        if (c == '*') {
            do {
                k = randint0(n);
            } while (seikaku_info[k].sex && (seikaku_info[k].sex != (creature_ptr->psex + 1)));
            cs = k;
            continue;
        }
        if (c == '8') {
            if (cs >= 4)
                cs -= 4;
            if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1))) {
                if ((cs - 4) > 0)
                    cs -= 4;
                else
                    cs += 4;
            }
        }
        if (c == '4') {
            if (cs > 0)
                cs--;
            if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1))) {
                if ((cs - 1) > 0)
                    cs--;
                else
                    cs++;
            }
        }
        if (c == '6') {
            if (cs < MAX_SEIKAKU)
                cs++;
            if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1))) {
                if ((cs + 1) <= MAX_SEIKAKU)
                    cs++;
                else
                    cs--;
            }
        }
        if (c == '2') {
            if ((cs + 4) <= MAX_SEIKAKU)
                cs += 4;
            if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1))) {
                if ((cs + 4) <= MAX_SEIKAKU)
                    cs += 4;
                else
                    cs -= 4;
            }
        }
        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_SEIKAKU)) {
            if ((seikaku_info[k].sex == 0) || (seikaku_info[k].sex == (creature_ptr->psex + 1))) {
                cs = k;
                continue;
            }
        }
        k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((k >= 26) && (k < MAX_SEIKAKU)) {
            if ((seikaku_info[k].sex == 0) || (seikaku_info[k].sex == (creature_ptr->psex + 1))) {
                cs = k;
                continue;
            }
        } else
            k = -1;
        if (c == '?') {
#ifdef JP
            show_help(creature_ptr, "jraceclas.txt#ThePersonalities");
#else
            show_help(creature_ptr, "raceclas.txt#ThePersonalities");
#endif
        } else if (c == '=') {
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
        } else if (c != '2' && c != '4' && c != '6' && c != '8')
            bell();
    }

    /* Set seikaku */
    creature_ptr->pseikaku = (CHARACTER_IDX)k;
    ap_ptr = &seikaku_info[creature_ptr->pseikaku];
#ifdef JP
    strcpy(tmp, ap_ptr->title);
    if (ap_ptr->no == 1)
        strcat(tmp, "の");
#else
    strcpy(tmp, ap_ptr->title);
    strcat(tmp, " ");
#endif
    strcat(tmp, creature_ptr->name);

    c_put_str(TERM_L_BLUE, tmp, 1, 34);

    return TRUE;
}

/*!
 * @brief オートローラで得たい能力値の基準を決める。
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static bool get_stat_limits(player_type* creature_ptr)
{
    int i, j, m, cs, os;
    int cval[6];
    char c;
    char buf[80], cur[80];
    char inp[80];

    /* Clean up */
    clear_from(10);

    /* Extra infomation */
    put_str(_("最低限得たい能力値を設定して下さい。", "Set minimum stats."), 10, 10);
    put_str(_("2/8で項目選択、4/6で値の増減、Enterで次へ", "2/8 for Select, 4/6 for Change value, Enter for Goto next"), 11, 10);

    put_str(_("         基本値  種族 職業 性格     合計値  最大値", "           Base   Rac  Cla  Per      Total  Maximum"), 13, 10);

    /* Output the maximum stats */
    for (i = 0; i < A_MAX; i++) {
        /* Reset the "success" counter */
        stat_match[i] = 0;
        cval[i] = 3;

        /* Race/Class bonus */
        j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];

        /* Obtain the "maximal" stat */
        m = adjust_stat(17, j);

        /* Above 18 */
        if (m > 18) {
            sprintf(cur, "18/%02d", (m - 18));
        }

        /* From 3 to 18 */
        else {
            sprintf(cur, "%2d", m);
        }

        /* Obtain the current stat */
        m = adjust_stat(cval[i], j);

        /* Above 18 */
        if (m > 18) {
            sprintf(inp, "18/%02d", (m - 18));
        }

        /* From 3 to 18 */
        else {
            sprintf(inp, "%2d", m);
        }

        /* Prepare a prompt */
        sprintf(buf, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s  %6s",
            stat_names[i], cval[i], rp_ptr->r_adj[i], cp_ptr->c_adj[i],
            ap_ptr->a_adj[i], inp, cur);

        /* Dump the prompt */
        put_str(buf, 14 + i, 10);
    }

    /* Get a minimum stat */
    cs = 0;
    os = 6;
    while (TRUE) {
        /* Move Cursol */
        if (cs != os) {
            if (os == 6) {
                c_put_str(TERM_WHITE, _("決定する", "Accept"), 21, 35);
            } else if (os < A_MAX) {
                c_put_str(TERM_WHITE, cur, 14 + os, 10);
            }
            if (cs == 6) {
                c_put_str(TERM_YELLOW, _("決定する", "Accept"), 21, 35);
            } else {
                /* Race/Class bonus */
                j = rp_ptr->r_adj[cs] + cp_ptr->c_adj[cs] + ap_ptr->a_adj[cs];

                /* Obtain the current stat */
                m = adjust_stat(cval[cs], j);

                /* Above 18 */
                if (m > 18) {
                    sprintf(inp, "18/%02d", (m - 18));
                }

                /* From 3 to 18 */
                else {
                    sprintf(inp, "%2d", m);
                }

                /* Prepare a prompt */
                sprintf(cur, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s",
                    stat_names[cs], cval[cs], rp_ptr->r_adj[cs],
                    cp_ptr->c_adj[cs], ap_ptr->a_adj[cs], inp);
                c_put_str(TERM_YELLOW, cur, 14 + cs, 10);
            }
            os = cs;
        }

        /* Prompt for the minimum stats */
        c = inkey();
        switch (c) {
        case 'Q':
            birth_quit();
            break;
        case 'S':
            return FALSE;
        case ESCAPE:
            break;
        case ' ':
        case '\r':
        case '\n':
            if (cs == 6)
                break;
            cs++;
            c = '2';
            break;
        case '8':
        case 'k':
            if (cs > 0)
                cs--;
            break;
        case '2':
        case 'j':
            if (cs < A_MAX)
                cs++;
            break;
        case '4':
        case 'h':
            if (cs != 6) {
                if (cval[cs] == 3) {
                    cval[cs] = 17;
                    os = 7;
                } else if (cval[cs] > 3) {
                    cval[cs]--;
                    os = 7;
                } else
                    return FALSE;
            }
            break;
        case '6':
        case 'l':
            if (cs != 6) {
                if (cval[cs] == 17) {
                    cval[cs] = 3;
                    os = 7;
                } else if (cval[cs] < 17) {
                    cval[cs]++;
                    os = 7;
                } else
                    return FALSE;
            }
            break;
        case 'm':
            if (cs != 6) {
                cval[cs] = 17;
                os = 7;
            }
            break;
        case 'n':
            if (cs != 6) {
                cval[cs] = 3;
                os = 7;
            }
            break;
        case '?':
#ifdef JP
            show_help(creature_ptr, "jbirth.txt#AutoRoller");
#else
            show_help(creature_ptr, "birth.txt#AutoRoller");
#endif
            break;
        case '=':
            screen_save();
#ifdef JP
            do_cmd_options_aux(OPT_PAGE_BIRTH, "初期オプション((*)はスコアに影響)");
#else
            do_cmd_options_aux(OPT_PAGE_BIRTH, "Birth Option((*)s effect score)");
#endif

            screen_load();
            break;
        default:
            bell();
            break;
        }
        if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == 6))
            break;
    }

    for (i = 0; i < A_MAX; i++) {
        /* Save the minimum stat */
        stat_limit[i] = (s16b)cval[i];
    }

    return TRUE;
}

/*!
 * @brief オートローラで得たい年齢、身長、体重、社会的地位の基準を決める。
 * @return なし
 */
static bool get_chara_limits(player_type* creature_ptr)
{
#define MAXITEMS 8

    int i, j, m, cs, os;
    int mval[MAXITEMS], cval[MAXITEMS];
    int max_percent, min_percent;
    char c;
    char buf[80], cur[80];
    concptr itemname[] = {
        _("年齢", "age"),
        _("身長(インチ)", "height"),
        _("体重(ポンド)", "weight"),
        _("社会的地位", "social class")
    };

    clear_from(10);

    /* Prompt for the minimum stats */
    put_str(_("2/4/6/8で項目選択、+/-で値の増減、Enterで次へ",
        "2/4/6/8 for Select, +/- for Change value, Enter for Goto next"), 11, 10);
    put_str(_("注意：身長と体重の最大値/最小値ぎりぎりの値は非常に出現確率が低くなります。",
        "Caution: Values near minimum or maximum are extremely rare."), 23, 2);

    if (creature_ptr->psex == SEX_MALE) {
        max_percent = (int)(rp_ptr->m_b_ht + rp_ptr->m_m_ht * 4 - 1) * 100 / (int)(rp_ptr->m_b_ht);
        min_percent = (int)(rp_ptr->m_b_ht - rp_ptr->m_m_ht * 4 + 1) * 100 / (int)(rp_ptr->m_b_ht);
    } else {
        max_percent = (int)(rp_ptr->f_b_ht + rp_ptr->f_m_ht * 4 - 1) * 100 / (int)(rp_ptr->f_b_ht);
        min_percent = (int)(rp_ptr->f_b_ht - rp_ptr->f_m_ht * 4 + 1) * 100 / (int)(rp_ptr->f_b_ht);
    }

    put_str(_("体格/地位の最小値/最大値を設定して下さい。", "Set minimum/maximum attribute."), 10, 10);
    put_str(_("  項    目                 最小値  最大値", " Parameter                    Min     Max"), 13, 20);

    /* Output the maximum stats */
    for (i = 0; i < MAXITEMS; i++) {
        /* Obtain the "maximal" stat */
        switch (i) {
        case 0: /* Minimum age */
            m = rp_ptr->b_age + 1;
            break;
        case 1: /* Maximum age */
            m = rp_ptr->b_age + rp_ptr->m_age;
            break;

        case 2: /* Minimum height */
            if (creature_ptr->psex == SEX_MALE)
                m = rp_ptr->m_b_ht - rp_ptr->m_m_ht * 4 + 1;
            else
                m = rp_ptr->f_b_ht - rp_ptr->f_m_ht * 4 + 1;
            break;
        case 3: /* Maximum height */
            if (creature_ptr->psex == SEX_MALE)
                m = rp_ptr->m_b_ht + rp_ptr->m_m_ht * 4 - 1;
            else
                m = rp_ptr->f_b_ht + rp_ptr->f_m_ht * 4 - 1;
            break;
        case 4: /* Minimum weight */
            if (creature_ptr->psex == SEX_MALE)
                m = (rp_ptr->m_b_wt * min_percent / 100) - (rp_ptr->m_m_wt * min_percent / 75) + 1;
            else
                m = (rp_ptr->f_b_wt * min_percent / 100) - (rp_ptr->f_m_wt * min_percent / 75) + 1;
            break;
        case 5: /* Maximum weight */
            if (creature_ptr->psex == SEX_MALE)
                m = (rp_ptr->m_b_wt * max_percent / 100) + (rp_ptr->m_m_wt * max_percent / 75) - 1;
            else
                m = (rp_ptr->f_b_wt * max_percent / 100) + (rp_ptr->f_m_wt * max_percent / 75) - 1;
            break;
        case 6: /* Minimum social class */
            m = 1;
            break;
        case 7: /* Maximum social class */
            m = 100;
            break;
        default:
            m = 1;
            break;
        }

        /* Save the maximum or minimum */
        mval[i] = m;
        cval[i] = m;
    }

    for (i = 0; i < 4; i++) {
        /* Prepare a prompt */
        sprintf(buf, "%-12s (%3d - %3d)", itemname[i], mval[i * 2], mval[i * 2 + 1]);

        /* Dump the prompt */
        put_str(buf, 14 + i, 20);

        for (j = 0; j < 2; j++) {
            sprintf(buf, "     %3d", cval[i * 2 + j]);
            put_str(buf, 14 + i, 45 + 8 * j);
        }
    }

    /* Get a minimum stat */
    cs = 0;
    os = MAXITEMS;
    while (TRUE) {
        /* Move Cursol */
        if (cs != os) {
            const char accept[] = _("決定する", "Accept");

            if (os == MAXITEMS) {
                c_put_str(TERM_WHITE, accept, 19, 35);
            } else {
                c_put_str(TERM_WHITE, cur, 14 + os / 2, 45 + 8 * (os % 2));
            }

            if (cs == MAXITEMS) {
                c_put_str(TERM_YELLOW, accept, 19, 35);
            } else {
                /* Prepare a prompt */
                sprintf(cur, "     %3d", cval[cs]);
                c_put_str(TERM_YELLOW, cur, 14 + cs / 2, 45 + 8 * (cs % 2));
            }
            os = cs;
        }

        /* Prompt for the minimum stats */
        c = inkey();
        switch (c) {
        case 'Q':
            birth_quit();
            break;
        case 'S':
            return FALSE;
        case ESCAPE:
            break; /*後でもう一回breakせんと*/
        case ' ':
        case '\r':
        case '\n':
            if (cs == MAXITEMS)
                break;
            cs++;
            c = '6';
            break;
        case '8':
        case 'k':
            if (cs - 2 >= 0)
                cs -= 2;
            break;
        case '2':
        case 'j':
            if (cs < MAXITEMS)
                cs += 2;
            if (cs > MAXITEMS)
                cs = MAXITEMS;
            break;
        case '4':
        case 'h':
            if (cs > 0)
                cs--;
            break;
        case '6':
        case 'l':
            if (cs < MAXITEMS)
                cs++;
            break;
        case '-':
        case '<':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] > cval[cs - 1]) {
                        cval[cs]--;
                        os = 127;
                    }
                } else {
                    if (cval[cs] > mval[cs]) {
                        cval[cs]--;
                        os = 127;
                    }
                }
            }
            break;
        case '+':
        case '>':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] < mval[cs]) {
                        cval[cs]++;
                        os = 127;
                    }
                } else {
                    if (cval[cs] < cval[cs + 1]) {
                        cval[cs]++;
                        os = 127;
                    }
                }
            }
            break;
        case 'm':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] < mval[cs]) {
                        cval[cs] = mval[cs];
                        os = 127;
                    }
                } else {
                    if (cval[cs] < cval[cs + 1]) {
                        cval[cs] = cval[cs + 1];
                        os = 127;
                    }
                }
            }
            break;
        case 'n':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] > cval[cs - 1]) {
                        cval[cs] = cval[cs - 1];
                        os = 255;
                    }
                } else {
                    if (cval[cs] > mval[cs]) {
                        cval[cs] = mval[cs];
                        os = 255;
                    }
                }
            }
            break;
        case '?':
#ifdef JP
            show_help(creature_ptr, "jbirth.txt#AutoRoller");
#else
            show_help(creature_ptr, "birth.txt#AutoRoller");
#endif
            break;
        case '=':
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
            break;
        default:
            bell();
            break;
        }
        if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == MAXITEMS))
            break;
    }

    /* Input the minimum stats */
    chara_limit.agemin = (s16b)cval[0];
    chara_limit.agemax = (s16b)cval[1];
    chara_limit.htmin = (s16b)cval[2];
    chara_limit.htmax = (s16b)cval[3];
    chara_limit.wtmin = (s16b)cval[4];
    chara_limit.wtmax = (s16b)cval[5];
    chara_limit.scmin = (s16b)cval[6];
    chara_limit.scmax = (s16b)cval[7];

    return TRUE;
}

#define HISTPREF_LIMIT 1024
static char* histpref_buf = NULL;

/*!
 * @brief 生い立ちメッセージの内容をバッファに加える。 / Hook function for reading the histpref.prf file.
 * @return なし
 */
void add_history_from_pref_line(concptr t)
{
    /* Do nothing if the buffer is not ready */
    if (!histpref_buf)
        return;

    my_strcat(histpref_buf, t, HISTPREF_LIMIT);
}

/*!
 * @brief 生い立ちメッセージをファイルからロードする。
 * @return なし
 */
static bool do_cmd_histpref(player_type* creature_ptr, void (*process_autopick_file_command)(char*))
{
    char buf[80];
    errr err;
    int i, j, n;
    char *s, *t;
    char temp[64 * 4];
    char histbuf[HISTPREF_LIMIT];

    if (!get_check(_("生い立ち設定ファイルをロードしますか? ", "Load background history preference file? ")))
        return FALSE;

    /* Prepare the buffer */
    histbuf[0] = '\0';
    histpref_buf = histbuf;

#ifdef JP
    sprintf(buf, "histedit-%s.prf", creature_ptr->base_name);
#else
    sprintf(buf, "histpref-%s.prf", creature_ptr->base_name);
#endif
    err = process_histpref_file(creature_ptr, buf, process_autopick_file_command);

    /* Process 'hist????.prf' if 'hist????-<name>.prf' doesn't exist */
    if (0 > err) {
#ifdef JP
        strcpy(buf, "histedit.prf");
#else
        strcpy(buf, "histpref.prf");
#endif
        err = process_histpref_file(creature_ptr, buf, process_autopick_file_command);
    }

    if (err) {
        msg_print(_("生い立ち設定ファイルの読み込みに失敗しました。", "Failed to load background history preference."));
        msg_print(NULL);

        /* Kill the buffer */
        histpref_buf = NULL;

        return FALSE;
    } else if (!histpref_buf[0]) {
        msg_print(_("有効な生い立ち設定はこのファイルにありません。", "There does not exist valid background history preference."));
        msg_print(NULL);

        /* Kill the buffer */
        histpref_buf = NULL;

        return FALSE;
    }

    /* Clear the previous history strings */
    for (i = 0; i < 4; i++)
        creature_ptr->history[i][0] = '\0';

    /* Skip leading spaces */
    for (s = histpref_buf; *s == ' '; s++) /* loop */
        ;

    /* Get apparent length */
    n = strlen(s);

    /* Kill trailing spaces */
    while ((n > 0) && (s[n - 1] == ' '))
        s[--n] = '\0';

    roff_to_buf(s, 60, temp, sizeof(temp));
    t = temp;
    for (i = 0; i < 4; i++) {
        if (t[0] == 0)
            break;
        else {
            strcpy(creature_ptr->history[i], t);
            t += strlen(t) + 1;
        }
    }

    /* Fill the remaining spaces */
    for (i = 0; i < 4; i++) {
        for (j = 0; creature_ptr->history[i][j]; j++) /* loop */
            ;

        for (; j < 59; j++)
            creature_ptr->history[i][j] = ' ';
        creature_ptr->history[i][59] = '\0';
    }

    /* Kill the buffer */
    histpref_buf = NULL;

    return TRUE;
}

/*!
 * @brief 生い立ちメッセージを編集する。/Character background edit-mode
 * @return なし
 */
static void edit_history(player_type* creature_ptr, void (*process_autopick_file_command)(char*))
{
    char old_history[4][60];
    TERM_LEN y = 0, x = 0;
    int i, j;

    /* Edit character background */
    for (i = 0; i < 4; i++) {
        sprintf(old_history[i], "%s", creature_ptr->history[i]);
    }
    /* Turn 0 to space */
    for (i = 0; i < 4; i++) {
        for (j = 0; creature_ptr->history[i][j]; j++) /* loop */
            ;

        for (; j < 59; j++)
            creature_ptr->history[i][j] = ' ';
        creature_ptr->history[i][59] = '\0';
    }

    display_player(creature_ptr, 1, map_name);
#ifdef JP
    c_put_str(TERM_L_GREEN, "(キャラクターの生い立ち - 編集モード)", 11, 20);
    put_str("[ カーソルキーで移動、Enterで終了、Ctrl-Aでファイル読み込み ]", 17, 10);
#else
    c_put_str(TERM_L_GREEN, "(Character Background - Edit Mode)", 11, 20);
    put_str("[ Cursor key for Move, Enter for End, Ctrl-A for Read pref ]", 17, 10);
#endif

    while (TRUE) {
        int skey;
        char c;

        for (i = 0; i < 4; i++) {
            put_str(creature_ptr->history[i], i + 12, 10);
        }
#ifdef JP
        if (iskanji2(creature_ptr->history[y], x))
            c_put_str(TERM_L_BLUE, format("%c%c", creature_ptr->history[y][x], creature_ptr->history[y][x + 1]), y + 12, x + 10);
        else
#endif
            c_put_str(TERM_L_BLUE, format("%c", creature_ptr->history[y][x]), y + 12, x + 10);

        /* Place cursor just after cost of current stat */
        Term_gotoxy(x + 10, y + 12);

        /* Get special key code */
        skey = inkey_special(TRUE);

        /* Get a character code */
        if (!(skey & SKEY_MASK))
            c = (char)skey;
        else
            c = 0;

        if (skey == SKEY_UP || c == KTRL('p')) {
            y--;
            if (y < 0)
                y = 3;
#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                x--;
#endif
        } else if (skey == SKEY_DOWN || c == KTRL('n')) {
            y++;
            if (y > 3)
                y = 0;
#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                x--;
#endif
        } else if (skey == SKEY_RIGHT || c == KTRL('f')) {
#ifdef JP
            if (iskanji2(creature_ptr->history[y], x))
                x++;
#endif
            x++;
            if (x > 58) {
                x = 0;
                if (y < 3)
                    y++;
            }
        } else if (skey == SKEY_LEFT || c == KTRL('b')) {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = 58;
                } else
                    x = 0;
            }

#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                x--;
#endif
        } else if (c == '\r' || c == '\n') {
            Term_erase(0, 11, 255);
            Term_erase(0, 17, 255);
#ifdef JP
            put_str("(キャラクターの生い立ち - 編集済み)", 11, 20);
#else
            put_str("(Character Background - Edited)", 11, 20);
#endif
            break;
        } else if (c == ESCAPE) {
            clear_from(11);
#ifdef JP
            put_str("(キャラクターの生い立ち)", 11, 25);
#else
            put_str("(Character Background)", 11, 25);
#endif

            for (i = 0; i < 4; i++) {
                sprintf(creature_ptr->history[i], "%s", old_history[i]);
                put_str(creature_ptr->history[i], i + 12, 10);
            }
            break;
        } else if (c == KTRL('A')) {
            if (do_cmd_histpref(creature_ptr, process_autopick_file_command)) {
#ifdef JP
                if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                    x--;
#endif
            }
        } else if (c == '\010') {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = 58;
                } else
                    x = 0;
            }

            creature_ptr->history[y][x] = ' ';
#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1))) {
                x--;
                creature_ptr->history[y][x] = ' ';
            }
#endif
        }
#ifdef JP
        else if (iskanji(c) || isprint(c))
#else
        else if (isprint(c)) /* BUGFIX */
#endif
        {
#ifdef JP
            if (iskanji2(creature_ptr->history[y], x)) {
                creature_ptr->history[y][x + 1] = ' ';
            }

            if (iskanji(c)) {
                if (x > 57) {
                    x = 0;
                    y++;
                    if (y > 3)
                        y = 0;
                }

                if (iskanji2(creature_ptr->history[y], x + 1)) {
                    creature_ptr->history[y][x + 2] = ' ';
                }

                creature_ptr->history[y][x++] = c;

                c = inkey();
            }
#endif
            creature_ptr->history[y][x++] = c;
            if (x > 58) {
                x = 0;
                y++;
                if (y > 3)
                    y = 0;
            }
        }
    } /* while (TRUE) */
}

/*!
 * @brief player_birth()関数のサブセット/Helper function for 'player_birth()'
 * @details
 * The delay may be reduced, but is recommended to keep players
 * from continuously rolling up characters, which can be VERY
 * expensive CPU wise.  And it cuts down on player stupidity.
 * @return なし
 */
static bool player_birth_aux(player_type* creature_ptr, void (*process_autopick_file_command)(char*))
{
    int i, k, n, cs, os;

    BIT_FLAGS mode = 0;

    bool flag = FALSE;
    bool prev = FALSE;

    concptr str;

    char c;

    char p2 = ')';
    char b1 = '[';
    char b2 = ']';

    char buf[80], cur[80];

    /*** Intro ***/
    Term_clear();

    /* Title everything */
    put_str(_("名前  :", "Name  :"), 1, 26);
    put_str(_("性別        :", "Sex         :"), 3, 1);
    put_str(_("種族        :", "Race        :"), 4, 1);
    put_str(_("職業        :", "Class       :"), 5, 1);

    /* Dump the default name */
    c_put_str(TERM_L_BLUE, creature_ptr->name, 1, 34);

    /*** Instructions ***/

    /* Display some helpful information */
    put_str(_("キャラクターを作成します。('S'やり直す, 'Q'終了, '?'ヘルプ)",
        "Make your charactor. ('S' Restart, 'Q' Quit, '?' Help)"), 8, 10);

    /*** Player sex ***/

    /* Extra info */
    put_str(_("注意：《性別》の違いはゲーム上ほとんど影響を及ぼしません。",
        "Note: Your 'sex' does not have any significant gameplay effects."), 23, 5);

    /* Prompt for "Sex" */
    for (n = 0; n < MAX_SEXES; n++) {
        /* Analyze */
        sp_ptr = &sex_info[n];

        /* Display */
#ifdef JP
        sprintf(buf, "%c%c%s", I2A(n), p2, sp_ptr->title);
#else
        sprintf(buf, "%c%c %s", I2A(n), p2, sp_ptr->title);
#endif
        put_str(buf, 12 + (n / 5), 2 + 15 * (n % 5));
    }

#ifdef JP
    sprintf(cur, "%c%c%s", '*', p2, "ランダム");
#else
    sprintf(cur, "%c%c %s", '*', p2, "Random");
#endif

    /* Choose */
    k = -1;
    cs = 0;
    os = MAX_SEXES;
    while (TRUE) {
        if (cs != os) {
            put_str(cur, 12 + (os / 5), 2 + 15 * (os % 5));
            if (cs == MAX_SEXES)
#ifdef JP
                sprintf(cur, "%c%c%s", '*', p2, "ランダム");
#else
                sprintf(cur, "%c%c %s", '*', p2, "Random");
#endif
            else {
                sp_ptr = &sex_info[cs];
                str = sp_ptr->title;
#ifdef JP
                sprintf(cur, "%c%c%s", I2A(cs), p2, str);
#else
                sprintf(cur, "%c%c %s", I2A(cs), p2, str);
#endif
            }
            c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 2 + 15 * (cs % 5));
            os = cs;
        }

        if (k >= 0)
            break;

#ifdef JP
        sprintf(buf, "性別を選んで下さい (%c-%c) ('='初期オプション設定): ", I2A(0), I2A(n - 1));
#else
        sprintf(buf, "Choose a sex (%c-%c) ('=' for options): ", I2A(0), I2A(n - 1));
#endif

        put_str(buf, 10, 10);
        c = inkey();
        if (c == 'Q')
            birth_quit();
        if (c == 'S')
            return FALSE;
        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_SEXES)
                k = randint0(MAX_SEXES);
            else
                k = cs;
            break;
        }
        if (c == '*') {
            k = randint0(MAX_SEXES);
            break;
        }
        if (c == '4') {
            if (cs > 0)
                cs--;
        }
        if (c == '6') {
            if (cs < MAX_SEXES)
                cs++;
        }
        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_SEXES)) {
            cs = k;
            continue;
        } else
            k = -1;
        if (c == '?')
            do_cmd_help(creature_ptr);
        else if (c == '=') {
            screen_save();
#ifdef JP
            do_cmd_options_aux(OPT_PAGE_BIRTH, "初期オプション((*)はスコアに影響)");
#else
            do_cmd_options_aux(OPT_PAGE_BIRTH, "Birth Option((*)s effect score)");
#endif

            screen_load();
        } else if (c != '4' && c != '6')
            bell();
    }

    /* Set sex */
    creature_ptr->psex = (byte)k;
    sp_ptr = &sex_info[creature_ptr->psex];

    /* Display */
    c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 15);

    /* Clean up */
    clear_from(10);

    /* Choose the players race */
    creature_ptr->prace = 0;
    while (TRUE) {
        char temp[80 * 10];
        concptr t;

        if (!get_player_race(creature_ptr))
            return FALSE;

        clear_from(10);

        roff_to_buf(race_explanations[creature_ptr->prace], 74, temp, sizeof(temp));
        t = temp;

        for (i = 0; i < 10; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }
#ifdef JP
        if (get_check_strict("よろしいですか？", CHECK_DEFAULT_Y))
            break;
#else
        if (get_check_strict("Are you sure? ", CHECK_DEFAULT_Y))
            break;
#endif
        clear_from(10);
        c_put_str(TERM_WHITE, "              ", 4, 15);
    }

    /* Clean up */
    clear_from(10);

    /* Choose the players class */
    creature_ptr->pclass = 0;
    while (TRUE) {
        char temp[80 * 9];
        concptr t;

        if (!get_player_class(creature_ptr))
            return FALSE;

        clear_from(10);
        roff_to_buf(class_explanations[creature_ptr->pclass], 74, temp, sizeof(temp));
        t = temp;

        for (i = 0; i < 9; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

#ifdef JP
        if (get_check_strict("よろしいですか？", CHECK_DEFAULT_Y))
            break;
#else
        if (get_check_strict("Are you sure? ", CHECK_DEFAULT_Y))
            break;
#endif
        c_put_str(TERM_WHITE, "              ", 5, 15);
    }

    /* Choose the magic realms */
    if (!get_player_realms(creature_ptr))
        return FALSE;

    /* Choose the players seikaku */
    creature_ptr->pseikaku = 0;
    while (TRUE) {
        char temp[80 * 8];
        concptr t;

        if (!get_player_seikaku(creature_ptr))
            return FALSE;

        clear_from(10);
        roff_to_buf(personality_explanations[creature_ptr->pseikaku], 74, temp, sizeof(temp));
        t = temp;

        for (i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }
#ifdef JP
        if (get_check_strict("よろしいですか？", CHECK_DEFAULT_Y))
            break;
#else
        if (get_check_strict("Are you sure? ", CHECK_DEFAULT_Y))
            break;
#endif
        c_put_str(TERM_L_BLUE, creature_ptr->name, 1, 34);
        prt("", 1, 34 + strlen(creature_ptr->name));
    }

    /* Clean up */
    clear_from(10);
    put_str("                                   ", 3, 40);
    put_str("                                   ", 4, 40);
    put_str("                                   ", 5, 40);

    screen_save();
#ifdef JP
    do_cmd_options_aux(OPT_PAGE_BIRTH, "初期オプション((*)はスコアに影響)");
#else
    do_cmd_options_aux(OPT_PAGE_BIRTH, "Birth Option((*)s effect score)");
#endif

    screen_load();

    if (autoroller || autochara) {
        /* Clear fields */
        auto_round = 0L;
    }

    if (autoroller) {
        if (!get_stat_limits(creature_ptr))
            return FALSE;
    }

    if (autochara) {
        if (!get_chara_limits(creature_ptr))
            return FALSE;
    }

    clear_from(10);

    /* Reset turn; before auto-roll and after choosing race */
    init_turn(creature_ptr);

    /*** Generate ***/

    /* Roll */
    while (TRUE) {
        int col;

        col = 42;

        if (autoroller || autochara) {
            Term_clear();

            /* Label count */
#ifdef JP
            put_str("回数 :", 10, col + 13);
#else
            put_str("Round:", 10, col + 13);
#endif

            /* Indicate the state */
#ifdef JP
            put_str("(ESCで停止)", 12, col + 13);
#else
            put_str("(Hit ESC to stop)", 12, col + 13);
#endif
        }

        /* Otherwise just get a character */
        else {
            get_stats(creature_ptr);
            get_ahw(creature_ptr);
            get_history(creature_ptr);
        }

        /* Feedback */
        if (autoroller) {
            /* Label */
#ifdef JP
            put_str("最小値", 2, col + 5);
#else
            put_str(" Limit", 2, col + 5);
#endif

            /* Label */
#ifdef JP
            put_str("成功率", 2, col + 13);
#else
            put_str("  Freq", 2, col + 13);
#endif

            /* Label */
#ifdef JP
            put_str("現在値", 2, col + 24);
#else
            put_str("  Roll", 2, col + 24);
#endif

            /* Put the minimal stats */
            for (i = 0; i < A_MAX; i++) {
                int j, m;

                /* Label stats */
                put_str(stat_names[i], 3 + i, col);

                /* Race/Class bonus */
                j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];

                /* Obtain the current stat */
                m = adjust_stat(stat_limit[i], j);

                /* Put the stat */
                cnv_stat(m, buf);
                c_put_str(TERM_L_BLUE, buf, 3 + i, col + 5);
            }
        }

        /* Auto-roll */
        while (autoroller || autochara) {
            bool accept = TRUE;

            /* Get a new character */
            get_stats(creature_ptr);

            /* Advance the round */
            auto_round++;

            /* Hack -- Prevent overflow */
            if (auto_round >= 1000000000L) {
                auto_round = 1;

                if (autoroller) {
                    for (i = 0; i < A_MAX; i++) {
                        stat_match[i] = 0;
                    }
                }
            }

            if (autoroller) {
                /* Check and count acceptable stats */
                for (i = 0; i < A_MAX; i++) {
                    /* This stat is okay */
                    if (creature_ptr->stat_max[i] >= stat_limit[i]) {
                        stat_match[i]++;
                    }

                    /* This stat is not okay */
                    else {
                        accept = FALSE;
                    }
                }
            }

            /* Break if "happy" */
            if (accept) {
                get_ahw(creature_ptr);
                get_history(creature_ptr);

                if (autochara) {
                    if ((creature_ptr->age < chara_limit.agemin) || (creature_ptr->age > chara_limit.agemax))
                        accept = FALSE;
                    if ((creature_ptr->ht < chara_limit.htmin) || (creature_ptr->ht > chara_limit.htmax))
                        accept = FALSE;
                    if ((creature_ptr->wt < chara_limit.wtmin) || (creature_ptr->wt > chara_limit.wtmax))
                        accept = FALSE;
                    if ((creature_ptr->sc < chara_limit.scmin) || (creature_ptr->sc > chara_limit.scmax))
                        accept = FALSE;
                }
                if (accept)
                    break;
            }

            /* Take note every x rolls */
            flag = (!(auto_round % AUTOROLLER_STEP));

            /* Update display occasionally */
            if (flag) {
                /* Dump data */
                birth_put_stats(creature_ptr);

                /* Dump round */
                put_str(format("%10ld", auto_round), 10, col + 20);

                /* Make sure they see everything */
                Term_fresh();

                /* Do not wait for a key */
                inkey_scan = TRUE;

                /* Check for a keypress */
                if (inkey()) {
                    get_ahw(creature_ptr);
                    get_history(creature_ptr);
                    break;
                }
            }
        }

        if (autoroller || autochara)
            sound(SOUND_LEVEL);

        flush();

        /*** Display ***/

        mode = 0;

        /* Roll for base hitpoints */
        get_extra(creature_ptr, TRUE);

        /* Roll for gold */
        get_money(creature_ptr);

        /* Hack -- get a chaos patron even if you are not a chaos warrior */
        creature_ptr->chaos_patron = (s16b)randint0(MAX_PATRON);

        /* Input loop */
        while (TRUE) {
            /* Calculate the bonuses and hitpoints */
            creature_ptr->update |= (PU_BONUS | PU_HP);
            update_creature(creature_ptr);

            creature_ptr->chp = creature_ptr->mhp;
            creature_ptr->csp = creature_ptr->msp;

            display_player(creature_ptr, mode, map_name);

            /* Prepare a prompt (must squeeze everything in) */
            Term_gotoxy(2, 23);
            Term_addch(TERM_WHITE, b1);
            Term_addstr(-1, TERM_WHITE, _("'r' 次の数値", "'r'eroll"));

            if (prev)
                Term_addstr(-1, TERM_WHITE, _(", 'p' 前の数値", "'p'previous"));
            if (mode)
                Term_addstr(-1, TERM_WHITE, _(", 'h' その他の情報", ", 'h' Misc."));
            else
                Term_addstr(-1, TERM_WHITE, _(", 'h' 生い立ちを表示", ", 'h'istory"));
            Term_addstr(-1, TERM_WHITE, _(", Enter この数値に決定", ", or Enter to accept"));
            Term_addch(TERM_WHITE, b2);

            c = inkey();

            /* Quit */
            if (c == 'Q')
                birth_quit();

            /* Start over */
            if (c == 'S')
                return FALSE;

            /* Escape accepts the roll */
            if (c == '\r' || c == '\n' || c == ESCAPE)
                break;

            /* Reroll this character */
            if ((c == ' ') || (c == 'r'))
                break;

            /* Previous character */
            if (prev && (c == 'p')) {
                load_prev_data(creature_ptr, TRUE);
                continue;
            }

            if ((c == 'H') || (c == 'h')) {
                mode = ((mode != 0) ? 0 : 1);
                continue;
            }

            /* Help */
            if (c == '?') {
#ifdef JP
                show_help(creature_ptr, "jbirth.txt#AutoRoller");
#else
                show_help(creature_ptr, "birth.txt#AutoRoller");
#endif
                continue;
            } else if (c == '=') {
                screen_save();
                do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
                screen_load();
                continue;
            }

            /* Warning */
            bell();
        }

        /* Are we done? */
        if (c == '\r' || c == '\n' || c == ESCAPE)
            break;

        /* Save this for the "previous" character */
        save_prev_data(creature_ptr, &previous_char);
        previous_char.quick_ok = FALSE;

        /* Note that a previous roll exists */
        prev = TRUE;
    }

    /* Clear prompt */
    clear_from(23);

    /* Get a name, recolor it, prepare savefile */
    get_name(creature_ptr);

    /* Process the player name */
    process_player_name(creature_ptr, current_world_ptr->creating_savefile);

    /*** Edit character background ***/
    edit_history(creature_ptr, process_autopick_file_command);

    /*** Finish up ***/

    get_max_stats(creature_ptr);

    get_virtues(creature_ptr);

    /* Prompt for it */
#ifdef JP
    prt("[ 'Q' 中断, 'S' 初めから, Enter ゲーム開始 ]", 23, 14);
#else
    prt("['Q'uit, 'S'tart over, or Enter to continue]", 23, 10);
#endif

    /* Get a key */
    c = inkey();

    /* Quit */
    if (c == 'Q')
        birth_quit();

    /* Start over */
    if (c == 'S')
        return FALSE;

    /* Initialize random quests */
    init_dungeon_quests(creature_ptr);

    /* Save character data for quick start */
    save_prev_data(creature_ptr, &previous_char);
    previous_char.quick_ok = TRUE;

    /* Accept */
    return TRUE;
}

/*!
 * @brief クイックスタート処理の問い合わせと実行を行う。/Ask whether the player use Quick Start or not.
 * @return なし
 */
static bool ask_quick_start(player_type* creature_ptr)
{
    /* Doesn't have previous data */
    if (!previous_char.quick_ok)
        return FALSE;

    Term_clear();

    /* Extra info */
    put_str(_("クイック・スタートを使うと以前と全く同じキャラクターで始められます。",
        "Do you want to use the quick start function(same character as your last one)."), 11, 2);

    /* Choose */
    while (TRUE) {
        char c;

        put_str(_("クイック・スタートを使いますか？[y/N]", "Use quick start? [y/N]"), 14, 10);
        c = inkey();

        if (c == 'Q')
            quit(NULL);
        else if (c == 'S')
            return FALSE;
        else if (c == '?') {
#ifdef JP
            show_help(creature_ptr, "jbirth.txt#QuickStart");
#else
            show_help(creature_ptr, "birth.txt#QuickStart");
#endif
        } else if ((c == 'y') || (c == 'Y')) {
            /* Yes */
            break;
        } else {
            /* No */
            return FALSE;
        }
    }

    load_prev_data(creature_ptr, FALSE);
    init_turn(creature_ptr);
    init_dungeon_quests(creature_ptr);

    sp_ptr = &sex_info[creature_ptr->psex];
    rp_ptr = &race_info[creature_ptr->prace];
    cp_ptr = &class_info[creature_ptr->pclass];
    mp_ptr = &m_info[creature_ptr->pclass];
    ap_ptr = &seikaku_info[creature_ptr->pseikaku];

    /* Calc hitdie, but don't roll */
    get_extra(creature_ptr, FALSE);

    creature_ptr->update |= (PU_BONUS | PU_HP);
    update_creature(creature_ptr);
    creature_ptr->chp = creature_ptr->mhp;
    creature_ptr->csp = creature_ptr->msp;

    /* Process the player name */
    process_player_name(creature_ptr, FALSE);

    return TRUE;
}

/*!
 * @brief プレイヤー作成処理のメインルーチン/ Create a new character.
 * @details
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 * @return なし
 */
void player_birth(player_type* creature_ptr, void (*process_autopick_file_command)(char*))
{
    int i, j;
    char buf[80];

    current_world_ptr->play_time = 0;

    wipe_monsters_list(creature_ptr);

    /* Wipe the player */
    player_wipe_without_name(creature_ptr);

    /* Create a new character */

    /* Quick start? */
    if (!ask_quick_start(creature_ptr)) {
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DEFAULT);

        /* No, normal start */
        while (TRUE) {
            /* Roll up a new character */
            if (player_birth_aux(creature_ptr, process_autopick_file_command))
                break;

            /* Wipe the player */
            player_wipe_without_name(creature_ptr);
        }
    }

    /* Note player birth in the message recall */
    message_add(" ");
    message_add("  ");
    message_add("====================");
    message_add(" ");
    message_add("  ");

    exe_write_diary(creature_ptr, DIARY_GAMESTART, 1, _("-------- 新規ゲーム開始 --------", "------- Started New Game -------"));
    exe_write_diary(creature_ptr, DIARY_DIALY, 0, NULL);

    sprintf(buf, _("                            性別に%sを選択した。", "                            chose %s gender."),
        sex_info[creature_ptr->psex].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);

    sprintf(buf, _("                            種族に%sを選択した。", "                            chose %s race."),
        race_info[creature_ptr->prace].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);

    sprintf(buf, _("                            職業に%sを選択した。", "                            chose %s class."),
        class_info[creature_ptr->pclass].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);

    if (creature_ptr->realm1) {
        sprintf(buf, _("                            魔法の領域に%s%sを選択した。", "                            chose %s%s."),
            realm_names[creature_ptr->realm1], creature_ptr->realm2 ? format(_("と%s", " and %s realms"), realm_names[creature_ptr->realm2]) : _("", " realm"));
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    }

    sprintf(buf, _("                            性格に%sを選択した。", "                            chose %s personality."),
        seikaku_info[creature_ptr->pseikaku].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);

    /* Init the shops */
    for (i = 1; i < max_towns; i++) {
        for (j = 0; j < MAX_STORES; j++) {
            store_init(i, j);
        }
    }

    /* Generate the random seeds for the wilderness */
    seed_wilderness();

    /* Give beastman a mutation at character birth */
    if (creature_ptr->prace == RACE_BEASTMAN)
        creature_ptr->hack_mutation = TRUE;
    else
        creature_ptr->hack_mutation = FALSE;

    /* Set the message window flag as default */
    if (!window_flag[1])
        window_flag[1] |= PW_MESSAGE;

    /* Set the inv/equip window flag as default */
    if (!window_flag[2])
        window_flag[2] |= PW_INVEN;
}

/*!
 * @brief プレイヤー作成処理中のステータス表示処理
 * @param fff ファイルポインタ
 * @return なし
 */
void dump_yourself(player_type* creature_ptr, FILE* fff)
{
    char temp[80 * 10];
    int i;
    concptr t;

    if (!fff)
        return;

    roff_to_buf(race_explanations[creature_ptr->prace], 78, temp, sizeof(temp));
    fprintf(fff, "\n\n");
    fprintf(fff, _("種族: %s\n", "Race: %s\n"), race_info[creature_ptr->prace].title);

    t = temp;
    for (i = 0; i < 10; i++) {
        if (t[0] == 0)
            break;
        fprintf(fff, "%s\n", t);
        t += strlen(t) + 1;
    }
    roff_to_buf(class_explanations[creature_ptr->pclass], 78, temp, sizeof(temp));
    fprintf(fff, "\n");
    fprintf(fff, _("職業: %s\n", "Class: %s\n"), class_info[creature_ptr->pclass].title);

    t = temp;
    for (i = 0; i < 10; i++) {
        if (t[0] == 0)
            break;
        fprintf(fff, "%s\n", t);
        t += strlen(t) + 1;
    }
    roff_to_buf(personality_explanations[creature_ptr->pseikaku], 78, temp, sizeof(temp));
    fprintf(fff, "\n");
    fprintf(fff, _("性格: %s\n", "Pesonality: %s\n"), seikaku_info[creature_ptr->pseikaku].title);

    t = temp;
    for (i = 0; i < A_MAX; i++) {
        if (t[0] == 0)
            break;
        fprintf(fff, "%s\n", t);
        t += strlen(t) + 1;
    }
    fprintf(fff, "\n");
    if (creature_ptr->realm1) {
        roff_to_buf(realm_explanations[technic2magic(creature_ptr->realm1) - 1], 78, temp, sizeof(temp));
        fprintf(fff, _("魔法: %s\n", "Realm: %s\n"), realm_names[creature_ptr->realm1]);

        t = temp;
        for (i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;
            fprintf(fff, "%s\n", t);
            t += strlen(t) + 1;
        }
    }
    fprintf(fff, "\n");
    if (creature_ptr->realm2) {
        roff_to_buf(realm_explanations[technic2magic(creature_ptr->realm2) - 1], 78, temp, sizeof(temp));
        fprintf(fff, _("魔法: %s\n", "Realm: %s\n"), realm_names[creature_ptr->realm2]);

        t = temp;
        for (i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;
            fprintf(fff, "%s\n", t);
            t += strlen(t) + 1;
        }
    }
}
