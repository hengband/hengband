/*!
 * @brief ウィザードモードの処理(特別処理中心) / Wizard commands
 * @date 2014/09/07
 * @author
 * Copyright (c) 1997 Ben Harrison, and others<br>
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.<br>
 * 2014 Deskull rearranged comment for Doxygen.<br>
 */

#include "wizard/wizard-special-process.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "birth/inventory-initializer.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-help.h"
#include "cmd-io/cmd-save.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "flavor/object-flavor.h"
#include "floor/floor-leaver.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/option-types-table.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "market/arena.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-kind-hook.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player-info/self-info.h"
#include "player-status/player-energy.h"
#include "player/digestion-processor.h"
#include "player/patron.h"
#include "player/player-skill.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/bad-status-setter.h"
#include "status/experience.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "target/grid-selector.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "wizard/spoiler-table.h"
#include "wizard/tval-descriptions-table.h"
#include "wizard/wizard-spells.h"
#include "wizard/wizard-spoiler.h"
#include "world/world.h"
#include <algorithm>
#include <optional>
#include <sstream>
#include <tuple>
#include <vector>

#define NUM_O_SET 8
#define NUM_O_BIT 32

/*!
 * @brief プレイヤーを完全回復する
 */
void wiz_cure_all(PlayerType *player_ptr)
{
    (void)life_stream(player_ptr, false, false);
    (void)restore_mana(player_ptr, true);
    (void)set_food(player_ptr, PY_FOOD_MAX - 1);
    BadStatusSetter bss(player_ptr);
    (void)bss.set_fear(0);
    (void)bss.set_deceleration(0, false);
    msg_print("You're fully cured by wizard command.");
}

static std::optional<short> wiz_select_tval()
{
    short list;
    char ch;
    for (list = 0; (list < 80) && (tvals[list].tval > ItemKindType::NONE); list++) {
        auto row = 2 + (list % 20);
        auto col = _(32, 24) * (list / 20);
        ch = listsym[list];
        prt(format("[%c] %s", ch, tvals[list].desc), row, col);
    }

    auto max_num = list;
    if (!get_com(_("アイテム種別を選んで下さい", "Get what type of object? "), &ch, false)) {
        return std::nullopt;
    }

    short selection;
    for (selection = 0; selection < max_num; selection++) {
        if (listsym[selection] == ch) {
            break;
        }
    }

    if ((selection < 0) || (selection >= max_num)) {
        return std::nullopt;
    }

    return selection;
}

static short wiz_select_sval(const ItemKindType tval, concptr tval_description)
{
    auto num = 0;
    short choice[80]{};
    char ch;
    for (const auto &baseitem : baseitems_info) {
        if (num >= 80) {
            break;
        }

        if ((baseitem.idx == 0) || baseitem.bi_key.tval() != tval) {
            continue;
        }

        auto row = 2 + (num % 20);
        auto col = _(30, 32) * (num / 20);
        ch = listsym[num];
        const auto buf = strip_name(baseitem.idx);
        prt(format("[%c] %s", ch, buf.data()), row, col);
        choice[num++] = baseitem.idx;
    }

    auto max_num = num;
    if (!get_com(format(_("%s群の具体的なアイテムを選んで下さい", "What Kind of %s? "), tval_description), &ch, false)) {
        return 0;
    }

    short selection;
    for (selection = 0; selection < max_num; selection++) {
        if (listsym[selection] == ch) {
            break;
        }
    }

    if ((selection < 0) || (selection >= max_num)) {
        return 0;
    }

    return choice[selection];
}

/*!
 * @brief ベースアイテムのウィザード生成のために大項目IDと小項目IDを取得する /
 * Specify tval and sval (type and subtype of object) originally
 * @return ベースアイテムID
 * @details
 * by RAK, heavily modified by -Bernd-
 * This function returns the bi_id of an object type, or zero if failed
 * List up to 50 choices in three columns
 */
static short wiz_create_itemtype()
{
    term_clear();
    auto selection = wiz_select_tval();
    if (!selection.has_value()) {
        return 0;
    }

    auto tval = tvals[selection.value()].tval;
    auto tval_description = tvals[selection.value()].desc;
    term_clear();
    return wiz_select_sval(tval, tval_description);
}

