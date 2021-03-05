﻿/*!
 * @brief モンスターの思い出表示に必要なフラグ類の処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "lore/monster-lore.h"
#include "game-option/cheat-options.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "lore/magic-types-setter.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-indice-types.h"
#include "term/term-color-types.h"
#include "view/display-lore-attacks.h"
#include "view/display-lore-drops.h"
#include "view/display-lore-magics.h"
#include "view/display-lore-status.h"
#include "view/display-lore.h"

static void set_msex_flags(lore_type *lore_ptr)
{
    lore_ptr->msex = MSEX_NONE;
    if (lore_ptr->r_ptr->flags1 & RF1_FEMALE) {
        lore_ptr->msex = MSEX_FEMALE;
        return;
    }

    if (lore_ptr->r_ptr->flags1 & RF1_MALE)
        lore_ptr->msex = MSEX_MALE;
}

static void set_flags1(lore_type *lore_ptr)
{
    if (lore_ptr->r_ptr->flags1 & RF1_UNIQUE)
        lore_ptr->flags1 |= (RF1_UNIQUE);

    if (lore_ptr->r_ptr->flags1 & RF1_QUESTOR)
        lore_ptr->flags1 |= (RF1_QUESTOR);

    if (lore_ptr->r_ptr->flags1 & RF1_MALE)
        lore_ptr->flags1 |= (RF1_MALE);

    if (lore_ptr->r_ptr->flags1 & RF1_FEMALE)
        lore_ptr->flags1 |= (RF1_FEMALE);

    if (lore_ptr->r_ptr->flags1 & RF1_FRIENDS)
        lore_ptr->flags1 |= (RF1_FRIENDS);

    if (lore_ptr->r_ptr->flags1 & RF1_ESCORT)
        lore_ptr->flags1 |= (RF1_ESCORT);

    if (lore_ptr->r_ptr->flags1 & RF1_ESCORTS)
        lore_ptr->flags1 |= (RF1_ESCORTS);
}

static void set_race_flags(lore_type *lore_ptr)
{
    if (!lore_ptr->r_ptr->r_tkills && !lore_ptr->know_everything)
        return;

    if (lore_ptr->r_ptr->flags3 & RF3_ORC)
        lore_ptr->flags3 |= (RF3_ORC);

    if (lore_ptr->r_ptr->flags3 & RF3_TROLL)
        lore_ptr->flags3 |= (RF3_TROLL);

    if (lore_ptr->r_ptr->flags3 & RF3_GIANT)
        lore_ptr->flags3 |= (RF3_GIANT);

    if (lore_ptr->r_ptr->flags3 & RF3_DRAGON)
        lore_ptr->flags3 |= (RF3_DRAGON);

    if (lore_ptr->r_ptr->flags3 & RF3_DEMON)
        lore_ptr->flags3 |= (RF3_DEMON);

    if (lore_ptr->r_ptr->flags3 & RF3_UNDEAD)
        lore_ptr->flags3 |= (RF3_UNDEAD);

    if (lore_ptr->r_ptr->flags3 & RF3_EVIL)
        lore_ptr->flags3 |= (RF3_EVIL);

    if (lore_ptr->r_ptr->flags3 & RF3_GOOD)
        lore_ptr->flags3 |= (RF3_GOOD);

    if (lore_ptr->r_ptr->flags3 & RF3_ANIMAL)
        lore_ptr->flags3 |= (RF3_ANIMAL);

    if (lore_ptr->r_ptr->flags3 & RF3_AMBERITE)
        lore_ptr->flags3 |= (RF3_AMBERITE);

    if (lore_ptr->r_ptr->flags2 & RF2_HUMAN)
        lore_ptr->flags2 |= (RF2_HUMAN);

    if (lore_ptr->r_ptr->flags2 & RF2_QUANTUM)
        lore_ptr->flags2 |= (RF2_QUANTUM);

    if (lore_ptr->r_ptr->flags1 & RF1_FORCE_DEPTH)
        lore_ptr->flags1 |= (RF1_FORCE_DEPTH);

    if (lore_ptr->r_ptr->flags1 & RF1_FORCE_MAXHP)
        lore_ptr->flags1 |= (RF1_FORCE_MAXHP);
}

/*!
 * @brief モンスターの思い出情報を表示するメインルーチン
 * Hack -- display monster information using "hooked_roff()"
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 * @return なし
 * @details
 * This function should only be called with the cursor placed at the
 * left edge of the screen, on a cleared line, in which the recall is
 * to take place.  One extra blank line is left after the recall.
 */
