#include "target/target-describer.h"
#include "action/travel-execution.h"
#include "core/stuff-handler.h"
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
#include "locale/english.h"
#include "lore/lore-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "object/item-tester-hooker.h"
#include "object/object-mark-types.h"
#include "player-base/player-race.h"
#include "player/player-status-table.h"
#include "system/building-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "system/terrain-type-definition.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-lore.h"
#include "view/display-messages.h"
#include "view/display-monster-status.h"
#include "window/display-sub-windows.h"
#include "world/world.h"

namespace {
constexpr short CONTINUOUS_DESCRIPTION = 256;

class GridExamination {
public:
    GridExamination(FloorType &floor, POSITION y, POSITION x, target_type mode, concptr info);
    POSITION y;
    POSITION x;
    target_type mode;
    bool boring = true;
    concptr info;
    concptr s1 = "";
    concptr s2 = "";
    concptr s3 = "";
    concptr x_info = "";
    char query = '\001';
    char out_val[MAX_NLEN + 80]{};
    OBJECT_IDX floor_list[23]{};
    ITEM_NUMBER floor_num = 0;
    grid_type *g_ptr;
    MonsterEntity *m_ptr;
    OBJECT_IDX next_o_idx = 0;
    FEAT_IDX feat = 0;
    TerrainType *f_ptr = nullptr;
    std::string name = "";
};

GridExamination::GridExamination(FloorType &floor, POSITION y, POSITION x, target_type mode, concptr info)
    : y(y)
    , x(x)
    , mode(mode)
    , info(info)
{
    this->g_ptr = &floor.grid_array[y][x];
    this->m_ptr = &floor.m_list[this->g_ptr->m_idx];
    this->next_o_idx = 0;
}
}

bool show_gold_on_floor = false;

/*
 * Evaluate number of kill needed to gain level
 */
static std::string evaluate_monster_exp(PlayerType *player_ptr, MonsterEntity *m_ptr)
{
    MonsterRaceInfo *ap_r_ptr = &monraces_info[m_ptr->ap_r_idx];
    if ((player_ptr->lev >= PY_MAX_LEVEL) || PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return "**";
    }

    if (!ap_r_ptr->r_tkills || m_ptr->mflag2.has(MonsterConstantFlagType::KAGE)) {
        if (!w_ptr->wizard) {
            return "??";
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

    auto num = std::min<uint>(999, exp_adv_frac);
    return format("%03ld", (long int)num);
}

static void describe_scan_result(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    if (!easy_floor) {
        return;
    }

    ge_ptr->floor_num = scan_floor_items(player_ptr, ge_ptr->floor_list, ge_ptr->y, ge_ptr->x, SCAN_FLOOR_ONLY_MARKED, AllMatchItemTester());
    if (ge_ptr->floor_num > 0) {
        ge_ptr->x_info = _("x物 ", "x,");
    }
}

static void describe_target(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    if (!player_bold(player_ptr, ge_ptr->y, ge_ptr->x)) {
        ge_ptr->s1 = _("ターゲット:", "Target:");
        return;
    }

#ifdef JP
    ge_ptr->s1 = "あなたは";
    ge_ptr->s2 = "の上";
    ge_ptr->s3 = "にいる";
#else
    ge_ptr->s1 = "You are ";
    ge_ptr->s2 = "on ";
#endif
}

static ProcessResult describe_hallucinated_target(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    if (!player_ptr->effects()->hallucination()->is_hallucinated()) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    concptr name = _("何か奇妙な物", "something strange");
#ifdef JP
    strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s [%s]", ge_ptr->s1, name, ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
    strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s [%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, name, ge_ptr->info);
#endif
    prt(ge_ptr->out_val, 0, 0);
    move_cursor_relative(ge_ptr->y, ge_ptr->x);
    ge_ptr->query = inkey();
    if ((ge_ptr->query != '\r') && (ge_ptr->query != '\n')) {
        return ProcessResult::PROCESS_TRUE;
    }

    return ProcessResult::PROCESS_FALSE;
}

static bool describe_grid_lore(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    screen_save();
    screen_roff(player_ptr, ge_ptr->m_ptr->ap_r_idx, MONSTER_LORE_NORMAL);
    term_addstr(-1, TERM_WHITE, format(_("  [r思 %s%s]", "  [r,%s%s]"), ge_ptr->x_info, ge_ptr->info));
    ge_ptr->query = inkey();
    screen_load();
    return ge_ptr->query != 'r';
}

static void describe_grid_monster(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    bool recall = false;
    const auto m_name = monster_desc(player_ptr, ge_ptr->m_ptr, MD_INDEF_VISIBLE);
    while (true) {
        if (recall) {
            if (describe_grid_lore(player_ptr, ge_ptr)) {
                return;
            }

            recall = false;
            continue;
        }

        std::string acount = evaluate_monster_exp(player_ptr, ge_ptr->m_ptr);
        const auto mon_desc = look_mon_desc(ge_ptr->m_ptr, 0x01);
#ifdef JP
        strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "[%s]%s%s(%s)%s%s [r思 %s%s]", acount.data(), ge_ptr->s1, m_name.data(), mon_desc.data(), ge_ptr->s2, ge_ptr->s3,
            ge_ptr->x_info, ge_ptr->info);
#else
        strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "[%s]%s%s%s%s(%s) [r, %s%s]", acount.data(), ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, m_name.data(), mon_desc.data(),
            ge_ptr->x_info, ge_ptr->info);
#endif
        prt(ge_ptr->out_val, 0, 0);
        move_cursor_relative(ge_ptr->y, ge_ptr->x);
        ge_ptr->query = inkey();
        if (ge_ptr->query != 'r') {
            return;
        }

        recall = true;
    }
}

