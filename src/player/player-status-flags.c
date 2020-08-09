#include "art-definition/art-sword-types.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "mutation/mutation-flag-types.h"
#include "object/object-flags.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-checker.h"
#include "player/player-class.h"
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
#include "util/quarks.h"
#include "util/string-processor.h"
#include "monster-race/race-flags7.h"

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

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_TELEPATHY))
            creature_ptr->telepathy = TRUE;
    }
}

void have_bless_blade(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->bless_blade = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_BLESSED))
            creature_ptr->bless_blade = TRUE;
    }
}

void have_easy2_weapon(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->easy_2weapon = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (o_ptr->name2 == EGO_2WEAPON)
            creature_ptr->easy_2weapon = TRUE;
    }
}

void have_down_saving(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->down_saving = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (o_ptr->name2 == EGO_AMU_NAIVETY)
            creature_ptr->down_saving = TRUE;
    }
}

void have_no_ac(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->yoiyami = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (o_ptr->name2 == EGO_YOIYAMI)
            creature_ptr->yoiyami = TRUE;
    }
}

void have_no_flowed(player_type *creature_ptr)
{
    object_type *o_ptr;
    bool have_sw = FALSE, have_kabe = FALSE;
    OBJECT_IDX this_o_idx, next_o_idx = 0;

    creature_ptr->no_flowed = FALSE;

    if (creature_ptr->pass_wall && !creature_ptr->kill_wall)
        creature_ptr->no_flowed = TRUE;

    for (int i = 0; i < INVEN_PACK; i++) {
        if ((creature_ptr->inventory_list[i].tval == TV_NATURE_BOOK) && (creature_ptr->inventory_list[i].sval == 2))
            have_sw = TRUE;
        if ((creature_ptr->inventory_list[i].tval == TV_CRAFT_BOOK) && (creature_ptr->inventory_list[i].sval == 2))
            have_kabe = TRUE;
    }

    for (this_o_idx = creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx) {
        o_ptr = &creature_ptr->current_floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;

        if ((o_ptr->tval == TV_NATURE_BOOK) && (o_ptr->sval == 2))
            have_sw = TRUE;
        if ((o_ptr->tval == TV_CRAFT_BOOK) && (o_ptr->sval == 2))
            have_kabe = TRUE;
    }

    if (have_sw && ((creature_ptr->realm1 == REALM_NATURE) || (creature_ptr->realm2 == REALM_NATURE) || (creature_ptr->pclass == CLASS_SORCERER))) {
        const magic_type *s_ptr = &mp_ptr->info[REALM_NATURE - 1][SPELL_SW];
        if (creature_ptr->lev >= s_ptr->slevel)
            creature_ptr->no_flowed = TRUE;
    }

    if (have_kabe && ((creature_ptr->realm1 == REALM_CRAFT) || (creature_ptr->realm2 == REALM_CRAFT) || (creature_ptr->pclass == CLASS_SORCERER))) {
        const magic_type *s_ptr = &mp_ptr->info[REALM_CRAFT - 1][SPELL_WALL];
        if (creature_ptr->lev >= s_ptr->slevel)
            creature_ptr->no_flowed = TRUE;
    }
}

void have_mighty_throw(player_type *creature_ptr)
{
    object_type *o_ptr;

    creature_ptr->mighty_throw = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        if (o_ptr->name2 == EGO_RING_THROW)
            creature_ptr->mighty_throw = TRUE;
    }
}

void have_dec_mana(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->xtra_might = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_DEC_MANA))
            creature_ptr->dec_mana = TRUE;
    }
}

void have_reflect(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->reflect = FALSE;

    if (creature_ptr->pclass == CLASS_BERSERKER && creature_ptr->lev > 39)
        creature_ptr->reflect = TRUE;

    if (creature_ptr->pclass == CLASS_MIRROR_MASTER && creature_ptr->lev > 39)
        creature_ptr->reflect = TRUE;

    if (creature_ptr->special_defense & KAMAE_GENBU) {
        creature_ptr->reflect = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->reflect = TRUE;
    }

    if (creature_ptr->wraith_form) {
        creature_ptr->reflect = TRUE;
    }

    if (creature_ptr->magicdef) {
        creature_ptr->reflect = TRUE;
    }

    if (creature_ptr->tim_reflect) {
        creature_ptr->reflect = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_REFLECT))
            creature_ptr->reflect = TRUE;
    }
}

