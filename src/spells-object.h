#pragma once

extern bool create_ammo(void);
extern bool import_magic_device(void);
extern void amusement(POSITION y1, POSITION x1, int num, bool known);
extern void acquirement(POSITION y1, POSITION x1, int num, bool great, bool special, bool known);
extern void acquire_chaos_weapon(player_type *creature_ptr);
extern bool curse_armor(void);
extern bool curse_weapon_object(bool force, object_type *o_ptr);
extern bool curse_weapon(bool force, int slot);
extern bool rustproof(void);
extern bool brand_bolts(void);
