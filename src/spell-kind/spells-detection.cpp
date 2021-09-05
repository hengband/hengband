#include "spell-kind/spells-detection.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "floor/floor-save-util.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "object/object-mark-types.h"
#include "object/tval-types.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-song.h"
#include "spell-realm/spells-song.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤー周辺の地形を感知する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @param flag 特定地形ID
 * @param known 地形から危険フラグを外すならTRUE
 * @return 効力があった場合TRUEを返す
 */
static bool detect_feat_flag(player_type *caster_ptr, POSITION range, FF flag, bool known)
{
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range /= 3;

    grid_type *g_ptr;
    bool detect = false;
    for (POSITION y = 1; y < caster_ptr->current_floor_ptr->height - 1; y++) {
        for (POSITION x = 1; x <= caster_ptr->current_floor_ptr->width - 1; x++) {
            int dist = distance(caster_ptr->y, caster_ptr->x, y, x);
            if (dist > range)
                continue;
            g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];
            if (flag == FF::TRAP) {
                /* Mark as detected */
                if (dist <= range && known) {
                    if (dist <= range - 1)
                        g_ptr->info |= (CAVE_IN_DETECT);

                    g_ptr->info &= ~(CAVE_UNSAFE);

                    lite_spot(caster_ptr, y, x);
                }
            }

            if (g_ptr->cave_has_flag(flag)) {
                disclose_grid(caster_ptr, y, x);
                g_ptr->info |= (CAVE_MARK);
                lite_spot(caster_ptr, y, x);
                detect = true;
            }
        }
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺のトラップを感知する / Detect all traps on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @param known 感知外範囲を超える警告フラグを立てる場合TRUEを返す
 * @return 効力があった場合TRUEを返す
 * @details
 * 吟遊詩人による感知についてはFALSEを返す
 */
bool detect_traps(player_type *caster_ptr, POSITION range, bool known)
{
    bool detect = detect_feat_flag(caster_ptr, range, FF::TRAP, known);
    if (!known && detect)
        detect_feat_flag(caster_ptr, range, FF::TRAP, true);

    if (known || detect)
        caster_ptr->dtrap = true;

    if (music_singing(caster_ptr, MUSIC_DETECT) && get_singing_count(caster_ptr) > 0)
        detect = false;

    if (detect)
        msg_print(_("トラップの存在を感じとった！", "You sense the presence of traps!"));

    return detect;
}

/*!
 * @brief プレイヤー周辺のドアを感知する / Detect all doors on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_doors(player_type *caster_ptr, POSITION range)
{
    bool detect = detect_feat_flag(caster_ptr, range, FF::DOOR, true);

    if (music_singing(caster_ptr, MUSIC_DETECT) && get_singing_count(caster_ptr) > 0)
        detect = false;
    if (detect) {
        msg_print(_("ドアの存在を感じとった！", "You sense the presence of doors!"));
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺の階段を感知する / Detect all stairs on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_stairs(player_type *caster_ptr, POSITION range)
{
    bool detect = detect_feat_flag(caster_ptr, range, FF::STAIRS, true);

    if (music_singing(caster_ptr, MUSIC_DETECT) && get_singing_count(caster_ptr) > 0)
        detect = false;
    if (detect) {
        msg_print(_("階段の存在を感じとった！", "You sense the presence of stairs!"));
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺の地形財宝を感知する / Detect any treasure on the current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_treasure(player_type *caster_ptr, POSITION range)
{
    bool detect = detect_feat_flag(caster_ptr, range, FF::HAS_GOLD, true);

    if (music_singing(caster_ptr, MUSIC_DETECT) && get_singing_count(caster_ptr) > 6)
        detect = false;
    if (detect) {
        msg_print(_("埋蔵された財宝の存在を感じとった！", "You sense the presence of buried treasure!"));
    }

    return detect;
}
/*!
 * @brief プレイヤー周辺のアイテム財宝を感知する / Detect all "gold" objects on the current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_gold(player_type *caster_ptr, POSITION range)
{
    POSITION range2 = range;
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range2 /= 3;

    /* Scan objects */
    bool detect = false;
    POSITION y, x;
    for (OBJECT_IDX i = 1; i < caster_ptr->current_floor_ptr->o_max; i++) {
        object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[i];

        if (!o_ptr->is_valid())
            continue;
        if (o_ptr->is_held_by_monster())
            continue;

        y = o_ptr->iy;
        x = o_ptr->ix;
        if (distance(caster_ptr->y, caster_ptr->x, y, x) > range2)
            continue;

        if (o_ptr->tval == TV_GOLD) {
            o_ptr->marked |= OM_FOUND;
            lite_spot(caster_ptr, y, x);
            detect = true;
        }
    }

    if (music_singing(caster_ptr, MUSIC_DETECT) && get_singing_count(caster_ptr) > 6)
        detect = false;
    if (detect) {
        msg_print(_("財宝の存在を感じとった！", "You sense the presence of treasure!"));
    }

    if (detect_monsters_string(caster_ptr, range, "$")) {
        detect = true;
    }

    return detect;
}