static void describe_monster_person(GridExamination *ge_ptr)
{
    MonsterRaceInfo *ap_r_ptr = &monraces_info[ge_ptr->m_ptr->ap_r_idx];
    ge_ptr->s1 = _("それは", "It is ");
    if (ap_r_ptr->flags1 & RF1_FEMALE) {
        ge_ptr->s1 = _("彼女は", "She is ");
    } else if (ap_r_ptr->flags1 & RF1_MALE) {
        ge_ptr->s1 = _("彼は", "He is ");
    }

#ifdef JP
    ge_ptr->s2 = "を";
    ge_ptr->s3 = "持っている";
#else
    ge_ptr->s2 = "carrying ";
#endif
}

static short describe_monster_item(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    for (const auto this_o_idx : ge_ptr->m_ptr->hold_o_idx_list) {
        auto *o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
        const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
#ifdef JP
        strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s[%s]", ge_ptr->s1, item_name.data(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
        strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s [%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, item_name.data(), ge_ptr->info);
#endif
        prt(ge_ptr->out_val, 0, 0);
        move_cursor_relative(ge_ptr->y, ge_ptr->x);
        ge_ptr->query = inkey();
        if ((ge_ptr->query != '\r') && (ge_ptr->query != '\n') && (ge_ptr->query != ' ') && (ge_ptr->query != 'x')) {
            return ge_ptr->query;
        }

        if ((ge_ptr->query == ' ') && !(ge_ptr->mode & TARGET_LOOK)) {
            return ge_ptr->query;
        }

        ge_ptr->s2 = _("をまた", "also carrying ");
    }

    return CONTINUOUS_DESCRIPTION;
}

static bool within_char_util(const short input)
{
    return (input > -127) && (input < 128);
}

static short describe_grid(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    if ((ge_ptr->g_ptr->m_idx == 0) || !player_ptr->current_floor_ptr->m_list[ge_ptr->g_ptr->m_idx].ml) {
        return CONTINUOUS_DESCRIPTION;
    }

    ge_ptr->boring = false;
    monster_race_track(player_ptr, ge_ptr->m_ptr->ap_r_idx);
    health_track(player_ptr, ge_ptr->g_ptr->m_idx);
    handle_stuff(player_ptr);
    describe_grid_monster(player_ptr, ge_ptr);
    if ((ge_ptr->query != '\r') && (ge_ptr->query != '\n') && (ge_ptr->query != ' ') && (ge_ptr->query != 'x')) {
        return ge_ptr->query;
    }

    if ((ge_ptr->query == ' ') && !(ge_ptr->mode & TARGET_LOOK)) {
        return ge_ptr->query;
    }

    describe_monster_person(ge_ptr);
    const auto monster_item_description = describe_monster_item(player_ptr, ge_ptr);
    if (within_char_util(monster_item_description)) {
        return (char)monster_item_description;
    }

#ifdef JP
    ge_ptr->s2 = "の上";
    ge_ptr->s3 = "にいる";
#else
    ge_ptr->s2 = "on ";
#endif
    return CONTINUOUS_DESCRIPTION;
}

