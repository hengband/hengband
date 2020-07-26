#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

void have_kill_wall(player_type *creature_ptr)
{
    creature_ptr->kill_wall = FALSE;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        creature_ptr->kill_wall = TRUE;
    }

    if (music_singing(creature_ptr, MUSIC_WALL)) {
        creature_ptr->kill_wall = TRUE;
    }

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (riding_r_ptr->flags2 & RF2_KILL_WALL)
            creature_ptr->kill_wall = TRUE;
    }
}

void have_pass_wall(player_type *creature_ptr)
{
    creature_ptr->pass_wall = FALSE;

    if (creature_ptr->wraith_form) {
        creature_ptr->pass_wall = TRUE;
    }

    if (creature_ptr->tim_pass_wall) {
        creature_ptr->pass_wall = TRUE;
    }

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (!(riding_r_ptr->flags2 & RF2_PASS_WALL))
            creature_ptr->pass_wall = FALSE;
    }
}

void have_xtra_might(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->xtra_might = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_XTRA_MIGHT))
            creature_ptr->xtra_might = TRUE;
    }
}

void have_esp_evil(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_evil = FALSE;

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_DETECT_EVIL))
            creature_ptr->esp_evil = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_EVIL))
            creature_ptr->esp_evil = TRUE;
    }
}

void have_esp_animal(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_animal = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_ANIMAL))
            creature_ptr->esp_animal = TRUE;
    }
}

void have_esp_undead(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_undead = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_UNDEAD))
            creature_ptr->esp_undead = TRUE;
    }
}

void have_esp_demon(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_demon = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_DEMON))
            creature_ptr->esp_demon = TRUE;
    }
}

void have_esp_orc(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_orc = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_ORC))
            creature_ptr->esp_orc = TRUE;
    }
}

void have_esp_troll(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_troll = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_TROLL))
            creature_ptr->esp_troll = TRUE;
    }
}

void have_esp_giant(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_giant = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_GIANT))
            creature_ptr->esp_giant = TRUE;
    }
}

void have_esp_dragon(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_dragon = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_DRAGON))
            creature_ptr->esp_dragon = TRUE;
    }
}

void have_esp_human(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_human = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_HUMAN))
            creature_ptr->esp_human = TRUE;
    }
}

void have_esp_good(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_good = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_GOOD))
            creature_ptr->esp_good = TRUE;
    }
}

void have_esp_nonliving(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_nonliving = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_NONLIVING))
            creature_ptr->esp_nonliving = TRUE;
    }
}

void have_esp_unique(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->esp_unique = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_ESP_UNIQUE))
            creature_ptr->esp_unique = TRUE;
    }
}

void have_esp_telepathy(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->telepathy = FALSE;

    if (is_time_limit_esp(creature_ptr)) {
        creature_ptr->telepathy = TRUE;
    }

	if (creature_ptr->muta3 & MUT3_ESP) {
        creature_ptr->telepathy = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_MIND_FLAYER && creature_ptr->lev > 29)
        creature_ptr->telepathy = TRUE;

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_SPECTRE && creature_ptr->lev > 34)
        creature_ptr->telepathy = TRUE;

	if (creature_ptr->pclass == CLASS_MINDCRAFTER && creature_ptr->lev > 39)
        creature_ptr->telepathy = TRUE;

	if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
            creature_ptr->telepathy = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->telepathy = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx)
			continue;

		object_flags(creature_ptr, o_ptr, flgs);

		if (have_flag(flgs, TR_TELEPATHY))
			creature_ptr->telepathy = TRUE;
	}
}