/*!
 * @brief 通常のアイテムオブジェクトを感知する / Detect all "normal" objects on the current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_normal(player_type *caster_ptr, POSITION range)
{
    POSITION range2 = range;
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range2 /= 3;

    bool detect = false;
    for (OBJECT_IDX i = 1; i < caster_ptr->current_floor_ptr->o_max; i++) {
        object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[i];

        if (!o_ptr->is_valid())
            continue;
        if (o_ptr->is_held_by_monster())
            continue;

        POSITION y = o_ptr->iy;
        POSITION x = o_ptr->ix;

        if (distance(caster_ptr->y, caster_ptr->x, y, x) > range2)
            continue;

        if (o_ptr->tval != TV_GOLD) {
            o_ptr->marked |= OM_FOUND;
            lite_spot(caster_ptr, y, x);
            detect = true;
        }
    }

    if (music_singing(caster_ptr, MUSIC_DETECT) && get_singing_count(caster_ptr) > 6)
        detect = false;
    if (detect) {
        msg_print(_("アイテムの存在を感じとった！", "You sense the presence of objects!"));
    }

    if (detect_monsters_string(caster_ptr, range, "!=?|/`")) {
        detect = true;
    }

    return detect;
}

/*!
 * @brief 魔法効果のあるのアイテムオブジェクトを感知する / Detect all "magic" objects on the current panel.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 * @details
 * <pre>
 * This will light up all spaces with "magic" items, including artifacts,
 * ego-items, potions, scrolls, books, rods, wands, staffs, amulets, rings,
 * and "enchanted" items of the "good" variety.
 *
 * It can probably be argued that this function is now too powerful.
 * </pre>
 */
bool detect_objects_magic(player_type *caster_ptr, POSITION range)
{
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range /= 3;

    tval_type tv;
    bool detect = false;
    for (OBJECT_IDX i = 1; i < caster_ptr->current_floor_ptr->o_max; i++) {
        object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[i];

        if (!o_ptr->is_valid())
            continue;
        if (o_ptr->is_held_by_monster())
            continue;

        POSITION y = o_ptr->iy;
        POSITION x = o_ptr->ix;

        if (distance(caster_ptr->y, caster_ptr->x, y, x) > range)
            continue;

        tv = o_ptr->tval;
        if (o_ptr->is_artifact() || o_ptr->is_ego() || (tv == TV_WHISTLE) || (tv == TV_AMULET) || (tv == TV_RING) || (tv == TV_STAFF)
            || (tv == TV_WAND) || (tv == TV_ROD) || (tv == TV_SCROLL) || (tv == TV_POTION) || (tv == TV_LIFE_BOOK) || (tv == TV_SORCERY_BOOK)
            || (tv == TV_NATURE_BOOK) || (tv == TV_CHAOS_BOOK) || (tv == TV_DEATH_BOOK) || (tv == TV_TRUMP_BOOK) || (tv == TV_ARCANE_BOOK)
            || (tv == TV_CRAFT_BOOK) || (tv == TV_DEMON_BOOK) || (tv == TV_CRUSADE_BOOK) || (tv == TV_MUSIC_BOOK) || (tv == TV_HISSATSU_BOOK)
            || (tv == TV_HEX_BOOK) || ((o_ptr->to_a > 0) || (o_ptr->to_h + o_ptr->to_d > 0))) {
            o_ptr->marked |= OM_FOUND;
            lite_spot(caster_ptr, y, x);
            detect = true;
        }
    }

    if (detect) {
        msg_print(_("魔法のアイテムの存在を感じとった！", "You sense the presence of magic objects!"));
    }

    return detect;
}