static short describe_footing(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    if (ge_ptr->floor_num != 1) {
        return CONTINUOUS_DESCRIPTION;
    }

    auto *o_ptr = &player_ptr->current_floor_ptr->o_list[ge_ptr->floor_list[0]];
    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
#ifdef JP
    strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s[%s]", ge_ptr->s1, item_name.data(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
    strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s [%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, item_name.data(), ge_ptr->info);
#endif
    prt(ge_ptr->out_val, 0, 0);
    move_cursor_relative(ge_ptr->y, ge_ptr->x);
    ge_ptr->query = inkey();
    return ge_ptr->query;
}

static short describe_footing_items(GridExamination *ge_ptr)
{
    if (!ge_ptr->boring) {
        return CONTINUOUS_DESCRIPTION;
    }

#ifdef JP
    strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s %d個のアイテム%s%s ['x'で一覧, %s]", ge_ptr->s1, (int)ge_ptr->floor_num, ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
    strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%sa pile of %d items [x,%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, (int)ge_ptr->floor_num, ge_ptr->info);
#endif
    prt(ge_ptr->out_val, 0, 0);
    move_cursor_relative(ge_ptr->y, ge_ptr->x);
    ge_ptr->query = inkey();
    if (ge_ptr->query != 'x' && ge_ptr->query != ' ') {
        return ge_ptr->query;
    }

    return CONTINUOUS_DESCRIPTION;
}

static char describe_footing_many_items(PlayerType *player_ptr, GridExamination *ge_ptr, int *min_width)
{
    while (true) {
        screen_save();
        show_gold_on_floor = true;
        (void)show_floor_items(player_ptr, 0, ge_ptr->y, ge_ptr->x, min_width, AllMatchItemTester());
        show_gold_on_floor = false;
#ifdef JP
        strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s %d個のアイテム%s%s [Enterで次へ, %s]", ge_ptr->s1, (int)ge_ptr->floor_num, ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
        strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%sa pile of %d items [Enter,%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, (int)ge_ptr->floor_num, ge_ptr->info);
#endif
        prt(ge_ptr->out_val, 0, 0);
        ge_ptr->query = inkey();
        screen_load();
        if (ge_ptr->query != '\n' && ge_ptr->query != '\r') {
            return ge_ptr->query;
        }

        if (ge_ptr->g_ptr->o_idx_list.size() < 2) {
            continue;
        }

        ge_ptr->g_ptr->o_idx_list.rotate(player_ptr->current_floor_ptr);

        // ターゲットしている床の座標を渡す必要があるので、window_stuff経由ではなく直接呼び出す
        fix_floor_item_list(player_ptr, ge_ptr->y, ge_ptr->x);
    }
}

static short loop_describing_grid(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    if (ge_ptr->floor_num == 0) {
        return CONTINUOUS_DESCRIPTION;
    }

    auto min_width = 0;
    while (true) {
        const auto footing_description = describe_footing(player_ptr, ge_ptr);
        if (within_char_util(footing_description)) {
            return (char)footing_description;
        }

        const auto footing_descriptions = describe_footing_items(ge_ptr);
        if (within_char_util(footing_descriptions)) {
            return (char)footing_descriptions;
        }

        return describe_footing_many_items(player_ptr, ge_ptr, &min_width);
    }
}

static short describe_footing_sight(PlayerType *player_ptr, GridExamination *ge_ptr, ItemEntity *o_ptr)
{
    if (o_ptr->marked.has_not(OmType::FOUND)) {
        return CONTINUOUS_DESCRIPTION;
    }

    ge_ptr->boring = false;
    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
#ifdef JP
    strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s[%s]", ge_ptr->s1, item_name.data(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
    strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s [%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, item_name.data(), ge_ptr->info);
#endif
    prt(ge_ptr->out_val, 0, 0);
    move_cursor_relative(ge_ptr->y, ge_ptr->x);
    ge_ptr->query = inkey();
    if ((ge_ptr->query != '\r') && (ge_ptr->query != '\n') && (ge_ptr->query != ' ') && (ge_ptr->query != 'x')) {
        return ge_ptr->query;
    }

    if ((ge_ptr->query == ' ') && !(ge_ptr->mode & TARGET_LOOK)) {
        return ge_ptr->query;
    }

    ge_ptr->s1 = _("それは", "It is ");
    if (o_ptr->number != 1) {
        ge_ptr->s1 = _("それらは", "They are ");
    }

#ifdef JP
    ge_ptr->s2 = "の上";
    ge_ptr->s3 = "に見える";
#else
    ge_ptr->s2 = "on ";
#endif
    return CONTINUOUS_DESCRIPTION;
}

