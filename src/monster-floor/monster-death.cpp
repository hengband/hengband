﻿#include "monster-floor/monster-death.h"
#include "artifact/fixed-art-generator.h"
#include "cmd-building/cmd-building.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest-completion-checker.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "io/write-diary.h"
#include "lore/lore-store.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena-info-table.h"
#include "monster-floor/monster-death-util.h"
#include "monster-floor/monster-object.h"
#include "monster-floor/special-death-switcher.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags9.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "pet/pet-fall-off.h"
#include "player/patron.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/artifact-type-definition.h"
#include "system/building-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "view/display-messages.h"
#include "world/world.h"

static void write_pet_death(player_type *player_ptr, monster_death_type *md_ptr)
{
    md_ptr->md_y = md_ptr->m_ptr->fy;
    md_ptr->md_x = md_ptr->m_ptr->fx;
    if (record_named_pet && is_pet(md_ptr->m_ptr) && md_ptr->m_ptr->nickname) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(player_ptr, m_name, md_ptr->m_ptr, MD_INDEF_VISIBLE);
        exe_write_diary(player_ptr, DIARY_NAMED_PET, 3, m_name);
    }
}

static void on_dead_explosion(player_type *player_ptr, monster_death_type *md_ptr)
{
    for (int i = 0; i < 4; i++) {
        if (md_ptr->r_ptr->blow[i].method != RBM_EXPLODE)
            continue;

        BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        EFFECT_ID typ = mbe_info[md_ptr->r_ptr->blow[i].effect].explode_type;
        DICE_NUMBER d_dice = md_ptr->r_ptr->blow[i].d_dice;
        DICE_SID d_side = md_ptr->r_ptr->blow[i].d_side;
        HIT_POINT damage = damroll(d_dice, d_side);
        (void)project(player_ptr, md_ptr->m_idx, 3, md_ptr->md_y, md_ptr->md_x, damage, typ, flg, -1);
        break;
    }
}

static void on_defeat_arena_monster(player_type *player_ptr, monster_death_type *md_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!floor_ptr->inside_arena || is_pet(md_ptr->m_ptr))
        return;

    player_ptr->exit_bldg = TRUE;
    if (player_ptr->arena_number > MAX_ARENA_MONS)
        msg_print(_("素晴らしい！君こそ真の勝利者だ。", "You are a Genuine Champion!"));
    else
        msg_print(_("勝利！チャンピオンへの道を進んでいる。", "Victorious! You're on your way to becoming Champion."));

    if (arena_info[player_ptr->arena_number].tval) {
        object_type forge;
        object_type *q_ptr = &forge;
        object_prep(player_ptr, q_ptr, lookup_kind(arena_info[player_ptr->arena_number].tval, arena_info[player_ptr->arena_number].sval));
        apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART);
        (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
    }

    if (player_ptr->arena_number > MAX_ARENA_MONS)
        player_ptr->arena_number++;

    player_ptr->arena_number++;
    if (!record_arena)
        return;

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(player_ptr, m_name, md_ptr->m_ptr, MD_WRONGDOER_NAME);
    exe_write_diary(player_ptr, DIARY_ARENA, player_ptr->arena_number, m_name);
}

static void drop_corpse(player_type *player_ptr, monster_death_type *md_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    bool is_drop_corpse = one_in_(md_ptr->r_ptr->flags1 & RF1_UNIQUE ? 1 : 4);
    is_drop_corpse &= (md_ptr->r_ptr->flags9 & (RF9_DROP_CORPSE | RF9_DROP_SKELETON)) != 0;
    is_drop_corpse &= !(floor_ptr->inside_arena || player_ptr->phase_out || md_ptr->cloned || ((md_ptr->m_ptr->r_idx == today_mon) && is_pet(md_ptr->m_ptr)));
    if (!is_drop_corpse)
        return;

    bool corpse = FALSE;
    if (!(md_ptr->r_ptr->flags9 & RF9_DROP_SKELETON))
        corpse = TRUE;
    else if ((md_ptr->r_ptr->flags9 & RF9_DROP_CORPSE) && (md_ptr->r_ptr->flags1 & RF1_UNIQUE))
        corpse = TRUE;
    else if (md_ptr->r_ptr->flags9 & RF9_DROP_CORPSE) {
        if ((0 - ((md_ptr->m_ptr->maxhp) / 4)) > md_ptr->m_ptr->hp) {
            if (one_in_(5))
                corpse = TRUE;
        } else {
            if (!one_in_(5))
                corpse = TRUE;
        }
    }

    object_type forge;
    object_type *q_ptr = &forge;
    object_prep(player_ptr, q_ptr, lookup_kind(TV_CORPSE, (corpse ? SV_CORPSE : SV_SKELETON)));
    apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART);
    q_ptr->pval = md_ptr->m_ptr->r_idx;
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

/*!
 * @brief アーティファクトのドロップ判定処理
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param md_ptr モンスター死亡構造体への参照ポインタ
 * @return 何かドロップするなら1以上、何もドロップしないなら0
 */
