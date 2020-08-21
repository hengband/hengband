/*!
 * @brief モンスター死亡時の特殊処理switch (一般的な処理もdefaultで実施)
 * @date 2020/08/21
 * @author Hourier
 */

#include "monster-floor/special-death-switcher.h"
#include "art-definition/art-armor-types.h"
#include "art-definition/art-bow-types.h"
#include "art-definition/art-protector-types.h"
#include "art-definition/art-weapon-types.h"
#include "artifact/fixed-art-generator.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "grid/grid.h"
#include "monster-floor/monster-death-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "spell/spell-types.h"
#include "spell/spells-summon.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/system-variables.h"
#include "view/display-messages.h"
#include "world/world.h"

static void on_dead_pink_horror(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena || player_ptr->phase_out)
        return;

    bool notice = FALSE;
    for (int i = 0; i < 2; i++) {
        POSITION wy = md_ptr->md_y;
        POSITION wx = md_ptr->md_x;
        bool pet = is_pet(md_ptr->m_ptr);
        BIT_FLAGS mode = pet ? PM_FORCE_PET : PM_NONE;
        if (summon_specific(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, 100, SUMMON_BLUE_HORROR, mode) && player_can_see_bold(player_ptr, wy, wx))
            notice = TRUE;
    }

    if (notice)
        msg_print(_("ピンク・ホラーは分裂した！", "The Pink horror divides!"));
}

static void on_dead_bloodletter(player_type* player_ptr, monster_death_type* md_ptr)
{
    if (!md_ptr->drop_chosen_item || (randint1(100) >= 15))
        return;

    object_type forge;
    object_type *q_ptr = &forge;
    object_prep(player_ptr, q_ptr, lookup_kind(TV_SWORD, SV_BLADE_OF_CHAOS));
    apply_magic(player_ptr, q_ptr, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART | md_ptr->mo_mode);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_raal(player_type *player_ptr, monster_death_type *md_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!md_ptr->drop_chosen_item || (floor_ptr->dun_level <= 9))
        return;

    object_type forge;
    object_type *q_ptr = &forge;
    object_wipe(q_ptr);
    if ((floor_ptr->dun_level > 49) && one_in_(5))
        get_obj_num_hook = kind_is_good_book;
    else
        get_obj_num_hook = kind_is_book;

    make_object(player_ptr, q_ptr, md_ptr->mo_mode);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_dawn(player_type *player_ptr, monster_death_type *md_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->inside_arena || player_ptr->phase_out || one_in_(7))
        return;

    POSITION wy = md_ptr->md_y;
    POSITION wx = md_ptr->md_x;
    int attempts = 100;
    bool pet = is_pet(md_ptr->m_ptr);
    do {
        scatter(player_ptr, &wy, &wx, md_ptr->md_y, md_ptr->md_x, 20, 0);
    } while (!(in_bounds(floor_ptr, wy, wx) && is_cave_empty_bold2(player_ptr, wy, wx)) && --attempts);

    if (attempts <= 0)
        return;

    BIT_FLAGS mode = pet ? PM_FORCE_PET : PM_NONE;
    if (summon_specific(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, 100, SUMMON_DAWN, mode) && player_can_see_bold(player_ptr, wy, wx))
        msg_print(_("新たな戦士が現れた！", "A new warrior steps forth!"));
}

static void on_dead_unmaker(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (is_seen(player_ptr, md_ptr->m_ptr)) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(player_ptr, m_name, md_ptr->m_ptr, MD_NONE);
        msg_format(_("%sは辺りにログルスの残り香を撒き散らした！", "%^s sprinkled the remaining incense from Logrus!"), m_name);
    }

    (void)project(player_ptr, md_ptr->m_idx, 6, md_ptr->md_y, md_ptr->md_x, 100, GF_CHAOS, PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, -1);
}

