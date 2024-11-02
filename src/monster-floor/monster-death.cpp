#include "monster-floor/monster-death.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest-completion-checker.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena-entry.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-floor/monster-death-util.h"
#include "monster-floor/monster-object.h"
#include "monster-floor/special-death-switcher.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "pet/pet-fall-off.h"
#include "player/patron.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/angband-system.h"
#include "system/artifact-type-definition.h"
#include "system/building-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/system-variables.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>

static void write_pet_death(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    md_ptr->md_y = md_ptr->m_ptr->fy;
    md_ptr->md_x = md_ptr->m_ptr->fx;
    if (record_named_pet && md_ptr->m_ptr->is_named_pet()) {
        const auto m_name = monster_desc(player_ptr, md_ptr->m_ptr, MD_INDEF_VISIBLE);
        exe_write_diary(*player_ptr->current_floor_ptr, DiaryKind::NAMED_PET, 3, m_name);
    }
}

static void on_dead_explosion(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    for (const auto &blow : md_ptr->r_ptr->blows) {
        if (blow.method != RaceBlowMethodType::EXPLODE) {
            continue;
        }

        BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        AttributeType typ = mbe_info[enum2i(blow.effect)].explode_type;
        const auto damage = blow.damage_dice.roll();
        (void)project(player_ptr, md_ptr->m_idx, 3, md_ptr->md_y, md_ptr->md_x, damage, typ, flg);
        break;
    }
}

static void on_defeat_arena_monster(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.inside_arena || md_ptr->m_ptr->is_pet()) {
        return;
    }

    AngbandWorld::get_instance().set_arena(true);
    auto &entries = ArenaEntryList::get_instance();
    const auto is_true_victor = entries.is_player_true_victor();
    if (is_true_victor) {
        msg_print(_("素晴らしい！君こそ真の勝利者だ。", "You are a Genuine Champion!"));
    } else {
        msg_print(_("勝利！チャンピオンへの道を進んでいる。", "Victorious! You're on your way to becoming Champion."));
    }

    const auto &bi_key = entries.get_bi_key();
    if (bi_key.is_valid()) {
        ItemEntity item(bi_key);
        ItemMagicApplier(player_ptr, &item, floor.object_level, AM_NO_FIXED_ART).execute();
        (void)drop_near(player_ptr, &item, -1, md_ptr->md_y, md_ptr->md_x);
    }

    if (is_true_victor) {
        entries.increment_entry();
    }

    entries.increment_entry();
    if (!record_arena) {
        return;
    }

    const auto m_name = monster_desc(player_ptr, md_ptr->m_ptr, MD_WRONGDOER_NAME);
    exe_write_diary(floor, DiaryKind::ARENA, 0, m_name);
}

static void drop_corpse(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto is_drop_corpse = one_in_(md_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE) ? 1 : 4);
    is_drop_corpse &= md_ptr->r_ptr->drop_flags.has_any_of({ MonsterDropType::DROP_CORPSE, MonsterDropType::DROP_SKELETON });
    is_drop_corpse &= !(floor_ptr->inside_arena || AngbandSystem::get_instance().is_phase_out() || md_ptr->cloned || ((md_ptr->m_ptr->r_idx == AngbandWorld::get_instance().today_mon) && md_ptr->m_ptr->is_pet()));
    if (!is_drop_corpse) {
        return;
    }

    bool corpse = false;
    if (md_ptr->r_ptr->drop_flags.has_not(MonsterDropType::DROP_SKELETON)) {
        corpse = true;
    } else if (md_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_CORPSE) && md_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        corpse = true;
    } else if (md_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_CORPSE)) {
        if ((0 - ((md_ptr->m_ptr->maxhp) / 4)) > md_ptr->m_ptr->hp) {
            if (one_in_(5)) {
                corpse = true;
            }
        } else {
            if (!one_in_(5)) {
                corpse = true;
            }
        }
    }

    ItemEntity item({ ItemKindType::MONSTER_REMAINS, (corpse ? SV_CORPSE : SV_SKELETON) });
    ItemMagicApplier(player_ptr, &item, floor_ptr->object_level, AM_NO_FIXED_ART).execute();
    item.pval = enum2i(md_ptr->m_ptr->r_idx);
    (void)drop_near(player_ptr, &item, -1, md_ptr->md_y, md_ptr->md_x);
}

/*!
 * @brief アーティファクトのドロップ判定処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター死亡構造体への参照ポインタ
 * @return 何かドロップするならドロップしたアーティファクトのID、何もドロップしないなら0
 */
static void drop_artifact_from_unique(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    const auto is_wizard = AngbandWorld::get_instance().wizard;
    for (const auto &[fa_id, chance] : md_ptr->r_ptr->get_drop_artifacts()) {
        if (!is_wizard && !evaluate_percent(chance)) {
            continue;
        }

        if (drop_single_artifact(player_ptr, md_ptr, fa_id)) {
            return;
        }
    }
}