/*!
 * @brief 一般のモンスターを感知する / Detect all "normal" monsters on the current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_normal(player_type *caster_ptr, POSITION range)
{
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range /= 3;

    bool flag = false;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (!monster_is_valid(m_ptr))
            continue;

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;
        if (distance(caster_ptr->y, caster_ptr->x, y, x) > range)
            continue;

        if (!(r_ptr->flags2 & RF2_INVISIBLE) || caster_ptr->see_inv) {
            m_ptr->mflag2.set({MFLAG2::MARK, MFLAG2::SHOW});
            update_monster(caster_ptr, i, false);
            flag = true;
        }
    }

    if (music_singing(caster_ptr, MUSIC_DETECT) && get_singing_count(caster_ptr) > 3)
        flag = false;
    if (flag) {
        msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
    }

    return flag;
}

/*!
 * @brief 不可視のモンスターを感知する / Detect all "invisible" monsters around the player
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_invis(player_type *caster_ptr, POSITION range)
{
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range /= 3;

    bool flag = false;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];

        if (!monster_is_valid(m_ptr))
            continue;

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;

        if (distance(caster_ptr->y, caster_ptr->x, y, x) > range)
            continue;

        if (r_ptr->flags2 & RF2_INVISIBLE) {
            if (caster_ptr->monster_race_idx == m_ptr->r_idx) {
                caster_ptr->window_flags |= (PW_MONSTER);
            }

            m_ptr->mflag2.set({MFLAG2::MARK, MFLAG2::SHOW});
            update_monster(caster_ptr, i, false);
            flag = true;
        }
    }

    if (music_singing(caster_ptr, MUSIC_DETECT) && get_singing_count(caster_ptr) > 3)
        flag = false;
    if (flag) {
        msg_print(_("透明な生物の存在を感じとった！", "You sense the presence of invisible creatures!"));
    }

    return flag;
}

/*!
 * @brief 邪悪なモンスターを感知する / Detect all "evil" monsters on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_evil(player_type *caster_ptr, POSITION range)
{
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range /= 3;

    bool flag = false;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (!monster_is_valid(m_ptr))
            continue;

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;

        if (distance(caster_ptr->y, caster_ptr->x, y, x) > range)
            continue;

        if (r_ptr->flags3 & RF3_EVIL) {
            if (is_original_ap(m_ptr)) {
                r_ptr->r_flags3 |= (RF3_EVIL);
                if (caster_ptr->monster_race_idx == m_ptr->r_idx) {
                    caster_ptr->window_flags |= (PW_MONSTER);
                }
            }

            m_ptr->mflag2.set({MFLAG2::MARK, MFLAG2::SHOW});
            update_monster(caster_ptr, i, false);
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
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_nonliving(player_type *caster_ptr, POSITION range)
{
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range /= 3;

    bool flag = false;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;
        if (distance(caster_ptr->y, caster_ptr->x, y, x) > range)
            continue;

        if (!monster_living(m_ptr->r_idx)) {
            if (caster_ptr->monster_race_idx == m_ptr->r_idx) {
                caster_ptr->window_flags |= (PW_MONSTER);
            }

            m_ptr->mflag2.set({MFLAG2::MARK, MFLAG2::SHOW});
            update_monster(caster_ptr, i, false);
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
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_mind(player_type *caster_ptr, POSITION range)
{
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range /= 3;

    bool flag = false;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (!monster_is_valid(m_ptr))
            continue;

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;

        if (distance(caster_ptr->y, caster_ptr->x, y, x) > range)
            continue;

        if (!(r_ptr->flags2 & RF2_EMPTY_MIND)) {
            if (caster_ptr->monster_race_idx == m_ptr->r_idx) {
                caster_ptr->window_flags |= (PW_MONSTER);
            }

            m_ptr->mflag2.set({MFLAG2::MARK, MFLAG2::SHOW});
            update_monster(caster_ptr, i, false);
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
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @param Match 対応シンボルの混じったモンスター文字列(複数指定化)
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_string(player_type *caster_ptr, POSITION range, concptr Match)
{
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range /= 3;

    bool flag = false;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (!monster_is_valid(m_ptr))
            continue;

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;

        if (distance(caster_ptr->y, caster_ptr->x, y, x) > range)
            continue;

        if (angband_strchr(Match, r_ptr->d_char)) {
            if (caster_ptr->monster_race_idx == m_ptr->r_idx) {
                caster_ptr->window_flags |= (PW_MONSTER);
            }

            m_ptr->mflag2.set({MFLAG2::MARK, MFLAG2::SHOW});
            update_monster(caster_ptr, i, false);
            flag = true;
        }
    }

    if (music_singing(caster_ptr, MUSIC_DETECT) && get_singing_count(caster_ptr) > 3)
        flag = false;
    if (flag) {
        msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
    }

    return flag;
}

/*!
 * @brief flags3に対応するモンスターを感知する / A "generic" detect monsters routine, tagged to flags3
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @param match_flag 感知フラグ
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_xxx(player_type *caster_ptr, POSITION range, uint32_t match_flag)
{
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::DARKNESS))
        range /= 3;

    bool flag = false;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (!monster_is_valid(m_ptr))
            continue;

        POSITION y = m_ptr->fy;
        POSITION x = m_ptr->fx;

        if (distance(caster_ptr->y, caster_ptr->x, y, x) > range)
            continue;

        if (r_ptr->flags3 & (match_flag)) {
            if (is_original_ap(m_ptr)) {
                r_ptr->r_flags3 |= (match_flag);
                if (caster_ptr->monster_race_idx == m_ptr->r_idx) {
                    caster_ptr->window_flags |= (PW_MONSTER);
                }
            }

            m_ptr->mflag2.set({MFLAG2::MARK, MFLAG2::SHOW});
            update_monster(caster_ptr, i, false);
            flag = true;
        }
    }

    concptr desc_monsters = _("変なモンスター", "weird monsters");
    if (flag) {
        switch (match_flag) {
        case RF3_DEMON:
            desc_monsters = _("デーモン", "demons");
            break;
        case RF3_UNDEAD:
            desc_monsters = _("アンデッド", "the undead");
            break;
        }

        msg_format(_("%sの存在を感じとった！", "You sense the presence of %s!"), desc_monsters);
        msg_print(nullptr);
    }

    return flag;
}

/*!
 * @brief 全感知処理 / Detect everything
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_all(player_type *caster_ptr, POSITION range)
{
    bool detect = false;
    if (detect_traps(caster_ptr, range, true))
        detect = true;
    if (detect_doors(caster_ptr, range))
        detect = true;
    if (detect_stairs(caster_ptr, range))
        detect = true;
    if (detect_objects_gold(caster_ptr, range))
        detect = true;
    if (detect_objects_normal(caster_ptr, range))
        detect = true;
    if (detect_monsters_invis(caster_ptr, range))
        detect = true;
    if (detect_monsters_normal(caster_ptr, range))
        detect = true;
    return (detect);
}