/*!
 * @brief 任意のベースアイテム生成のメインルーチン /
 * Wizard routine for creating objects		-RAK-
 * @details
 * Heavily modified to allow magification and artifactification  -Bernd-
 *
 * Note that wizards cannot create objects on top of other objects.
 *
 * Hack -- this routine always makes a "dungeon object", and applies
 * magic to it, and attempts to decline cursed items.
 */
void wiz_create_item(PlayerType *player_ptr)
{
    screen_save();
    const auto bi_id = wiz_create_itemtype();
    screen_load();
    if (bi_id == 0) {
        return;
    }

    const auto &baseitem = baseitems_info[bi_id];
    if (baseitem.gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        for (const auto &[a_idx, a_ref] : artifacts_info) {
            if ((a_idx == FixedArtifactId::NONE) || (a_ref.bi_key != baseitem.bi_key)) {
                continue;
            }

            (void)create_named_art(player_ptr, a_idx, player_ptr->y, player_ptr->x);
            msg_print("Allocated(INSTA_ART).");
            return;
        }
    }

    ItemEntity item;
    item.prep(bi_id);
    ItemMagicApplier(player_ptr, &item, player_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART).execute();
    (void)drop_near(player_ptr, &item, -1, player_ptr->y, player_ptr->x);
    msg_print("Allocated.");
}

/*!
 * @brief 指定したIDの固定アーティファクトの名称を取得する
 *
 * @param a_idx 固定アーティファクトのID
 * @return 固定アーティファクトの名称(Ex. ★ロング・ソード『リンギル』)を保持する std::string オブジェクト
 */
static std::string wiz_make_named_artifact_desc(PlayerType *player_ptr, FixedArtifactId a_idx)
{
    const auto &a_ref = artifacts_info.at(a_idx);
    ItemEntity item;
    item.prep(lookup_baseitem_id(a_ref.bi_key));
    item.fixed_artifact_idx = a_idx;
    object_known(&item);
    char buf[MAX_NLEN];
    describe_flavor(player_ptr, buf, &item, OD_NAME_ONLY);
    return buf;
}

/**
 * @brief 固定アーティファクトをリストから選択する
 *
 * @param a_idx_list 選択する候補となる固定アーティファクトのIDのリスト
 * @return 選択した固定アーティファクトのIDを返す。但しキャンセルした場合は std::nullopt を返す。
 */
static std::optional<FixedArtifactId> wiz_select_named_artifact(PlayerType *player_ptr, const std::vector<FixedArtifactId> &a_idx_list)
{
    constexpr auto MAX_PER_PAGE = 20UL;
    const auto page_max = (a_idx_list.size() - 1) / MAX_PER_PAGE + 1;
    auto current_page = 0UL;

    screen_save();

    std::optional<FixedArtifactId> selected_a_idx;

    while (!selected_a_idx.has_value()) {
        const auto page_base_idx = current_page * MAX_PER_PAGE;
        for (auto i = 0U; i < MAX_PER_PAGE + 1; ++i) {
            term_erase(14, i + 1, 255);
        }
        const auto page_item_count = std::min(MAX_PER_PAGE, a_idx_list.size() - page_base_idx);
        for (auto i = 0U; i < page_item_count; ++i) {
            std::stringstream ss;
            ss << I2A(i) << ") " << wiz_make_named_artifact_desc(player_ptr, a_idx_list[page_base_idx + i]);
            put_str(ss.str().data(), i + 1, 15);
        }
        if (page_max > 1) {
            put_str(format("-- more (%lu/%lu) --", current_page + 1, page_max), page_item_count + 1, 15);
        }

        char cmd = ESCAPE;
        get_com("Which artifact: ", &cmd, false);
        switch (cmd) {
        case ESCAPE:
            screen_load();
            return selected_a_idx;
        case ' ':
            current_page++;
            if (current_page >= page_max) {
                current_page = 0;
            }
            break;
        default:
            const auto select_idx = A2I(cmd) + page_base_idx;
            if (select_idx < a_idx_list.size()) {
                selected_a_idx = a_idx_list[select_idx];
            }
            break;
        }
    }

    screen_load();

    return selected_a_idx;
}