/*!
 * @brief 特定アーティファクトのドロップ処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター死亡構造体への参照ポインタ
 * @param a_ix ドロップを試みるアーティファクトID
 * @return ドロップするならtrue
 */
bool drop_single_artifact(PlayerType *player_ptr, MonsterDeath *md_ptr, FixedArtifactId a_idx)
{
    auto &artifact = ArtifactList::get_instance().get_artifact(a_idx);
    if (artifact.is_generated) {
        return false;
    }

    return create_named_art(player_ptr, a_idx, md_ptr->md_y, md_ptr->md_x);
}

static std::optional<short> drop_dungeon_final_artifact(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    const auto &dungeon = player_ptr->current_floor_ptr->get_dungeon_definition();
    const auto has_reward = dungeon.final_object > 0;
    const auto bi_id = has_reward ? dungeon.final_object : BaseitemList::get_instance().lookup_baseitem_id({ ItemKindType::SCROLL, SV_SCROLL_ACQUIREMENT });
    if (dungeon.final_artifact == FixedArtifactId::NONE) {
        return bi_id;
    }

    const auto a_idx = dungeon.final_artifact;
    const auto &artifact = ArtifactList::get_instance().get_artifact(a_idx);
    if (artifact.is_generated) {
        return bi_id;
    }

    create_named_art(player_ptr, a_idx, md_ptr->md_y, md_ptr->md_x);
    return dungeon.final_object ? std::make_optional<short>(bi_id) : std::nullopt;
}

static void drop_artifacts(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!md_ptr->drop_chosen_item) {
        return;
    }

    drop_artifact_from_unique(player_ptr, md_ptr);
    const auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto &dungeon = floor_ptr->get_dungeon_definition();
    if (md_ptr->r_ptr->misc_flags.has_not(MonsterMiscType::GUARDIAN) || (dungeon.final_guardian != md_ptr->m_ptr->r_idx)) {
        return;
    }

    const auto bi_id = drop_dungeon_final_artifact(player_ptr, md_ptr);
    if (bi_id) {
        ItemEntity item(*bi_id);
        ItemMagicApplier(player_ptr, &item, floor_ptr->object_level, AM_NO_FIXED_ART | AM_GOOD).execute();
        (void)drop_near(player_ptr, &item, -1, md_ptr->md_y, md_ptr->md_x);
    }

    msg_format(_("あなたは%sを制覇した！", "You have conquered %s!"), dungeon.name.data());
}

static void decide_drop_quality(MonsterDeath *md_ptr)
{
    md_ptr->mo_mode = 0L;
    if (md_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_GOOD)) {
        md_ptr->mo_mode |= AM_GOOD;
    }

    if (md_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_GREAT)) {
        md_ptr->mo_mode |= (AM_GOOD | AM_GREAT);
    }
}

static int decide_drop_numbers(MonsterDeath *md_ptr, const bool drop_item, const bool inside_arena)
{
    int drop_numbers = 0;
    if (md_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_60) && evaluate_percent(60)) {
        drop_numbers++;
    }

    if (md_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_90) && evaluate_percent(90)) {
        drop_numbers++;
    }

    if (md_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_1D2)) {
        drop_numbers += Dice::roll(1, 2);
    }

    if (md_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_2D2)) {
        drop_numbers += Dice::roll(2, 2);
    }

    if (md_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_3D2)) {
        drop_numbers += Dice::roll(3, 2);
    }

    if (md_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_4D2)) {
        drop_numbers += Dice::roll(4, 2);
    }

    if (md_ptr->cloned && md_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        drop_numbers = 0;
    }

    if (md_ptr->m_ptr->is_pet() || AngbandSystem::get_instance().is_phase_out() || inside_arena) {
        drop_numbers = 0;
    }

    if (!drop_item && !md_ptr->r_ptr->symbol_char_is_any_of("$")) {
        drop_numbers = 0;
    }

    if (md_ptr->r_ptr->misc_flags.has(MonsterMiscType::MULTIPLY) && (md_ptr->r_ptr->r_akills > 1024)) {
        drop_numbers = 0;
    }

    return drop_numbers;
}

