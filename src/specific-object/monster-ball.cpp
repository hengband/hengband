#include "specific-object/monster-ball.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "game-option/input-options.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "object-activation/activation-util.h"
#include "pet/pet-util.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/flag-group.h"
#include "util/quarks.h"
#include "view/display-messages.h"

static void inscribe_nickname(ae_type *ae_ptr)
{
    if (!cap_nickname)
        return;

    concptr t;
    char *s;
    char buf[80] = "";
    if (ae_ptr->o_ptr->inscription)
        strcpy(buf, quark_str(ae_ptr->o_ptr->inscription));

    s = buf;
    for (s = buf; *s && (*s != '#'); s++) {
#ifdef JP
        if (iskanji(*s))
            s++;
#endif
    }

    *s = '#';
    s++;
#ifdef JP
#else
    *s++ = '\'';
#endif
    t = quark_str(cap_nickname);
    while (*t) {
        *s = *t;
        s++;
        t++;
    }
#ifdef JP
#else
    *s++ = '\'';
#endif
    *s = '\0';
    ae_ptr->o_ptr->inscription = quark_add(buf);
}

static bool set_activation_target(player_type *player_ptr, ae_type *ae_ptr)
{
    bool old_target_pet = target_pet;
    target_pet = true;
    if (!get_aim_dir(player_ptr, &ae_ptr->dir)) {
        target_pet = old_target_pet;
        return false;
    }

    target_pet = old_target_pet;
    if (!fire_ball(player_ptr, GF_CAPTURE, ae_ptr->dir, 0, 0))
        return true;

    ae_ptr->o_ptr->pval = (PARAMETER_VALUE)cap_mon;
    ae_ptr->o_ptr->xtra3 = (XTRA8)cap_mspeed;
    ae_ptr->o_ptr->xtra4 = (XTRA16)cap_hp;
    ae_ptr->o_ptr->xtra5 = (XTRA16)cap_maxhp;
    inscribe_nickname(ae_ptr);
    return true;
}

static void add_quark_to_inscription(player_type *player_ptr, ae_type *ae_ptr, concptr t, char *buf)
{
    if (!*t)
        return;

    char *s = buf;
    t++;
#ifdef JP
#else
    bool quote = false;
    if (*t == '\'') {
        t++;
        quote = true;
    }
#endif

    while (*t) {
        *s = *t;
        t++;
        s++;
    }

#ifdef JP
#else
    if (quote && *(s - 1) == '\'')
        s--;
#endif

    *s = '\0';
    player_ptr->current_floor_ptr->m_list[hack_m_idx_ii].nickname = quark_add(buf);
    t = quark_str(ae_ptr->o_ptr->inscription);
    s = buf;
    while (*t && (*t != '#')) {
        *s = *t;
        t++;
        s++;
    }

    *s = '\0';
    ae_ptr->o_ptr->inscription = quark_add(buf);
}

static void check_inscription_value(player_type *player_ptr, ae_type *ae_ptr)
{
    if (ae_ptr->o_ptr->inscription == 0)
        return;

    char buf[80];
    concptr t = quark_str(ae_ptr->o_ptr->inscription);
    for (t = quark_str(ae_ptr->o_ptr->inscription); *t && (*t != '#'); t++) {
#ifdef JP
        if (iskanji(*t))
            t++;
#endif
    }

    add_quark_to_inscription(player_ptr, ae_ptr, t, buf);
}

static void check_monster_ball_use(player_type *player_ptr, ae_type *ae_ptr)
{
    if (!monster_can_enter(player_ptr, player_ptr->y + ddy[ae_ptr->dir], player_ptr->x + ddx[ae_ptr->dir], &r_info[ae_ptr->o_ptr->pval], 0))
        return;

    if (!place_monster_aux(player_ptr, 0, player_ptr->y + ddy[ae_ptr->dir], player_ptr->x + ddx[ae_ptr->dir], ae_ptr->o_ptr->pval, PM_FORCE_PET | PM_NO_KAGE))
        return;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (ae_ptr->o_ptr->xtra3)
        floor_ptr->m_list[hack_m_idx_ii].mspeed = ae_ptr->o_ptr->xtra3;

    if (ae_ptr->o_ptr->xtra5)
        floor_ptr->m_list[hack_m_idx_ii].max_maxhp = ae_ptr->o_ptr->xtra5;

    if (ae_ptr->o_ptr->xtra4)
        floor_ptr->m_list[hack_m_idx_ii].hp = ae_ptr->o_ptr->xtra4;

    floor_ptr->m_list[hack_m_idx_ii].maxhp = floor_ptr->m_list[hack_m_idx_ii].max_maxhp;
    check_inscription_value(player_ptr, ae_ptr);
    ae_ptr->o_ptr->pval = 0;
    ae_ptr->o_ptr->xtra3 = 0;
    ae_ptr->o_ptr->xtra4 = 0;
    ae_ptr->o_ptr->xtra5 = 0;
    ae_ptr->success = true;
}

bool exe_monster_capture(player_type *player_ptr, ae_type *ae_ptr)
{
    if (ae_ptr->o_ptr->tval != ItemKindType::CAPTURE)
        return false;

    if (ae_ptr->o_ptr->pval == 0) {
        if (!set_activation_target(player_ptr, ae_ptr))
            return true;

        calc_android_exp(player_ptr);
        return true;
    }

    ae_ptr->success = false;
    if (!get_direction(player_ptr, &ae_ptr->dir, false, false))
        return true;

    check_monster_ball_use(player_ptr, ae_ptr);
    if (!ae_ptr->success)
        msg_print(_("おっと、解放に失敗した。", "Oops.  You failed to release your pet."));

    calculate_upkeep(player_ptr);
    calc_android_exp(player_ptr);
    return true;
}