/**
 * @brief 指定したカテゴリの固定アーティファクトのIDのリストを得る
 *
 * @param group_artifact 固定アーティファクトのカテゴリ
 * @return 該当のカテゴリの固定アーティファクトのIDのリスト
 */
static std::vector<FixedArtifactId> wiz_collect_group_a_idx(const grouper &group_artifact)
{
    const auto &[tval_list, name] = group_artifact;
    std::vector<FixedArtifactId> a_idx_list;
    for (auto tval : tval_list) {
        for (const auto &[a_idx, a_ref] : artifacts_info) {
            if (a_ref.bi_key.tval() == tval) {
                a_idx_list.push_back(a_idx);
            }
        }
    }

    return a_idx_list;
}

/*!
 * @brief 固定アーティファクトを生成する / Create the artifact
 */
void wiz_create_named_art(PlayerType *player_ptr)
{
    screen_save();
    for (auto i = 0U; i < group_artifact_list.size(); ++i) {
        const auto &[tval_lit, name] = group_artifact_list[i];
        std::stringstream ss;
        ss << I2A(i) << ") " << name;
        term_erase(14, i + 1, 255);
        put_str(ss.str().data(), i + 1, 15);
    }

    std::optional<FixedArtifactId> create_a_idx;
    while (!create_a_idx.has_value()) {
        char cmd = ESCAPE;
        get_com("Kind of artifact: ", &cmd, false);
        switch (cmd) {
        case ESCAPE:
            screen_load();
            return;
        default:
            if (auto idx = A2I(cmd); idx < group_artifact_list.size()) {
                const auto &a_idx_list = wiz_collect_group_a_idx(group_artifact_list[idx]);
                create_a_idx = wiz_select_named_artifact(player_ptr, a_idx_list);
            }

            break;
        }
    }

    screen_load();
    const auto a_idx = create_a_idx.value();
    const auto it = artifacts_info.find(a_idx);
    if (it == artifacts_info.end()) {
        msg_print("The specified artifact is obsoleted for now.");
        return;
    }

    auto &a_ref = it->second;
    if (a_ref.is_generated) {
        msg_print("It's already allocated.");
        return;
    }

    (void)create_named_art(player_ptr, a_idx, player_ptr->y, player_ptr->x);
    msg_print("Allocated.");
}