void have_see_nocto(player_type *creature_ptr)
{
    creature_ptr->see_nocto = FALSE;

    if (creature_ptr->pclass == CLASS_NINJA)
        creature_ptr->see_nocto = TRUE;
}

void have_warning(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->warning = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_WARNING)) {
            if (!o_ptr->inscription || !(angband_strchr(quark_str(o_ptr->inscription), '$')))
                creature_ptr->warning = TRUE;
        }
    }
}

void have_anti_magic(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->anti_magic = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_NO_MAGIC))
            creature_ptr->anti_magic = TRUE;
    }
}

void have_anti_tele(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->anti_tele = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_NO_TELE))
            creature_ptr->anti_tele = TRUE;
    }
}

void have_sh_fire(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->sh_fire = FALSE;

    if (creature_ptr->muta3 & MUT3_FIRE_BODY) {
        creature_ptr->sh_fire = TRUE;
    }

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        creature_ptr->sh_fire = TRUE;
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_DEMON_AURA)) {
            creature_ptr->sh_fire = TRUE;
        }
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        creature_ptr->sh_fire = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->sh_fire = TRUE;
    }

    if (creature_ptr->tim_sh_fire) {
        creature_ptr->sh_fire = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SH_FIRE))
            creature_ptr->sh_fire = TRUE;
    }
}

void have_sh_elec(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->sh_elec = FALSE;

    if (creature_ptr->muta3 & MUT3_ELEC_TOUC)
        creature_ptr->sh_elec = TRUE;

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_SHOCK_CLOAK)) {
            creature_ptr->sh_elec = TRUE;
        }
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        creature_ptr->sh_elec = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->sh_elec = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SH_ELEC))
            creature_ptr->sh_elec = TRUE;
    }
}

void have_sh_cold(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->sh_cold = FALSE;

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_ICE_ARMOR)) {
            creature_ptr->sh_cold = TRUE;
        }
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        creature_ptr->sh_cold = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->sh_cold = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SH_COLD))
            creature_ptr->sh_cold = TRUE;
    }
}

void have_easy_spell(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->easy_spell = FALSE;
    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_EASY_SPELL))
            creature_ptr->easy_spell = TRUE;
    }
}

void have_heavy_spell(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->heavy_spell = FALSE;
    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (o_ptr->name2 == EGO_AMU_FOOL)
            creature_ptr->heavy_spell = TRUE;
    }
}

void have_hold_exp(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->hold_exp = FALSE;

    if (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) {
        creature_ptr->hold_exp = TRUE;
    }

    if (creature_ptr->mimic_form == MIMIC_DEMON || creature_ptr->mimic_form == MIMIC_DEMON_LORD || creature_ptr->mimic_form == MIMIC_VAMPIRE) {
        creature_ptr->hold_exp = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_HOBBIT) {
        creature_ptr->hold_exp = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_GOLEM) {
        if (creature_ptr->lev > 34)
            creature_ptr->hold_exp = TRUE;
    }

    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_SKELETON || creature_ptr->prace == RACE_ZOMBIE || creature_ptr->prace == RACE_VAMPIRE
            || creature_ptr->prace == RACE_SPECTRE || creature_ptr->prace == RACE_BALROG || creature_ptr->prace == RACE_ANDROID)) {
        creature_ptr->hold_exp = TRUE;    
	}

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->hold_exp = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_HOLD_EXP))
            creature_ptr->hold_exp = TRUE;
    }
}

