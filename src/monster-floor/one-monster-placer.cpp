/*!
 * @brief モンスターをフロアに1体配置する処理
 * @date 2020/06/13
 * @author Hourier
 */

#include "monster-floor/one-monster-placer.h"
#include "core/speed-table.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-save-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "grid/grid.h"
#include "monster-floor/monster-move.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "object/warning.h"
#include "player/player-status.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"
#include <range/v3/algorithm.hpp>

/*!
 * @param player_ptr プレイヤーへの参照ポインタ
 * @brief モンスターの表層IDを設定する / Set initial racial appearance of a monster
 * @param monrace_id モンスター種族ID
 * @return モンスター種族の表層ID
 */
static MonraceId initial_monrace_appearance(PlayerType *player_ptr, MonraceId monrace_id, BIT_FLAGS generate_mode)
{
    if (is_chargeman(player_ptr) && any_bits(generate_mode, PM_JURAL) && none_bits(generate_mode, PM_MULTIPLY | PM_KAGE)) {
        return MonraceId::ALIEN_JURAL;
    }

    const auto &monraces = MonraceList::get_instance();
    if (monraces.get_monrace(monrace_id).misc_flags.has_not(MonsterMiscType::TANUKI)) {
        return monrace_id;
    }

    get_mon_num_prep_enum(player_ptr, MonraceHook::TANUKI);
    auto attempts = 1000;
    const auto &floor = *player_ptr->current_floor_ptr;
    auto min = std::min(floor.base_level - 5, 50);
    while (--attempts) {
        auto appearance_monrace_id = get_mon_num(player_ptr, 0, floor.base_level + 10, PM_NONE);
        if (monraces.get_monrace(appearance_monrace_id).level >= min) {
            return appearance_monrace_id;
        }
    }

    return monrace_id;
}

/*!
 * @brief ユニークが生成可能か評価する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monrace_id 生成モンスター種族
 * @return ユニークの生成が不可能な条件ならFALSE、それ以外はTRUE
 */
static bool check_unique_placeable(const FloorType &floor, MonraceId monrace_id, BIT_FLAGS mode)
{
    if (AngbandSystem::get_instance().is_phase_out()) {
        return true;
    }

    if (any_bits(mode, PM_CLONE)) {
        return true;
    }

    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    auto is_unique = monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.population_flags.has(MonsterPopulationType::NAZGUL);
    is_unique &= monrace.cur_num >= monrace.max_num;
    if (is_unique) {
        return false;
    }

    if (monrace.population_flags.has(MonsterPopulationType::ONLY_ONE) && monrace.has_entity()) {
        return false;
    }

    if (!MonraceList::get_instance().is_selectable(monrace_id)) {
        return false;
    }

    const auto is_deep = monrace.misc_flags.has(MonsterMiscType::FORCE_DEPTH) && (floor.dun_level < monrace.level);
    const auto is_questor = !ironman_nightmare || monrace.misc_flags.has(MonsterMiscType::QUESTOR);
    return !is_deep || !is_questor;
}

/*!
 * @brief クエスト内に生成可能か評価する
 * @param floor フロアへの参照
 * @param r_idx 生成モンスター種族
 * @return 生成が可能ならTRUE、不可能ならFALSE
 */
static bool check_quest_placeable(const FloorType &floor, MonraceId r_idx)
{
    if (!inside_quest(floor.get_quest_id())) {
        return true;
    }

    const auto &quests = QuestList::get_instance();
    const auto quest_id = floor.get_quest_id();
    const auto &quest = quests.get_quest(quest_id);
    if ((quest.type != QuestKindType::KILL_LEVEL) && (quest.type != QuestKindType::RANDOM)) {
        return true;
    }
    if (r_idx != quest.r_idx) {
        return true;
    }
    const auto has_quest_monrace = [&](const Pos2D &pos) {
        const auto &grid = floor.get_grid(pos);
        return grid.has_monster() && (floor.m_list[grid.m_idx].r_idx == quest.r_idx);
    };
    const auto number_mon = ranges::count_if(floor.get_area(), has_quest_monrace);

    if (number_mon + quest.cur_num >= quest.max_num) {
        return false;
    }
    return true;
}

/*!
 * @brief 守りのルーン上にモンスターの配置を試みる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monrace_id 生成モンスター種族
 * @param pos 生成位置
 * @return 生成が可能ならTRUE、不可能ならFALSE
 */
static bool check_procection_rune(PlayerType *player_ptr, MonraceId monrace_id, const Pos2D &pos)
{
    auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    if (!grid.is_rune_protection()) {
        return true;
    }

    auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    if (randint1(BREAK_RUNE_PROTECTION) >= (monrace.level + 20)) {
        return false;
    }

    if (any_bits(grid.info, CAVE_MARK)) {
        msg_print(_("守りのルーンが壊れた！", "The rune of protection is broken!"));
    }

    reset_bits(grid.info, CAVE_MARK);
    reset_bits(grid.info, CAVE_OBJECT);
    grid.mimic = 0;
    note_spot(player_ptr, pos);
    return true;
}

