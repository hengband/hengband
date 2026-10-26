#pragma once

#include "system/angband.h"

#include "effect/attribute-types.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-data.h"
#include "mspell/mspell-util.h"
#include "system/player-type-definition.h"
#include <fmt/base.h>
#include <string_view>
#include <type_traits>

struct MonsterSpellResult;

/*!
 * @brief 呪いの呪文
 * @details メッセージの書式の引数は、{0} が唱えたモンスターの名前 (先頭を大文字にする)、{1} が対象のモンスターの名前
 */
class CurseData : public MSpellData {
public:
    using Format = fmt::format_string<std::string_view, std::string_view>;
    CurseData(Format msg1, Format msg2, Format msg3, const AttributeType &typ);
    template <typename FUNC>
    CurseData(FUNC msg_func, AttributeType typ)
        : MSpellData(msg_func, typ)
    {
        static_assert(std::is_invocable<decltype(msg_func), PlayerType *, MONSTER_IDX, MONSTER_IDX, int>::value);
    }
};

class PlayerType;
MonsterSpellResult spell_RF5_CAUSE(PlayerType *player_ptr, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