/*!
 * @brief プレイヤーの現能力値を調整する / Change various "permanent" player variables.
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wiz_change_status(PlayerType *player_ptr)
{
    int tmp_int;
    char tmp_val[160];
    char ppp[80];
    for (int i = 0; i < A_MAX; i++) {
        strnfmt(ppp, sizeof(ppp), "%s (3-%d): ", stat_names[i], player_ptr->stat_max_max[i]);
        strnfmt(tmp_val, sizeof(tmp_val), "%d", player_ptr->stat_max[i]);
        if (!get_string(ppp, tmp_val, 3)) {
            return;
        }

        tmp_int = atoi(tmp_val);
        if (tmp_int > player_ptr->stat_max_max[i]) {
            tmp_int = player_ptr->stat_max_max[i];
        } else if (tmp_int < 3) {
            tmp_int = 3;
        }

        player_ptr->stat_cur[i] = player_ptr->stat_max[i] = (BASE_STATUS)tmp_int;
    }

    strnfmt(tmp_val, sizeof(tmp_val), "%d", PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER));
    if (!get_string(_("熟練度: ", "Proficiency: "), tmp_val, 4)) {
        return;
    }

    auto tmp_s16b = std::clamp(static_cast<SUB_EXP>(atoi(tmp_val)),
        PlayerSkill::weapon_exp_at(PlayerSkillRank::UNSKILLED),
        PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER));

    for (auto tval : TV_WEAPON_RANGE) {
        for (int i = 0; i < 64; i++) {
            player_ptr->weapon_exp[tval][i] = tmp_s16b;
        }
    }
    PlayerSkill(player_ptr).limit_weapon_skills_by_max_value();

    for (auto j : PLAYER_SKILL_KIND_TYPE_RANGE) {
        player_ptr->skill_exp[j] = tmp_s16b;
        auto short_pclass = enum2i(player_ptr->pclass);
        if (player_ptr->skill_exp[j] > class_skills_info[short_pclass].s_max[j]) {
            player_ptr->skill_exp[j] = class_skills_info[short_pclass].s_max[j];
        }
    }

    int k;
    for (k = 0; k < 32; k++) {
        player_ptr->spell_exp[k] = std::min(PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER), tmp_s16b);
    }

    for (; k < 64; k++) {
        player_ptr->spell_exp[k] = std::min(PlayerSkill::spell_exp_at(PlayerSkillRank::EXPERT), tmp_s16b);
    }

    strnfmt(tmp_val, sizeof(tmp_val), "%ld", (long)(player_ptr->au));
    if (!get_string("Gold: ", tmp_val, 9)) {
        return;
    }

    long tmp_long = atol(tmp_val);
    if (tmp_long < 0) {
        tmp_long = 0L;
    }

    player_ptr->au = tmp_long;
    strnfmt(tmp_val, sizeof(tmp_val), "%ld", (long)(player_ptr->max_exp));
    if (!get_string("Experience: ", tmp_val, 9)) {
        return;
    }

    tmp_long = atol(tmp_val);
    if (tmp_long < 0) {
        tmp_long = 0L;
    }

    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return;
    }

    player_ptr->max_exp = tmp_long;
    player_ptr->exp = tmp_long;
    check_experience(player_ptr);
    do_cmd_redraw(player_ptr);
}

/*!
 * @brief 指定された地点の地形IDを変更する /
 * Create desired feature
 * @param creaturer_ptr プレイヤーへの参照ポインタ
 */
void wiz_create_feature(PlayerType *player_ptr)
{
    int f_val1, f_val2;
    POSITION y, x;
    if (!tgt_pt(player_ptr, &x, &y)) {
        return;
    }

    grid_type *g_ptr;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];

    f_val1 = g_ptr->feat;
    if (!get_value(_("実地形ID", "FeatureID"), 0, terrains_info.size() - 1, &f_val1)) {
        return;
    }

    f_val2 = f_val1;
    if (!get_value(_("偽装地形ID", "FeatureID"), 0, terrains_info.size() - 1, &f_val2)) {
        return;
    }

    cave_set_feat(player_ptr, y, x, static_cast<FEAT_IDX>(f_val1));
    g_ptr->mimic = (int16_t)f_val2;
    TerrainType *f_ptr;
    f_ptr = &terrains_info[g_ptr->get_feat_mimic()];

    if (f_ptr->flags.has(TerrainCharacteristics::RUNE_PROTECTION) || f_ptr->flags.has(TerrainCharacteristics::RUNE_EXPLOSION)) {
        g_ptr->info |= CAVE_OBJECT;
    } else if (f_ptr->flags.has(TerrainCharacteristics::MIRROR)) {
        g_ptr->info |= CAVE_GLOW | CAVE_OBJECT;
    }

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
    player_ptr->update |= PU_FLOW;
}

/*!
 * @brief デバッグ帰還のダンジョンを選ぶ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 範囲外の値が選択されたら再入力を促す
 */
static bool select_debugging_dungeon(PlayerType *player_ptr, DUNGEON_IDX *dungeon_type)
{
    if (command_arg > 0) {
        return true;
    }

    while (true) {
        char tmp_val[160];
        strnfmt(tmp_val, sizeof(tmp_val), "%d", player_ptr->dungeon_idx);
        if (!get_string("Jump which dungeon : ", tmp_val, 2)) {
            return false;
        }

        *dungeon_type = (DUNGEON_IDX)atoi(tmp_val);
        if ((*dungeon_type < DUNGEON_ANGBAND) || (*dungeon_type > DUNGEON_MAX)) {
            msg_print("Invalid dungeon. Please re-input.");
            continue;
        }

        return true;
    }
}

