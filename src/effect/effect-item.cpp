#include "effect/effect-item.h"
#include "autopick/autopick.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-expendable.h"
#include "object/object-broken.h"
#include "object/object-mark-types.h"
#include "spell-kind/spells-perception.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <set>

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるアイテムオブジェクトへの効果処理 / Handle a beam/bolt/ball causing damage to a monster.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param src_idx 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_item(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION r, POSITION y, POSITION x, int dam, AttributeType typ)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const Pos2D pos(y, x);
    const auto &grid = floor.get_grid(pos);

    auto is_item_affected = false;
    const auto known = grid.has_los();
    src_idx = is_monster(src_idx) ? src_idx : 0;
    dam = (dam + r) / (r + 1);
    std::set<OBJECT_IDX> processed_list;
    for (auto it = grid.o_idx_list.begin(); it != grid.o_idx_list.end();) {
        const OBJECT_IDX this_o_idx = *it++;

        if (auto pit = processed_list.find(this_o_idx); pit != processed_list.end()) {
            continue;
        }

        processed_list.insert(this_o_idx);

        auto &item = player_ptr->current_floor_ptr->o_list[this_o_idx];
        auto ignore = false;
        auto do_kill = false;
        concptr note_kill = nullptr;

#ifdef JP
#else
        bool plural = (item.number > 1);
#endif
        const auto flags = item.get_flags();
        bool is_fixed_or_random_artifact = item.is_fixed_or_random_artifact();
        switch (typ) {
        case AttributeType::ACID: {
            if (BreakerAcid().hates(&item)) {
                do_kill = true;
                note_kill = _("融けてしまった！", (plural ? " melt!" : " melts!"));
                if (flags.has(TR_IGNORE_ACID)) {
                    ignore = true;
                }
            }

            break;
        }
        case AttributeType::ELEC: {
            if (BreakerElec().hates(&item)) {
                do_kill = true;
                note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
                if (flags.has(TR_IGNORE_ELEC)) {
                    ignore = true;
                }
            }

            break;
        }
        case AttributeType::FIRE: {
            if (BreakerFire().hates(&item)) {
                do_kill = true;
                note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
                if (flags.has(TR_IGNORE_FIRE)) {
                    ignore = true;
                }
            }

            break;
        }
        case AttributeType::COLD: {
            if (BreakerCold().hates(&item)) {
                note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
                do_kill = true;
                if (flags.has(TR_IGNORE_COLD)) {
                    ignore = true;
                }
            }

            break;
        }
        case AttributeType::PLASMA: {
            if (BreakerFire().hates(&item)) {
                do_kill = true;
                note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
                if (flags.has(TR_IGNORE_FIRE)) {
                    ignore = true;
                }
            }

            if (BreakerElec().hates(&item)) {
                ignore = false;
                do_kill = true;
                note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
                if (flags.has(TR_IGNORE_ELEC)) {
                    ignore = true;
                }
            }

            break;
        }
        case AttributeType::METEOR: {
            if (BreakerFire().hates(&item)) {
                do_kill = true;
                note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
                if (flags.has(TR_IGNORE_FIRE)) {
                    ignore = true;
                }
            }

            if (BreakerCold().hates(&item)) {
                ignore = false;
                do_kill = true;
                note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
                if (flags.has(TR_IGNORE_COLD)) {
                    ignore = true;
                }
            }

            break;
        }
        case AttributeType::ICE:
        case AttributeType::SHARDS:
        case AttributeType::FORCE:
        case AttributeType::SOUND: {
            if (BreakerCold().hates(&item)) {
                note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
                do_kill = true;
            }

            break;
        }
        case AttributeType::MANA:
        case AttributeType::SEEKER:
        case AttributeType::SUPER_RAY: {
            do_kill = true;
            note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
            break;
        }
        case AttributeType::DISINTEGRATE: {
            do_kill = true;
            note_kill = _("蒸発してしまった！", (plural ? " evaporate!" : " evaporates!"));
            break;
        }
        case AttributeType::CHAOS: {
            do_kill = true;
            note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
            if (flags.has(TR_RES_CHAOS)) {
                ignore = true;
            } else if (item.bi_key == BaseitemKey(ItemKindType::SCROLL, SV_SCROLL_CHAOS)) {
                ignore = true;
            }
            break;
        }
        case AttributeType::HOLY_FIRE:
        case AttributeType::HELL_FIRE: {
            if (item.is_cursed()) {
                do_kill = true;
                note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
            }

            break;
        }
        case AttributeType::VOID_MAGIC: {
            do_kill = true;
            note_kill = _("消滅してしまった！", (plural ? " vanish!" : " vanishes!"));
            break;
        }
        case AttributeType::IDENTIFY: {
            identify_item(player_ptr, &item);
            autopick_alter_item(player_ptr, (-this_o_idx), false);
            break;
        }
        case AttributeType::KILL_TRAP:
        case AttributeType::KILL_DOOR: {
            if (item.bi_key.tval() != ItemKindType::CHEST) {
                break;
            }

            if (item.pval <= 0) {
                break;
            }

            item.pval = (0 - item.pval);
            item.mark_as_known();
            if (known && item.marked.has(OmType::FOUND)) {
                msg_print(_("カチッと音がした！", "Click!"));
                is_item_affected = true;
            }

            break;
        }
        case AttributeType::ANIM_DEAD: {
            if (item.bi_key.tval() != ItemKindType::MONSTER_REMAINS) {
                break;
            }

            BIT_FLAGS mode = 0L;
            if (is_player(src_idx) || player_ptr->current_floor_ptr->m_list[src_idx].is_pet()) {
                mode |= PM_FORCE_PET;
            }

            for (int i = 0; i < item.number; i++) {
                const auto &monrace = item.get_monrace();
                const auto sval = *item.bi_key.sval();
                if (((sval == SV_CORPSE) && (randint1(100) > 80)) || ((sval == SV_SKELETON) && (randint1(100) > 60))) {
                    if (!note_kill) {
                        note_kill = _("灰になった。", (plural ? " become dust." : " becomes dust."));
                    }

                    continue;
                } else if (summon_named_creature(player_ptr, src_idx, y, x, monrace.idx, mode)) {
                    note_kill = _("生き返った。", " revived.");
                } else if (!note_kill) {
                    note_kill = _("灰になった。", (plural ? " become dust." : " becomes dust."));
                }
            }

            do_kill = true;
            is_item_affected = true;
            break;
        }
        default:
            break;
        }

        if (!do_kill) {
            continue;
        }

        std::string item_name("");
        if (known && item.marked.has(OmType::FOUND)) {
            is_item_affected = true;
            item_name = describe_flavor(player_ptr, item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        }

        if ((is_fixed_or_random_artifact || ignore)) {
            if (known && item.marked.has(OmType::FOUND)) {
                msg_format(_("%sは影響を受けない！", (plural ? "The %s are unaffected!" : "The %s is unaffected!")), item_name.data());
            }

            continue;
        }

        if (known && item.marked.has(OmType::FOUND) && note_kill) {
            msg_format(_("%sは%s", "The %s%s"), item_name.data(), note_kill);
        }

        const auto bi_id = item.bi_id;
        const auto is_potion = item.is_potion();
        delete_object_idx(player_ptr, this_o_idx);
        if (is_potion) {
            (void)potion_smash_effect(player_ptr, src_idx, y, x, bi_id);

            // 薬の破壊効果によりリストの次のアイテムが破壊された可能性があるのでリストの最初から処理をやり直す
            // 処理済みのアイテムは processed_list に登録されており、スキップされる
            it = grid.o_idx_list.begin();
        }

        lite_spot(player_ptr, y, x);
    }

    return is_item_affected;
}
