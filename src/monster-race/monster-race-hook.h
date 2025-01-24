#pragma once

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"
#include <string>

enum class MonraceId : short;
enum class NestKind;
enum class PitKind;
class PitNestFilter {
public:
    ~PitNestFilter() = default;
    PitNestFilter(const PitNestFilter &) = delete;
    PitNestFilter(PitNestFilter &&) = delete;
    PitNestFilter &operator=(const PitNestFilter &) = delete;
    PitNestFilter &operator=(PitNestFilter &&) = delete;
    static PitNestFilter &get_instance();

    MonraceId monrace_id{}; //<! 通常pit/nest生成時のモンスターの構成条件ID.
    char monrace_symbol = '\0'; //!< 単一シンボルpit/nest生成時の指定シンボル.
    EnumClassFlagGroup<MonsterAbilityType> dragon_breaths{}; //!< ブレス属性に基づくドラゴンpit生成時条件マスク.

    std::string pit_subtype(PitKind type) const;
    std::string nest_subtype(NestKind type) const;

private:
    PitNestFilter() = default;

    static PitNestFilter instance;
};

class PlayerType;
void vault_prep_clone(PlayerType *player_ptr);
void vault_prep_dragon(PlayerType *player_ptr);
void vault_prep_symbol(PlayerType *player_ptr);

bool vault_aux_clone(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_symbol_e(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_symbol_g(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_dragon(PlayerType *player_ptr, MonraceId r_idx);
