#pragma once

#include "system/angband.h"
#include <string>
#include <vector>

struct vault_type {
    int16_t idx;

    std::string name; /* Name (offset) */
    std::string text; /* Text (offset) */

    byte typ{}; /* Vault type */
    PROB rat{}; /* Vault rating (unused) */
    POSITION hgt{}; /* Vault height */
    POSITION wid{}; /* Vault width */
};

extern std::vector<vault_type> vaults_info;

struct dun_data_type;
class PlayerType;
bool build_type10(PlayerType *player_ptr, dun_data_type *dd_ptr);
bool build_fixed_room(PlayerType *player_ptr, dun_data_type *dd_ptr, int typ, bool more_space);