void process_monster_lore(player_type *player_ptr, MONRACE_IDX r_idx, monster_lore_mode mode)
{
    lore_type tmp_lore;
    lore_type *lore_ptr = initialize_lore_type(&tmp_lore, r_idx, mode);
    for (int n = 0; n < A_MAX; n++) {
        if (lore_ptr->r_ptr->reinforce_id[n] > 0)
            lore_ptr->reinforce = TRUE;
    }

    if (cheat_know || (mode == MONSTER_LORE_RESEARCH) || (mode == MONSTER_LORE_DEBUG))
        lore_ptr->know_everything = TRUE;

    set_drop_flags(lore_ptr);
    set_msex_flags(lore_ptr);
    set_flags1(lore_ptr);
    set_race_flags(lore_ptr);
    display_kill_numbers(lore_ptr);
    concptr tmp = r_text + lore_ptr->r_ptr->text;
    if (tmp[0]) {
        hooked_roff(tmp);
        hooked_roff("\n");
    }

    if (r_idx == MON_KAGE) {
        hooked_roff("\n");
        return;
    }

    if (!display_where_to_appear(lore_ptr))
        return;

    display_monster_move(lore_ptr);
    display_monster_never_move(lore_ptr);
    if (lore_ptr->old) {
        hooked_roff(_("。", ".  "));
        lore_ptr->old = FALSE;
    }

    display_lore_this(player_ptr, lore_ptr);
    display_monster_aura(lore_ptr);
    if (lore_ptr->flags2 & RF2_REFLECTING)
        hooked_roff(format(_("%^sは矢の呪文を跳ね返す。", "%^s reflects bolt spells.  "), wd_he[lore_ptr->msex]));

    display_monster_collective(lore_ptr);
    lore_ptr->vn = 0;
    if (lore_ptr->flags4 & RF4_SHRIEK) {
        lore_ptr->vp[lore_ptr->vn] = _("悲鳴で助けを求める", "shriek for help");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_WHITE;
    }

    display_monster_launching(player_ptr, lore_ptr);
    if (lore_ptr->a_ability_flags2 & (RF6_SPECIAL)) {
        lore_ptr->vp[lore_ptr->vn] = _("特別な行動をする", "do something");
        lore_ptr->color[lore_ptr->vn++] = TERM_VIOLET;
    }

    display_monster_sometimes(lore_ptr);
    set_breath_types(player_ptr, lore_ptr);
    display_monster_breath(lore_ptr);

    lore_ptr->vn = 0;
    set_ball_types(player_ptr, lore_ptr);
    set_particular_types(player_ptr, lore_ptr);
    set_bolt_types(player_ptr, lore_ptr);
    set_status_types(lore_ptr);
    set_teleport_types(lore_ptr);
    set_floor_types(player_ptr, lore_ptr);
    set_summon_types(lore_ptr);
    display_monster_magic_types(lore_ptr);
    display_mosnter_magic_possibility(lore_ptr);
    display_monster_hp_ac(lore_ptr);

    lore_ptr->vn = 0;
    display_monster_concrete_abilities(lore_ptr);
    display_monster_abilities(lore_ptr);
    display_monster_constitutions(lore_ptr);

    lore_ptr->vn = 0;
    display_monster_concrete_weakness(lore_ptr);
    display_monster_weakness(lore_ptr);

    lore_ptr->vn = 0;
    display_monster_concrete_resistances(lore_ptr);
    display_monster_resistances(lore_ptr);
    display_monster_evolution(lore_ptr);

    lore_ptr->vn = 0;
    display_monster_concrete_immunities(lore_ptr);
    display_monster_immunities(lore_ptr);
    display_monster_alert(lore_ptr);
    display_monster_drops(lore_ptr);
    display_monster_blows(lore_ptr);
    display_monster_guardian(lore_ptr);
}