void have_see_inv(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->see_inv = FALSE;

    if (creature_ptr->pclass == CLASS_NINJA || creature_ptr->lev > 29)
        creature_ptr->see_inv = TRUE;

    if (creature_ptr->mimic_form == MIMIC_DEMON || creature_ptr->mimic_form == MIMIC_DEMON_LORD || creature_ptr->mimic_form == MIMIC_VAMPIRE) {
        creature_ptr->see_inv = TRUE;
    }

    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_HIGH_ELF || creature_ptr->prace == RACE_GOLEM || creature_ptr->prace == RACE_SKELETON
            || creature_ptr->prace == RACE_ZOMBIE || creature_ptr->prace == RACE_SPECTRE || creature_ptr->prace == RACE_ARCHON)) {
        creature_ptr->see_inv = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_DARK_ELF) {
        if (creature_ptr->lev > 19)
            creature_ptr->see_inv = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_MIND_FLAYER) {
        if (creature_ptr->lev > 14)
            creature_ptr->see_inv = TRUE;
    }

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_IMP || creature_ptr->prace == RACE_BALROG)) {
        if (creature_ptr->lev > 9)
            creature_ptr->see_inv = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->see_inv = TRUE;
    }

	if (creature_ptr->tim_invis) {
        creature_ptr->see_inv = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SEE_INVIS))
            creature_ptr->see_inv = TRUE;
    }
}

void have_free_act(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->free_act = FALSE;
	
	if (creature_ptr->muta3 & MUT3_MOTION)
        creature_ptr->free_act = TRUE;

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_GNOME) {
		creature_ptr->free_act = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_GOLEM) {
        creature_ptr->free_act = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_SPECTRE) {
        creature_ptr->free_act = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_ANDROID) {
        creature_ptr->free_act = TRUE;
    }

    if (heavy_armor(creature_ptr) && (!creature_ptr->inventory_list[INVEN_RARM].k_idx || creature_ptr->right_hand_weapon)
        && (!creature_ptr->inventory_list[INVEN_LARM].k_idx || creature_ptr->left_hand_weapon)) {
        if (creature_ptr->lev > 24)
            creature_ptr->free_act = TRUE;
    }

    if (creature_ptr->pclass == CLASS_MONK || creature_ptr->pclass == CLASS_FORCETRAINER) {
        if (!(heavy_armor(creature_ptr))) {
            if (creature_ptr->lev > 24)
                creature_ptr->free_act = TRUE;
        }
    }

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        creature_ptr->free_act = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->free_act = TRUE;
    }

    if (creature_ptr->magicdef) {
        creature_ptr->free_act = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_FREE_ACT))
            creature_ptr->free_act = TRUE;
    }
}

void have_sustain_str(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->sustain_str = FALSE;
    if (creature_ptr->pclass == CLASS_BERSERKER) {
        creature_ptr->sustain_str = TRUE;
	}
    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_HALF_TROLL || creature_ptr->prace == RACE_HALF_OGRE || creature_ptr->prace == RACE_HALF_GIANT)) {
        creature_ptr->sustain_str = TRUE;
    }
    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->sustain_str = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SUST_STR))
            creature_ptr->sustain_str = TRUE;
    }
}

void have_sustain_int(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->sustain_int = FALSE;
    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_MIND_FLAYER)) {
        creature_ptr->sustain_int = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->sustain_int = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SUST_INT))
            creature_ptr->sustain_int = TRUE;
    }
}

void have_sustain_wis(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->sustain_wis = FALSE;
    if (creature_ptr->pclass == CLASS_MINDCRAFTER && creature_ptr->lev > 19)
        creature_ptr->sustain_wis = TRUE;
	
	if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_MIND_FLAYER)) {
        creature_ptr->sustain_wis = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->sustain_wis = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SUST_WIS))
            creature_ptr->sustain_wis = TRUE;
    }
}

void have_sustain_dex(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->sustain_dex = FALSE;
    if (creature_ptr->pclass == CLASS_BERSERKER) {
        creature_ptr->sustain_dex = TRUE;
    }

    if (creature_ptr->pclass == CLASS_NINJA && creature_ptr->lev > 24)
        creature_ptr->sustain_dex = TRUE;

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->sustain_dex = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SUST_DEX))
            creature_ptr->sustain_dex = TRUE;
    }
}