static ARTIFACT_IDX drop_artifact_index(player_type *player_ptr, monster_death_type *md_ptr)
{
    ARTIFACT_IDX a_idx = 0;
    PERCENTAGE chance = 0;
    for (int i = 0; i < 4; i++) {
        if (!md_ptr->r_ptr->artifact_id[i])
            break;

        a_idx = md_ptr->r_ptr->artifact_id[i];
        chance = md_ptr->r_ptr->artifact_percent[i];
        if ((randint0(100) >= chance) && !current_world_ptr->wizard)
            continue;

        artifact_type *a_ptr = &a_info[a_idx];
        if (a_ptr->cur_num == 1)
            continue;

        if (create_named_art(player_ptr, a_idx, md_ptr->md_y, md_ptr->md_x)) {
            a_ptr->cur_num = 1;
            if (current_world_ptr->character_dungeon)
                a_ptr->floor_id = player_ptr->floor_id;

            break;
        }

        if (!preserve_mode) {
            a_ptr->cur_num = 1;
            break;
        }
    }

    return a_idx;
}

static KIND_OBJECT_IDX drop_dungeon_final_artifact(player_type *player_ptr, monster_death_type *md_ptr, ARTIFACT_IDX a_idx)
{
    KIND_OBJECT_IDX k_idx
        = d_info[player_ptr->dungeon_idx].final_object != 0 ? d_info[player_ptr->dungeon_idx].final_object : lookup_kind(TV_SCROLL, SV_SCROLL_ACQUIREMENT);
    if (d_info[player_ptr->dungeon_idx].final_artifact == 0)
        return k_idx;

    a_idx = d_info[player_ptr->dungeon_idx].final_artifact;
    artifact_type *a_ptr = &a_info[a_idx];
    if (a_ptr->cur_num == 1)
        return k_idx;
    if (create_named_art(player_ptr, a_idx, md_ptr->md_y, md_ptr->md_x)) {
        a_ptr->cur_num = 1;
        if (current_world_ptr->character_dungeon)
            a_ptr->floor_id = player_ptr->floor_id;
    } else if (!preserve_mode) {
        a_ptr->cur_num = 1;
    }

    return d_info[player_ptr->dungeon_idx].final_object ? k_idx : 0;
}

static void drop_artifact(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!md_ptr->drop_chosen_item)
        return;

    ARTIFACT_IDX a_idx = drop_artifact_index(player_ptr, md_ptr);
    if (((md_ptr->r_ptr->flags7 & RF7_GUARDIAN) == 0) || (d_info[player_ptr->dungeon_idx].final_guardian != md_ptr->m_ptr->r_idx))
        return;

    KIND_OBJECT_IDX k_idx = drop_dungeon_final_artifact(player_ptr, md_ptr, a_idx);
    if (k_idx != 0) {
        object_type forge;
        object_type *q_ptr = &forge;
        object_prep(player_ptr, q_ptr, k_idx);
        apply_magic(player_ptr, q_ptr, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART | AM_GOOD);
        (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
    }

    msg_format(_("あなたは%sを制覇した！", "You have conquered %s!"), d_name + d_info[player_ptr->dungeon_idx].name);
}

static void decide_drop_quality(monster_death_type *md_ptr)
{
    md_ptr->mo_mode = 0L;
    if (md_ptr->r_ptr->flags1 & RF1_DROP_GOOD)
        md_ptr->mo_mode |= AM_GOOD;

    if (md_ptr->r_ptr->flags1 & RF1_DROP_GREAT)
        md_ptr->mo_mode |= (AM_GOOD | AM_GREAT);
}

static int decide_drop_numbers(player_type *player_ptr, monster_death_type *md_ptr, const bool drop_item)
{
    int drop_numbers = 0;
    if ((md_ptr->r_ptr->flags1 & RF1_DROP_60) && (randint0(100) < 60))
        drop_numbers++;

    if ((md_ptr->r_ptr->flags1 & RF1_DROP_90) && (randint0(100) < 90))
        drop_numbers++;

    if (md_ptr->r_ptr->flags1 & RF1_DROP_1D2)
        drop_numbers += damroll(1, 2);

    if (md_ptr->r_ptr->flags1 & RF1_DROP_2D2)
        drop_numbers += damroll(2, 2);

    if (md_ptr->r_ptr->flags1 & RF1_DROP_3D2)
        drop_numbers += damroll(3, 2);

    if (md_ptr->r_ptr->flags1 & RF1_DROP_4D2)
        drop_numbers += damroll(4, 2);

    if (md_ptr->cloned && !(md_ptr->r_ptr->flags1 & RF1_UNIQUE))
        drop_numbers = 0;

    if (is_pet(md_ptr->m_ptr) || player_ptr->phase_out || player_ptr->current_floor_ptr->inside_arena)
        drop_numbers = 0;

    if (!drop_item && (md_ptr->r_ptr->d_char != '$'))
        drop_numbers = 0;

    if ((md_ptr->r_ptr->flags2 & (RF2_MULTIPLY)) && (md_ptr->r_ptr->r_akills > 1024))
        drop_numbers = 0;

    return drop_numbers;
}