static void on_dead_sacred_treasures(player_type *player_ptr, monster_death_type *md_ptr)
{
    if ((player_ptr->pseikaku != PERSONALITY_LAZY) || !md_ptr->drop_chosen_item)
        return;

    ARTIFACT_IDX a_idx = 0;
    artifact_type *a_ptr = NULL;
    do {
        switch (randint0(3)) {
        case 0:
            a_idx = ART_NAMAKE_HAMMER;
            break;
        case 1:
            a_idx = ART_NAMAKE_BOW;
            break;
        case 2:
            a_idx = ART_NAMAKE_ARMOR;
            break;
        }

        a_ptr = &a_info[a_idx];
    } while (a_ptr->cur_num == 1);

    if (create_named_art(player_ptr, a_idx, md_ptr->md_y, md_ptr->md_x)) {
        a_ptr->cur_num = 1;
        if (current_world_ptr->character_dungeon)
            a_ptr->floor_id = player_ptr->floor_id;

        return;
    }
    
    if (!preserve_mode)
        a_ptr->cur_num = 1;
}

static void on_dead_serpent(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!md_ptr->drop_chosen_item)
        return;

    object_type forge;
    object_type *q_ptr = &forge;
    object_prep(player_ptr, q_ptr, lookup_kind(TV_HAFTED, SV_GROND));
    q_ptr->name1 = ART_GROND;
    apply_magic(player_ptr, q_ptr, -1, AM_GOOD | AM_GREAT);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
    q_ptr = &forge;
    object_prep(player_ptr, q_ptr, lookup_kind(TV_CROWN, SV_CHAOS));
    q_ptr->name1 = ART_CHAOS;
    apply_magic(player_ptr, q_ptr, -1, AM_GOOD | AM_GREAT);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_death_sword(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!md_ptr->drop_chosen_item)
        return;

    object_type forge;
    object_type *q_ptr = &forge;
    object_prep(player_ptr, q_ptr, lookup_kind(TV_SWORD, randint1(2)));
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_can_angel(player_type *player_ptr, monster_death_type *md_ptr)
{
    bool is_drop_can = md_ptr->drop_chosen_item;
    bool is_silver = md_ptr->m_ptr->r_idx == MON_A_SILVER;
    is_silver &= md_ptr->r_ptr->r_akills % 5 == 0;
    is_drop_can &= (md_ptr->m_ptr->r_idx == MON_A_GOLD) || is_silver;
    if (!is_drop_can)
        return;

    object_type forge;
    object_type *q_ptr = &forge;
    object_prep(player_ptr, q_ptr, lookup_kind(TV_CHEST, SV_CHEST_KANDUME));
    apply_magic(player_ptr, q_ptr, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_rolento(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (is_seen(player_ptr, md_ptr->m_ptr)) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(player_ptr, m_name, md_ptr->m_ptr, MD_NONE);
        msg_format(_("%sは手榴弾を抱えて自爆した！", "%^s broke himself with grenades!"), m_name);
    }

    (void)project(player_ptr, md_ptr->m_idx, 3, md_ptr->md_y, md_ptr->md_x, damroll(20, 10), GF_FIRE, PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, -1);
}

void switch_special_death(player_type *player_ptr, monster_death_type *md_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    switch (md_ptr->m_ptr->r_idx) {
    case MON_PINK_HORROR:
        on_dead_pink_horror(player_ptr, md_ptr);
        return;
    case MON_BLOODLETTER:
        on_dead_bloodletter(player_ptr, md_ptr);
        return;
    case MON_RAAL:
        on_dead_raal(player_ptr, md_ptr);
        return;
    case MON_DAWN:
        on_dead_dawn(player_ptr, md_ptr);
        return;
    case MON_UNMAKER:
        on_dead_unmaker(player_ptr, md_ptr);
        break;
    case MON_UNICORN_ORD:
    case MON_MORGOTH:
    case MON_ONE_RING:
        on_dead_sacred_treasures(player_ptr, md_ptr);
        return;
    case MON_SERPENT:
        on_dead_serpent(player_ptr, md_ptr);
        return;
    case MON_B_DEATH_SWORD:
        on_dead_death_sword(player_ptr, md_ptr);
        return;
    case MON_A_GOLD:
    case MON_A_SILVER:
        on_dead_can_angel(player_ptr, md_ptr);
        return;
    case MON_ROLENTO:
        on_dead_rolento(player_ptr, md_ptr);
    case MON_MIDDLE_AQUA_FIRST:
    case MON_LARGE_AQUA_FIRST:
    case MON_EXTRA_LARGE_AQUA_FIRST:
    case MON_MIDDLE_AQUA_SECOND:
    case MON_LARGE_AQUA_SECOND:
    case MON_EXTRA_LARGE_AQUA_SECOND: {
        if (floor_ptr->inside_arena || player_ptr->phase_out)
            break;

        bool notice = FALSE;
        const int popped_bubbles = 4;
        for (int i = 0; i < popped_bubbles; i++) {
            POSITION wy = md_ptr->md_y;
            POSITION wx = md_ptr->md_x;
            bool pet = is_pet(md_ptr->m_ptr);
            BIT_FLAGS mode = pet ? PM_FORCE_PET : PM_NONE;
            MONSTER_IDX smaller_bubblle = md_ptr->m_ptr->r_idx - 1;
            if (summon_named_creature(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, smaller_bubblle, mode) && player_can_see_bold(player_ptr, wy, wx))
                notice = TRUE;
        }

        if (notice)
            msg_print(_("泡が弾けた！", "The bubble pops!"));

        break;
    }
    case MON_TOTEM_MOAI: {
        if (floor_ptr->inside_arena || player_ptr->phase_out || one_in_(8))
            break;

        POSITION wy = md_ptr->md_y;
        POSITION wx = md_ptr->md_x;
        int attempts = 100;
        bool pet = is_pet(md_ptr->m_ptr);
        do {
            scatter(player_ptr, &wy, &wx, md_ptr->md_y, md_ptr->md_x, 20, 0);
        } while (!(in_bounds(floor_ptr, wy, wx) && is_cave_empty_bold2(player_ptr, wy, wx)) && --attempts);

        if (attempts <= 0)
            break;

        BIT_FLAGS mode = pet ? PM_FORCE_PET : PM_NONE;
        if (summon_named_creature(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, MON_TOTEM_MOAI, mode) && player_can_see_bold(player_ptr, wy, wx))
            msg_print(_("新たなモアイが現れた！", "A new moai steps forth!"));

        break;
    }
    default: {
        if (!md_ptr->drop_chosen_item)
            break;

        switch (md_ptr->r_ptr->d_char) {
        case '(': {
            if (floor_ptr->dun_level <= 0)
                break;

            object_type forge;
            object_type *q_ptr = &forge;
            object_wipe(q_ptr);
            get_obj_num_hook = kind_is_cloak;
            make_object(player_ptr, q_ptr, md_ptr->mo_mode);
            (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
            break;
        }
        case '/': {
            if (floor_ptr->dun_level <= 4)
                break;

            object_type forge;
            object_type *q_ptr = &forge;
            object_wipe(q_ptr);
            get_obj_num_hook = kind_is_polearm;
            make_object(player_ptr, q_ptr, md_ptr->mo_mode);
            (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
            break;
        }
        case '[': {
            if (floor_ptr->dun_level <= 19)
                break;

            object_type forge;
            object_type *q_ptr = &forge;
            object_wipe(q_ptr);
            get_obj_num_hook = kind_is_armor;
            make_object(player_ptr, q_ptr, md_ptr->mo_mode);
            (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
            break;
        }
        case '\\': {
            if (floor_ptr->dun_level <= 4)
                break;

            object_type forge;
            object_type *q_ptr = &forge;
            object_wipe(q_ptr);
            get_obj_num_hook = kind_is_hafted;
            make_object(player_ptr, q_ptr, md_ptr->mo_mode);
            (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
            break;
        }
        case '|': {
            if (md_ptr->m_ptr->r_idx == MON_STORMBRINGER)
                break;

            object_type forge;
            object_type *q_ptr = &forge;
            object_wipe(q_ptr);
            get_obj_num_hook = kind_is_sword;
            make_object(player_ptr, q_ptr, md_ptr->mo_mode);
            (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
            break;
        }
        }
    }
    }
}
