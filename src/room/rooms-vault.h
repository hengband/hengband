#pragma once

#include <stdint.h>
#include <string>
#include <vector>

struct vault_type {
    vault_type() = default;
    short idx = 0;

    std::string name = ""; /* Name (offset) */
    std::string text = ""; /* Text (offset) */

    uint8_t typ = 0; /* Vault type */
    int rat = 0; /* Vault rating (unused) */
    int hgt = 0; /* Vault height */
    int wid = 0; /* Vault width */
};

extern std::vector<vault_type> vaults_info;

struct dun_data_type;
class PlayerType;
bool build_type10(PlayerType *player_ptr, dun_data_type *dd_ptr);
bool build_fixed_room(PlayerType *player_ptr, dun_data_type *dd_ptr, int typ, bool more_space);
