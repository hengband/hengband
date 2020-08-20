#include "monster-floor/monster-death.h"
#include "art-definition/art-armor-types.h"
#include "art-definition/art-bow-types.h"
#include "art-definition/art-protector-types.h"
#include "art-definition/art-weapon-types.h"
#include "art-definition/random-art-effects.h"
#include "artifact/fixed-art-generator.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest-completion-checker.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "lore/lore-store.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena-info-table.h"
#include "monster-floor/monster-object.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
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
#include "monster/smart-learn-types.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "pet/pet-fall-off.h"
#include "player/patron.h"
#include "spell/spell-types.h"
#include "spell/spells-summon.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/system-variables.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief モンスターを倒した際の財宝svalを返す
 * @param r_idx 倒したモンスターの種族ID
 * @return 財宝のsval
 * @details
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 * Note the use of actual "monster names"
 */
static OBJECT_SUBTYPE_VALUE get_coin_type(MONRACE_IDX r_idx)
{
    switch (r_idx) {
    case MON_COPPER_COINS:
        return 2;
    case MON_SILVER_COINS:
        return 5;
    case MON_GOLD_COINS:
        return 10;
    case MON_MITHRIL_COINS:
    case MON_MITHRIL_GOLEM:
        return 16;
    case MON_ADAMANT_COINS:
        return 17;
    }

    return 0;
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
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    bool do_gold = (!(r_ptr->flags1 & RF1_ONLY_ITEM));
    bool do_item = (!(r_ptr->flags1 & RF1_ONLY_GOLD));
    bool cloned = (m_ptr->smart & SM_CLONED) ? TRUE : FALSE;
    int force_coin = get_coin_type(m_ptr->r_idx);

    bool drop_chosen_item = drop_item && !cloned && !floor_ptr->inside_arena && !player_ptr->phase_out && !is_pet(m_ptr);

    if (current_world_ptr->timewalk_m_idx && current_world_ptr->timewalk_m_idx == m_idx) {
        current_world_ptr->timewalk_m_idx = 0;
    }

    if (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)) {
        player_ptr->update |= (PU_MON_LITE);
    }

    POSITION y = m_ptr->fy;
    POSITION x = m_ptr->fx;

    if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
        GAME_TEXT m_name[MAX_NLEN];

        monster_desc(player_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
        exe_write_diary(player_ptr, DIARY_NAMED_PET, 3, m_name);
    }

    for (int i = 0; i < 4; i++) {
        if (r_ptr->blow[i].method != RBM_EXPLODE)
            continue;

        BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        EFFECT_ID typ = mbe_info[r_ptr->blow[i].effect].explode_type;
        DICE_NUMBER d_dice = r_ptr->blow[i].d_dice;
        DICE_SID d_side = r_ptr->blow[i].d_side;
        HIT_POINT damage = damroll(d_dice, d_side);

        project(player_ptr, m_idx, 3, y, x, damage, typ, flg, -1);
        break;
    }

    if (m_ptr->mflag2 & MFLAG2_CHAMELEON) {
        choose_new_monster(player_ptr, m_idx, TRUE, MON_CHAMELEON);
        r_ptr = &r_info[m_ptr->r_idx];
    }

    check_quest_completion(player_ptr, m_ptr);

    object_type forge;
    object_type *q_ptr;
    if (floor_ptr->inside_arena && !is_pet(m_ptr)) {
        player_ptr->exit_bldg = TRUE;
        if (player_ptr->arena_number > MAX_ARENA_MONS) {
            msg_print(_("素晴らしい！君こそ真の勝利者だ。", "You are a Genuine Champion!"));
        } else {
            msg_print(_("勝利！チャンピオンへの道を進んでいる。", "Victorious! You're on your way to becoming Champion."));
        }

        if (arena_info[player_ptr->arena_number].tval) {
            q_ptr = &forge;
            object_prep(player_ptr, q_ptr, lookup_kind(arena_info[player_ptr->arena_number].tval, arena_info[player_ptr->arena_number].sval));
            apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART);
            (void)drop_near(player_ptr, q_ptr, -1, y, x);
        }

        if (player_ptr->arena_number > MAX_ARENA_MONS)
            player_ptr->arena_number++;
        player_ptr->arena_number++;
        if (record_arena) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(player_ptr, m_name, m_ptr, MD_WRONGDOER_NAME);
            exe_write_diary(player_ptr, DIARY_ARENA, player_ptr->arena_number, m_name);
        }
    }

    if (m_idx == player_ptr->riding && process_fall_off_horse(player_ptr, -1, FALSE)) {
        msg_print(_("地面に落とされた。", "You have fallen from the pet you were riding."));
    }

    bool is_drop_corpse = one_in_(r_ptr->flags1 & RF1_UNIQUE ? 1 : 4);
    is_drop_corpse &= (r_ptr->flags9 & (RF9_DROP_CORPSE | RF9_DROP_SKELETON)) != 0;
    is_drop_corpse &= !(floor_ptr->inside_arena || player_ptr->phase_out || cloned || ((m_ptr->r_idx == today_mon) && is_pet(m_ptr)));
    if (is_drop_corpse) {
        bool corpse = FALSE;

        if (!(r_ptr->flags9 & RF9_DROP_SKELETON))
            corpse = TRUE;
        else if ((r_ptr->flags9 & RF9_DROP_CORPSE) && (r_ptr->flags1 & RF1_UNIQUE))
            corpse = TRUE;
        else if (r_ptr->flags9 & RF9_DROP_CORPSE) {
            if ((0 - ((m_ptr->maxhp) / 4)) > m_ptr->hp) {
                if (one_in_(5))
                    corpse = TRUE;
            } else {
                if (!one_in_(5))
                    corpse = TRUE;
            }
        }

        q_ptr = &forge;
        object_prep(player_ptr, q_ptr, lookup_kind(TV_CORPSE, (corpse ? SV_CORPSE : SV_SKELETON)));
        apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART);
        q_ptr->pval = m_ptr->r_idx;
        (void)drop_near(player_ptr, q_ptr, -1, y, x);
    }

    monster_drop_carried_objects(player_ptr, m_ptr);

    u32b mo_mode = 0L;
    if (r_ptr->flags1 & RF1_DROP_GOOD)
        mo_mode |= AM_GOOD;
    if (r_ptr->flags1 & RF1_DROP_GREAT)
        mo_mode |= AM_GREAT;

    switch (m_ptr->r_idx) {
    case MON_PINK_HORROR: {
        if (floor_ptr->inside_arena || player_ptr->phase_out)
            break;

        bool notice = FALSE;
        for (int i = 0; i < 2; i++) {
            POSITION wy = y, wx = x;
            bool pet = is_pet(m_ptr);
            BIT_FLAGS mode = 0L;

            if (pet) {
                mode |= PM_FORCE_PET;
            }

            if (summon_specific(player_ptr, (pet ? -1 : m_idx), wy, wx, 100, SUMMON_BLUE_HORROR, mode)) {
                if (player_can_see_bold(player_ptr, wy, wx))
                    notice = TRUE;
            }
        }

        if (notice) {
            msg_print(_("ピンク・ホラーは分裂した！", "The Pink horror divides!"));
        }

        break;
    }
    case MON_BLOODLETTER: {
        if (!drop_chosen_item || (randint1(100) >= 15))
            break;

        q_ptr = &forge;
        object_prep(player_ptr, q_ptr, lookup_kind(TV_SWORD, SV_BLADE_OF_CHAOS));
        apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART | mo_mode);
        (void)drop_near(player_ptr, q_ptr, -1, y, x);
        break;
    }
    case MON_RAAL: {
        if (!drop_chosen_item || (floor_ptr->dun_level <= 9))
            break;

        q_ptr = &forge;
        object_wipe(q_ptr);
        if ((floor_ptr->dun_level > 49) && one_in_(5))
            get_obj_num_hook = kind_is_good_book;
        else
            get_obj_num_hook = kind_is_book;

        make_object(player_ptr, q_ptr, mo_mode);
        (void)drop_near(player_ptr, q_ptr, -1, y, x);
        break;
    }
    case MON_DAWN: {
        if (floor_ptr->inside_arena || player_ptr->phase_out)
            break;
        if (one_in_(7))
            break;

        POSITION wy = y, wx = x;
        int attempts = 100;
        bool pet = is_pet(m_ptr);
        do {
            scatter(player_ptr, &wy, &wx, y, x, 20, 0);
        } while (!(in_bounds(floor_ptr, wy, wx) && is_cave_empty_bold2(player_ptr, wy, wx)) && --attempts);

        if (attempts <= 0)
            break;

        BIT_FLAGS mode = 0L;
        if (pet)
            mode |= PM_FORCE_PET;

        if (summon_specific(player_ptr, (pet ? -1 : m_idx), wy, wx, 100, SUMMON_DAWN, mode)) {
            if (player_can_see_bold(player_ptr, wy, wx))
                msg_print(_("新たな戦士が現れた！", "A new warrior steps forth!"));
        }

        break;
    }
    case MON_UNMAKER: {
        BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        (void)project(player_ptr, m_idx, 6, y, x, 100, GF_CHAOS, flg, -1);
        break;
    }
    case MON_UNICORN_ORD:
    case MON_MORGOTH:
    case MON_ONE_RING: {
        if (player_ptr->pseikaku != PERSONALITY_LAZY)
            break;
        if (!drop_chosen_item)
            break;

        ARTIFACT_IDX a_idx = 0;
        artifact_type *a_ptr = NULL;
        do {
            switch (randint0(3)) {
            case 0:
                a_idx = ART_NAMAKE_HAMMER;
                break;
            case 1:
                a_idx = ART_NAMAKE_BOW;
                break;
            case 2:
                a_idx = ART_NAMAKE_ARMOR;
                break;
            }

            a_ptr = &a_info[a_idx];
        } while (a_ptr->cur_num);

        if (create_named_art(player_ptr, a_idx, y, x)) {
            a_ptr->cur_num = 1;
            if (current_world_ptr->character_dungeon)
                a_ptr->floor_id = player_ptr->floor_id;
        } else if (!preserve_mode)
            a_ptr->cur_num = 1;

        break;
    }
    case MON_SERPENT: {
        if (!drop_chosen_item)
            break;

        q_ptr = &forge;
        object_prep(player_ptr, q_ptr, lookup_kind(TV_HAFTED, SV_GROND));
        q_ptr->name1 = ART_GROND;
        apply_magic(player_ptr, q_ptr, -1, AM_GOOD | AM_GREAT);
        (void)drop_near(player_ptr, q_ptr, -1, y, x);
        q_ptr = &forge;
        object_prep(player_ptr, q_ptr, lookup_kind(TV_CROWN, SV_CHAOS));
        q_ptr->name1 = ART_CHAOS;
        apply_magic(player_ptr, q_ptr, -1, AM_GOOD | AM_GREAT);
        (void)drop_near(player_ptr, q_ptr, -1, y, x);
        break;
    }
    case MON_B_DEATH_SWORD: {
        if (!drop_chosen_item)
            break;

        q_ptr = &forge;
        object_prep(player_ptr, q_ptr, lookup_kind(TV_SWORD, randint1(2)));
        (void)drop_near(player_ptr, q_ptr, -1, y, x);
        break;
    }
    case MON_A_GOLD:
    case MON_A_SILVER: {
        bool is_drop_can = drop_chosen_item;
        bool is_silver = m_ptr->r_idx == MON_A_SILVER;
        is_silver &= r_ptr->r_akills % 5 == 0;
        is_drop_can &= (m_ptr->r_idx == MON_A_GOLD) || is_silver;
        if (!is_drop_can)
            break;

        q_ptr = &forge;
        object_prep(player_ptr, q_ptr, lookup_kind(TV_CHEST, SV_CHEST_KANDUME));
        apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART);
        (void)drop_near(player_ptr, q_ptr, -1, y, x);
        break;
    }
    case MON_ROLENTO: {
        BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        (void)project(player_ptr, m_idx, 3, y, x, damroll(20, 10), GF_FIRE, flg, -1);
        break;
    }
    case MON_MIDDLE_AQUA_FIRST:
    case MON_LARGE_AQUA_FIRST:
    case MON_EXTRA_LARGE_AQUA_FIRST:
    case MON_MIDDLE_AQUA_SECOND:
    case MON_LARGE_AQUA_SECOND:
    case MON_EXTRA_LARGE_AQUA_SECOND: {
        if (floor_ptr->inside_arena || player_ptr->phase_out)
            break;

        bool notice = FALSE;
        const int popped_bubbles = 4;
        for (int i = 0; i < popped_bubbles; i++) {
            POSITION wy = y, wx = x;
            bool pet = is_pet(m_ptr);
            BIT_FLAGS mode = PM_NONE;

            if (pet)
                mode |= PM_FORCE_PET;

            MONSTER_IDX smaller_bubblle = m_ptr->r_idx - 1;
            if (summon_named_creature(player_ptr, (pet ? -1 : m_idx), wy, wx, smaller_bubblle, mode) && player_can_see_bold(player_ptr, wy, wx))
                notice = TRUE;
        }

        if (notice)
            msg_print(_("泡が弾けた！", "The bubble pops!"));

        break;
    }
    case MON_TOTEM_MOAI: {
        if (floor_ptr->inside_arena || player_ptr->phase_out || one_in_(8))
            break;

        POSITION wy = y, wx = x;
        int attempts = 100;
        bool pet = is_pet(m_ptr);
        do {
            scatter(player_ptr, &wy, &wx, y, x, 20, 0);
        } while (!(in_bounds(floor_ptr, wy, wx) && is_cave_empty_bold2(player_ptr, wy, wx)) && --attempts);

        if (attempts <= 0)
            break;

        BIT_FLAGS mode = PM_NONE;
        if (pet)
            mode |= PM_FORCE_PET;

        if (summon_named_creature(player_ptr, (pet ? -1 : m_idx), wy, wx, MON_TOTEM_MOAI, mode) && player_can_see_bold(player_ptr, wy, wx))
            msg_print(_("新たなモアイが現れた！", "A new moai steps forth!"));

        break;
    }
    default: {
        if (!drop_chosen_item)
            break;

        switch (r_ptr->d_char) {
        case '(': {
            if (floor_ptr->dun_level <= 0)
                break;

            q_ptr = &forge;
            object_wipe(q_ptr);
            get_obj_num_hook = kind_is_cloak;
            make_object(player_ptr, q_ptr, mo_mode);
            (void)drop_near(player_ptr, q_ptr, -1, y, x);
            break;
        }
        case '/': {
            if (floor_ptr->dun_level <= 4)
                break;

            q_ptr = &forge;
            object_wipe(q_ptr);
            get_obj_num_hook = kind_is_polearm;
            make_object(player_ptr, q_ptr, mo_mode);
            (void)drop_near(player_ptr, q_ptr, -1, y, x);
            break;
        }
        case '[': {
            if (floor_ptr->dun_level <= 19)
                break;

            q_ptr = &forge;
            object_wipe(q_ptr);
            get_obj_num_hook = kind_is_armor;
            make_object(player_ptr, q_ptr, mo_mode);
            (void)drop_near(player_ptr, q_ptr, -1, y, x);
            break;
        }
        case '\\': {
            if (floor_ptr->dun_level <= 4)
                break;
            q_ptr = &forge;
            object_wipe(q_ptr);
            get_obj_num_hook = kind_is_hafted;
            make_object(player_ptr, q_ptr, mo_mode);
            (void)drop_near(player_ptr, q_ptr, -1, y, x);
            break;
        }
        case '|': {
            if (m_ptr->r_idx == MON_STORMBRINGER)
                break;

            q_ptr = &forge;
            object_wipe(q_ptr);
            get_obj_num_hook = kind_is_sword;
            make_object(player_ptr, q_ptr, mo_mode);
            (void)drop_near(player_ptr, q_ptr, -1, y, x);
            break;
        }
        }
    }
    }

    if (drop_chosen_item) {
        ARTIFACT_IDX a_idx = 0;
        PERCENTAGE chance = 0;
        for (int i = 0; i < 4; i++) {
            if (!r_ptr->artifact_id[i])
                break;
            a_idx = r_ptr->artifact_id[i];
            chance = r_ptr->artifact_percent[i];
            if (randint0(100) < chance || current_world_ptr->wizard) {
                artifact_type *a_ptr = &a_info[a_idx];
                if (!a_ptr->cur_num) {
                    if (create_named_art(player_ptr, a_idx, y, x)) {
                        a_ptr->cur_num = 1;
                        if (current_world_ptr->character_dungeon)
                            a_ptr->floor_id = player_ptr->floor_id;
                    } else if (!preserve_mode) {
                        a_ptr->cur_num = 1;
                    }
                }
            }
        }

        if ((r_ptr->flags7 & RF7_GUARDIAN) && (d_info[player_ptr->dungeon_idx].final_guardian == m_ptr->r_idx)) {
            KIND_OBJECT_IDX k_idx = d_info[player_ptr->dungeon_idx].final_object != 0 ? d_info[player_ptr->dungeon_idx].final_object
                                                                                      : lookup_kind(TV_SCROLL, SV_SCROLL_ACQUIREMENT);

            if (d_info[player_ptr->dungeon_idx].final_artifact != 0) {
                a_idx = d_info[player_ptr->dungeon_idx].final_artifact;
                artifact_type *a_ptr = &a_info[a_idx];
                if (!a_ptr->cur_num) {
                    if (create_named_art(player_ptr, a_idx, y, x)) {
                        a_ptr->cur_num = 1;
                        if (current_world_ptr->character_dungeon)
                            a_ptr->floor_id = player_ptr->floor_id;
                    } else if (!preserve_mode) {
                        a_ptr->cur_num = 1;
                    }

                    if (!d_info[player_ptr->dungeon_idx].final_object)
                        k_idx = 0;
                }
            }

            if (k_idx != 0) {
                q_ptr = &forge;
                object_prep(player_ptr, q_ptr, k_idx);
                apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART | AM_GOOD);
                (void)drop_near(player_ptr, q_ptr, -1, y, x);
            }

            msg_format(_("あなたは%sを制覇した！", "You have conquered %s!"), d_name + d_info[player_ptr->dungeon_idx].name);
        }
    }

    int number = 0;
    if ((r_ptr->flags1 & RF1_DROP_60) && (randint0(100) < 60))
        number++;
    if ((r_ptr->flags1 & RF1_DROP_90) && (randint0(100) < 90))
        number++;
    if (r_ptr->flags1 & RF1_DROP_1D2)
        number += damroll(1, 2);
    if (r_ptr->flags1 & RF1_DROP_2D2)
        number += damroll(2, 2);
    if (r_ptr->flags1 & RF1_DROP_3D2)
        number += damroll(3, 2);
    if (r_ptr->flags1 & RF1_DROP_4D2)
        number += damroll(4, 2);

    if (cloned && !(r_ptr->flags1 & RF1_UNIQUE))
        number = 0;

    if (is_pet(m_ptr) || player_ptr->phase_out || floor_ptr->inside_arena)
        number = 0;

    if (!drop_item && (r_ptr->d_char != '$'))
        number = 0;

    if ((r_ptr->flags2 & (RF2_MULTIPLY)) && (r_ptr->r_akills > 1024))
        number = 0;

    coin_type = force_coin;
    floor_ptr->object_level = (floor_ptr->dun_level + r_ptr->level) / 2;

    int dump_item = 0;
    int dump_gold = 0;
    for (int i = 0; i < number; i++) {
        q_ptr = &forge;
        object_wipe(q_ptr);

        if (do_gold && (!do_item || (randint0(100) < 50))) {
            if (!make_gold(player_ptr, q_ptr))
                continue;
            dump_gold++;
        } else {
            if (!make_object(player_ptr, q_ptr, mo_mode))
                continue;
            dump_item++;
        }

        (void)drop_near(player_ptr, q_ptr, -1, y, x);
    }

    floor_ptr->object_level = floor_ptr->base_level;
    coin_type = 0;
    bool visible = (m_ptr->ml && !player_ptr->image) || ((r_ptr->flags1 & RF1_UNIQUE) != 0);
    if (visible && (dump_item || dump_gold)) {
        lore_treasure(player_ptr, m_idx, dump_item, dump_gold);
    }

    if (!(r_ptr->flags1 & RF1_QUESTOR))
        return;
    if (player_ptr->phase_out)
        return;
    if ((m_ptr->r_idx != MON_SERPENT) || cloned)
        return;

    current_world_ptr->total_winner = TRUE;
    player_ptr->redraw |= (PR_TITLE);
    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FINAL_QUEST_CLEAR);
    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, _("見事に変愚蛮怒の勝利者となった！", "finally became *WINNER* of Hengband!"));
    admire_from_patron(player_ptr);
    msg_print(_("*** おめでとう ***", "*** CONGRATULATIONS ***"));
    msg_print(_("あなたはゲームをコンプリートしました。", "You have won the game!"));
    msg_print(_("準備が整ったら引退(自殺コマンド)しても結構です。", "You may retire (commit suicide) when you are ready."));
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

    for (int i = 0; i < 4; i++) {
        if (r_ptr->blow[i].method == RBM_EXPLODE) {
            return _("は爆発して粉々になった。", " explodes into tiny shreds.");
        }
    }

    return _("を倒した。", " is destroyed.");
}