/*
 * @brief 選択したダンジョンの任意フロアを選択する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dungeon_type ダンジョン番号
 * @return フロアを選択したらtrue、キャンセルならfalse
 * @details 0を指定すると地上に飛ぶが、元いた場所にしか飛ばない
 * @todo 可能ならダンジョンの入口 (例：ルルイエなら大洋の真ん中)へ飛べるようにしたい
 */
static bool select_debugging_floor(PlayerType *player_ptr, int dungeon_type)
{
    auto max_depth = dungeons_info[dungeon_type].maxdepth;
    if ((max_depth == 0) || (dungeon_type > static_cast<int>(dungeons_info.size()))) {
        dungeon_type = DUNGEON_ANGBAND;
    }

    auto min_depth = (int)dungeons_info[dungeon_type].mindepth;
    while (true) {
        char ppp[80];
        char tmp_val[160];
        strnfmt(ppp, sizeof(ppp), "Jump to level (0, %d-%d): ", min_depth, max_depth);
        strnfmt(tmp_val, sizeof(tmp_val), "%d", (int)player_ptr->current_floor_ptr->dun_level);
        if (!get_string(ppp, tmp_val, 10)) {
            return false;
        }

        auto tmp_command_arg = (COMMAND_ARG)atoi(tmp_val);
        if (tmp_command_arg == 0) {
            command_arg = tmp_command_arg;
            break;
        }

        auto is_valid_floor = tmp_command_arg > 0;
        is_valid_floor &= tmp_command_arg >= min_depth;
        is_valid_floor &= tmp_command_arg <= max_depth;
        if (is_valid_floor) {
            command_arg = tmp_command_arg;
            break;
        }

        msg_print("Invalid floor. Please re-input.");
        continue;
    }

    return true;
}

/*!
 * @brief 任意のダンジョン及び階層に飛ぶ
 * Go to any level
 */
static void wiz_jump_floor(PlayerType *player_ptr, DUNGEON_IDX dun_idx, DEPTH depth)
{
    player_ptr->dungeon_idx = dun_idx;
    auto &floor_ref = *player_ptr->current_floor_ptr;
    floor_ref.dun_level = depth;
    prepare_change_floor_mode(player_ptr, CFM_RAND_PLACE);
    if (!floor_ref.is_in_dungeon()) {
        player_ptr->dungeon_idx = 0;
    }

    floor_ref.inside_arena = false;
    player_ptr->wild_mode = false;
    leave_quest_check(player_ptr);
    if (record_stair) {
        exe_write_diary(player_ptr, DIARY_WIZ_TELE, 0, nullptr);
    }

    floor_ref.quest_number = QuestId::NONE;
    PlayerEnergy(player_ptr).reset_player_turn();
    player_ptr->energy_need = 0;
    prepare_change_floor_mode(player_ptr, CFM_FIRST_FLOOR);
    player_ptr->leaving = true;
}

/*!
 * @brief 任意のダンジョン及び階層に飛ぶtための選択処理
 * Go to any level
 */
void wiz_jump_to_dungeon(PlayerType *player_ptr)
{
    DUNGEON_IDX dungeon_type = 1;
    if (!select_debugging_dungeon(player_ptr, &dungeon_type)) {
        return;
    }

    if (!select_debugging_floor(player_ptr, dungeon_type)) {
        return;
    }

    if (command_arg < dungeons_info[dungeon_type].mindepth) {
        command_arg = 0;
    }

    if (command_arg > dungeons_info[dungeon_type].maxdepth) {
        command_arg = (COMMAND_ARG)dungeons_info[dungeon_type].maxdepth;
    }

    msg_format("You jump to dungeon level %d.", command_arg);
    if (autosave_l) {
        do_cmd_save_game(player_ptr, true);
    }

    wiz_jump_floor(player_ptr, dungeon_type, command_arg);
}

