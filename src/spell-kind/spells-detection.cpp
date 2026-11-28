#include "spell-kind/spells-detection.h"
#include "floor/floor-save-util.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-update.h"
#include "system/deferred-update-requests.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

namespace {
/*!
 * @brief 感知の効果範囲を求める
 * @param floor フロアへの参照
 * @param range 元の効果範囲
 * @return 暗いダンジョン (DARKNESS) なら 1/3 にした効果範囲、そうでなければ元の効果範囲
 */
POSITION calc_detection_range(const FloorType &floor, POSITION range)
{
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        return range / 3;
    }

    return range;
}

/*!
 * @brief 感知のメッセージを出すかを返す
 * @param notice メッセージを出す条件
 * @param detected 対象を 1 つでも感知したか
 * @param has_new 新しく感知した対象があったか
 * @return メッセージを出すならtrue
 */
bool should_notify(DetectionNotice notice, bool detected, bool has_new)
{
    switch (notice) {
    case DetectionNotice::ALWAYS:
        return detected;
    case DetectionNotice::NEW_ONLY:
        return has_new;
    case DetectionNotice::NEVER:
        return false;
    }

    return false;
}

/*!
 * @brief 地形の感知の結果
 */
struct FeatDetectionResult {
    bool detected = false; //!< 対象の地形が 1 つでもあったか
    bool has_disclosed = false; //!< ほかの地形に見せかけていたものを暴いたか
};

/*!
 * @brief 対象を感知した結果
 */
struct DetectionResult {
    bool detected = false; //!< 対象を 1 つでも感知したか
    bool has_new = false; //!< 新しく感知した対象があったか
};

/*!
 * @brief 効果範囲の中で条件に合う床のアイテムを感知する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param pred 感知するアイテムか
 * @return 感知の結果
 */
template <typename Predicate>
DetectionResult detect_objects_if(PlayerType *player_ptr, POSITION range, Predicate pred)
{
    auto &floor = *player_ptr->current_floor_ptr;
    range = calc_detection_range(floor, range);
    const auto p_pos = player_ptr->get_position();
    DetectionResult result;
    for (const auto &item_ptr : floor.o_list) {
        if (!item_ptr->is_valid() || item_ptr->is_held_by_monster()) {
            continue;
        }

        const auto i_pos = item_ptr->get_position();
        if (Grid::calc_distance(p_pos, i_pos) > range) {
            continue;
        }

        if (!pred(*item_ptr)) {
            continue;
        }

        result.has_new |= item_ptr->marked.has_not(OmType::FOUND);
        item_ptr->marked.set(OmType::FOUND);
        lite_spot(player_ptr, i_pos);
        result.detected = true;
    }

    return result;
}

/*!
 * @brief 効果範囲の中で条件に合うモンスターを感知する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param pred 感知するモンスターか (感知したときの思い出への記録もここで行う)
 * @return 感知の結果
 */
template <typename Predicate>
DetectionResult detect_monsters_if(PlayerType *player_ptr, POSITION range, Predicate pred)
{
    auto &floor = *player_ptr->current_floor_ptr;
    range = calc_detection_range(floor, range);
    const auto p_pos = player_ptr->get_position();
    DetectionResult result;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        auto &monster = floor.m_list[i];
        if (!monster.is_valid()) {
            continue;
        }

        if (Grid::calc_distance(p_pos, monster.get_position()) > range) {
            continue;
        }

        if (!pred(monster)) {
            continue;
        }

        result.has_new |= monster.mflag2.has_not(MonsterConstantFlagType::MARK);
        monster.mflag2.set({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(player_ptr, i, false);
        result.detected = true;
    }

    return result;
}
}

/*!
 * @brief プレイヤー周辺の地形を感知する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param flag 特定地形ID
 * @param known 地形から危険フラグを外すならTRUE
 * @return 感知の結果
 */
