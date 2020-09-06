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
#include "birth/inventory-initializer.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-help.h"
#include "cmd-io/cmd-save.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "flavor/object-flavor.h"
#include "floor/floor-leaver.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/option-types-table.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
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
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-generator.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-info/self-info.h"
#include "player/digestion-processor.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-skill.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/experience.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "target/grid-selector.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "wizard/tval-descriptions-table.h"
#include "wizard/wizard-spells.h"
#include "wizard/wizard-spoiler.h"
#include "world/world.h"
#define NUM_O_SET 8
#define NUM_O_BIT 32

/*!
 * @brief プレイヤーを完全回復する /
 * Cure everything instantly
 * @return なし
 */
void wiz_cure_all(player_type *creature_ptr)
{
    (void)life_stream(creature_ptr, FALSE, FALSE);
    (void)restore_mana(creature_ptr, TRUE);
    (void)set_food(creature_ptr, PY_FOOD_MAX - 1);
}

/*!
 * @brief ベースアイテムのウィザード生成のために大項目IDと小項目IDを取得する /
 * Specify tval and sval (type and subtype of object) originally
 * @return ベースアイテムID
 * @details
 * by RAK, heavily modified by -Bernd-
 * This function returns the k_idx of an object type, or zero if failed
 * List up to 50 choices in three columns
 */
KIND_OBJECT_IDX wiz_create_itemtype(void)
{
    term_clear();
    int num;
    TERM_LEN col, row;
    char ch;
    for (num = 0; (num < 80) && tvals[num].tval; num++) {
        row = 2 + (num % 20);
        col = 20 * (num / 20);
        ch = listsym[num];
        prt(format("[%c] %s", ch, tvals[num].desc), row, col);
    }

    int max_num = num;
    if (!get_com("Get what type of object? ", &ch, FALSE))
        return 0;

    for (num = 0; num < max_num; num++)
        if (listsym[num] == ch)
            break;

    if ((num < 0) || (num >= max_num))
        return 0;

    tval_type tval = tvals[num].tval;
    concptr tval_desc = tvals[num].desc;
    term_clear();
    num = 0;
    KIND_OBJECT_IDX choice[80];
    char buf[160];
    for (KIND_OBJECT_IDX i = 1; (num < 80) && (i < max_k_idx); i++) {
        object_kind *k_ptr = &k_info[i];
        if (k_ptr->tval != tval)
            continue;

        row = 2 + (num % 20);
        col = 20 * (num / 20);
        ch = listsym[num];
        strcpy(buf, "                    ");
        strip_name(buf, i);
        prt(format("[%c] %s", ch, buf), row, col);
        choice[num++] = i;
    }

    max_num = num;
    if (!get_com(format("What Kind of %s? ", tval_desc), &ch, FALSE))
        return 0;

    for (num = 0; num < max_num; num++)
        if (listsym[num] == ch)
            break;

    if ((num < 0) || (num >= max_num))
        return 0;

    return choice[num];
}

/*!
 * @brief 任意のベースアイテム生成のメインルーチン /
 * Wizard routine for creating objects		-RAK-
 * @return なし
 * @details
 * Heavily modified to allow magification and artifactification  -Bernd-
 *
 * Note that wizards cannot create objects on top of other objects.
 *
 * Hack -- this routine always makes a "dungeon object", and applies
 * magic to it, and attempts to decline cursed items.
 */
void wiz_create_item(player_type *caster_ptr)
{
    screen_save(caster_ptr);
    OBJECT_IDX k_idx = wiz_create_itemtype();
    screen_load(caster_ptr);
    if (!k_idx)
        return;

    if (k_info[k_idx].gen_flags & TRG_INSTA_ART) {
        for (ARTIFACT_IDX i = 1; i < max_a_idx; i++) {
            if ((a_info[i].tval != k_info[k_idx].tval) || (a_info[i].sval != k_info[k_idx].sval))
                continue;

            (void)create_named_art(caster_ptr, i, caster_ptr->y, caster_ptr->x);
            msg_print("Allocated(INSTA_ART).");
            return;
        }
    }

    object_type forge;
    object_type *q_ptr;
    q_ptr = &forge;
    object_prep(caster_ptr, q_ptr, k_idx);
    apply_magic(caster_ptr, q_ptr, caster_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART);
    (void)drop_near(caster_ptr, q_ptr, -1, caster_ptr->y, caster_ptr->x);
    msg_print("Allocated.");
}

