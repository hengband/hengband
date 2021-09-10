#pragma once

#include "system/angband.h"

struct object_type;;
struct player_type;
void amusement(player_type *player_ptr, POSITION y1, POSITION x1, int num, bool known);
void acquirement(player_type *player_ptr, POSITION y1, POSITION x1, int num, bool great, bool special, bool known);
bool curse_armor(player_type *player_ptr);
bool curse_weapon_object(player_type *player_ptr, bool force, object_type *o_ptr);
void brand_bolts(player_type *player_ptr);
bool perilous_secrets(player_type *player_ptr);

/*
 * Bit flags for the "enchant()" function
 */
#define ENCH_TOHIT      0x01 /*!< 装備強化処理: 命中強化 / Enchant to hit */
#define ENCH_TODAM      0x02 /*!< 装備強化処理: ダメージ強化 / Enchant to damage */
#define ENCH_TOAC       0x04 /*!< 装備強化処理: AC強化 / Enchant to AC */
#define ENCH_FORCE      0x08 /*!< 装備強化処理: 無条件に成功させる / Force enchantment */
bool enchant_equipment(player_type *player_ptr, object_type *o_ptr, int n, int eflag);
bool enchant_spell(player_type *player_ptr, HIT_PROB num_hit, HIT_POINT num_dam, ARMOUR_CLASS num_ac);
void brand_weapon(player_type *player_ptr, int brand_type);
