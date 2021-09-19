#include "target/target-describer.h"
#include "action/travel-execution.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-town.h"
#include "floor/geometry.h"
#include "floor/object-scanner.h"
#include "game-option/input-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "lore/lore-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "object/item-tester-hooker.h"
#include "object/object-mark-types.h"
#include "player/player-status-table.h"
#include "system/building-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "view/display-lore.h"
#include "view/display-messages.h"
#include "view/display-monster-status.h"
#include "window/display-sub-windows.h"
#include "world/world.h"
#ifdef JP
#else
#include "locale/english.h"
#endif

static const int16_t CONTINUOUS_DESCRIPTION = 256;

bool show_gold_on_floor = false;

// Examine grid
typedef struct eg_type {
    POSITION y;
    POSITION x;
    target_type mode;
    bool boring;
    concptr info;
    concptr s1;
    concptr s2;
    concptr s3;
    concptr x_info;
    char query;
    char out_val[MAX_NLEN + 80];
    OBJECT_IDX floor_list[23];
    ITEM_NUMBER floor_num;
    grid_type *g_ptr;
    monster_type *m_ptr;
    OBJECT_IDX next_o_idx;
    FEAT_IDX feat;
    feature_type *f_ptr;
    concptr name;
} eg_type;

static eg_type *initialize_eg_type(player_type *player_ptr, eg_type *eg_ptr, POSITION y, POSITION x, target_type mode, concptr info)
{
    eg_ptr->y = y;
    eg_ptr->x = x;
    eg_ptr->boring = true;
    eg_ptr->mode = mode;
    eg_ptr->info = info;
    eg_ptr->s1 = "";
    eg_ptr->s2 = "";
    eg_ptr->s3 = "";
    eg_ptr->x_info = "";
    eg_ptr->query = '\001';
    eg_ptr->floor_num = 0;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    eg_ptr->g_ptr = &floor_ptr->grid_array[y][x];
    eg_ptr->m_ptr = &floor_ptr->m_list[eg_ptr->g_ptr->m_idx];
    eg_ptr->next_o_idx = 0;
    return eg_ptr;
}

/*
 * Evaluate number of kill needed to gain level
 */
static void evaluate_monster_exp(player_type *player_ptr, char *buf, monster_type *m_ptr)
{
    monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
    if ((player_ptr->lev >= PY_MAX_LEVEL) || (player_ptr->prace == player_race_type::ANDROID)) {
        sprintf(buf, "**");
        return;
    }

    if (!ap_r_ptr->r_tkills || m_ptr->mflag2.has(MFLAG2::KAGE)) {
        if (!w_ptr->wizard) {
            sprintf(buf, "??");
            return;
        }
    }

    int32_t exp_mon = ap_r_ptr->mexp * ap_r_ptr->level;
    uint32_t exp_mon_frac = 0;
    s64b_div(&exp_mon, &exp_mon_frac, 0, (player_ptr->max_plv + 2));

    int32_t exp_adv = player_exp[player_ptr->lev - 1] * player_ptr->expfact;
    uint32_t exp_adv_frac = 0;
    s64b_div(&exp_adv, &exp_adv_frac, 0, 100);

    s64b_sub(&exp_adv, &exp_adv_frac, player_ptr->exp, player_ptr->exp_frac);

    s64b_add(&exp_adv, &exp_adv_frac, exp_mon, exp_mon_frac);
    s64b_sub(&exp_adv, &exp_adv_frac, 0, 1);

    s64b_div(&exp_adv, &exp_adv_frac, exp_mon, exp_mon_frac);

    uint32_t num = MIN(999, exp_adv_frac);
    sprintf(buf, "%03ld", (long int)num);
}

static void describe_scan_result(player_type *player_ptr, eg_type *eg_ptr)
{
    if (!easy_floor)
        return;

    eg_ptr->floor_num = scan_floor_items(player_ptr, eg_ptr->floor_list, eg_ptr->y, eg_ptr->x, SCAN_FLOOR_ONLY_MARKED, AllMatchItemTester());
    if (eg_ptr->floor_num > 0)
        eg_ptr->x_info = _("x物 ", "x,");
}