static FeatDetectionResult detect_feat_flag(PlayerType *player_ptr, POSITION range, TerrainCharacteristics flag, bool known)
{
    auto &floor = *player_ptr->current_floor_ptr;
    range = calc_detection_range(floor, range);

    FeatDetectionResult result;
    for (const auto &pos : floor.get_area(FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
        auto dist = Grid::calc_distance(player_ptr->get_position(), pos);
        if (dist > range) {
            continue;
        }

        auto &grid = floor.get_grid(pos);
        if (flag == TerrainCharacteristics::TRAP) {
            /* Mark as detected */
            if (dist <= range && known) {
                if (dist <= range - 1) {
                    grid.info |= (CAVE_IN_DETECT);
                }

                grid.info &= ~(CAVE_UNSAFE);

                lite_spot(player_ptr, pos);
            }
        }

        if (grid.has(flag)) {
            if (grid.is_hidden()) {
                result.has_disclosed = true;
            }

            disclose_grid(player_ptr, pos);
            grid.info |= (CAVE_MARK);
            lite_spot(player_ptr, pos);
            result.detected = true;
        }
    }

    return result;
}

/*!
 * @brief プレイヤー周辺のトラップを感知する / Detect all traps on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param known 感知外範囲を超える警告フラグを立てる場合TRUEを返す
 * @param notice メッセージを出す条件
 * @return 効力があった場合TRUEを返す
 */
bool detect_traps(PlayerType *player_ptr, POSITION range, bool known, DetectionNotice notice)
{
    const auto result = detect_feat_flag(player_ptr, range, TerrainCharacteristics::TRAP, known);
    const auto detect = result.detected;
    if (!known && detect) {
        detect_feat_flag(player_ptr, range, TerrainCharacteristics::TRAP, true);
    }

    if (known || detect) {
        player_ptr->dtrap = true;
    }

    if (should_notify(notice, detect, result.has_disclosed)) {
        msg_print(_("トラップの存在を感じとった！", "You sense the presence of traps!"));
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺のドアを感知する / Detect all doors on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param notice メッセージを出す条件
 * @return 効力があった場合TRUEを返す
 */
bool detect_doors(PlayerType *player_ptr, POSITION range, DetectionNotice notice)
{
    const auto result = detect_feat_flag(player_ptr, range, TerrainCharacteristics::DOOR, true);
    const auto detect = result.detected;
    if (should_notify(notice, detect, result.has_disclosed)) {
        msg_print(_("ドアの存在を感じとった！", "You sense the presence of doors!"));
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺の階段を感知する / Detect all stairs on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param notice メッセージを出す条件
 * @return 効力があった場合TRUEを返す
 */
bool detect_stairs(PlayerType *player_ptr, POSITION range, DetectionNotice notice)
{
    const auto result = detect_feat_flag(player_ptr, range, TerrainCharacteristics::STAIRS, true);
    const auto detect = result.detected;
    if (should_notify(notice, detect, result.has_disclosed)) {
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
    const auto detect = detect_feat_flag(player_ptr, range, TerrainCharacteristics::HAS_GOLD, true).detected;
    if (detect) {
        msg_print(_("埋蔵された財宝の存在を感じとった！", "You sense the presence of buried treasure!"));
    }

    return detect;
}
/*!
 * @brief プレイヤー周辺のアイテム財宝を感知する / Detect all "gold" objects on the current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param notice メッセージを出す条件
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_gold(PlayerType *player_ptr, POSITION range, DetectionNotice notice)
{
    const auto result = detect_objects_if(player_ptr, range, [](const ItemEntity &item) {
        return item.bi_key.tval() == ItemKindType::GOLD;
    });
    if (should_notify(notice, result.detected, result.has_new)) {
        msg_print(_("財宝の存在を感じとった！", "You sense the presence of treasure!"));
    }

    const auto detected_mimic = detect_monsters_string(player_ptr, range, "$", notice);
    return result.detected || detected_mimic;
}

/*!
 * @brief 通常のアイテムオブジェクトを感知する / Detect all "normal" objects on the current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param notice メッセージを出す条件
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_normal(PlayerType *player_ptr, POSITION range, DetectionNotice notice)
{
    const auto result = detect_objects_if(player_ptr, range, [](const ItemEntity &item) {
        return item.bi_key.tval() != ItemKindType::GOLD;
    });

    // 歌い続けている間は毎ターン感知するので、新しく見つけたときだけ描き直す
    if (result.has_new) {
        DeferredUpdateRequests::get_instance().request_floor_item_lists_redrawing();
    }

    if (should_notify(notice, result.detected, result.has_new)) {
        msg_print(_("アイテムの存在を感じとった！", "You sense the presence of objects!"));
    }

    const auto detected_mimic = detect_monsters_string(player_ptr, range, "!=?|/`", notice);
    return result.detected || detected_mimic;
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
    const auto result = detect_objects_if(player_ptr, range, [](const ItemEntity &item) {
        const auto has_bonus = (item.to_a > 0) || (item.to_h + item.to_d > 0);
        return item.is_fixed_or_random_artifact() || item.is_ego() || is_object_magically(item.bi_key.tval()) || item.is_spell_book() || has_bonus;
    });
    if (result.has_new) {
        DeferredUpdateRequests::get_instance().request_floor_item_lists_redrawing();
    }

    if (result.detected) {
        msg_print(_("魔法のアイテムの存在を感じとった！", "You sense the presence of magic objects!"));
    }

    return result.detected;
}

/*!
 * @brief 一般のモンスターを感知する / Detect all "normal" monsters on the current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param notice メッセージを出す条件
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_normal(PlayerType *player_ptr, POSITION range, DetectionNotice notice)
{
    const auto see_inv = player_ptr->see_inv;
    const auto result = detect_monsters_if(player_ptr, range, [see_inv](const MonsterEntity &monster) {
        return monster.get_monrace().misc_flags.has_not(MonsterMiscType::INVISIBLE) || see_inv;
    });
    if (should_notify(notice, result.detected, result.has_new)) {
        msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
    }

    return result.detected;
}

/*!
 * @brief 不可視のモンスターを感知する / Detect all "invisible" monsters around the player
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param notice メッセージを出す条件
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_invis(PlayerType *player_ptr, POSITION range, DetectionNotice notice)
{
    const auto result = detect_monsters_if(player_ptr, range, [](const MonsterEntity &monster) {
        return monster.get_monrace().misc_flags.has(MonsterMiscType::INVISIBLE);
    });
    if (should_notify(notice, result.detected, result.has_new)) {
        msg_print(_("透明な生物の存在を感じとった！", "You sense the presence of invisible creatures!"));
    }

    return result.detected;
}

/*!
 * @brief 邪悪なモンスターを感知する / Detect all "evil" monsters on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param notice メッセージを出す条件
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_evil(PlayerType *player_ptr, POSITION range, DetectionNotice notice)
{
    const auto result = detect_monsters_if(player_ptr, range, [](const MonsterEntity &monster) {
        auto &monrace = monster.get_monrace();
        if (monrace.kind_flags.has_not(MonsterKindType::EVIL)) {
            return false;
        }

        if (monster.is_original_ap()) {
            monrace.r_kind_flags.set(MonsterKindType::EVIL);
        }

        return true;
    });
    if (should_notify(notice, result.detected, result.has_new)) {
        msg_print(_("邪悪なる生物の存在を感じとった！", "You sense the presence of evil creatures!"));
    }

    return result.detected;
}

/*!
 * @brief 無生命のモンスターを感知する(アンデッド、悪魔系を含む) / Detect all "nonliving", "undead" or "demonic" monsters on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_nonliving(PlayerType *player_ptr, POSITION range)
{
    const auto detected = detect_monsters_if(player_ptr, range, [](const MonsterEntity &monster) {
        return !monster.has_living_flag();
    }).detected;
    if (detected) {
        msg_print(_("自然でないモンスターの存在を感じた！", "You sense the presence of unnatural beings!"));
    }

    return detected;
}

/*!
 * @brief 精神のあるモンスターを感知する / Detect all monsters it has mind on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_mind(PlayerType *player_ptr, POSITION range)
{
    const auto detected = detect_monsters_if(player_ptr, range, [](const MonsterEntity &monster) {
        return monster.get_monrace().misc_flags.has_not(MonsterMiscType::EMPTY_MIND);
    }).detected;
    if (detected) {
        msg_print(_("殺気を感じとった！", "You sense the presence of someone's mind!"));
    }

    return detected;
}

/*!
 * @brief 該当シンボルのモンスターを感知する / Detect all (string) monsters on current panel
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @param Match 対応シンボルの混じったモンスター文字列(複数指定化)
 * @param notice メッセージを出す条件
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_string(PlayerType *player_ptr, POSITION range, concptr Match, DetectionNotice notice)
{
    const auto result = detect_monsters_if(player_ptr, range, [Match](const MonsterEntity &monster) {
        return angband_strchr(Match, monster.get_monrace().symbol_definition.character) != nullptr;
    });
    if (should_notify(notice, result.detected, result.has_new)) {
        msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
    }

    return result.detected;
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

/*!
 * @brief 感知の印が付いている範囲内のモンスターの印を、次の行動まで保つ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 効果範囲
 * @details
 * 感知の印 (MARK) は、SHOW が付いていなければ行動ごとに外れる。感知し続ける効果で、感知し直すまでの間に
 * 印が外れないよう SHOW を付け直す。新しくは感知しない。
 */
void keep_detected_monsters(PlayerType *player_ptr, POSITION range)
{
    auto &floor = *player_ptr->current_floor_ptr;
    range = calc_detection_range(floor, range);

    const auto p_pos = player_ptr->get_position();
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        auto &monster = floor.m_list[i];
        if (!monster.is_valid() || monster.mflag2.has_not(MonsterConstantFlagType::MARK)) {
            continue;
        }

        if (Grid::calc_distance(p_pos, monster.get_position()) > range) {
            continue;
        }

        monster.mflag2.set(MonsterConstantFlagType::SHOW);
    }
}
