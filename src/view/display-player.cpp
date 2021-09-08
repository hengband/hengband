/*!
 * @brief プレーヤーのステータス表示メインルーチン群
 * @date 2020/02/25
 * @author Hourier
 * @details
 * ここにこれ以上関数を引っ越してくるのは禁止
 */

#include "view/display-player.h"
#include "dungeon/quest.h"
#include "floor/floor-util.h"
#include "game-option/text-display-options.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-slot-types.h"
#include "knowledge/knowledge-mutations.h"
#include "mind/mind-elementalist.h"
#include "mutation/mutation-flag-types.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "player-info/alignment.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player/patron.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "realm/realm-names-table.h"
#include "status-first-page.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/buffer-shaper.h"
#include "view/display-characteristic.h"
#include "view/display-player-middle.h"
#include "view/display-player-misc-info.h"
#include "view/display-player-stat-info.h"
#include "view/display-util.h"
#include "world/world.h"
#include <string>

/*!
 * @brief
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param mode ステータス表示モード
 * @return どれかの処理をこなしたらTRUE、何もしなかったらFALSE
 */
static bool display_player_info(player_type *creature_ptr, int mode)
{
    if (mode == 2) {
        display_player_misc_info(creature_ptr);
        display_player_stat_info(creature_ptr);
        display_player_flag_info_1(creature_ptr, display_player_equippy);
        return true;
    }

    if (mode == 3) {
        display_player_flag_info_2(creature_ptr, display_player_equippy);
        return true;
    }

    if (mode == 4) {
        display_player_flag_info_3(creature_ptr, display_player_equippy);
        return true;
    }

    if (mode == 5) {
        do_cmd_knowledge_mutations(creature_ptr);
        return true;
    }

    return false;
}

/*!
 * @brief 名前、性別、種族、職業を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void display_player_basic_info(player_type *creature_ptr)
{
    char tmp[64];
#ifdef JP
    sprintf(tmp, "%s%s%s", ap_ptr->title, ap_ptr->no == 1 ? "の" : "", creature_ptr->name);
#else
    sprintf(tmp, "%s %s", ap_ptr->title, creature_ptr->name);
#endif

    display_player_one_line(ENTRY_NAME, tmp, TERM_L_BLUE);
    display_player_one_line(ENTRY_SEX, sp_ptr->title, TERM_L_BLUE);
    display_player_one_line(ENTRY_RACE, (creature_ptr->mimic_form ? mimic_info[creature_ptr->mimic_form].title : rp_ptr->title), TERM_L_BLUE);
    display_player_one_line(ENTRY_CLASS, cp_ptr->title, TERM_L_BLUE);
}

/*!
 * @brief 魔法領域を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void display_magic_realms(player_type *creature_ptr)
{
    if (creature_ptr->realm1 == REALM_NONE && creature_ptr->element == REALM_NONE)
        return;

    char tmp[64];
    if (creature_ptr->pclass == CLASS_ELEMENTALIST)
        sprintf(tmp, "%s", get_element_title(creature_ptr->element));
    else if (creature_ptr->realm2)
        sprintf(tmp, "%s, %s", realm_names[creature_ptr->realm1], realm_names[creature_ptr->realm2]);
    else
        strcpy(tmp, realm_names[creature_ptr->realm1]);

    display_player_one_line(ENTRY_REALM, tmp, TERM_L_BLUE);
}

/*!
 * @ brief 年齢、身長、体重、社会的地位を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details
 * 日本語版では、身長はcmに、体重はkgに変更してある
 */
static void display_phisique(player_type *creature_ptr)
{
#ifdef JP
    display_player_one_line(ENTRY_AGE, format("%d才", (int)creature_ptr->age), TERM_L_BLUE);
    display_player_one_line(ENTRY_HEIGHT, format("%dcm", (int)((creature_ptr->ht * 254) / 100)), TERM_L_BLUE);
    display_player_one_line(ENTRY_WEIGHT, format("%dkg", (int)((creature_ptr->wt * 4536) / 10000)), TERM_L_BLUE);
    display_player_one_line(ENTRY_SOCIAL, format("%d  ", (int)creature_ptr->sc), TERM_L_BLUE);
#else
    display_player_one_line(ENTRY_AGE, format("%d", (int)creature_ptr->age), TERM_L_BLUE);
    display_player_one_line(ENTRY_HEIGHT, format("%d", (int)creature_ptr->ht), TERM_L_BLUE);
    display_player_one_line(ENTRY_WEIGHT, format("%d", (int)creature_ptr->wt), TERM_L_BLUE);
    display_player_one_line(ENTRY_SOCIAL, format("%d", (int)creature_ptr->sc), TERM_L_BLUE);
#endif
    std::string alg = PlayerAlignment(creature_ptr).get_alignment_description();
    display_player_one_line(ENTRY_ALIGN, format("%s", alg.c_str()), TERM_L_BLUE);
}