static void describe_target(player_type *player_ptr, eg_type *eg_ptr)
{
    if (!player_bold(player_ptr, eg_ptr->y, eg_ptr->x)) {
        eg_ptr->s1 = _("ターゲット:", "Target:");
        return;
    }

#ifdef JP
    eg_ptr->s1 = "あなたは";
    eg_ptr->s2 = "の上";
    eg_ptr->s3 = "にいる";
#else
    eg_ptr->s1 = "You are ";
    eg_ptr->s2 = "on ";
#endif
}

static process_result describe_hallucinated_target(player_type *player_ptr, eg_type *eg_ptr)
{
    if (!player_ptr->image)
        return PROCESS_CONTINUE;

    concptr name = _("何か奇妙な物", "something strange");
#ifdef JP
    sprintf(eg_ptr->out_val, "%s%s%s%s [%s]", eg_ptr->s1, name, eg_ptr->s2, eg_ptr->s3, eg_ptr->info);
#else
    sprintf(eg_ptr->out_val, "%s%s%s%s [%s]", eg_ptr->s1, eg_ptr->s2, eg_ptr->s3, name, eg_ptr->info);
#endif
    prt(eg_ptr->out_val, 0, 0);
    move_cursor_relative(eg_ptr->y, eg_ptr->x);
    eg_ptr->query = inkey();
    if ((eg_ptr->query != '\r') && (eg_ptr->query != '\n'))
        return PROCESS_TRUE;

    return PROCESS_FALSE;
}

static bool describe_grid_lore(player_type *player_ptr, eg_type *eg_ptr)
{
    screen_save();
    screen_roff(player_ptr, eg_ptr->m_ptr->ap_r_idx, MONSTER_LORE_NORMAL);
    term_addstr(-1, TERM_WHITE, format(_("  [r思 %s%s]", "  [r,%s%s]"), eg_ptr->x_info, eg_ptr->info));
    eg_ptr->query = inkey();
    screen_load();
    return eg_ptr->query != 'r';
}

static void describe_grid_monster(player_type *player_ptr, eg_type *eg_ptr)
{
    bool recall = false;
    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(player_ptr, m_name, eg_ptr->m_ptr, MD_INDEF_VISIBLE);
    while (true) {
        char acount[10];
        if (recall) {
            if (describe_grid_lore(player_ptr, eg_ptr))
                return;

            recall = false;
            continue;
        }

        evaluate_monster_exp(player_ptr, acount, eg_ptr->m_ptr);
#ifdef JP
        sprintf(eg_ptr->out_val, "[%s]%s%s(%s)%s%s [r思 %s%s]", acount, eg_ptr->s1, m_name, look_mon_desc(eg_ptr->m_ptr, 0x01), eg_ptr->s2, eg_ptr->s3,
            eg_ptr->x_info, eg_ptr->info);
#else
        sprintf(eg_ptr->out_val, "[%s]%s%s%s%s(%s) [r, %s%s]", acount, eg_ptr->s1, eg_ptr->s2, eg_ptr->s3, m_name, look_mon_desc(eg_ptr->m_ptr, 0x01),
            eg_ptr->x_info, eg_ptr->info);
#endif
        prt(eg_ptr->out_val, 0, 0);
        move_cursor_relative(eg_ptr->y, eg_ptr->x);
        eg_ptr->query = inkey();
        if (eg_ptr->query != 'r')
            return;

        recall = true;
    }
}

static void describe_monster_person(eg_type *eg_ptr)
{
    monster_race *ap_r_ptr = &r_info[eg_ptr->m_ptr->ap_r_idx];
    eg_ptr->s1 = _("それは", "It is ");
    if (ap_r_ptr->flags1 & RF1_FEMALE)
        eg_ptr->s1 = _("彼女は", "She is ");
    else if (ap_r_ptr->flags1 & RF1_MALE)
        eg_ptr->s1 = _("彼は", "He is ");

#ifdef JP
    eg_ptr->s2 = "を";
    eg_ptr->s3 = "持っている";
#else
    eg_ptr->s2 = "carrying ";
#endif
}