static void drop_items_golds(player_type *player_ptr, monster_death_type *md_ptr, int drop_numbers)
{
    int dump_item = 0;
    int dump_gold = 0;
    for (int i = 0; i < drop_numbers; i++) {
        object_type forge;
        object_type *q_ptr = &forge;
        object_wipe(q_ptr);
        if (md_ptr->do_gold && (!md_ptr->do_item || (randint0(100) < 50))) {
            if (!make_gold(player_ptr, q_ptr))
                continue;

            dump_gold++;
        } else {
            if (!make_object(player_ptr, q_ptr, md_ptr->mo_mode))
                continue;

            dump_item++;
        }

        (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
    }

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->object_level = floor_ptr->base_level;
    coin_type = 0;
    bool visible = (md_ptr->m_ptr->ml && !player_ptr->image) || ((md_ptr->r_ptr->flags1 & RF1_UNIQUE) != 0);
    if (visible && (dump_item || dump_gold))
        lore_treasure(player_ptr, md_ptr->m_idx, dump_item, dump_gold);
}

static void on_defeat_last_boss(player_type *player_ptr)
{
    current_world_ptr->total_winner = TRUE;
    player_ptr->redraw |= PR_TITLE;
    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FINAL_QUEST_CLEAR);
    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, _("見事に変愚蛮怒の勝利者となった！", "finally became *WINNER* of Hengband!"));
    admire_from_patron(player_ptr);
    msg_print(_("*** おめでとう ***", "*** CONGRATULATIONS ***"));
    msg_print(_("あなたはゲームをコンプリートしました。", "You have won the game!"));
    msg_print(_("準備が整ったら引退(自殺コマンド)しても結構です。", "You may retire (commit suicide) when you are ready."));
}

/*!
 * @brief モンスターが死亡した時の処理 /
 * Handle the "death" of a monster.
 * @param m_idx 死亡したモンスターのID
 * @param drop_item TRUEならばモンスターのドロップ処理を行う
 * @return 撃破されたモンスターの述語
 * @details
 * <pre>
 * Disperse treasures centered at the monster location based on the
 * various flags contained in the monster flags fields.
 * Check for "Quest" completion when a quest monster is killed.
 * Note that only the player can induce "monster_death()" on Uniques.
 * Thus (for now) all Quest monsters should be Uniques.
 * Note that monsters can now carry objects, and when a monster dies,
 * it drops all of its objects, which may disappear in crowded rooms.
 * </pre>
 */
void monster_death(player_type *player_ptr, MONSTER_IDX m_idx, bool drop_item)
{
    monster_death_type tmp_md;
    monster_death_type *md_ptr = initialize_monster_death_type(player_ptr, &tmp_md, m_idx, drop_item);
    if (current_world_ptr->timewalk_m_idx && current_world_ptr->timewalk_m_idx == m_idx)
        current_world_ptr->timewalk_m_idx = 0;

    if (md_ptr->r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
        player_ptr->update |= PU_MON_LITE;

    write_pet_death(player_ptr, md_ptr);
    on_dead_explosion(player_ptr, md_ptr);
    if (md_ptr->m_ptr->mflag2 & MFLAG2_CHAMELEON) {
        choose_new_monster(player_ptr, m_idx, TRUE, MON_CHAMELEON);
        md_ptr->r_ptr = &r_info[md_ptr->m_ptr->r_idx];
    }

    check_quest_completion(player_ptr, md_ptr->m_ptr);
    on_defeat_arena_monster(player_ptr, md_ptr);
    if (m_idx == player_ptr->riding && process_fall_off_horse(player_ptr, -1, FALSE))
        msg_print(_("地面に落とされた。", "You have fallen from the pet you were riding."));

    drop_corpse(player_ptr, md_ptr);
    monster_drop_carried_objects(player_ptr, md_ptr->m_ptr);
    decide_drop_quality(md_ptr);
    switch_special_death(player_ptr, md_ptr);
    drop_artifact(player_ptr, md_ptr);
    int drop_numbers = decide_drop_numbers(player_ptr, md_ptr, drop_item);
    coin_type = md_ptr->force_coin;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->object_level = (floor_ptr->dun_level + md_ptr->r_ptr->level) / 2;
    drop_items_golds(player_ptr, md_ptr, drop_numbers);
    if (((md_ptr->r_ptr->flags1 & RF1_QUESTOR) == 0) || player_ptr->phase_out || (md_ptr->m_ptr->r_idx != MON_SERPENT) || md_ptr->cloned)
        return;

    on_defeat_last_boss(player_ptr);
}

/*!
 * @brief モンスターを撃破した際の述語メッセージを返す /
 * Return monster death string
 * @param r_ptr 撃破されたモンスターの種族情報を持つ構造体の参照ポインタ
 * @return 撃破されたモンスターの述語
 */
concptr extract_note_dies(MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (monster_living(r_idx))
        return _("は死んだ。", " dies.");

    for (int i = 0; i < 4; i++)
        if (r_ptr->blow[i].method == RBM_EXPLODE)
            return _("は爆発して粉々になった。", " explodes into tiny shreds.");

    return _("を倒した。", " is destroyed.");
}