/*!
 * @brief 指定されたIDの固定アーティファクトを生成する / Create the artifact of the specified number
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void wiz_create_named_art(player_type *caster_ptr)
{
    char tmp_val[10] = "";
    if (!get_string("Artifact ID:", tmp_val, 3))
        return;

    ARTIFACT_IDX a_idx = (ARTIFACT_IDX)atoi(tmp_val);
    if ((a_idx < 0) || (a_idx >= max_a_idx))
        a_idx = 0;

    (void)create_named_art(caster_ptr, a_idx, caster_ptr->y, caster_ptr->x);
    msg_print("Allocated.");
}

/*!
 * @brief プレイヤーの現能力値を調整する / Change various "permanent" player variables.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void wiz_change_status(player_type *creature_ptr)
{
    int tmp_int;
    char tmp_val[160];
    char ppp[80];
    for (int i = 0; i < A_MAX; i++) {
        sprintf(ppp, "%s (3-%d): ", stat_names[i], creature_ptr->stat_max_max[i]);
        sprintf(tmp_val, "%d", creature_ptr->stat_max[i]);
        if (!get_string(ppp, tmp_val, 3))
            return;

        tmp_int = atoi(tmp_val);
        if (tmp_int > creature_ptr->stat_max_max[i])
            tmp_int = creature_ptr->stat_max_max[i];
        else if (tmp_int < 3)
            tmp_int = 3;

        creature_ptr->stat_cur[i] = creature_ptr->stat_max[i] = (BASE_STATUS)tmp_int;
    }

    sprintf(tmp_val, "%d", WEAPON_EXP_MASTER);
    if (!get_string(_("熟練度: ", "Proficiency: "), tmp_val, 9))
        return;

    s16b tmp_s16b = (s16b)atoi(tmp_val);
    if (tmp_s16b < WEAPON_EXP_UNSKILLED)
        tmp_s16b = WEAPON_EXP_UNSKILLED;

    if (tmp_s16b > WEAPON_EXP_MASTER)
        tmp_s16b = WEAPON_EXP_MASTER;

    for (int j = 0; j <= TV_WEAPON_END - TV_WEAPON_BEGIN; j++) {
        for (int i = 0; i < 64; i++) {
            creature_ptr->weapon_exp[j][i] = tmp_s16b;
            if (creature_ptr->weapon_exp[j][i] > s_info[creature_ptr->pclass].w_max[j][i])
                creature_ptr->weapon_exp[j][i] = s_info[creature_ptr->pclass].w_max[j][i];
        }
    }

    for (int j = 0; j < 10; j++) {
        creature_ptr->skill_exp[j] = tmp_s16b;
        if (creature_ptr->skill_exp[j] > s_info[creature_ptr->pclass].s_max[j])
            creature_ptr->skill_exp[j] = s_info[creature_ptr->pclass].s_max[j];
    }

    int k;
    for (k = 0; k < 32; k++)
        creature_ptr->spell_exp[k] = (tmp_s16b > SPELL_EXP_MASTER ? SPELL_EXP_MASTER : tmp_s16b);

    for (; k < 64; k++)
        creature_ptr->spell_exp[k] = (tmp_s16b > SPELL_EXP_EXPERT ? SPELL_EXP_EXPERT : tmp_s16b);

    sprintf(tmp_val, "%ld", (long)(creature_ptr->au));
    if (!get_string("Gold: ", tmp_val, 9))
        return;

    long tmp_long = atol(tmp_val);
    if (tmp_long < 0)
        tmp_long = 0L;

    creature_ptr->au = tmp_long;
    sprintf(tmp_val, "%ld", (long)(creature_ptr->max_exp));
    if (!get_string("Experience: ", tmp_val, 9))
        return;

    tmp_long = atol(tmp_val);
    if (tmp_long < 0)
        tmp_long = 0L;

    if (creature_ptr->prace == RACE_ANDROID)
        return;

    creature_ptr->max_exp = tmp_long;
    creature_ptr->exp = tmp_long;
    check_experience(creature_ptr);
    do_cmd_redraw(creature_ptr);
}

/*!
 * @brief 指定された地点の地形IDを変更する /
 * Create desired feature
 * @param creaturer_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void wiz_create_feature(player_type *creature_ptr)
{
    POSITION y, x;
    if (!tgt_pt(creature_ptr, &x, &y))
        return;

    grid_type *g_ptr;
    g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    static int prev_feat = 0;
    char tmp_val[160];
    sprintf(tmp_val, "%d", prev_feat);

    if (!get_string(_("地形: ", "Feature: "), tmp_val, 3))
        return;

    FEAT_IDX tmp_feat = (FEAT_IDX)atoi(tmp_val);
    if (tmp_feat < 0)
        tmp_feat = 0;
    else if (tmp_feat >= max_f_idx)
        tmp_feat = max_f_idx - 1;

    static int prev_mimic = 0;
    sprintf(tmp_val, "%d", prev_mimic);

    if (!get_string(_("地形 (mimic): ", "Feature (mimic): "), tmp_val, 3))
        return;

    FEAT_IDX tmp_mimic = (FEAT_IDX)atoi(tmp_val);
    if (tmp_mimic < 0)
        tmp_mimic = 0;
    else if (tmp_mimic >= max_f_idx)
        tmp_mimic = max_f_idx - 1;

    cave_set_feat(creature_ptr, y, x, tmp_feat);
    g_ptr->mimic = (s16b)tmp_mimic;
    feature_type *f_ptr;
    f_ptr = &f_info[get_feat_mimic(g_ptr)];

    if (has_flag(f_ptr->flags, FF_GLYPH) || has_flag(f_ptr->flags, FF_MINOR_GLYPH))
        g_ptr->info |= CAVE_OBJECT;
    else if (has_flag(f_ptr->flags, FF_MIRROR))
        g_ptr->info |= CAVE_GLOW | CAVE_OBJECT;

    note_spot(creature_ptr, y, x);
    lite_spot(creature_ptr, y, x);
    creature_ptr->update |= PU_FLOW;
    prev_feat = tmp_feat;
    prev_mimic = tmp_mimic;
}

/*!
 * @brief 任意のダンジョン及び階層に飛ぶ /
 * Go to any level
 * @return なし
 */