static uint16_t describe_monster_item(player_type *player_ptr, eg_type *eg_ptr)
{
    for (const auto this_o_idx : eg_ptr->m_ptr->hold_o_idx_list) {
        GAME_TEXT o_name[MAX_NLEN];
        object_type *o_ptr;
        o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
        describe_flavor(player_ptr, o_name, o_ptr, 0);
#ifdef JP
        sprintf(eg_ptr->out_val, "%s%s%s%s[%s]", eg_ptr->s1, o_name, eg_ptr->s2, eg_ptr->s3, eg_ptr->info);
#else
        sprintf(eg_ptr->out_val, "%s%s%s%s [%s]", eg_ptr->s1, eg_ptr->s2, eg_ptr->s3, o_name, eg_ptr->info);
#endif
        prt(eg_ptr->out_val, 0, 0);
        move_cursor_relative(eg_ptr->y, eg_ptr->x);
        eg_ptr->query = inkey();
        if ((eg_ptr->query != '\r') && (eg_ptr->query != '\n') && (eg_ptr->query != ' ') && (eg_ptr->query != 'x'))
            return eg_ptr->query;

        if ((eg_ptr->query == ' ') && !(eg_ptr->mode & TARGET_LOOK))
            return eg_ptr->query;

        eg_ptr->s2 = _("をまた", "also carrying ");
    }

    return CONTINUOUS_DESCRIPTION;
}

static bool within_char_util(int16_t input)
{
    return (input > -127) && (input < 128);
}

static int16_t describe_grid(player_type *player_ptr, eg_type *eg_ptr)
{
    if ((eg_ptr->g_ptr->m_idx == 0) || !player_ptr->current_floor_ptr->m_list[eg_ptr->g_ptr->m_idx].ml)
        return CONTINUOUS_DESCRIPTION;

    eg_ptr->boring = false;
    monster_race_track(player_ptr, eg_ptr->m_ptr->ap_r_idx);
    health_track(player_ptr, eg_ptr->g_ptr->m_idx);
    handle_stuff(player_ptr);
    describe_grid_monster(player_ptr, eg_ptr);
    if ((eg_ptr->query != '\r') && (eg_ptr->query != '\n') && (eg_ptr->query != ' ') && (eg_ptr->query != 'x'))
        return eg_ptr->query;

    if ((eg_ptr->query == ' ') && !(eg_ptr->mode & TARGET_LOOK))
        return eg_ptr->query;

    describe_monster_person(eg_ptr);
    uint16_t monster_item_description = describe_monster_item(player_ptr, eg_ptr);
    if (within_char_util(monster_item_description))
        return (char)monster_item_description;

#ifdef JP
    eg_ptr->s2 = "の上";
    eg_ptr->s3 = "にいる";
#else
    eg_ptr->s2 = "on ";
#endif
    return CONTINUOUS_DESCRIPTION;
}

static int16_t describe_footing(player_type *player_ptr, eg_type *eg_ptr)
{
    if (eg_ptr->floor_num != 1)
        return CONTINUOUS_DESCRIPTION;

    GAME_TEXT o_name[MAX_NLEN];
    object_type *o_ptr;
    o_ptr = &player_ptr->current_floor_ptr->o_list[eg_ptr->floor_list[0]];
    describe_flavor(player_ptr, o_name, o_ptr, 0);
#ifdef JP
    sprintf(eg_ptr->out_val, "%s%s%s%s[%s]", eg_ptr->s1, o_name, eg_ptr->s2, eg_ptr->s3, eg_ptr->info);
#else
    sprintf(eg_ptr->out_val, "%s%s%s%s [%s]", eg_ptr->s1, eg_ptr->s2, eg_ptr->s3, o_name, eg_ptr->info);
#endif
    prt(eg_ptr->out_val, 0, 0);
    move_cursor_relative(eg_ptr->y, eg_ptr->x);
    eg_ptr->query = inkey();
    return eg_ptr->query;
}

