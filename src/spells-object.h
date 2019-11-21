#pragma once

extern bool create_ammo(player_type *creature_ptr);
extern bool import_magic_device(player_type *creature_ptr);
extern void amusement(POSITION y1, POSITION x1, int num, bool known);
extern void acquirement(POSITION y1, POSITION x1, int num, bool great, bool special, bool known);
extern void acquire_chaos_weapon(player_type *creature_ptr);
extern bool curse_armor(player_type *owner_ptr);
extern bool curse_weapon_object(player_type *creature_ptr, bool force, object_type *o_ptr);
extern bool curse_weapon(bool force, int slot);
extern bool rustproof(void);
extern bool brand_bolts(void);
extern bool perilous_secrets(player_type *user_ptr);
extern void get_bloody_moon_flags(object_type *o_ptr);
extern void phlogiston(player_type *caster_ptr);
extern bool bless_weapon(void);
extern bool pulish_shield(void);

/*
 * Bit flags for the "enchant()" function
 */
#define ENCH_TOHIT      0x01 /*!< 装備強化処理: 命中強化 / Enchant to hit */
#define ENCH_TODAM      0x02 /*!< 装備強化処理: ダメージ強化 / Enchant to damage */
#define ENCH_TOAC       0x04 /*!< 装備強化処理: AC強化 / Enchant to AC */
#define ENCH_FORCE      0x08 /*!< 装備強化処理: 無条件に成功させる / Force enchantment */
extern bool enchant(object_type *o_ptr, int n, int eflag);
extern bool enchant_spell(HIT_PROB num_hit, HIT_POINT num_dam, ARMOUR_CLASS num_ac);
extern void brand_weapon(player_type *caster_ptr, int brand_type);
