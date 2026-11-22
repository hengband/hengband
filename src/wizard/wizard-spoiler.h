#pragma once

#include "system/angband.h"
#include <vector>

enum class PlayerClassType : short;
enum class RealmPosition;
enum class RealmType;
enum class SpoilerOutputResultType;
struct player_magic;
void exe_output_spoilers();
SpoilerOutputResultType output_all_spoilers();
std::vector<RealmPosition> spoiler_realm_positions(PlayerClassType pclass, const player_magic &magic, RealmType realm);