static int16_t sweep_footing_items(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    for (const auto this_o_idx : ge_ptr->g_ptr->o_idx_list) {
        auto *o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
        const auto ret = describe_footing_sight(player_ptr, ge_ptr, o_ptr);
        if (within_char_util(ret)) {
            return (char)ret;
        }
    }

    return CONTINUOUS_DESCRIPTION;
}

static std::string decide_target_floor(PlayerType *player_ptr, GridExamination *ge_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (ge_ptr->f_ptr->flags.has(TerrainCharacteristics::QUEST_ENTER)) {
        QuestId old_quest = floor_ptr->quest_number;
        const auto &quest_list = QuestList::get_instance();
        const QuestId number = i2enum<QuestId>(ge_ptr->g_ptr->special);
        const auto *q_ptr = &quest_list[number];
        std::string_view msg(_("クエスト「%s」(%d階相当)", "the entrance to the quest '%s'(level %d)"));
        for (int j = 0; j < 10; j++) {
            quest_text[j][0] = '\0';
        }

        quest_text_line = 0;
        floor_ptr->quest_number = number;
        init_flags = INIT_NAME_ONLY;
        parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
        floor_ptr->quest_number = old_quest;
        return format(msg.data(), q_ptr->name.data(), q_ptr->level);
    }

    if (ge_ptr->f_ptr->flags.has(TerrainCharacteristics::BLDG) && !floor_ptr->inside_arena) {
        return buildings[ge_ptr->f_ptr->subtype].name;
    }

    if (ge_ptr->f_ptr->flags.has(TerrainCharacteristics::ENTRANCE)) {
        const auto &dungeon = dungeons_info[ge_ptr->g_ptr->special];
        return format(_("%s(%d階相当)", "%s(level %d)"), dungeon.text.data(), dungeon.mindepth);
    }

    if (ge_ptr->f_ptr->flags.has(TerrainCharacteristics::TOWN)) {
        return towns_info[ge_ptr->g_ptr->special].name;
    }

    if (player_ptr->wild_mode && (ge_ptr->feat == feat_floor)) {
        return _("道", "road");
    }

    return ge_ptr->f_ptr->name;
}

static void describe_grid_monster_all(GridExamination *ge_ptr)
{
    if (!w_ptr->wizard) {
#ifdef JP
        strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s[%s]", ge_ptr->s1, ge_ptr->name.data(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
        strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s [%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, ge_ptr->name.data(), ge_ptr->info);
#endif
        return;
    }

    std::string f_idx_str;
    if (ge_ptr->g_ptr->mimic) {
        f_idx_str = format("%d/%d", ge_ptr->g_ptr->feat, ge_ptr->g_ptr->mimic);
    } else {
        f_idx_str = std::to_string(ge_ptr->g_ptr->feat);
    }

#ifdef JP
    strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s[%s] %x %s %d %d %d (%d,%d) %d", ge_ptr->s1, ge_ptr->name.data(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info,
        (uint)ge_ptr->g_ptr->info, f_idx_str.data(), ge_ptr->g_ptr->dists[FLOW_NORMAL], ge_ptr->g_ptr->costs[FLOW_NORMAL], ge_ptr->g_ptr->when, (int)ge_ptr->y,
        (int)ge_ptr->x, travel.cost[ge_ptr->y][ge_ptr->x]);