/*!
 * @brief 全ベースアイテムを鑑定済みにする /
 * Become aware of a lot of objects
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wiz_learn_items_all(PlayerType *player_ptr)
{
    ItemEntity forge;
    ItemEntity *q_ptr;
    for (const auto &baseitem : baseitems_info) {
        if (baseitem.idx > 0 && baseitem.level <= command_arg) {
            q_ptr = &forge;
            q_ptr->prep(baseitem.idx);
            object_aware(player_ptr, q_ptr);
        }
    }
}

/*!
 * @brief プレイヤーの種族を変更する
 */
void wiz_reset_race(PlayerType *player_ptr)
{
    int val = enum2i<PlayerRaceType>(player_ptr->prace);
    if (!get_value("RaceID", 0, MAX_RACES - 1, &val)) {
        return;
    }

    player_ptr->prace = i2enum<PlayerRaceType>(val);
    rp_ptr = &race_info[enum2i(player_ptr->prace)];

    player_ptr->window_flags |= PW_PLAYER;
    player_ptr->update |= PU_BONUS | PU_HP | PU_MANA | PU_SPELLS;
    player_ptr->redraw |= PR_BASIC | PR_HP | PR_MANA | PR_STATS;
    handle_stuff(player_ptr);
}

/*!
 * @brief プレイヤーの職業を変更する
 * @todo 魔法領域の再選択などがまだ不完全、要実装。
 */
void wiz_reset_class(PlayerType *player_ptr)
{
    int val = enum2i<PlayerClassType>(player_ptr->pclass);
    if (!get_value("ClassID", 0, PLAYER_CLASS_TYPE_MAX - 1, &val)) {
        return;
    }

    player_ptr->pclass = i2enum<PlayerClassType>(val);
    cp_ptr = &class_info[val];
    mp_ptr = &class_magics_info[val];
    PlayerClass(player_ptr).init_specific_data();
    player_ptr->window_flags |= PW_PLAYER;
    player_ptr->update |= PU_BONUS | PU_HP | PU_MANA | PU_SPELLS;
    player_ptr->redraw |= PR_BASIC | PR_HP | PR_MANA | PR_STATS;
    handle_stuff(player_ptr);
}

/*!
 * @brief プレイヤーの領域を変更する
 * @todo 存在有無などは未判定。そのうちすべき。
 */
void wiz_reset_realms(PlayerType *player_ptr)
{
    int val1 = player_ptr->realm1;
    if (!get_value("1st Realm (None=0)", 0, MAX_REALM - 1, &val1)) {
        return;
    }

    int val2 = player_ptr->realm2;
    if (!get_value("2nd Realm (None=0)", 0, MAX_REALM - 1, &val2)) {
        return;
    }

    player_ptr->realm1 = static_cast<int16_t>(val1);
    player_ptr->realm2 = static_cast<int16_t>(val2);
    player_ptr->window_flags |= PW_PLAYER;
    player_ptr->update |= PU_BONUS | PU_HP | PU_MANA | PU_SPELLS;
    player_ptr->redraw |= PR_BASIC;
    handle_stuff(player_ptr);
}

/*!
 * @brief 現在のオプション設定をダンプ出力する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * Hack -- Dump option bits usage
 */
void wiz_dump_options(void)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "opt_info.txt");
    FILE *fff;
    fff = angband_fopen(buf, "a");
    if (fff == nullptr) {
        msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), buf);
        msg_print(nullptr);
        return;
    }

    std::vector<std::vector<int>> exist(NUM_O_SET, std::vector<int>(NUM_O_BIT));

    for (int i = 0; option_info[i].o_desc; i++) {
        const option_type *ot_ptr = &option_info[i];
        if (ot_ptr->o_var) {
            exist[ot_ptr->o_set][ot_ptr->o_bit] = i + 1;
        }
    }

    fprintf(fff, "[Option bits usage on %s\n]", get_version().data());
    fputs("Set - Bit (Page) Option Name\n", fff);
    fputs("------------------------------------------------\n", fff);
    for (int i = 0; i < NUM_O_SET; i++) {
        for (int j = 0; j < NUM_O_BIT; j++) {
            if (exist[i][j]) {
                const option_type *ot_ptr = &option_info[exist[i][j] - 1];
                fprintf(fff, "  %d -  %02d (%4d) %s\n", i, j, ot_ptr->o_page, ot_ptr->o_text);
            } else {
                fprintf(fff, "  %d -  %02d\n", i, j);
            }
        }

        fputc('\n', fff);
    }

    angband_fclose(fff);
    msg_format(_("オプションbit使用状況をファイル %s に書き出しました。", "Option bits usage dump saved to file %s."), buf);
}

