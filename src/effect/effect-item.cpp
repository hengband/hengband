#include "effect/effect-item.h"
#include "autopick/autopick.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-expendable.h"
#include "object/object-broken.h"
#include "object/object-flags.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "spell-kind/spells-perception.h"
#include "spell/spell-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

#include <set>

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるアイテムオブジェクトへの効果処理 / Handle a beam/bolt/ball causing damage to a monster.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_item(player_type *caster_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ)
{
    grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];

    bool is_item_affected = false;
    bool known = player_has_los_bold(caster_ptr, y, x);
    who = who ? who : 0;
    dam = (dam + r) / (r + 1);
    std::set<OBJECT_IDX> processed_list;
    for (auto it = g_ptr->o_idx_list.begin(); it != g_ptr->o_idx_list.end();) {
        const OBJECT_IDX this_o_idx = *it++;

        if (auto pit = processed_list.find(this_o_idx); pit != processed_list.end())
            continue;

        processed_list.insert(this_o_idx);

        object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[this_o_idx];
        bool ignore = false;
        bool do_kill = false;
        concptr note_kill = nullptr;

#ifdef JP
#else
        bool plural = (o_ptr->number > 1);
#endif
        auto flags = object_flags(o_ptr);
        bool is_artifact = o_ptr->is_artifact();
        switch (typ) {
        case GF_ACID: {
            if (BreakerAcid().hates(o_ptr)) {
                do_kill = true;
                note_kill = _("融けてしまった！", (plural ? " melt!" : " melts!"));
                if (has_flag(flags, TR_IGNORE_ACID))
                    ignore = true;
            }

            break;
        }
        case GF_ELEC: {
            if (BreakerElec().hates(o_ptr)) {
                do_kill = true;
                note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
                if (has_flag(flags, TR_IGNORE_ELEC))
                    ignore = true;
            }

            break;
        }
        case GF_FIRE: {
            if (BreakerFire().hates(o_ptr)) {
                do_kill = true;
                note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
                if (has_flag(flags, TR_IGNORE_FIRE))
                    ignore = true;
            }

            break;
        }
        case GF_COLD: {
            if (BreakerCold().hates(o_ptr)) {
                note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
                do_kill = true;
                if (has_flag(flags, TR_IGNORE_COLD))
                    ignore = true;
            }

            break;
        }
        case GF_PLASMA: {
            if (BreakerFire().hates(o_ptr)) {
                do_kill = true;
                note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
                if (has_flag(flags, TR_IGNORE_FIRE))
                    ignore = true;
            }

            if (BreakerElec().hates(o_ptr)) {
                ignore = false;
                do_kill = true;
                note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
                if (has_flag(flags, TR_IGNORE_ELEC))
                    ignore = true;
            }

            break;
        }
        case GF_METEOR: {
            if (BreakerFire().hates(o_ptr)) {
                do_kill = true;
                note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
                if (has_flag(flags, TR_IGNORE_FIRE))
                    ignore = true;
            }

            if (BreakerCold().hates(o_ptr)) {
                ignore = false;
                do_kill = true;
                note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
                if (has_flag(flags, TR_IGNORE_COLD))
                    ignore = true;
            }

            break;
        }
        case GF_ICE:
        case GF_SHARDS:
        case GF_FORCE:
        case GF_SOUND: {
            if (BreakerCold().hates(o_ptr)) {
                note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
                do_kill = true;
            }

            break;
        }
        case GF_MANA:
        case GF_SEEKER:
        case GF_SUPER_RAY: {
            do_kill = true;
            note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
            break;
        }
        case GF_DISINTEGRATE: {
            do_kill = true;
            note_kill = _("蒸発してしまった！", (plural ? " evaporate!" : " evaporates!"));
            break;
        }
        case GF_CHAOS: {
            do_kill = true;
            note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
            if (has_flag(flags, TR_RES_CHAOS))
                ignore = true;
            else if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_CHAOS))
                ignore = true;
            break;
        }
        case GF_HOLY_FIRE:
        case GF_HELL_FIRE: {
            if (o_ptr->is_cursed()) {
                do_kill = true;
                note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
            }

            break;
        }
        case GF_VOID: {
            do_kill = true;
            note_kill = _("消滅してしまった！", (plural ? " vanish!" : " vanishes!"));
            break;
        }
        case GF_IDENTIFY: {
            identify_item(caster_ptr, o_ptr);
            autopick_alter_item(caster_ptr, (-this_o_idx), false);
            break;
        }
        case GF_KILL_TRAP:
        case GF_KILL_DOOR: {
            if (o_ptr->tval != TV_CHEST)
                break;
            if (o_ptr->pval <= 0)
                break;

            o_ptr->pval = (0 - o_ptr->pval);
            object_known(o_ptr);
            if (known && (o_ptr->marked & OM_FOUND)) {
                msg_print(_("カチッと音がした！", "Click!"));
                is_item_affected = true;
            }

            break;
        }
        case GF_ANIM_DEAD: {
            if (o_ptr->tval != TV_CORPSE)
                break;

            BIT_FLAGS mode = 0L;
            if (!who || is_pet(&caster_ptr->current_floor_ptr->m_list[who]))
                mode |= PM_FORCE_PET;

            for (int i = 0; i < o_ptr->number; i++) {
                if (((o_ptr->sval == SV_CORPSE) && (randint1(100) > 80)) || ((o_ptr->sval == SV_SKELETON) && (randint1(100) > 60))) {
                    if (!note_kill) {
                        note_kill = _("灰になった。", (plural ? " become dust." : " becomes dust."));
                    }

                    continue;
                } else if (summon_named_creature(caster_ptr, who, y, x, o_ptr->pval, mode)) {
                    note_kill = _("生き返った。", " revived.");
                } else if (!note_kill) {
                    note_kill = _("灰になった。", (plural ? " become dust." : " becomes dust."));
                }
            }

            do_kill = true;
            is_item_affected = true;
            break;
        }
        }

        if (!do_kill)
            continue;

        GAME_TEXT o_name[MAX_NLEN];
        if (known && (o_ptr->marked & OM_FOUND)) {
            is_item_affected = true;
            describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        }

        if ((is_artifact || ignore)) {
            if (known && (o_ptr->marked & OM_FOUND))
                msg_format(_("%sは影響を受けない！", (plural ? "The %s are unaffected!" : "The %s is unaffected!")), o_name);

            continue;
        }

        if (known && (o_ptr->marked & OM_FOUND) && note_kill)
            msg_format(_("%sは%s", "The %s%s"), o_name, note_kill);

        KIND_OBJECT_IDX k_idx = o_ptr->k_idx;
        bool is_potion = o_ptr->is_potion();
        delete_object_idx(caster_ptr, this_o_idx);
        if (is_potion) {
            (void)potion_smash_effect(caster_ptr, who, y, x, k_idx);

            // 薬の破壊効果によりリストの次のアイテムが破壊された可能性があるのでリストの最初から処理をやり直す
            // 処理済みのアイテムは processed_list に登録されており、スキップされる
            it = g_ptr->o_idx_list.begin();
        }

        lite_spot(caster_ptr, y, x);
    }

    return is_item_affected;
}
