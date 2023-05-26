#include "mspell/summon-checker.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-util.h"
#include "player-base/player-race.h"
#include "spell/summon-types.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"

/*!
 * @brief 指定されたモンスター種族がsummon_specific_typeで指定された召喚条件に合うかどうかを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 召喚条件が一致するならtrue
 * @details
 */
bool check_summon_specific(PlayerType *player_ptr, MonsterRaceId summoner_idx, MonsterRaceId r_idx)
{
    const auto &monrace = monraces_info[r_idx];
    switch (summon_specific_type) {
    case SUMMON_ANT:
        return monrace.d_char == 'a';
    case SUMMON_SPIDER:
        return monrace.d_char == 'S';
    case SUMMON_HOUND:
        return (monrace.d_char == 'C') || (monrace.d_char == 'Z');
    case SUMMON_HYDRA:
        return monrace.d_char == 'M';
    case SUMMON_ANGEL:
        return (monrace.d_char == 'A') && ((monrace.kind_flags.has(MonsterKindType::EVIL)) || (monrace.kind_flags.has(MonsterKindType::GOOD)));
    case SUMMON_DEMON:
        return monrace.kind_flags.has(MonsterKindType::DEMON);
    case SUMMON_UNDEAD:
        return monrace.kind_flags.has(MonsterKindType::UNDEAD);
    case SUMMON_DRAGON:
        return monrace.kind_flags.has(MonsterKindType::DRAGON);
    case SUMMON_HI_UNDEAD:
        return (monrace.d_char == 'L') || (monrace.d_char == 'V') || (monrace.d_char == 'W');
    case SUMMON_HI_DRAGON:
        return monrace.d_char == 'D';
    case SUMMON_HI_DEMON:
        return ((monrace.d_char == 'U') || (monrace.d_char == 'H') || (monrace.d_char == 'B')) && (monrace.kind_flags.has(MonsterKindType::DEMON));
    case SUMMON_AMBERITES:
        return monrace.kind_flags.has(MonsterKindType::AMBERITE);
    case SUMMON_UNIQUE:
        return monrace.kind_flags.has(MonsterKindType::UNIQUE);
    case SUMMON_MOLD:
        return monrace.d_char == 'm';
    case SUMMON_BAT:
        return monrace.d_char == 'b';
    case SUMMON_QUYLTHULG:
        return monrace.d_char == 'Q';
    case SUMMON_COIN_MIMIC:
        return monrace.d_char == '$';
    case SUMMON_MIMIC:
        return (monrace.d_char == '!') || (monrace.d_char == '?') || (monrace.d_char == '=') || (monrace.d_char == '$') || (monrace.d_char == '|');
    case SUMMON_GOLEM:
        return (monrace.d_char == 'g');
    case SUMMON_CYBER:
        return (monrace.d_char == 'U') && monrace.ability_flags.has(MonsterAbilityType::ROCKET);
    case SUMMON_KIN: {
        auto summon_kin_type = MonsterRace(summoner_idx).is_valid() ? monraces_info[summoner_idx].d_char : PlayerRace(player_ptr).get_summon_symbol();
        return (monrace.d_char == summon_kin_type) && (r_idx != MonsterRaceId::HAGURE);
    }
    case SUMMON_DAWN:
        return r_idx == MonsterRaceId::DAWN;
    case SUMMON_ANIMAL:
        return monrace.kind_flags.has(MonsterKindType::ANIMAL);
    case SUMMON_ANIMAL_RANGER: {
        auto is_match = monrace.kind_flags.has(MonsterKindType::ANIMAL);
        is_match &= angband_strchr("abcflqrwBCHIJKMRS", monrace.d_char) != nullptr;
        is_match &= monrace.kind_flags.has_not(MonsterKindType::DRAGON);
        is_match &= monrace.kind_flags.has_not(MonsterKindType::EVIL);
        is_match &= monrace.kind_flags.has_not(MonsterKindType::UNDEAD);
        is_match &= monrace.kind_flags.has_not(MonsterKindType::DEMON);
        is_match &= none_bits(monrace.flags2, RF2_MULTIPLY);
        is_match &= monrace.ability_flags.none();
        return is_match;
    }
    case SUMMON_SMALL_MOAI:
        return r_idx == MonsterRaceId::SMALL_MOAI;
    case SUMMON_PYRAMID:
        return one_in_(16) ? monrace.d_char == 'z' : r_idx == MonsterRaceId::SCARAB;
    case SUMMON_PHANTOM:
        return (r_idx == MonsterRaceId::PHANTOM_B) || (r_idx == MonsterRaceId::PHANTOM_W);
    case SUMMON_BLUE_HORROR:
        return r_idx == MonsterRaceId::BLUE_HORROR;
    case SUMMON_TOTEM_MOAI:
        return r_idx == MonsterRaceId::TOTEM_MOAI;
    case SUMMON_LIVING:
        return monrace.has_living_flag();
    case SUMMON_HI_DRAGON_LIVING:
        return (monrace.d_char == 'D') && monrace.has_living_flag();
    case SUMMON_ELEMENTAL:
        return monrace.d_char == 'E';
    case SUMMON_VORTEX:
        return monrace.d_char == 'v';
    case SUMMON_HYBRID:
        return monrace.d_char == 'H';
    case SUMMON_BIRD:
        return monrace.d_char == 'B';
    case SUMMON_KAMIKAZE:
        return monrace.is_explodable();
    case SUMMON_KAMIKAZE_LIVING: {
        return monrace.is_explodable() && monrace.has_living_flag();
    case SUMMON_MANES:
        return r_idx == MonsterRaceId::MANES;
    case SUMMON_LOUSE:
        return r_idx == MonsterRaceId::LOUSE;
    case SUMMON_GUARDIANS:
        return any_bits(monrace.flags7, RF7_GUARDIAN);
    case SUMMON_KNIGHTS: {
        auto is_match = r_idx == MonsterRaceId::NOV_PALADIN;
        is_match |= r_idx == MonsterRaceId::NOV_PALADIN_G;
        is_match |= r_idx == MonsterRaceId::PALADIN;
        is_match |= r_idx == MonsterRaceId::W_KNIGHT;
        is_match |= r_idx == MonsterRaceId::ULTRA_PALADIN;
        is_match |= r_idx == MonsterRaceId::KNI_TEMPLAR;
        return is_match;
    }
    case SUMMON_EAGLES: {
        auto is_match = monrace.d_char == 'B';
        is_match &= monrace.wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN);
        is_match &= monrace.wilderness_flags.has(MonsterWildernessType::WILD_ONLY);
        return is_match;
    }
    case SUMMON_PIRANHAS:
        return r_idx == MonsterRaceId::PIRANHA;
    case SUMMON_ARMAGE_GOOD:
        return (monrace.d_char == 'A') && (monrace.kind_flags.has(MonsterKindType::GOOD));
    case SUMMON_ARMAGE_EVIL:
        return (monrace.kind_flags.has(MonsterKindType::DEMON)) || ((monrace.d_char == 'A') && (monrace.kind_flags.has(MonsterKindType::EVIL)));
    case SUMMON_APOCRYPHA_FOLLOWERS:
        return (r_idx == MonsterRaceId::FOLLOWER_WARRIOR) || (r_idx == MonsterRaceId::FOLLOWER_MAGE);
    case SUMMON_APOCRYPHA_DRAGONS:
        return (monrace.d_char == 'D') && (monrace.level >= 60) && (r_idx != MonsterRaceId::WYRM_COLOURS) && (r_idx != MonsterRaceId::ALDUIN);
    case SUMMON_VESPOID:
        return r_idx == MonsterRaceId::VESPOID;
    case SUMMON_ANTI_TIGERS: {
        auto is_match = one_in_(32) ? (monrace.d_char == 'P') : false;
        is_match |= one_in_(48) ? (monrace.d_char == 'd') : false;
        is_match |= one_in_(16) ? (monrace.d_char == 'l') : false;
        is_match |= (r_idx == MonsterRaceId::STAR_VAMPIRE) || (r_idx == MonsterRaceId::SWALLOW) || (r_idx == MonsterRaceId::HAWK);
        is_match |= (r_idx == MonsterRaceId::LION) || (r_idx == MonsterRaceId::BUFFALO) || (r_idx == MonsterRaceId::FIGHTER) || (r_idx == MonsterRaceId::GOLDEN_EAGLE);
        is_match |= (r_idx == MonsterRaceId::SHALLOW_PUDDLE) || (r_idx == MonsterRaceId::DEEP_PUDDLE) || (r_idx == MonsterRaceId::SKY_WHALE);
        return is_match;
    }
    default:
        return false;
    }
    }
}