#else
    strnfmt(ge_ptr->out_val, sizeof(ge_ptr->out_val), "%s%s%s%s [%s] %x %s %d %d %d (%d,%d)", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, ge_ptr->name.data(), ge_ptr->info, ge_ptr->g_ptr->info,
        f_idx_str.data(), ge_ptr->g_ptr->dists[FLOW_NORMAL], ge_ptr->g_ptr->costs[FLOW_NORMAL], ge_ptr->g_ptr->when, (int)ge_ptr->y, (int)ge_ptr->x);
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
char examine_grid(PlayerType *player_ptr, const POSITION y, const POSITION x, target_type mode, concptr info)
{
    GridExamination tmp_eg(*player_ptr->current_floor_ptr, y, x, mode, info);
    GridExamination *ge_ptr = &tmp_eg;
    describe_scan_result(player_ptr, ge_ptr);
    describe_target(player_ptr, ge_ptr);
    ProcessResult next_target = describe_hallucinated_target(player_ptr, ge_ptr);
    switch (next_target) {
    case ProcessResult::PROCESS_FALSE:
        return '\0';
    case ProcessResult::PROCESS_TRUE:
        return ge_ptr->query;
    default:
        break;
    }

    const auto description_grid = describe_grid(player_ptr, ge_ptr);
    if (within_char_util(description_grid)) {
        return (char)description_grid;
    }

    const auto loop_description = loop_describing_grid(player_ptr, ge_ptr);
    if (within_char_util(loop_description)) {
        return (char)loop_description;
    }

    const auto footing_items_description = sweep_footing_items(player_ptr, ge_ptr);
    if (within_char_util(footing_items_description)) {
        return (char)footing_items_description;
    }

    ge_ptr->feat = ge_ptr->g_ptr->get_feat_mimic();
    if (!ge_ptr->g_ptr->is_mark() && !player_can_see_bold(player_ptr, y, x)) {
        ge_ptr->feat = feat_none;
    }

    ge_ptr->f_ptr = &terrains_info[ge_ptr->feat];
    if (!ge_ptr->boring && ge_ptr->f_ptr->flags.has_not(TerrainCharacteristics::REMEMBER)) {
        return (ge_ptr->query != '\r') && (ge_ptr->query != '\n') ? ge_ptr->query : 0;
    }

    /*
     * グローバル変数への代入をここで行っているので動かしたくない
     * 安全を確保できたら構造体から外すことも検討する
     */
    ge_ptr->name = decide_target_floor(player_ptr, ge_ptr);
    auto is_in = ge_ptr->f_ptr->flags.has_none_of({ TerrainCharacteristics::MOVE, TerrainCharacteristics::CAN_FLY });
    is_in |= ge_ptr->f_ptr->flags.has_none_of({ TerrainCharacteristics::LOS, TerrainCharacteristics::TREE });
    is_in |= ge_ptr->f_ptr->flags.has(TerrainCharacteristics::TOWN);
    if (*ge_ptr->s2 && is_in) {
        ge_ptr->s2 = _("の中", "in ");
    }

    auto is_entrance = ge_ptr->f_ptr->flags.has(TerrainCharacteristics::STORE);
    is_entrance |= ge_ptr->f_ptr->flags.has(TerrainCharacteristics::QUEST_ENTER);
    is_entrance |= ge_ptr->f_ptr->flags.has(TerrainCharacteristics::BLDG) && !player_ptr->current_floor_ptr->inside_arena;
    is_entrance |= ge_ptr->f_ptr->flags.has(TerrainCharacteristics::ENTRANCE);
    if (is_entrance) {
        ge_ptr->s2 = _("の入口", "");
    }
#ifdef JP
#else
    else {
        auto is_normal_terrain = ge_ptr->f_ptr->flags.has(TerrainCharacteristics::FLOOR);
        is_normal_terrain |= ge_ptr->f_ptr->flags.has(TerrainCharacteristics::TOWN);
        is_normal_terrain |= ge_ptr->f_ptr->flags.has(TerrainCharacteristics::SHALLOW);
        is_normal_terrain |= ge_ptr->f_ptr->flags.has(TerrainCharacteristics::DEEP);
        if (is_normal_terrain) {
            ge_ptr->s3 = "";
        } else {
            ge_ptr->s3 = (is_a_vowel(ge_ptr->name[0])) ? "an " : "a ";
        }
    }
#endif

    describe_grid_monster_all(ge_ptr);
    prt(ge_ptr->out_val, 0, 0);
    move_cursor_relative(y, x);
    ge_ptr->query = inkey();
    if ((ge_ptr->query != '\r') && (ge_ptr->query != '\n')) {
        return ge_ptr->query;
    }

    return 0;
}