static void warn_unique_generation(PlayerType *player_ptr, MonraceId r_idx)
{
    if (!player_ptr->warning || !AngbandWorld::get_instance().character_dungeon) {
        return;
    }

    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return;
    }

    std::string color;
    if (monrace.level > player_ptr->lev + 30) {
        color = _("黒く", "black");
    } else if (monrace.level > player_ptr->lev + 15) {
        color = _("紫色に", "purple");
    } else if (monrace.level > player_ptr->lev + 5) {
        color = _("ルビー色に", "deep red");
    } else if (monrace.level > player_ptr->lev - 5) {
        color = _("赤く", "red");
    } else if (monrace.level > player_ptr->lev - 15) {
        color = _("ピンク色に", "pink");
    } else {
        color = _("白く", "white");
    }

    auto *o_ptr = choose_warning_item(player_ptr);
    if (o_ptr != nullptr) {
        const auto item_name = describe_flavor(player_ptr, *o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        msg_format(_("%sは%s光った。", "%s glows %s."), item_name.data(), color.data());
    } else {
        msg_format(_("%s光る物が頭に浮かんだ。", "A %s image forms in your mind."), color.data());
    }
}

/*!
 * @brief モンスターを一体生成する / Attempt to place a monster of the given race at the given location.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 生成位置y座標
 * @param x 生成位置x座標
 * @param r_idx 生成モンスター種族
 * @param mode 生成オプション
 * @param summoner_m_idx モンスターの召喚による場合、召喚主のモンスターID
 * @return 生成に成功したらモンスターID、失敗したらtl::nullopt
 */