void wiz_jump_to_dungeon(player_type *creature_ptr)
{
    if (command_arg <= 0) {
        char ppp[80];
        char tmp_val[160];
        DUNGEON_IDX tmp_dungeon_type;
        sprintf(ppp, "Jump which dungeon : ");
        sprintf(tmp_val, "%d", creature_ptr->dungeon_idx);
        if (!get_string(ppp, tmp_val, 2))
            return;

        tmp_dungeon_type = (DUNGEON_IDX)atoi(tmp_val);
        if (!d_info[tmp_dungeon_type].maxdepth || (tmp_dungeon_type > current_world_ptr->max_d_idx))
            tmp_dungeon_type = DUNGEON_ANGBAND;

        sprintf(ppp, "Jump to level (0, %d-%d): ", (int)d_info[tmp_dungeon_type].mindepth, (int)d_info[tmp_dungeon_type].maxdepth);
        sprintf(tmp_val, "%d", (int)creature_ptr->current_floor_ptr->dun_level);
        if (!get_string(ppp, tmp_val, 10))
            return;

        command_arg = (COMMAND_ARG)atoi(tmp_val);
        creature_ptr->dungeon_idx = tmp_dungeon_type;
    }

    if (command_arg < d_info[creature_ptr->dungeon_idx].mindepth)
        command_arg = 0;

    if (command_arg > d_info[creature_ptr->dungeon_idx].maxdepth)
        command_arg = (COMMAND_ARG)d_info[creature_ptr->dungeon_idx].maxdepth;

    msg_format("You jump to dungeon level %d.", command_arg);
    if (autosave_l)
        do_cmd_save_game(creature_ptr, TRUE);

    creature_ptr->current_floor_ptr->dun_level = command_arg;
    prepare_change_floor_mode(creature_ptr, CFM_RAND_PLACE);
    if (!creature_ptr->current_floor_ptr->dun_level)
        creature_ptr->dungeon_idx = 0;

    creature_ptr->current_floor_ptr->inside_arena = FALSE;
    creature_ptr->wild_mode = FALSE;
    leave_quest_check(creature_ptr);
    if (record_stair)
        exe_write_diary(creature_ptr, DIARY_WIZ_TELE, 0, NULL);

    creature_ptr->current_floor_ptr->inside_quest = 0;
    free_turn(creature_ptr);
    creature_ptr->energy_need = 0;
    prepare_change_floor_mode(creature_ptr, CFM_FIRST_FLOOR);
    creature_ptr->leaving = TRUE;
}