/*!
 * @brief 能力値を (減少していたら色を変えて)表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void display_player_stats(player_type *creature_ptr)
{
    char buf[80];
    for (int i = 0; i < A_MAX; i++) {
        if (creature_ptr->stat_cur[i] < creature_ptr->stat_max[i]) {
            put_str(stat_names_reduced[i], 3 + i, 53);
            int value = creature_ptr->stat_use[i];
            cnv_stat(value, buf);
            c_put_str(TERM_YELLOW, buf, 3 + i, 60);
            value = creature_ptr->stat_top[i];
            cnv_stat(value, buf);
            c_put_str(TERM_L_GREEN, buf, 3 + i, 67);
        } else {
            put_str(stat_names[i], 3 + i, 53);
            cnv_stat(creature_ptr->stat_use[i], buf);
            c_put_str(TERM_L_GREEN, buf, 3 + i, 60);
        }

        if (creature_ptr->stat_max[i] == creature_ptr->stat_max_max[i])
            c_put_str(TERM_WHITE, "!", 3 + i, _(58, 58 - 2));
    }
}

/*!
 * @brief ゲームオーバーの原因を探る (生きていたら何もしない)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param statmsg メッセージバッファ
 * @return 生きていたらFALSE、死んでいたらTRUE
 */
static bool search_death_cause(player_type *creature_ptr, char *statmsg)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (!creature_ptr->is_dead)
        return false;

    if (current_world_ptr->total_winner) {
        sprintf(statmsg, _("…あなたは勝利の後%sした。", "...You %s after winning."),
            streq(creature_ptr->died_from, "Seppuku") ? _("切腹", "committed seppuku") : _("引退", "retired from the adventure"));

        return true;
    }

    if (!floor_ptr->dun_level) {
#ifdef JP
        sprintf(statmsg, "…あなたは%sで%sに殺された。", map_name(creature_ptr), creature_ptr->died_from);
#else
        sprintf(statmsg, "...You were killed by %s in %s.", creature_ptr->died_from, map_name(creature_ptr));
#endif
        return true;
    }

    if (floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest)) {
        /* Get the quest text */
        /* Bewere that INIT_ASSIGN resets the cur_num. */
        init_flags = INIT_NAME_ONLY;
        parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
#ifdef JP
        sprintf(statmsg, "…あなたは、クエスト「%s」で%sに殺された。", quest[floor_ptr->inside_quest].name, creature_ptr->died_from);
#else
        sprintf(statmsg, "...You were killed by %s in the quest '%s'.", creature_ptr->died_from, quest[floor_ptr->inside_quest].name);
#endif
        return true;
    }

#ifdef JP
    sprintf(statmsg, "…あなたは、%sの%d階で%sに殺された。", map_name(creature_ptr), (int)floor_ptr->dun_level, creature_ptr->died_from);
#else
    sprintf(statmsg, "...You were killed by %s on level %d of %s.", creature_ptr->died_from, floor_ptr->dun_level, map_name(creature_ptr));
#endif

    return true;
}

/*!
 * @brief クエストフロアで生きている場合、クエスト名をバッファに詰める
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param statmsg メッセージバッファ
 * @return クエスト内であればTRUE、いなければFALSE
 */
static bool decide_death_in_quest(player_type *creature_ptr, char *statmsg)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (!floor_ptr->inside_quest || !is_fixed_quest_idx(floor_ptr->inside_quest))
        return false;

    for (int i = 0; i < 10; i++)
        quest_text[i][0] = '\0';

    quest_text_line = 0;
    init_flags = INIT_NAME_ONLY;
    parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
    sprintf(statmsg, _("…あなたは現在、 クエスト「%s」を遂行中だ。", "...Now, you are in the quest '%s'."), quest[floor_ptr->inside_quest].name);
    return true;
}

