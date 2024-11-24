#pragma once

enum summon_type : int;
enum class MonraceId : short;
class PlayerType;
bool check_summon_specific(PlayerType *player_ptr, MonraceId summoner_idx, MonraceId r_idx, summon_type type);
