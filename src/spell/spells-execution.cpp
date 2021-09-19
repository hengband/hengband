#include "spell/spells-execution.h"
#include "realm/realm-names-table.h"
#include "realm/realm-arcane.h"
#include "realm/realm-chaos.h"
#include "realm/realm-craft.h"
#include "realm/realm-crusade.h"
#include "realm/realm-demon.h"
#include "realm/realm-death.h"
#include "realm/realm-hex.h"
#include "realm/realm-hissatsu.h"
#include "realm/realm-life.h"
#include "realm/realm-nature.h"
#include "realm/realm-song.h"
#include "realm/realm-sorcery.h"
#include "realm/realm-trump.h"
#include "system/player-type-definition.h"

/*!
 * @brief 魔法処理のメインルーチン
 * @param realm 魔法領域のID
 * @param spell 各領域の魔法ID
 * @param mode 求める処理
 * @return 各領域魔法に各種テキストを求めた場合は文字列参照ポインタ、そうでない場合はnullptrを返す。
 */
concptr exe_spell(player_type *player_ptr, int16_t realm, SPELL_IDX spell, spell_type mode)
{
	switch (realm)
	{
	case REALM_LIFE:     return do_life_spell(player_ptr, spell, mode);
	case REALM_SORCERY:  return do_sorcery_spell(player_ptr, spell, mode);
	case REALM_NATURE:   return do_nature_spell(player_ptr, spell, mode);
	case REALM_CHAOS:    return do_chaos_spell(player_ptr, spell, mode);
	case REALM_DEATH:    return do_death_spell(player_ptr, spell, mode);
	case REALM_TRUMP:    return do_trump_spell(player_ptr, spell, mode);
	case REALM_ARCANE:   return do_arcane_spell(player_ptr, spell, mode);
	case REALM_CRAFT:    return do_craft_spell(player_ptr, spell, mode);
	case REALM_DAEMON:   return do_daemon_spell(player_ptr, spell, mode);
	case REALM_CRUSADE:  return do_crusade_spell(player_ptr, spell, mode);
	case REALM_MUSIC:    return do_music_spell(player_ptr, spell, mode);
	case REALM_HISSATSU: return do_hissatsu_spell(player_ptr, spell, mode);
	case REALM_HEX:      return do_hex_spell(player_ptr, i2enum<spell_hex_type>(spell), mode);
	}

	return nullptr;
}