static int16_t describe_footing_items(eg_type *eg_ptr)
{
    if (!eg_ptr->boring)
        return CONTINUOUS_DESCRIPTION;

#ifdef JP
    sprintf(eg_ptr->out_val, "%s %d個のアイテム%s%s ['x'で一覧, %s]", eg_ptr->s1, (int)eg_ptr->floor_num, eg_ptr->s2, eg_ptr->s3, eg_ptr->info);
#else
    sprintf(eg_ptr->out_val, "%s%s%sa pile of %d items [x,%s]", eg_ptr->s1, eg_ptr->s2, eg_ptr->s3, (int)eg_ptr->floor_num, eg_ptr->info);
#endif
    prt(eg_ptr->out_val, 0, 0);
    move_cursor_relative(eg_ptr->y, eg_ptr->x);
    eg_ptr->query = inkey();
    if (eg_ptr->query != 'x' && eg_ptr->query != ' ')
        return eg_ptr->query;

    return CONTINUOUS_DESCRIPTION;
}

static char describe_footing_many_items(player_type *player_ptr, eg_type *eg_ptr, int *min_width)
{
    while (true) {
        screen_save();
        show_gold_on_floor = true;
        (void)show_floor_items(player_ptr, 0, eg_ptr->y, eg_ptr->x, min_width, AllMatchItemTester());
        show_gold_on_floor = false;
#ifdef JP
        sprintf(eg_ptr->out_val, "%s %d個のアイテム%s%s [Enterで次へ, %s]", eg_ptr->s1, (int)eg_ptr->floor_num, eg_ptr->s2, eg_ptr->s3, eg_ptr->info);
#else
        sprintf(eg_ptr->out_val, "%s%s%sa pile of %d items [Enter,%s]", eg_ptr->s1, eg_ptr->s2, eg_ptr->s3, (int)eg_ptr->floor_num, eg_ptr->info);
#endif
        prt(eg_ptr->out_val, 0, 0);
        eg_ptr->query = inkey();
        screen_load();
        if (eg_ptr->query != '\n' && eg_ptr->query != '\r')
            return eg_ptr->query;

        if (eg_ptr->g_ptr->o_idx_list.size() < 2)
            continue;

        eg_ptr->g_ptr->o_idx_list.rotate(player_ptr->current_floor_ptr);

        // ターゲットしている床の座標を渡す必要があるので、window_stuff経由ではなく直接呼び出す
        fix_floor_item_list(player_ptr, eg_ptr->y, eg_ptr->x);
    }
}

static int16_t loop_describing_grid(player_type *player_ptr, eg_type *eg_ptr)
{
    if (eg_ptr->floor_num == 0)
        return CONTINUOUS_DESCRIPTION;

    int min_width = 0;
    while (true) {
        int16_t footing_description = describe_footing(player_ptr, eg_ptr);
        if (within_char_util(footing_description))
            return (char)footing_description;

        int16_t footing_descriptions = describe_footing_items(eg_ptr);
        if (within_char_util(footing_descriptions))
            return (char)footing_descriptions;

        return describe_footing_many_items(player_ptr, eg_ptr, &min_width);
    }
}

static int16_t describe_footing_sight(player_type *player_ptr, eg_type *eg_ptr, object_type *o_ptr)
{
    if ((o_ptr->marked & OM_FOUND) == 0)
        return CONTINUOUS_DESCRIPTION;

    GAME_TEXT o_name[MAX_NLEN];
    eg_ptr->boring = false;
    describe_flavor(player_ptr, o_name, o_ptr, 0);
#ifdef JP
    sprintf(eg_ptr->out_val, "%s%s%s%s[%s]", eg_ptr->s1, o_name, eg_ptr->s2, eg_ptr->s3, eg_ptr->info);
#else
    sprintf(eg_ptr->out_val, "%s%s%s%s [%s]", eg_ptr->s1, eg_ptr->s2, eg_ptr->s3, o_name, eg_ptr->info);
#endif
    prt(eg_ptr->out_val, 0, 0);
    move_cursor_relative(eg_ptr->y, eg_ptr->x);
    eg_ptr->query = inkey();
    if ((eg_ptr->query != '\r') && (eg_ptr->query != '\n') && (eg_ptr->query != ' ') && (eg_ptr->query != 'x'))
        return eg_ptr->query;

    if ((eg_ptr->query == ' ') && !(eg_ptr->mode & TARGET_LOOK))
        return eg_ptr->query;

    eg_ptr->s1 = _("それは", "It is ");
    if (o_ptr->number != 1)
        eg_ptr->s1 = _("それらは", "They are ");

#ifdef JP
    eg_ptr->s2 = "の上";
    eg_ptr->s3 = "に見える";
#else
    eg_ptr->s2 = "on ";
#endif
    return CONTINUOUS_DESCRIPTION;
}