void have_sustain_con(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->sustain_con = FALSE;
    if (creature_ptr->pclass == CLASS_BERSERKER) {
        creature_ptr->sustain_con = TRUE;
    }

	if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_AMBERITE || creature_ptr->prace == RACE_DUNADAN)) {
        creature_ptr->sustain_con = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->sustain_con = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SUST_CON))
            creature_ptr->sustain_con = TRUE;
    }
}

void have_sustain_chr(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->sustain_chr = FALSE;

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->sustain_chr = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SUST_CHR))
            creature_ptr->sustain_chr = TRUE;
    }
}

void have_levitation(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->levitation = FALSE;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        creature_ptr->levitation = TRUE;
    }

    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_DRACONIAN || creature_ptr->prace == RACE_SPECTRE || creature_ptr->prace == RACE_SPRITE
            || creature_ptr->prace == RACE_ARCHON || creature_ptr->prace == RACE_S_FAIRY)) {
        creature_ptr->levitation = TRUE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU || creature_ptr->special_defense & KAMAE_SUZAKU) {
        creature_ptr->levitation = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->levitation = TRUE;
	}

    if (creature_ptr->magicdef) {
    }

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        creature_ptr->levitation = (riding_r_ptr->flags7 & RF7_CAN_FLY) ? TRUE : FALSE;
    }

    if (creature_ptr->tim_levitation) {
        creature_ptr->levitation = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_LEVITATION))
            creature_ptr->levitation = TRUE;
    }
}

void have_can_swim(player_type *creature_ptr)
{
	creature_ptr->can_swim = FALSE;
    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (riding_r_ptr->flags7 & (RF7_CAN_SWIM | RF7_AQUATIC))
            creature_ptr->can_swim = TRUE;
    }
}

void have_slow_digest(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->slow_digest = FALSE;

    if (creature_ptr->pclass == CLASS_NINJA) {
        creature_ptr->slow_digest = TRUE;
    }

	if (creature_ptr->lev > 14 && !creature_ptr->mimic_form && creature_ptr->prace == RACE_HALF_TROLL) {
        if (creature_ptr->pclass == CLASS_WARRIOR || creature_ptr->pclass == CLASS_BERSERKER) {
            creature_ptr->slow_digest = TRUE;
            /* Let's not make Regeneration
             * a disadvantage for the poor warriors who can
             * never learn a spell that satisfies hunger (actually
             * neither can rogues, but half-trolls are not
             * supposed to play rogues) */
        }
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->slow_digest = TRUE;
    }

    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_GOLEM || creature_ptr->prace == RACE_ZOMBIE || creature_ptr->prace == RACE_SPECTRE
            || creature_ptr->prace == RACE_ANDROID)) {
        creature_ptr->slow_digest = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_SLOW_DIGEST))
            creature_ptr->slow_digest = TRUE;
    }
}

void have_regenerate(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->regenerate = FALSE;

    if (!creature_ptr->mimic_form) {
        switch (creature_ptr->prace) {
        case RACE_HALF_TROLL:
            if (creature_ptr->lev > 14) {
                creature_ptr->regenerate = TRUE;
            }
            break;
        case RACE_AMBERITE:
            creature_ptr->regenerate = TRUE;
            break;
        }
    }

    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
        if (creature_ptr->lev > 44)
            creature_ptr->regenerate = TRUE;
        break;
    case CLASS_BERSERKER:
        creature_ptr->regenerate = TRUE;
        break;
    }

    if (creature_ptr->muta3 & MUT3_FLESH_ROT)
        creature_ptr->regenerate = FALSE;

    if (creature_ptr->muta3 & MUT3_REGEN)
        creature_ptr->regenerate = TRUE;

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->regenerate = TRUE;
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_DEMON_AURA)) {
            creature_ptr->regenerate = TRUE;
        }
    }

	if (creature_ptr->tim_regen) {
        creature_ptr->regenerate = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_REGEN))
            creature_ptr->regenerate = TRUE;
    }
}