/*!
 * @brief 現在いるフロアを、または死んでいたらどこでどう死んだかをバッファに詰める
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param statmsg メッセージバッファ
 */
static void decide_current_floor(player_type *creature_ptr, char *statmsg)
{
    if (search_death_cause(creature_ptr, statmsg))
        return;
    if (!current_world_ptr->character_dungeon)
        return;

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (floor_ptr->dun_level == 0) {
        sprintf(statmsg, _("…あなたは現在、 %s にいる。", "...Now, you are in %s."), map_name(creature_ptr));
        return;
    }

    if (decide_death_in_quest(creature_ptr, statmsg))
        return;

#ifdef JP
    sprintf(statmsg, "…あなたは現在、 %s の %d 階で探索している。", map_name(creature_ptr), (int)floor_ptr->dun_level);
#else
    sprintf(statmsg, "...Now, you are exploring level %d of %s.", (int)floor_ptr->dun_level, map_name(creature_ptr));
#endif
}

/*!
 * @brief 今いる、または死亡した場所を表示する
 * @param statmsg メッセージバッファ
 */
static void display_current_floor(char *statmsg)
{
    char temp[128];
    shape_buffer(statmsg, 60, temp, sizeof(temp));
    char *t;
    t = temp;
    for (int i = 0; i < 2; i++) {
        if (t[0] == 0)
            return;

        put_str(t, i + 5 + 12, 10);
        t += strlen(t) + 1;
    }
}

/*!
 * @brief プレイヤーのステータス表示メイン処理
 * Display the character on the screen (various modes)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param mode 表示モードID
 * @details
 * <pre>
 * The top one and bottom two lines are left blank.
 * Mode 0 = standard display with skills
 * Mode 1 = standard display with history
 * Mode 2 = summary of various things
 * Mode 3 = summary of various things (part 2)
 * Mode 4 = mutations
 * </pre>
 */
void display_player(player_type *creature_ptr, int mode)
{
    if ((creature_ptr->muta.any() || has_good_luck(creature_ptr)) && display_mutations)
        mode = (mode % 6);
    else
        mode = (mode % 5);

    clear_from(0);
    if (display_player_info(creature_ptr, mode))
        return;

    display_player_basic_info(creature_ptr);
    display_magic_realms(creature_ptr);

    if ((creature_ptr->pclass == CLASS_CHAOS_WARRIOR) || (creature_ptr->muta.has(MUTA::CHAOS_GIFT)))
        display_player_one_line(ENTRY_PATRON, chaos_patrons[creature_ptr->chaos_patron], TERM_L_BLUE);

    display_phisique(creature_ptr);
    display_player_stats(creature_ptr);

    if (mode == 0) {
        display_player_middle(creature_ptr);
        display_player_various(creature_ptr);
        return;
    }

    char statmsg[1000];
    put_str(_("(キャラクターの生い立ち)", "(Character Background)"), 11, 25);
    for (int i = 0; i < 4; i++)
        put_str(creature_ptr->history[i], i + 12, 10);

    *statmsg = '\0';
    decide_current_floor(creature_ptr, statmsg);
    if (!*statmsg)
        return;

    display_current_floor(statmsg);
}

/*!
 * @brief プレイヤーの装備一覧をシンボルで並べる
 * Equippy chars
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param y 表示するコンソールの行
 * @param x 表示するコンソールの列
 * @param mode オプション
 * @todo y = 6、x = 0、mode = 0で固定。何とかする
 */
void display_player_equippy(player_type *creature_ptr, TERM_LEN y, TERM_LEN x, BIT_FLAGS16 mode)
{
    int max_i = (mode & DP_WP) ? INVEN_BOW + 1 : INVEN_TOTAL;
    for (int i = INVEN_MAIN_HAND; i < max_i; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[i];

        TERM_COLOR a = object_attr(o_ptr);
        SYMBOL_CODE c = object_char(o_ptr);

        if (!equippy_chars || !o_ptr->k_idx) {
            c = ' ';
            a = TERM_DARK;
        }

        term_putch(x + i - INVEN_MAIN_HAND, y, a, c);
    }
}
