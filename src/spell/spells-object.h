#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
void generate_amusement(PlayerType *player_ptr, int num, bool known);
void acquirement(PlayerType *player_ptr, POSITION y1, POSITION x1, int num, bool great);
bool curse_armor(PlayerType *player_ptr);
bool curse_weapon_object(PlayerType *player_ptr, bool force, ItemEntity &item);
void brand_bolts(PlayerType *player_ptr);

/*
 * Bit flags for the "enchant()" function
 */
constexpr auto ENCH_TOHIT = 0x01; /*!< 装備強化処理: 命中強化 / Enchant to hit */
constexpr auto ENCH_TODAM = 0x02; /*!< 装備強化処理: ダメージ強化 / Enchant to damage */
constexpr auto ENCH_TOAC = 0x04; /*!< 装備強化処理: AC強化 / Enchant to AC */
constexpr auto ENCH_FORCE = 0x08; /*!< 装備強化処理: 無条件に成功させる / Force enchantment */
bool enchant_equipment(ItemEntity &item, int n, int eflag);
bool enchant_spell(PlayerType *player_ptr, HIT_PROB num_hit, int num_dam, ARMOUR_CLASS num_ac);
void brand_weapon(PlayerType *player_ptr, int brand_type);
