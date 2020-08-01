/*!
 * @file mutation.c
 * @brief 突然変異ルールの実装 / Mutation effects (and racial powers)
 * @date 2014/01/11
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "mutation/mutation.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-item/cmd-throw.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "effect/spells-effect-util.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/write-diary.h"
#include "mind/mind-mage.h"
#include "mind/mind-warrior.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h" // todo 相互依存している、このファイルからの依存はOK.
#include "mutation/mutation-techniques.h"
#include "mutation/mutation-util.h"
#include "object-enchant/item-feeling.h"
#include "object-hook/hook-checker.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-personalities-types.h"
#include "player/player-race-types.h"
#include "player/selfinfo.h"
#include "racial/racial-vampire.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-sorcery.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"
/*!
 * @brief 現在プレイヤー得ている突然変異の数を返す。
 * @return 現在得ている突然変異の数
 */
static int count_mutations(player_type *creature_ptr)
{
    return count_bits(creature_ptr->muta1) + count_bits(creature_ptr->muta2) + count_bits(creature_ptr->muta3);
}

/*!
 * @brief 突然変異による自然回復ペナルティをパーセント値で返す /
 * Return the modifier to the regeneration rate (in percent)
 * @return ペナルティ修正(%)
 * @details
 * Beastman get 10 "free" mutations and only 5% decrease per additional mutation.
 * Max 90% decrease in regeneration speed.
 */
int calc_mutant_regenerate_mod(player_type *creature_ptr)
{
    int regen;
    int mod = 10;
    int count = count_mutations(creature_ptr);
    if (creature_ptr->pseikaku == PERSONALITY_LUCKY)
        count--;

    if (creature_ptr->prace == RACE_BEASTMAN) {
        count -= 10;
        mod = 5;
    }

    if (count <= 0)
        return 100;

    regen = 100 - count * mod;
    if (regen < 10)
        regen = 10;

    return (regen);
}

void become_living_trump(player_type *creature_ptr)
{
    /* 1/7 Teleport control and 6/7 Random teleportation (uncontrolled) */
    MUTATION_IDX mutation = one_in_(7) ? 12 : 77;
    if (gain_mutation(creature_ptr, mutation))
        msg_print(_("あなたは生きているカードに変わった。", "You have turned into a Living Trump."));
}

void set_mutation_flags(player_type *creature_ptr)
{
    if (creature_ptr->muta3 == 0)
        return;

    if (creature_ptr->muta3 & MUT3_FLESH_ROT)
        creature_ptr->regenerate = FALSE;

    if (creature_ptr->muta3 & MUT3_ELEC_TOUC)
        creature_ptr->sh_elec = TRUE;

    if (creature_ptr->muta3 & MUT3_FIRE_BODY) {
        creature_ptr->sh_fire = TRUE;
        creature_ptr->lite = TRUE;
    }

    if (creature_ptr->muta3 & MUT3_WINGS)
        creature_ptr->levitation = TRUE;

    if (creature_ptr->muta3 & MUT3_FEARLESS)
        creature_ptr->resist_fear = TRUE;

    if (creature_ptr->muta3 & MUT3_REGEN)
        creature_ptr->regenerate = TRUE;

    if (creature_ptr->muta3 & MUT3_ESP)
        creature_ptr->telepathy = TRUE;

    if (creature_ptr->muta3 & MUT3_MOTION)
        creature_ptr->free_act = TRUE;
}
