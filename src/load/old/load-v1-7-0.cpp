#include "load/old/load-v1-7-0.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "load/load-util.h"
#include "load/old/load-v1-5-0.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"

void set_hp_old(PlayerType *player_ptr)
{
    player_ptr->mhp = rd_s16b();

    player_ptr->chp = rd_s16b();
    player_ptr->chp_frac = rd_u16b();
}

void set_mana_old(PlayerType *player_ptr)
{
    player_ptr->msp = rd_s16b();

    player_ptr->csp = rd_s16b();
    player_ptr->csp_frac = rd_u16b();
}

void set_exp_frac_old(PlayerType *player_ptr)
{
    player_ptr->exp_frac = rd_u16b();
}

void remove_water_cave(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.quest_number != i2enum<QuestId>(OLD_QUEST_WATER_CAVE)) {
        return;
    }

    floor.set_dungeon_index(lite_town ? DUNGEON_ANGBAND : DUNGEON_GALGALS);
    floor.dun_level = 1;
    floor.quest_number = QuestId::NONE;
}