/*!
 * @brief プレイ日数を変更する / Set gametime.
 * @return 実際に変更を行ったらTRUEを返す
 */
void set_gametime(void)
{
    int game_time = 0;
    if (!get_value("Dungeon Turn", 0, w_ptr->dungeon_turn_limit - 1, &game_time)) {
        return;
    }

    w_ptr->dungeon_turn = w_ptr->game_turn = game_time;
}

/*!
 * @brief プレイヤー近辺の全モンスターを消去する / Delete all nearby monsters
 */
void wiz_zap_surrounding_monsters(PlayerType *player_ptr)
{
    for (MONSTER_IDX i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!m_ptr->is_valid() || (i == player_ptr->riding) || (m_ptr->cdis > MAX_PLAYER_SIGHT)) {
            continue;
        }

        if (record_named_pet && m_ptr->is_named_pet()) {
            const auto m_name = monster_desc(player_ptr, m_ptr, MD_INDEF_VISIBLE);
            exe_write_diary(player_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name.data());
        }

        delete_monster_idx(player_ptr, i);
    }
}

/*!
 * @brief フロアに存在する全モンスターを消去する / Delete all monsters
 * @param player_ptr 術者の参照ポインタ
 */
void wiz_zap_floor_monsters(PlayerType *player_ptr)
{
    for (MONSTER_IDX i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!m_ptr->is_valid() || (i == player_ptr->riding)) {
            continue;
        }

        if (record_named_pet && m_ptr->is_named_pet()) {
            const auto m_name = monster_desc(player_ptr, m_ptr, MD_INDEF_VISIBLE);
            exe_write_diary(player_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name.data());
        }

        delete_monster_idx(player_ptr, i);
    }
}

void cheat_death(PlayerType *player_ptr)
{
    if (player_ptr->sc) {
        player_ptr->sc = player_ptr->age = 0;
    }
    player_ptr->age++;

    w_ptr->noscore |= 0x0001;
    msg_print(_("ウィザードモードに念を送り、死を欺いた。", "You invoke wizard mode and cheat death."));
    msg_print(nullptr);

    player_ptr->is_dead = false;
    (void)life_stream(player_ptr, false, false);
    (void)restore_mana(player_ptr, true);
    (void)recall_player(player_ptr, 0);
    reserve_alter_reality(player_ptr, 0);

    player_ptr->died_from = _("死の欺き", "Cheating death");
    (void)set_food(player_ptr, PY_FOOD_MAX - 1);

    auto *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->dun_level = 0;
    floor_ptr->inside_arena = false;
    player_ptr->phase_out = false;
    leaving_quest = QuestId::NONE;
    floor_ptr->quest_number = QuestId::NONE;
    if (player_ptr->dungeon_idx) {
        player_ptr->recall_dungeon = player_ptr->dungeon_idx;
    }
    player_ptr->dungeon_idx = 0;
    if (lite_town || vanilla_town) {
        player_ptr->wilderness_y = 1;
        player_ptr->wilderness_x = 1;
        if (vanilla_town) {
            player_ptr->oldpy = 10;
            player_ptr->oldpx = 34;
        } else {
            player_ptr->oldpy = 33;
            player_ptr->oldpx = 131;
        }
    } else {
        player_ptr->wilderness_y = 48;
        player_ptr->wilderness_x = 5;
        player_ptr->oldpy = 33;
        player_ptr->oldpx = 131;
    }

    player_ptr->wild_mode = false;
    player_ptr->leaving = true;

    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 1, _("                            しかし、生き返った。", "                            but revived."));
    leave_floor(player_ptr);
}