/*!
 * @brief 全ベースアイテムを鑑定済みにする /
 * Become aware of a lot of objects
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void wiz_learn_items_all(player_type *caster_ptr)
{
    object_type forge;
    object_type *q_ptr;
    for (KIND_OBJECT_IDX i = 1; i < max_k_idx; i++) {
        object_kind *k_ptr = &k_info[i];
        if (k_ptr->level <= command_arg) {
            q_ptr = &forge;
            object_prep(caster_ptr, q_ptr, i);
            object_aware(caster_ptr, q_ptr);
        }
    }
}

/*!
 * @brief プレイヤーの職業を変更する
 * @return なし
 * @todo 魔法領域の再選択などがまだ不完全、要実装。
 */
void wiz_reset_class(player_type *creature_ptr)
{
    char ppp[80];
    sprintf(ppp, "Class (0-%d): ", MAX_CLASS - 1);

    char tmp_val[160];
    sprintf(tmp_val, "%d", creature_ptr->pclass);

    if (!get_string(ppp, tmp_val, 2))
        return;

    int tmp_int = atoi(tmp_val);
    if (tmp_int < 0 || tmp_int >= MAX_CLASS)
        return;

    creature_ptr->pclass = (byte)tmp_int;
    creature_ptr->window |= PW_PLAYER;
    creature_ptr->update |= PU_BONUS | PU_HP | PU_MANA | PU_SPELLS;
    handle_stuff(creature_ptr);
}

/*!
 * @brief 現在のオプション設定をダンプ出力する /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Hack -- Dump option bits usage
 * @return なし
 */
void wiz_dump_options(void)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "opt_info.txt");
    FILE *fff;
    fff = angband_fopen(buf, "a");
    if (fff == NULL) {
        msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), buf);
        msg_print(NULL);
        return;
    }

    int **exist;
    C_MAKE(exist, NUM_O_SET, int *);
    C_MAKE(*exist, NUM_O_BIT * NUM_O_SET, int);
    for (int i = 1; i < NUM_O_SET; i++)
        exist[i] = *exist + i * NUM_O_BIT;

    for (int i = 0; option_info[i].o_desc; i++) {
        const option_type *ot_ptr = &option_info[i];
        if (ot_ptr->o_var)
            exist[ot_ptr->o_set][ot_ptr->o_bit] = i + 1;
    }

    fprintf(fff, "[Option bits usage on Hengband %d.%d.%d]\n\n", FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
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

    C_KILL(*exist, NUM_O_BIT * NUM_O_SET, int);
    C_KILL(exist, NUM_O_SET, int *);
    angband_fclose(fff);
    msg_format(_("オプションbit使用状況をファイル %s に書き出しました。", "Option bits usage dump saved to file %s."), buf);
}

