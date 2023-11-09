#include "effect/effect-feature.h"
#include "dungeon/dungeon-flag-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h" // 暫定、後で消す.
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-elementalist.h"
#include "mind/mind-ninja.h"
#include "monster/monster-update.h"
#include "player/special-defense-types.h"
#include "room/door-definition.h"
#include "spell-class/spells-mirror-master.h"
#include "system/angband-system.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*
 * Determine if a "legal" grid is an "naked" floor grid
 *
 * Line 1 -- forbid non-clean gird
 * Line 2 -- forbid monsters
 * Line 3 -- forbid the player
 */
static bool cave_naked_bold(PlayerType *player_ptr, const Pos2D &pos)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    return cave_clean_bold(floor_ptr, pos.y, pos.x) && (floor_ptr->get_grid(pos).m_idx == 0) && !player_ptr->is_located_at(pos);
}

/*!
 * @brief 汎用的なビーム/ボルト/ボール系による地形効果処理 / We are called from "project()" to "damage" terrain features
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 * @details
 * <pre>
 * We are called both for "beam" effects and "ball" effects.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 *
 * We also "see" grids which are "memorized", probably a hack
 *
 * Perhaps we should affect doors?
 * </pre>
 */
bool affect_feature(PlayerType *player_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, int dam, AttributeType typ)
{
    const Pos2D pos(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    const auto &terrain = grid.get_terrain();

    auto obvious = false;
    auto known = grid.has_los();

    who = who ? who : 0;
    dam = (dam + r) / (r + 1);

    if (terrain.flags.has(TerrainCharacteristics::TREE)) {
        concptr message;
        switch (typ) {
        case AttributeType::POIS:
        case AttributeType::NUKE:
        case AttributeType::DEATH_RAY:
            message = _("枯れた", "was blasted.");
            break;
        case AttributeType::TIME:
            message = _("縮んだ", "shrank.");
            break;
        case AttributeType::ACID:
            message = _("溶けた", "melted.");
            break;
        case AttributeType::COLD:
        case AttributeType::ICE:
            message = _("凍り、砕け散った", "was frozen and smashed.");
            break;
        case AttributeType::FIRE:
        case AttributeType::ELEC:
        case AttributeType::PLASMA:
            message = _("燃えた", "burns up!");
            break;
        case AttributeType::METEOR:
        case AttributeType::CHAOS:
        case AttributeType::MANA:
        case AttributeType::SEEKER:
        case AttributeType::SUPER_RAY:
        case AttributeType::SHARDS:
        case AttributeType::ROCKET:
        case AttributeType::SOUND:
        case AttributeType::DISENCHANT:
        case AttributeType::FORCE:
        case AttributeType::GRAVITY:
            message = _("粉砕された", "was crushed.");
            break;
        case AttributeType::VOID_MAGIC:
            message = _("消滅した", "vanished.");
            break;
        default:
            message = nullptr;
            break;
        }

        if (message) {
            msg_format(_("木は%s。", "A tree %s"), message);
            cave_set_feat(player_ptr, y, x, one_in_(3) ? feat_brake : feat_grass);

            /* Observe */
            if (grid.is_mark()) {
                obvious = true;
            }
        }
    }

    /* Analyze the type */
    switch (typ) {
        /* Ignore most effects */
    case AttributeType::CAPTURE:
    case AttributeType::HAND_DOOM:
    case AttributeType::CAUSE_1:
    case AttributeType::CAUSE_2:
    case AttributeType::CAUSE_3:
    case AttributeType::CAUSE_4:
    case AttributeType::MIND_BLAST:
    case AttributeType::BRAIN_SMASH:
    case AttributeType::DRAIN_MANA:
    case AttributeType::PSY_SPEAR:
    case AttributeType::FORCE:
    case AttributeType::HOLY_FIRE:
    case AttributeType::HELL_FIRE:
    case AttributeType::PSI:
    case AttributeType::PSI_DRAIN:
    case AttributeType::TELEKINESIS:
    case AttributeType::DOMINATION:
    case AttributeType::IDENTIFY:
    case AttributeType::ATTACK:
    case AttributeType::ACID:
    case AttributeType::ELEC:
    case AttributeType::COLD:
    case AttributeType::ICE:
    case AttributeType::FIRE:
    case AttributeType::PLASMA:
    case AttributeType::METEOR:
    case AttributeType::CHAOS:
    case AttributeType::MANA:
    case AttributeType::SEEKER:
    case AttributeType::SUPER_RAY: {
        break;
    }
    case AttributeType::KILL_TRAP: {
        if (is_hidden_door(player_ptr, grid)) {
            disclose_grid(player_ptr, y, x);
            if (known) {
                obvious = true;
            }
        }

        if (is_trap(player_ptr, grid.feat)) {
            if (known) {
                msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
                obvious = true;
            }

            cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::DISARM);
        }

        if (is_closed_door(player_ptr, grid.feat) && terrain.power && terrain.flags.has(TerrainCharacteristics::OPEN)) {
            FEAT_IDX old_feat = grid.feat;
            cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::DISARM);
            if (known && (old_feat != grid.feat)) {
                msg_print(_("カチッと音がした！", "Click!"));
                obvious = true;
            }
        }

        if (player_ptr->effects()->blindness()->is_blind() || !grid.has_los()) {
            break;
        }

        grid.info &= ~(CAVE_UNSAFE);
        lite_spot(player_ptr, y, x);
        obvious = true;
        break;
    }
    case AttributeType::KILL_DOOR: {
        if (is_trap(player_ptr, grid.feat) || terrain.flags.has(TerrainCharacteristics::DOOR)) {
            if (known) {
                msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
                obvious = true;
            }

            cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::TUNNEL);
        }

        if (player_ptr->effects()->blindness()->is_blind() || !grid.has_los()) {
            break;
        }

        grid.info &= ~(CAVE_UNSAFE);
        lite_spot(player_ptr, y, x);
        obvious = true;
        break;
    }
    case AttributeType::JAM_DOOR: {
        if (terrain.flags.has_not(TerrainCharacteristics::SPIKE)) {
            break;
        }

        int16_t old_mimic = grid.mimic;
        const auto &terrain_mimic = grid.get_terrain_mimic();

        cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::SPIKE);
        grid.mimic = old_mimic;

        note_spot(player_ptr, y, x);
        lite_spot(player_ptr, y, x);

        if (!known || terrain_mimic.flags.has_not(TerrainCharacteristics::OPEN)) {
            break;
        }

        msg_format(_("%sに何かがつっかえて開かなくなった。", "The %s seems stuck."), terrain_mimic.name.data());
        obvious = true;
        break;
    }
    case AttributeType::KILL_WALL: {
        if (terrain.flags.has_not(TerrainCharacteristics::HURT_ROCK)) {
            break;
        }

        if (known && grid.is_mark()) {
            msg_format(_("%sが溶けて泥になった！", "The %s turns into mud!"), grid.get_terrain_mimic().name.data());
            obvious = true;
        }

        cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::HURT_ROCK);
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
        break;
    }
    case AttributeType::MAKE_DOOR: {
        if (!cave_naked_bold(player_ptr, pos)) {
            break;
        }
        if (player_ptr->is_located_at(pos)) {
            break;
        }
        cave_set_feat(player_ptr, y, x, feat_door[DOOR_DOOR].closed);
        if (grid.is_mark()) {
            obvious = true;
        }
        break;
    }
    case AttributeType::MAKE_TRAP: {
        place_trap(&floor, y, x);
        break;
    }
    case AttributeType::MAKE_TREE: {
        if (!cave_naked_bold(player_ptr, pos)) {
            break;
        }
        if (player_ptr->is_located_at(pos)) {
            break;
        }
        cave_set_feat(player_ptr, y, x, feat_tree);
        if (grid.is_mark()) {
            obvious = true;
        }
        break;
    }
    case AttributeType::MAKE_RUNE_PROTECTION: {
        if (!cave_naked_bold(player_ptr, pos)) {
            break;
        }
        grid.info |= CAVE_OBJECT;
        grid.mimic = feat_rune_protection;
        note_spot(player_ptr, y, x);
        lite_spot(player_ptr, y, x);
        break;
    }
    case AttributeType::STONE_WALL: {
        if (!cave_naked_bold(player_ptr, pos)) {
            break;
        }
        if (player_ptr->is_located_at(pos)) {
            break;
        }
        cave_set_feat(player_ptr, y, x, feat_granite);
        break;
    }
    case AttributeType::LAVA_FLOW: {
        if (terrain.flags.has(TerrainCharacteristics::PERMANENT)) {
            break;
        }
        if (dam == 1) {
            if (terrain.flags.has_not(TerrainCharacteristics::FLOOR)) {
                break;
            }
            cave_set_feat(player_ptr, y, x, feat_shallow_lava);
        } else if (dam) {
            cave_set_feat(player_ptr, y, x, feat_deep_lava);
        }

        break;
    }
    case AttributeType::WATER_FLOW: {
        if (terrain.flags.has(TerrainCharacteristics::PERMANENT)) {
            break;
        }
        if (dam == 1) {
            if (terrain.flags.has_not(TerrainCharacteristics::FLOOR)) {
                break;
            }
            cave_set_feat(player_ptr, y, x, feat_shallow_water);
        } else if (dam) {
            cave_set_feat(player_ptr, y, x, feat_deep_water);
        }

        break;
    }
    case AttributeType::LITE_WEAK:
    case AttributeType::LITE: {
        if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
            break;
        }

        grid.info |= (CAVE_GLOW);
        note_spot(player_ptr, y, x);
        lite_spot(player_ptr, y, x);
        update_local_illumination(player_ptr, y, x);

        if (player_can_see_bold(player_ptr, y, x)) {
            obvious = true;
        }
        if (grid.m_idx) {
            update_monster(player_ptr, grid.m_idx, false);
        }

        if (player_ptr->is_located_at(pos)) {
            set_superstealth(player_ptr, false);
        }

        break;
    }
    case AttributeType::DARK_WEAK:
    case AttributeType::DARK:
    case AttributeType::ABYSS: {
        auto do_dark = !AngbandSystem::get_instance().is_phase_out() && !grid.is_mirror();
        if (!do_dark) {
            break;
        }

        if ((floor.dun_level > 0) || !is_daytime()) {
            for (int j = 0; j < 9; j++) {
                const Pos2D pos_neighbor(y + ddy_ddd[j], x + ddx_ddd[j]);
                if (!in_bounds2(&floor, pos_neighbor.y, pos_neighbor.x)) {
                    continue;
                }

                const auto &grid_neighbor = floor.get_grid(pos_neighbor);
                if (grid_neighbor.get_terrain_mimic().flags.has(TerrainCharacteristics::GLOW)) {
                    do_dark = false;
                    break;
                }
            }

            if (!do_dark) {
                break;
            }
        }

        grid.info &= ~(CAVE_GLOW);

        /* Hack -- Forget "boring" grids */
        if (terrain.flags.has_not(TerrainCharacteristics::REMEMBER) || has_element_resist(player_ptr, ElementRealmType::DARKNESS, 1)) {
            /* Forget */
            grid.info &= ~(CAVE_MARK);
            note_spot(player_ptr, y, x);
        }

        lite_spot(player_ptr, y, x);

        update_local_illumination(player_ptr, y, x);

        if (player_can_see_bold(player_ptr, y, x)) {
            obvious = true;
        }
        if (grid.m_idx) {
            update_monster(player_ptr, grid.m_idx, false);
        }

        break;
    }
    case AttributeType::SHARDS:
    case AttributeType::ROCKET: {
        if (grid.is_mirror()) {
            msg_print(_("鏡が割れた！", "The mirror was shattered!"));
            sound(SOUND_GLASS);
            SpellsMirrorMaster(player_ptr).remove_mirror(y, x);
            project(player_ptr, 0, 2, y, x, player_ptr->lev / 2 + 5, AttributeType::SHARDS,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI));
        }

        if (terrain.flags.has_not(TerrainCharacteristics::GLASS) || terrain.flags.has(TerrainCharacteristics::PERMANENT) || (dam < 50)) {
            break;
        }

        if (known && (grid.is_mark())) {
            msg_format(_("%sが割れた！", "The %s crumbled!"), grid.get_terrain_mimic().name.data());
            sound(SOUND_GLASS);
        }

        cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::HURT_ROCK);
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
        break;
    }
    case AttributeType::SOUND: {
        if (grid.is_mirror() && player_ptr->lev < 40) {
            msg_print(_("鏡が割れた！", "The mirror was shattered!"));
            sound(SOUND_GLASS);
            SpellsMirrorMaster(player_ptr).remove_mirror(y, x);
            project(player_ptr, 0, 2, y, x, player_ptr->lev / 2 + 5, AttributeType::SHARDS,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI));
        }

        if (terrain.flags.has_not(TerrainCharacteristics::GLASS) || terrain.flags.has(TerrainCharacteristics::PERMANENT) || (dam < 200)) {
            break;
        }

        if (known && (grid.is_mark())) {
            msg_format(_("%sが割れた！", "The %s crumbled!"), grid.get_terrain_mimic().name.data());
            sound(SOUND_GLASS);
        }

        cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::HURT_ROCK);
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
        break;
    }
    case AttributeType::DISINTEGRATE: {
        if (grid.is_mirror() || grid.is_rune_protection() || grid.is_rune_explosion()) {
            SpellsMirrorMaster(player_ptr).remove_mirror(y, x);
        }

        if (terrain.flags.has_not(TerrainCharacteristics::HURT_DISI) || terrain.flags.has(TerrainCharacteristics::PERMANENT)) {
            break;
        }

        cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::HURT_DISI);
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
        break;
    }
    default:
        break;
    }

    lite_spot(player_ptr, y, x);
    return obvious;
}