static int16_t sweep_footing_items(player_type *player_ptr, eg_type *eg_ptr)
{
    for (const auto this_o_idx : eg_ptr->g_ptr->o_idx_list) {
        object_type *o_ptr;
        o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
        int16_t ret = describe_footing_sight(player_ptr, eg_ptr, o_ptr);
        if (within_char_util(ret))
            return (char)ret;
    }

    return CONTINUOUS_DESCRIPTION;
}

static concptr decide_target_floor(player_type *player_ptr, eg_type *eg_ptr)
{
    if (eg_ptr->f_ptr->flags.has(FF::QUEST_ENTER)) {
        QUEST_IDX old_quest = player_ptr->current_floor_ptr->inside_quest;
        for (int j = 0; j < 10; j++)
            quest_text[j][0] = '\0';

        quest_text_line = 0;
        player_ptr->current_floor_ptr->inside_quest = eg_ptr->g_ptr->special;
        init_flags = INIT_NAME_ONLY;
        parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
        player_ptr->current_floor_ptr->inside_quest = old_quest;
        return format(
            _("クエスト「%s」(%d階相当)", "the entrance to the quest '%s'(level %d)"), quest[eg_ptr->g_ptr->special].name, quest[eg_ptr->g_ptr->special].level);
    }

    if (eg_ptr->f_ptr->flags.has(FF::BLDG) && !player_ptr->current_floor_ptr->inside_arena)
        return building[eg_ptr->f_ptr->subtype].name;

    if (eg_ptr->f_ptr->flags.has(FF::ENTRANCE))
        return format(_("%s(%d階相当)", "%s(level %d)"), d_info[eg_ptr->g_ptr->special].text.c_str(), d_info[eg_ptr->g_ptr->special].mindepth);

    if (eg_ptr->f_ptr->flags.has(FF::TOWN))
        return town_info[eg_ptr->g_ptr->special].name;

    if (player_ptr->wild_mode && (eg_ptr->feat == feat_floor))
        return _("道", "road");

    return eg_ptr->f_ptr->name.c_str();
}

static void describe_grid_monster_all(eg_type *eg_ptr)
{
    if (!w_ptr->wizard) {
#ifdef JP
        sprintf(eg_ptr->out_val, "%s%s%s%s[%s]", eg_ptr->s1, eg_ptr->name, eg_ptr->s2, eg_ptr->s3, eg_ptr->info);
#else
        sprintf(eg_ptr->out_val, "%s%s%s%s [%s]", eg_ptr->s1, eg_ptr->s2, eg_ptr->s3, eg_ptr->name, eg_ptr->info);
#endif
        return;
    }

    char f_idx_str[32];
    if (eg_ptr->g_ptr->mimic)
        sprintf(f_idx_str, "%d/%d", eg_ptr->g_ptr->feat, eg_ptr->g_ptr->mimic);
    else
        sprintf(f_idx_str, "%d", eg_ptr->g_ptr->feat);

#ifdef JP
    sprintf(eg_ptr->out_val, "%s%s%s%s[%s] %x %s %d %d %d (%d,%d) %d", eg_ptr->s1, eg_ptr->name, eg_ptr->s2, eg_ptr->s3, eg_ptr->info,
        (uint)eg_ptr->g_ptr->info, f_idx_str, eg_ptr->g_ptr->dists[FLOW_NORMAL], eg_ptr->g_ptr->costs[FLOW_NORMAL], eg_ptr->g_ptr->when, (int)eg_ptr->y,
        (int)eg_ptr->x, travel.cost[eg_ptr->y][eg_ptr->x]);
#else
    sprintf(eg_ptr->out_val, "%s%s%s%s [%s] %x %s %d %d %d (%d,%d)", eg_ptr->s1, eg_ptr->s2, eg_ptr->s3, eg_ptr->name, eg_ptr->info, eg_ptr->g_ptr->info,
        f_idx_str, eg_ptr->g_ptr->dists[FLOW_NORMAL], eg_ptr->g_ptr->costs[FLOW_NORMAL], eg_ptr->g_ptr->when, (int)eg_ptr->y, (int)eg_ptr->x);
#endif
}