/*!
 * @brief プレイ日数を変更する / Set gametime.
 * @return 実際に変更を行ったらTRUEを返す
 */
void set_gametime(void)
{
    int tmp_int = 0;
    char ppp[80], tmp_val[40];
    sprintf(ppp, "Dungeon Turn (0-%ld): ", (long)current_world_ptr->dungeon_turn_limit);
    sprintf(tmp_val, "%ld", (long)current_world_ptr->dungeon_turn);
    if (!get_string(ppp, tmp_val, 10))
        return;

    tmp_int = atoi(tmp_val);
    if (tmp_int >= current_world_ptr->dungeon_turn_limit)
        tmp_int = current_world_ptr->dungeon_turn_limit - 1;
    else if (tmp_int < 0)
        tmp_int = 0;

    current_world_ptr->dungeon_turn = current_world_ptr->game_turn = tmp_int;
}

/*!
 * @brief プレイヤー近辺の全モンスターを消去する / Delete all nearby monsters
 * @return なし
 */
void wiz_zap_surrounding_monsters(player_type *caster_ptr)
{
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr) || (i == caster_ptr->riding) || (m_ptr->cdis > MAX_SIGHT))
            continue;

        if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
            GAME_TEXT m_name[MAX_NLEN];

            monster_desc(caster_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
            exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name);
        }

        delete_monster_idx(caster_ptr, i);
    }
}

/*!
 * @brief フロアに存在する全モンスターを消去する / Delete all monsters
 * @param caster_ptr 術者の参照ポインタ
 * @return なし
 */
void wiz_zap_floor_monsters(player_type *caster_ptr)
{
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr) || (i == caster_ptr->riding))
            continue;

        if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(caster_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
            exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name);
        }

        delete_monster_idx(caster_ptr, i);
    }
}

void cheat_death(player_type *creature_ptr)
{
    if (creature_ptr->sc)
        creature_ptr->sc = creature_ptr->age = 0;
    creature_ptr->age++;

    current_world_ptr->noscore |= 0x0001;
    msg_print(_("ウィザードモードに念を送り、死を欺いた。", "You invoke wizard mode and cheat death."));
    msg_print(NULL);

    (void)life_stream(creature_ptr, FALSE, FALSE);
    (void)restore_mana(creature_ptr, TRUE);

    (void)recall_player(creature_ptr, 0);
    reserve_alter_reality(creature_ptr, 0);

    (void)strcpy(creature_ptr->died_from, _("死の欺き", "Cheating death"));
    creature_ptr->is_dead = FALSE;
    (void)set_food(creature_ptr, PY_FOOD_MAX - 1);

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    floor_ptr->dun_level = 0;
    floor_ptr->inside_arena = FALSE;
    creature_ptr->phase_out = FALSE;
    leaving_quest = 0;
    floor_ptr->inside_quest = 0;
    if (creature_ptr->dungeon_idx)
        creature_ptr->recall_dungeon = creature_ptr->dungeon_idx;
    creature_ptr->dungeon_idx = 0;
    if (lite_town || vanilla_town) {
        creature_ptr->wilderness_y = 1;
        creature_ptr->wilderness_x = 1;
        if (vanilla_town) {
            creature_ptr->oldpy = 10;
            creature_ptr->oldpx = 34;
        } else {
            creature_ptr->oldpy = 33;
            creature_ptr->oldpx = 131;
        }
    } else {
        creature_ptr->wilderness_y = 48;
        creature_ptr->wilderness_x = 5;
        creature_ptr->oldpy = 33;
        creature_ptr->oldpx = 131;
    }

    creature_ptr->wild_mode = FALSE;
    creature_ptr->leaving = TRUE;

    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, _("                            しかし、生き返った。", "                            but revived."));
    leave_floor(creature_ptr);
}