void have_curses(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->cursed = 0L;

    if (creature_ptr->pseikaku == PERSONALITY_SEXY)
        creature_ptr->cursed |= (TRC_AGGRAVATE);

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_AGGRAVATE))
            creature_ptr->cursed |= TRC_AGGRAVATE;
        if (have_flag(flgs, TR_DRAIN_EXP))
            creature_ptr->cursed |= TRC_DRAIN_EXP;
        if (have_flag(flgs, TR_TY_CURSE))
            creature_ptr->cursed |= TRC_TY_CURSE;
        if (have_flag(flgs, TR_ADD_L_CURSE))
            creature_ptr->cursed |= TRC_ADD_L_CURSE;
        if (have_flag(flgs, TR_ADD_H_CURSE))
            creature_ptr->cursed |= TRC_ADD_H_CURSE;
        if (have_flag(flgs, TR_DRAIN_HP))
            creature_ptr->cursed |= TRC_DRAIN_HP;
        if (have_flag(flgs, TR_DRAIN_MANA))
            creature_ptr->cursed |= TRC_DRAIN_MANA;
        if (have_flag(flgs, TR_CALL_ANIMAL))
            creature_ptr->cursed |= TRC_CALL_ANIMAL;
        if (have_flag(flgs, TR_CALL_DEMON))
            creature_ptr->cursed |= TRC_CALL_DEMON;
        if (have_flag(flgs, TR_CALL_DRAGON))
            creature_ptr->cursed |= TRC_CALL_DRAGON;
        if (have_flag(flgs, TR_CALL_UNDEAD))
            creature_ptr->cursed |= TRC_CALL_UNDEAD;
        if (have_flag(flgs, TR_COWARDICE))
            creature_ptr->cursed |= TRC_COWARDICE;
        if (have_flag(flgs, TR_LOW_MELEE))
            creature_ptr->cursed |= TRC_LOW_MELEE;
        if (have_flag(flgs, TR_LOW_AC))
            creature_ptr->cursed |= TRC_LOW_AC;
        if (have_flag(flgs, TR_LOW_MAGIC))
            creature_ptr->cursed |= TRC_LOW_MAGIC;
        if (have_flag(flgs, TR_FAST_DIGEST))
            creature_ptr->cursed |= TRC_FAST_DIGEST;
        if (have_flag(flgs, TR_SLOW_REGEN))
            creature_ptr->cursed |= TRC_SLOW_REGEN;

        creature_ptr->cursed |= (o_ptr->curse_flags & (0xFFFFFFF0L));
        if (o_ptr->name1 == ART_CHAINSWORD)
            creature_ptr->cursed |= TRC_CHAINSWORD;

        if (have_flag(flgs, TR_TELEPORT)) {
            if (object_is_cursed(o_ptr))
                creature_ptr->cursed |= TRC_TELEPORT;
            else {
                concptr insc = quark_str(o_ptr->inscription);

                /* {.} will stop random teleportation. */
                if (o_ptr->inscription && angband_strchr(insc, '.')) {
                } else {
                    creature_ptr->cursed |= TRC_TELEPORT_SELF;
                }
            }
        }
    }

    if (creature_ptr->cursed & TRC_TELEPORT)
        creature_ptr->cursed &= ~(TRC_TELEPORT_SELF);

    if ((is_specific_player_race(creature_ptr, RACE_S_FAIRY)) && (creature_ptr->pseikaku != PERSONALITY_SEXY) && (creature_ptr->cursed & TRC_AGGRAVATE)) {
        creature_ptr->cursed &= ~(TRC_AGGRAVATE);
    }
}

void have_impact(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->impact[0] = FALSE;
    creature_ptr->impact[1] = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_IMPACT))
            creature_ptr->impact[(i == INVEN_RARM) ? 0 : 1] = TRUE;
    }

}

void have_extra_blow(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->extra_blows[0] = creature_ptr->extra_blows[1] = 0;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_INFRA))
            creature_ptr->see_infra += o_ptr->pval;
        if (have_flag(flgs, TR_BLOWS)) {
            if ((i == INVEN_RARM || i == INVEN_RIGHT) && !creature_ptr->two_handed_weapon)
                creature_ptr->extra_blows[0] += o_ptr->pval;
            else if ((i == INVEN_LARM || i == INVEN_LEFT) && !creature_ptr->two_handed_weapon)
                creature_ptr->extra_blows[1] += o_ptr->pval;
            else {
                creature_ptr->extra_blows[0] += o_ptr->pval;
                creature_ptr->extra_blows[1] += o_ptr->pval;
            }
        }
    }
}