tl::optional<MONSTER_IDX> place_monster_one(PlayerType *player_ptr, POSITION y, POSITION x, MonraceId r_idx, BIT_FLAGS mode, tl::optional<MONSTER_IDX> summoner_m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const Pos2D pos(y, x);
    auto &grid = floor.get_grid(pos);
    auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &world = AngbandWorld::get_instance();
    if (world.is_wild_mode() || !floor.contains(pos) || !MonraceList::is_valid(r_idx)) {
        return tl::nullopt;
    }

    if (none_bits(mode, PM_IGNORE_TERRAIN) && (grid.has(TerrainCharacteristics::PATTERN) || !monster_can_enter(player_ptr, pos.y, pos.x, monrace, 0))) {
        return tl::nullopt;
    }

    if (!check_unique_placeable(floor, r_idx, mode) || !check_quest_placeable(floor, r_idx) || !check_procection_rune(player_ptr, r_idx, pos)) {
        return tl::nullopt;
    }

    msg_format_wizard(player_ptr, CHEAT_MONSTER, _("%s(Lv%d)を生成しました。", "%s(Lv%d) was generated."), monrace.name.data(), monrace.level);
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.population_flags.has(MonsterPopulationType::NAZGUL) || (monrace.level < 10)) {
        reset_bits(mode, PM_KAGE);
    }

    const auto m_idx = floor.pop_empty_index_monster();
    grid.m_idx = m_idx;
    if (!grid.has_monster()) {
        return tl::nullopt;
    }

    auto &monster = floor.m_list[grid.m_idx];
    monster.mflag.clear();
    monster.mflag2.clear();
    if (monrace.misc_flags.has(MonsterMiscType::CHAMELEON)) {
        monster.r_idx = r_idx;
        choose_chameleon_polymorph(player_ptr, grid.m_idx, grid.get_terrain_id(), summoner_m_idx);
        monster.mflag2.set(MonsterConstantFlagType::CHAMELEON);
    } else {
        monster.r_idx = r_idx;
        if (any_bits(mode, PM_KAGE) && none_bits(mode, PM_FORCE_PET)) {
            monster.ap_r_idx = MonraceId::KAGE;
            monster.mflag2.set(MonsterConstantFlagType::KAGE);
        } else {
            monster.ap_r_idx = initial_monrace_appearance(player_ptr, r_idx, mode);
        }
    }

    const auto &new_monrace = monster.get_monrace();
    const auto is_summoned = summoner_m_idx.has_value();
    const auto &summoner = floor.m_list[summoner_m_idx.value_or(0)];

    auto same_appearance_as_parent = monster.mflag2.has_not(MonsterConstantFlagType::CHAMELEON);
    same_appearance_as_parent &= any_bits(mode, PM_MULTIPLY);
    same_appearance_as_parent &= is_summoned && !summoner.is_original_ap();

    if (same_appearance_as_parent) {
        monster.ap_r_idx = summoner.ap_r_idx;
        if (summoner.mflag2.has(MonsterConstantFlagType::KAGE)) {
            monster.mflag2.set(MonsterConstantFlagType::KAGE);
        }
    }

    if (monster.mflag2.has_not(MonsterConstantFlagType::CHAMELEON) && is_summoned && new_monrace.kind_flags.has_none_of(alignment_mask)) {
        monster.sub_align = summoner.sub_align;
    } else if (monster.mflag2.has(MonsterConstantFlagType::CHAMELEON) && new_monrace.kind_flags.has(MonsterKindType::UNIQUE) && !is_summoned) {
        monster.sub_align = SUB_ALIGN_NEUTRAL;
    } else {
        monster.sub_align = SUB_ALIGN_NEUTRAL;
        if (new_monrace.kind_flags.has(MonsterKindType::EVIL)) {
            set_bits(monster.sub_align, SUB_ALIGN_EVIL);
        }
        if (new_monrace.kind_flags.has(MonsterKindType::GOOD)) {
            set_bits(monster.sub_align, SUB_ALIGN_GOOD);
        }
    }

    monster.fy = pos.y;
    monster.fx = pos.x;
    monster.current_floor_ptr = &floor;

    for (const auto mte : MONSTER_TIMED_EFFECT_RANGE) {
        monster.mtimed[mte] = 0;
    }

    monster.cdis = 0;
    monster.reset_target();
    monster.nickname.clear();
    monster.exp = 0;

    if (is_summoned && summoner.is_pet()) {
        set_bits(mode, PM_FORCE_PET);
        monster.parent_m_idx = *summoner_m_idx;
    } else {
        monster.parent_m_idx = 0;
    }

    if (any_bits(mode, PM_CLONE)) {
        monster.mflag2.set(MonsterConstantFlagType::CLONED);
    }

    if (any_bits(mode, PM_NO_PET)) {
        monster.mflag2.set(MonsterConstantFlagType::NOPET);
    }

    monster.ml = false;
    if (any_bits(mode, PM_FORCE_PET)) {
        set_pet(player_ptr, monster);
    } else {
        auto should_be_friendly = !is_summoned && new_monrace.behavior_flags.has(MonsterBehaviorType::FRIENDLY);
        should_be_friendly |= is_summoned && summoner.is_friendly();
        should_be_friendly |= any_bits(mode, PM_FORCE_FRIENDLY);
        auto force_hostile = monster_has_hostile_to_player(player_ptr, 0, -1, new_monrace);
        force_hostile |= player_ptr->current_floor_ptr->inside_arena;
        if (should_be_friendly && !force_hostile) {
            monster.set_friendly();
        }
    }

    monster.mtimed[MonsterTimedEffect::SLEEP] = 0;
    if (any_bits(mode, PM_ALLOW_SLEEP) && new_monrace.sleep && !ironman_nightmare) {
        const auto val = new_monrace.sleep;
        (void)set_monster_csleep(player_ptr, grid.m_idx, (val * 2) + randint1(val * 10));
    }

    if (new_monrace.misc_flags.has(MonsterMiscType::FORCE_MAXHP)) {
        monster.max_maxhp = new_monrace.hit_dice.maxroll();
    } else {
        monster.max_maxhp = new_monrace.hit_dice.roll();
    }

    if (ironman_nightmare) {
        auto hp = monster.max_maxhp * 2;
        monster.max_maxhp = std::min(MONSTER_MAXHP, hp);
    }

    monster.maxhp = monster.max_maxhp;
    if (new_monrace.cur_hp_per != 0) {
        monster.hp = monster.maxhp * new_monrace.cur_hp_per / 100;
    } else {
        monster.hp = monster.maxhp;
    }

    monster.dealt_damage = 0;

    monster.set_individual_speed(floor.inside_arena);

    if (any_bits(mode, PM_HASTE)) {
        (void)set_monster_fast(player_ptr, grid.m_idx, 100);
    }

    if (!ironman_nightmare) {
        monster.energy_need = ENERGY_NEED() - randnum0<short>(100);
    } else {
        monster.energy_need = ENERGY_NEED() - randnum0<short>(100) * 2;
    }

    if (!ironman_nightmare) {
        monster.mflag.set(MonsterTemporaryFlagType::PREVENT_MAGIC);
    }

    auto is_awake_lightning_monster = new_monrace.brightness_flags.has_any_of(self_ld_mask);
    is_awake_lightning_monster |= new_monrace.brightness_flags.has_any_of(has_ld_mask) && !monster.is_asleep();
    if (is_awake_lightning_monster) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    update_monster(player_ptr, grid.m_idx, true);
    monster.get_real_monrace().increment_current_numbers();

    /*
     * Memorize location of the unique monster in saved floors.
     * A unique monster move from old saved floor.
     */
    if (world.character_dungeon && (new_monrace.kind_flags.has(MonsterKindType::UNIQUE) || new_monrace.population_flags.has(MonsterPopulationType::NAZGUL))) {
        monster.get_real_monrace().floor_id = player_ptr->floor_id;
    }

    if (new_monrace.misc_flags.has(MonsterMiscType::MULTIPLY)) {
        floor.num_repro++;
    }

    warn_unique_generation(player_ptr, r_idx);
    activate_explosive_rune(player_ptr, pos, new_monrace);
    return monster.is_valid() ? tl::make_optional(grid.m_idx) : tl::nullopt;
}
