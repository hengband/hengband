#include "spell-kind/spells-detection.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "floor/floor-save-util.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "object/object-mark-types.h"
#include "object/tval-types.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-song.h"
#include "spell-realm/spells-song.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤー周辺の地形を感知する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param flag 特定地形ID
 * @param known 地形から危険フラグを外すならTRUE
 * @return 効力があった場合TRUEを返す
 */
static bool detect_feat_flag(PlayerType *player_ptr, POSITION range, TerrainCharacteristics flag, bool known)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    bool detect = false;
    for (POSITION y = 1; y < floor.height - 1; y++) {
        for (POSITION x = 1; x <= floor.width - 1; x++) {
            int dist = distance(player_ptr->y, player_ptr->x, y, x);
            if (dist > range) {
                continue;
            }

            auto *g_ptr = &floor.grid_array[y][x];
            if (flag == TerrainCharacteristics::TRAP) {
                /* Mark as detected */
                if (dist <= range && known) {
                    if (dist <= range - 1) {
                        g_ptr->info |= (CAVE_IN_DETECT);
                    }

                    g_ptr->info &= ~(CAVE_UNSAFE);

                    lite_spot(player_ptr, y, x);
                }
            }

            if (g_ptr->cave_has_flag(flag)) {
                disclose_grid(player_ptr, y, x);
                g_ptr->info |= (CAVE_MARK);
                lite_spot(player_ptr, y, x);
                detect = true;
            }
        }
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺のトラップを感知する / Detect all traps on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param known 感知外範囲を超える警告フラグを立てる場合TRUEを返す
 * @return 効力があった場合TRUEを返す
 * @details
 * 吟遊詩人による感知についてはFALSEを返す
 */
bool detect_traps(PlayerType *player_ptr, POSITION range, bool known)
{
    bool detect = detect_feat_flag(player_ptr, range, TerrainCharacteristics::TRAP, known);
    if (!known && detect) {
        detect_feat_flag(player_ptr, range, TerrainCharacteristics::TRAP, true);
    }

    if (known || detect) {
        player_ptr->dtrap = true;
    }

    if (music_singing(player_ptr, MUSIC_DETECT) && get_singing_count(player_ptr) > 0) {
        detect = false;
    }

    if (detect) {
        msg_print(_("トラップの存在を感じとった！", "You sense the presence of traps!"));
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺のドアを感知する / Detect all doors on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_doors(PlayerType *player_ptr, POSITION range)
{
    bool detect = detect_feat_flag(player_ptr, range, TerrainCharacteristics::DOOR, true);

    if (music_singing(player_ptr, MUSIC_DETECT) && get_singing_count(player_ptr) > 0) {
        detect = false;
    }
    if (detect) {
        msg_print(_("ドアの存在を感じとった！", "You sense the presence of doors!"));
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺の階段を感知する / Detect all stairs on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_stairs(PlayerType *player_ptr, POSITION range)
{
    bool detect = detect_feat_flag(player_ptr, range, TerrainCharacteristics::STAIRS, true);

    if (music_singing(player_ptr, MUSIC_DETECT) && get_singing_count(player_ptr) > 0) {
        detect = false;
    }
    if (detect) {
        msg_print(_("階段の存在を感じとった！", "You sense the presence of stairs!"));
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺の地形財宝を感知する / Detect any treasure on the current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_treasure(PlayerType *player_ptr, POSITION range)
{
    bool detect = detect_feat_flag(player_ptr, range, TerrainCharacteristics::HAS_GOLD, true);

    if (music_singing(player_ptr, MUSIC_DETECT) && get_singing_count(player_ptr) > 6) {
        detect = false;
    }
    if (detect) {
        msg_print(_("埋蔵された財宝の存在を感じとった！", "You sense the presence of buried treasure!"));
    }

    return detect;
}
/*!
 * @brief プレイヤー周辺のアイテム財宝を感知する / Detect all "gold" objects on the current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_gold(PlayerType *player_ptr, POSITION range)
{
    auto &floor = *player_ptr->current_floor_ptr;
    POSITION range2 = range;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        range2 /= 3;
    }

    /* Scan objects */
    bool detect = false;
    POSITION y, x;
    for (OBJECT_IDX i = 1; i < floor.o_max; i++) {
        auto *o_ptr = &floor.o_list[i];

        if (!o_ptr->is_valid()) {
            continue;
        }
        if (o_ptr->is_held_by_monster()) {
            continue;
        }

        y = o_ptr->iy;
        x = o_ptr->ix;
        if (distance(player_ptr->y, player_ptr->x, y, x) > range2) {
            continue;
        }

        if (o_ptr->bi_key.tval() == ItemKindType::GOLD) {
            o_ptr->marked.set(OmType::FOUND);
            lite_spot(player_ptr, y, x);
            detect = true;
        }
    }

    if (music_singing(player_ptr, MUSIC_DETECT) && get_singing_count(player_ptr) > 6) {
        detect = false;
    }
    if (detect) {
        msg_print(_("財宝の存在を感じとった！", "You sense the presence of treasure!"));
    }

    if (detect_monsters_string(player_ptr, range, "$")) {
        detect = true;
    }

    return detect;
}

/*!
 * @brief 通常のアイテムオブジェクトを感知する / Detect all "normal" objects on the current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_normal(PlayerType *player_ptr, POSITION range)
{
    auto &floor = *player_ptr->current_floor_ptr;
    POSITION range2 = range;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        range2 /= 3;
    }

    bool detect = false;
    for (OBJECT_IDX i = 1; i < floor.o_max; i++) {
        auto *o_ptr = &floor.o_list[i];

        if (!o_ptr->is_valid()) {
            continue;
        }
        if (o_ptr->is_held_by_monster()) {
            continue;
        }

        POSITION y = o_ptr->iy;
        POSITION x = o_ptr->ix;

        if (distance(player_ptr->y, player_ptr->x, y, x) > range2) {
            continue;
        }

        if (o_ptr->bi_key.tval() != ItemKindType::GOLD) {
            o_ptr->marked.set(OmType::FOUND);
            lite_spot(player_ptr, y, x);
            detect = true;
        }
    }

    if (music_singing(player_ptr, MUSIC_DETECT) && get_singing_count(player_ptr) > 6) {
        detect = false;
    }
    if (detect) {
        player_ptr->window_flags |= PW_FOUND_ITEMS;
        msg_print(_("アイテムの存在を感じとった！", "You sense the presence of objects!"));
    }

    if (detect_monsters_string(player_ptr, range, "!=?|/`")) {
        detect = true;
    }

    return detect;
}

static bool is_object_magically(const ItemKindType tval)
{
    switch (tval) {
    case ItemKindType::WHISTLE:
    case ItemKindType::AMULET:
    case ItemKindType::RING:
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
    case ItemKindType::SCROLL:
    case ItemKindType::POTION:
        return true;
    default:
        return false;
    }
}

/*!
 * @brief 魔法効果のあるのアイテムオブジェクトを感知する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 1つ以上感知したか否か
 */
bool detect_objects_magic(PlayerType *player_ptr, POSITION range)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    auto detect = false;
    for (OBJECT_IDX i = 1; i < floor.o_max; i++) {
        auto *o_ptr = &floor.o_list[i];
        if (!o_ptr->is_valid() || o_ptr->is_held_by_monster()) {
            continue;
        }

        auto y = o_ptr->iy;
        auto x = o_ptr->ix;
        if (distance(player_ptr->y, player_ptr->x, y, x) > range) {
            continue;
        }

        auto has_bonus = o_ptr->to_a > 0;
        has_bonus |= o_ptr->to_h + o_ptr->to_d > 0;
        if (o_ptr->is_fixed_or_random_artifact() || o_ptr->is_ego() || is_object_magically(o_ptr->bi_key.tval()) || o_ptr->is_spell_book() || has_bonus) {
            o_ptr->marked.set(OmType::FOUND);
            lite_spot(player_ptr, y, x);
            detect = true;
        }
    }

    if (detect) {
        player_ptr->window_flags |= PW_FOUND_ITEMS;
        msg_print(_("魔法のアイテムの存在を感じとった！", "You sense the presence of magic objects!"));
    }

    return detect;
}

/*!
 * @brief 一般のモンスターを感知する / Detect all "normal" monsters on the current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_normal(PlayerType *player_ptr, POSITION range)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    bool flag = false;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        auto *m_ptr = &floor.m_list[i];
        auto *r_ptr = &monraces_info[m_ptr->r_idx];
        if (!m_ptr->is_valid()) {
            continue;
        }

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;
        if (distance(player_ptr->y, player_ptr->x, y, x) > range) {
            continue;
        }

        if (!(r_ptr->flags2 & RF2_INVISIBLE) || player_ptr->see_inv) {
            m_ptr->mflag2.set({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
            update_monster(player_ptr, i, false);
            flag = true;
        }
    }

    if (music_singing(player_ptr, MUSIC_DETECT) && get_singing_count(player_ptr) > 3) {
        flag = false;
    }
    if (flag) {
        msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
    }

    return flag;
}

/*!
 * @brief 不可視のモンスターを感知する / Detect all "invisible" monsters around the player
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_invis(PlayerType *player_ptr, POSITION range)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    bool flag = false;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        auto *m_ptr = &floor.m_list[i];
        auto *r_ptr = &monraces_info[m_ptr->r_idx];

        if (!m_ptr->is_valid()) {
            continue;
        }

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;

        if (distance(player_ptr->y, player_ptr->x, y, x) > range) {
            continue;
        }

        if (r_ptr->flags2 & RF2_INVISIBLE) {
            if (player_ptr->monster_race_idx == m_ptr->r_idx) {
                player_ptr->window_flags |= (PW_MONSTER_LORE);
            }

            m_ptr->mflag2.set({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
            update_monster(player_ptr, i, false);
            flag = true;
        }
    }

    if (music_singing(player_ptr, MUSIC_DETECT) && get_singing_count(player_ptr) > 3) {
        flag = false;
    }
    if (flag) {
        msg_print(_("透明な生物の存在を感じとった！", "You sense the presence of invisible creatures!"));
    }

    return flag;
}

/*!
 * @brief 邪悪なモンスターを感知する / Detect all "evil" monsters on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_evil(PlayerType *player_ptr, POSITION range)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    bool flag = false;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        auto *m_ptr = &floor.m_list[i];
        auto *r_ptr = &monraces_info[m_ptr->r_idx];
        if (!m_ptr->is_valid()) {
            continue;
        }

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;

        if (distance(player_ptr->y, player_ptr->x, y, x) > range) {
            continue;
        }

        if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
            if (m_ptr->is_original_ap()) {
                r_ptr->r_kind_flags.set(MonsterKindType::EVIL);
                if (player_ptr->monster_race_idx == m_ptr->r_idx) {
                    player_ptr->window_flags |= (PW_MONSTER_LORE);
                }
            }

            m_ptr->mflag2.set({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
            update_monster(player_ptr, i, false);
            flag = true;
        }
    }

    if (flag) {
        msg_print(_("邪悪なる生物の存在を感じとった！", "You sense the presence of evil creatures!"));
    }

    return flag;
}

/*!
 * @brief 無生命のモンスターを感知する(アンデッド、悪魔系を含む) / Detect all "nonliving", "undead" or "demonic" monsters on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_nonliving(PlayerType *player_ptr, POSITION range)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    bool flag = false;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        auto *m_ptr = &floor.m_list[i];
        if (!m_ptr->is_valid()) {
            continue;
        }

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;
        if (distance(player_ptr->y, player_ptr->x, y, x) > range) {
            continue;
        }

        if (!m_ptr->has_living_flag()) {
            if (player_ptr->monster_race_idx == m_ptr->r_idx) {
                player_ptr->window_flags |= (PW_MONSTER_LORE);
            }

            m_ptr->mflag2.set({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
            update_monster(player_ptr, i, false);
            flag = true;
        }
    }

    if (flag) {
        msg_print(_("自然でないモンスターの存在を感じた！", "You sense the presence of unnatural beings!"));
    }

    return flag;
}

/*!
 * @brief 精神のあるモンスターを感知する / Detect all monsters it has mind on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_mind(PlayerType *player_ptr, POSITION range)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    bool flag = false;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        auto *m_ptr = &floor.m_list[i];
        auto *r_ptr = &monraces_info[m_ptr->r_idx];
        if (!m_ptr->is_valid()) {
            continue;
        }

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;

        if (distance(player_ptr->y, player_ptr->x, y, x) > range) {
            continue;
        }

        if (!(r_ptr->flags2 & RF2_EMPTY_MIND)) {
            if (player_ptr->monster_race_idx == m_ptr->r_idx) {
                player_ptr->window_flags |= (PW_MONSTER_LORE);
            }

            m_ptr->mflag2.set({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
            update_monster(player_ptr, i, false);
            flag = true;
        }
    }

    if (flag) {
        msg_print(_("殺気を感じとった！", "You sense the presence of someone's mind!"));
    }

    return flag;
}

/*!
 * @brief 該当シンボルのモンスターを感知する / Detect all (string) monsters on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param Match 対応シンボルの混じったモンスター文字列(複数指定化)
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_string(PlayerType *player_ptr, POSITION range, concptr Match)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (dungeons_info[floor.dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    bool flag = false;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        auto *m_ptr = &floor.m_list[i];
        auto *r_ptr = &monraces_info[m_ptr->r_idx];
        if (!m_ptr->is_valid()) {
            continue;
        }

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;

        if (distance(player_ptr->y, player_ptr->x, y, x) > range) {
            continue;
        }

        if (angband_strchr(Match, r_ptr->d_char)) {
            if (player_ptr->monster_race_idx == m_ptr->r_idx) {
                player_ptr->window_flags |= (PW_MONSTER_LORE);
            }

            m_ptr->mflag2.set({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
            update_monster(player_ptr, i, false);
            flag = true;
        }
    }

    if (music_singing(player_ptr, MUSIC_DETECT) && get_singing_count(player_ptr) > 3) {
        flag = false;
    }
    if (flag) {
        msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
    }

    return flag;
}

/*!
 * @brief 全感知処理 / Detect everything
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_all(PlayerType *player_ptr, POSITION range)
{
    bool detect = false;
    if (detect_traps(player_ptr, range, true)) {
        detect = true;
    }
    if (detect_doors(player_ptr, range)) {
        detect = true;
    }
    if (detect_stairs(player_ptr, range)) {
        detect = true;
    }
    if (detect_objects_gold(player_ptr, range)) {
        detect = true;
    }
    if (detect_objects_normal(player_ptr, range)) {
        detect = true;
    }
    if (detect_monsters_invis(player_ptr, range)) {
        detect = true;
    }
    if (detect_monsters_normal(player_ptr, range)) {
        detect = true;
    }
    return detect;
}