/*!
 * @brief xまたはlで指定したグリッドにあるアイテムやモンスターの説明を記述する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 指定グリッドのY座標
 * @param x 指定グリッドのX座標
 * @param mode x (KILL)かl (LOOK)
 * @param info 記述用文字列
 * @return 入力キー
 * @todo xとlで処理を分ける？
 */
char examine_grid(player_type *player_ptr, const POSITION y, const POSITION x, target_type mode, concptr info)
{
    eg_type tmp_eg;
    eg_type *eg_ptr = initialize_eg_type(player_ptr, &tmp_eg, y, x, mode, info);
    describe_scan_result(player_ptr, eg_ptr);
    describe_target(player_ptr, eg_ptr);
    process_result next_target = describe_hallucinated_target(player_ptr, eg_ptr);
    switch (next_target) {
    case PROCESS_FALSE:
        return 0;
    case PROCESS_TRUE:
        return eg_ptr->query;
    case PROCESS_CONTINUE:
        break;
    }

    int16_t description_grid = describe_grid(player_ptr, eg_ptr);
    if (within_char_util(description_grid))
        return (char)description_grid;

    int16_t loop_description = loop_describing_grid(player_ptr, eg_ptr);
    if (within_char_util(loop_description))
        return (char)loop_description;

    int16_t footing_items_description = sweep_footing_items(player_ptr, eg_ptr);
    if (within_char_util(footing_items_description))
        return (char)footing_items_description;

    eg_ptr->feat = eg_ptr->g_ptr->get_feat_mimic();
    if (!eg_ptr->g_ptr->is_mark() && !player_can_see_bold(player_ptr, y, x))
        eg_ptr->feat = feat_none;

    eg_ptr->f_ptr = &f_info[eg_ptr->feat];
    if (!eg_ptr->boring && eg_ptr->f_ptr->flags.has_not(FF::REMEMBER))
        return (eg_ptr->query != '\r') && (eg_ptr->query != '\n') ? eg_ptr->query : 0;

    /*
     * グローバル変数への代入をここで行っているので動かしたくない
     * 安全を確保できたら構造体から外すことも検討する
     */
    eg_ptr->name = decide_target_floor(player_ptr, eg_ptr);
    if (*eg_ptr->s2
        && (eg_ptr->f_ptr->flags.has_none_of({FF::MOVE, FF::CAN_FLY})
            || eg_ptr->f_ptr->flags.has_none_of({FF::LOS, FF::TREE}) || eg_ptr->f_ptr->flags.has(FF::TOWN))) {
        eg_ptr->s2 = _("の中", "in ");
    }

    if (eg_ptr->f_ptr->flags.has(FF::STORE) || eg_ptr->f_ptr->flags.has(FF::QUEST_ENTER)
        || (eg_ptr->f_ptr->flags.has(FF::BLDG) && !player_ptr->current_floor_ptr->inside_arena) || eg_ptr->f_ptr->flags.has(FF::ENTRANCE))
        eg_ptr->s2 = _("の入口", "");
#ifdef JP
#else
    else if (eg_ptr->f_ptr->flags.has(FF::FLOOR) || eg_ptr->f_ptr->flags.has(FF::TOWN) || eg_ptr->f_ptr->flags.has(FF::SHALLOW)
        || eg_ptr->f_ptr->flags.has(FF::DEEP))
        eg_ptr->s3 = "";
    else
        eg_ptr->s3 = (is_a_vowel(eg_ptr->name[0])) ? "an " : "a ";
#endif

    describe_grid_monster_all(eg_ptr);
    prt(eg_ptr->out_val, 0, 0);
    move_cursor_relative(y, x);
    eg_ptr->query = inkey();
    if ((eg_ptr->query != '\r') && (eg_ptr->query != '\n'))
        return eg_ptr->query;

    return 0;
}