static void drop_items_golds(PlayerType *player_ptr, MonsterDeath *md_ptr, int drop_numbers)
{
    int dump_item = 0;
    int dump_gold = 0;
    for (int i = 0; i < drop_numbers; i++) {
        ItemEntity forge;
        auto *q_ptr = &forge;
        q_ptr->wipe();
        if (md_ptr->do_gold && (!md_ptr->do_item || one_in_(2))) {
            if (!make_gold(q_ptr)) {
                continue;
            }

            dump_gold++;
        } else {
            if (!make_object(player_ptr, q_ptr, md_ptr->mo_mode)) {
                continue;
            }

            dump_item++;
        }

        (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->object_level = floor_ptr->base_level;
    coin_type = 0;
    auto visible = md_ptr->m_ptr->ml && !player_ptr->effects()->hallucination().is_hallucinated();
    visible |= (md_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE));
    if (visible && (dump_item || dump_gold)) {
        md_ptr->m_ptr->make_lore_treasure(dump_item, dump_gold);
    }
}

/*!
 * @brief 最終ボス(混沌のサーペント)を倒したときの処理
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
static void on_defeat_last_boss(PlayerType *player_ptr)
{
    auto &world = AngbandWorld::get_instance();
    world.total_winner = true;
    world.add_winner_class(player_ptr->pclass);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TITLE);
    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FINAL_QUEST_CLEAR);
    exe_write_diary(*player_ptr->current_floor_ptr, DiaryKind::DESCRIPTION, 0, _("見事に変愚蛮怒の勝利者となった！", "finally became *WINNER* of Hengband!"));
    patron_list[player_ptr->chaos_patron].admire(player_ptr);
    msg_print(_("*** おめでとう ***", "*** CONGRATULATIONS ***"));
    msg_print(_("あなたはゲームをコンプリートしました。", "You have won the game!"));
    msg_print(_("準備が整ったら引退(自殺コマンド)しても結構です。", "You may retire (commit suicide) when you are ready."));
}

/*!
 * @brief モンスターが死亡した時の処理 /
 * Handle the "death" of a monster.
 * @param m_idx 死亡したモンスターのID
 * @param drop_item TRUEならばモンスターのドロップ処理を行う
 * @param type ラストアタックの属性 (単一属性)
 */
void monster_death(PlayerType *player_ptr, MONSTER_IDX m_idx, bool drop_item, AttributeType type)
{
    AttributeFlags flags;
    flags.clear();
    flags.set(type);
    monster_death(player_ptr, m_idx, drop_item, flags);
}

/*!
 * @brief モンスターが死亡した時の処理 /
 * Handle the "death" of a monster.
 * @param m_idx 死亡したモンスターのID
 * @param drop_item TRUEならばモンスターのドロップ処理を行う
 * @param attribute_flags ラストアタックの属性 (複数属性)
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
void monster_death(PlayerType *player_ptr, MONSTER_IDX m_idx, bool drop_item, AttributeFlags attribute_flags)
{
    auto &floor = *player_ptr->current_floor_ptr;
    MonsterDeath tmp_md(floor, m_idx, drop_item);
    MonsterDeath *md_ptr = &tmp_md;
    auto &world = AngbandWorld::get_instance();
    if ((world.timewalk_m_idx > 0) && world.timewalk_m_idx == m_idx) {
        world.timewalk_m_idx = 0;
    }

    // プレイヤーしかユニークを倒せないのでここで時間を記録
    if (md_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE) && md_ptr->m_ptr->mflag2.has_not(MonsterConstantFlagType::CLONED)) {
        world.update_playtime();
        md_ptr->r_ptr->defeat_time = world.play_time;
        md_ptr->r_ptr->defeat_level = player_ptr->lev;
    }

    if (md_ptr->r_ptr->brightness_flags.has_any_of(ld_mask)) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    write_pet_death(player_ptr, md_ptr);
    on_dead_explosion(player_ptr, md_ptr);
    if (md_ptr->m_ptr->mflag2.has(MonsterConstantFlagType::CHAMELEON)) {
        md_ptr->m_ptr->reset_chameleon_polymorph();
        md_ptr->r_ptr = &md_ptr->m_ptr->get_monrace();
    }

    QuestCompletionChecker(player_ptr, md_ptr->m_ptr).complete();
    on_defeat_arena_monster(player_ptr, md_ptr);
    if (md_ptr->m_ptr->is_riding() && process_fall_off_horse(player_ptr, -1, false)) {
        msg_print(_("地面に落とされた。", "You have fallen from the pet you were riding."));
    }

    drop_corpse(player_ptr, md_ptr);
    monster_drop_carried_objects(player_ptr, md_ptr->m_ptr);
    decide_drop_quality(md_ptr);
    switch_special_death(player_ptr, md_ptr, attribute_flags);
    drop_artifacts(player_ptr, md_ptr);
    int drop_numbers = decide_drop_numbers(md_ptr, drop_item, floor.inside_arena);
    coin_type = md_ptr->force_coin;
    floor.object_level = (floor.dun_level + md_ptr->r_ptr->level) / 2;
    drop_items_golds(player_ptr, md_ptr, drop_numbers);
    if ((md_ptr->r_ptr->misc_flags.has_not(MonsterMiscType::QUESTOR)) || AngbandSystem::get_instance().is_phase_out() || (md_ptr->m_ptr->r_idx != MonsterRaceId::SERPENT) || md_ptr->cloned) {
        return;
    }

    on_defeat_last_boss(player_ptr);
}
