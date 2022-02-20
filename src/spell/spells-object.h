#pragma once

#include "system/angband.h"

class ObjectType;
class PlayerType;
void amusement(PlayerType *player_ptr, POSITION y1, POSITION x1, int num, bool known);
void acquirement(PlayerType *player_ptr, POSITION y1, POSITION x1, int num, bool great, bool special, bool known);
bool curse_armor(PlayerType *player_ptr);
bool curse_weapon_object(PlayerType *player_ptr, bool force, ObjectType *o_ptr);
void brand_bolts(PlayerType *player_ptr);

/*
 * Bit flags for the "enchant()" function
 */
#define ENCH_TOHIT      0x01 /*!< 装備強化処理: 命中強化 / Enchant to hit */
#define ENCH_TODAM      0x02 /*!< 装備強化処理: ダメージ強化 / Enchant to damage */
#define ENCH_TOAC       0x04 /*!< 装備強化処理: AC強化 / Enchant to AC */
#define ENCH_FORCE      0x08 /*!< 装備強化処理: 無条件に成功させる / Force enchantment */
bool enchant_equipment(PlayerType *player_ptr, ObjectType *o_ptr, int n, int eflag);
bool enchant_spell(PlayerType *player_ptr, HIT_PROB num_hit, int num_dam, ARMOUR_CLASS num_ac);
void brand_weapon(PlayerType *player_ptr, int brand_type);