void have_resist_acid(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->resist_acid = FALSE;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        creature_ptr->resist_acid = TRUE;
    }

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_YEEK || creature_ptr->prace == RACE_KLACKON)) {
        creature_ptr->resist_acid = TRUE;
    }

	if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_DRACONIAN && creature_ptr->lev > 14) {
        creature_ptr->resist_acid = TRUE;
	}

	if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        creature_ptr->resist_acid = TRUE;
	}

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->resist_acid = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_RES_ACID))
            creature_ptr->resist_acid = TRUE;
    }

	if (creature_ptr->immune_acid)
        creature_ptr->resist_acid = TRUE;
}

void have_resist_elec(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->resist_elec = FALSE;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        creature_ptr->resist_elec = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_DRACONIAN && creature_ptr->lev > 19) {
        creature_ptr->resist_elec = TRUE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        creature_ptr->resist_elec = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->resist_elec = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);
        if (have_flag(flgs, TR_RES_ELEC))
            creature_ptr->resist_elec = TRUE;
    }

    if (creature_ptr->immune_elec)
        creature_ptr->resist_elec = TRUE;
}

void have_resist_fire(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->resist_fire = FALSE;

    if (creature_ptr->mimic_form == MIMIC_DEMON || creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        creature_ptr->resist_fire = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_DRACONIAN && creature_ptr->lev > 4) {
        creature_ptr->resist_fire = TRUE;
    }

	if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_IMP || creature_ptr->prace == RACE_BALROG)) {
        creature_ptr->resist_fire = TRUE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        creature_ptr->resist_fire = TRUE;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->resist_fire = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_RES_FIRE))
            creature_ptr->resist_fire = TRUE;
    }

    if (creature_ptr->immune_fire)
        creature_ptr->resist_fire = TRUE;
}

void have_resist_cold(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->resist_cold = FALSE;


    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD || creature_ptr->mimic_form == MIMIC_VAMPIRE) {
        creature_ptr->resist_cold = TRUE;
    }

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_ZOMBIE) && creature_ptr->lev > 4) {
        creature_ptr->resist_cold = TRUE;
    }

    if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_DRACONIAN || creature_ptr->prace == RACE_SKELETON)
                        && creature_ptr->lev > 9) {
        creature_ptr->resist_cold = TRUE;
    }

	if (!creature_ptr->mimic_form && (creature_ptr->prace == RACE_VAMPIRE || creature_ptr->prace == RACE_SPECTRE)) {
        creature_ptr->resist_fire = TRUE;
    }

	if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        creature_ptr->resist_cold = TRUE;
    }

	if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->resist_cold = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

	    if (have_flag(flgs, TR_RES_COLD))
            creature_ptr->resist_cold = TRUE;
    }

    if (creature_ptr->immune_cold)
        creature_ptr->resist_cold = TRUE;
}

void have_resist_pois(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    creature_ptr->resist_pois = FALSE;

    if (creature_ptr->pclass == CLASS_NINJA && creature_ptr->lev > 19)
        creature_ptr->resist_pois = TRUE;

    if (creature_ptr->mimic_form == MIMIC_VAMPIRE || creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        creature_ptr->resist_pois = TRUE;
    }

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_DRACONIAN && creature_ptr->lev > 34) {
        creature_ptr->resist_pois = TRUE;
    }

    if (!creature_ptr->mimic_form
        && (creature_ptr->prace == RACE_GOLEM || creature_ptr->prace == RACE_SKELETON || creature_ptr->prace == RACE_VAMPIRE
            || creature_ptr->prace == RACE_SPECTRE || creature_ptr->prace == RACE_ANDROID)) {
        creature_ptr->resist_pois = TRUE;
    }

    if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        creature_ptr->resist_pois = TRUE;
    }

	if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        creature_ptr->resist_pois = TRUE;
    }

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

	    if (have_flag(flgs, TR_RES_POIS))
            creature_ptr->resist_pois = TRUE;
    }
}

