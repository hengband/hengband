#pragma once

enum class MonsterRaceId : short;
class MonsterEntity;
bool lore_do_probe(MonsterRaceId r_idx);
void lore_treasure(const MonsterEntity &monster, int num_item, int num_gold);
