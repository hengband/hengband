#include "mspell/summon-checker.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
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
bool check_summon_specific(PlayerType *player_ptr, MonsterRaceId summoner_idx, MonsterRaceId r_idx, summon_type type)
{
    const auto &monrace = monraces_info[r_idx];
    switch (type) {
    case SUMMON_ANT:
        return monrace.cc_def.character == 'a';
    case SUMMON_SPIDER:
        return monrace.cc_def.character == 'S';
    case SUMMON_HOUND:
        return (monrace.cc_def.character == 'C') || (monrace.cc_def.character == 'Z');
    case SUMMON_HYDRA:
        return monrace.cc_def.character == 'M';
    case SUMMON_ANGEL:
        return (monrace.cc_def.character == 'A') && ((monrace.kind_flags.has(MonsterKindType::EVIL)) || (monrace.kind_flags.has(MonsterKindType::GOOD)));
    case SUMMON_DEMON:
        return monrace.kind_flags.has(MonsterKindType::DEMON);
    case SUMMON_UNDEAD:
        return monrace.kind_flags.has(MonsterKindType::UNDEAD);
    case SUMMON_DRAGON:
        return monrace.kind_flags.has(MonsterKindType::DRAGON);
    case SUMMON_HI_UNDEAD:
        return (monrace.cc_def.character == 'L') || (monrace.cc_def.character == 'V') || (monrace.cc_def.character == 'W');
    case SUMMON_HI_DRAGON:
        return monrace.cc_def.character == 'D';
    case SUMMON_HI_DEMON:
        return ((monrace.cc_def.character == 'U') || (monrace.cc_def.character == 'H') || (monrace.cc_def.character == 'B')) && (monrace.kind_flags.has(MonsterKindType::DEMON));
    case SUMMON_AMBERITES:
        return monrace.kind_flags.has(MonsterKindType::AMBERITE);
    case SUMMON_UNIQUE:
        return monrace.kind_flags.has(MonsterKindType::UNIQUE);
    case SUMMON_MOLD:
        return monrace.cc_def.character == 'm';
    case SUMMON_BAT:
        return monrace.cc_def.character == 'b';
    case SUMMON_QUYLTHULG:
        return monrace.cc_def.character == 'Q';
    case SUMMON_COIN_MIMIC:
        return monrace.cc_def.character == '$';
    case SUMMON_MIMIC:
        return (monrace.cc_def.character == '!') || (monrace.cc_def.character == '?') || (monrace.cc_def.character == '=') || (monrace.cc_def.character == '$') || (monrace.cc_def.character == '|');
    case SUMMON_GOLEM:
        return (monrace.cc_def.character == 'g');
    case SUMMON_CYBER:
        return (monrace.cc_def.character == 'U') && monrace.ability_flags.has(MonsterAbilityType::ROCKET);
    case SUMMON_KIN: {
        auto summon_kin_type = MonsterRace(summoner_idx).is_valid() ? monraces_info[summoner_idx].cc_def.character : PlayerRace(player_ptr).get_summon_symbol();
        return (monrace.cc_def.character == summon_kin_type) && (r_idx != MonsterRaceId::HAGURE);
    }
    case SUMMON_DAWN:
        return r_idx == MonsterRaceId::DAWN;
    case SUMMON_ANIMAL:
        return monrace.kind_flags.has(MonsterKindType::ANIMAL);
    case SUMMON_ANIMAL_RANGER: {
        auto is_match = monrace.kind_flags.has(MonsterKindType::ANIMAL);
        is_match &= angband_strchr("abcflqrwBCHIJKMRS", monrace.cc_def.character) != nullptr;
        is_match &= monrace.kind_flags.has_not(MonsterKindType::DRAGON);
        is_match &= monrace.kind_flags.has_not(MonsterKindType::EVIL);
        is_match &= monrace.kind_flags.has_not(MonsterKindType::UNDEAD);
        is_match &= monrace.kind_flags.has_not(MonsterKindType::DEMON);
        is_match &= monrace.misc_flags.has_not(MonsterMiscType::MULTIPLY);
        is_match &= monrace.ability_flags.none();
        return is_match;
    }
    case SUMMON_SMALL_MOAI:
        return r_idx == MonsterRaceId::SMALL_MOAI;
    case SUMMON_PYRAMID:
        return one_in_(16) ? monrace.cc_def.character == 'z' : r_idx == MonsterRaceId::SCARAB;
    case SUMMON_PHANTOM:
        return (r_idx == MonsterRaceId::PHANTOM_B) || (r_idx == MonsterRaceId::PHANTOM_W);
    case SUMMON_BLUE_HORROR:
        return r_idx == MonsterRaceId::BLUE_HORROR;
    case SUMMON_TOTEM_MOAI:
        return r_idx == MonsterRaceId::TOTEM_MOAI;
    case SUMMON_LIVING:
        return monrace.has_living_flag();
    case SUMMON_HI_DRAGON_LIVING:
        return (monrace.cc_def.character == 'D') && monrace.has_living_flag();
    case SUMMON_ELEMENTAL:
        return monrace.cc_def.character == 'E';
    case SUMMON_VORTEX:
        return monrace.cc_def.character == 'v';
    case SUMMON_HYBRID:
        return monrace.cc_def.character == 'H';
    case SUMMON_BIRD:
        return monrace.cc_def.character == 'B';
    case SUMMON_KAMIKAZE:
        return monrace.is_explodable();
    case SUMMON_KAMIKAZE_LIVING: {
        return monrace.is_explodable() && monrace.has_living_flag();
    case SUMMON_MANES:
        return r_idx == MonsterRaceId::MANES;
    case SUMMON_LOUSE:
        return r_idx == MonsterRaceId::LOUSE;
    case SUMMON_GUARDIANS:
        return monrace.misc_flags.has(MonsterMiscType::GUARDIAN);
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
        auto is_match = monrace.cc_def.character == 'B';
        is_match &= monrace.wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN);
        is_match &= monrace.wilderness_flags.has(MonsterWildernessType::WILD_ONLY);
        return is_match;
    }
    case SUMMON_PIRANHAS:
        return r_idx == MonsterRaceId::PIRANHA;
    case SUMMON_ARMAGE_GOOD:
        return (monrace.cc_def.character == 'A') && (monrace.kind_flags.has(MonsterKindType::GOOD));
    case SUMMON_ARMAGE_EVIL:
        return (monrace.kind_flags.has(MonsterKindType::DEMON)) || ((monrace.cc_def.character == 'A') && (monrace.kind_flags.has(MonsterKindType::EVIL)));
    case SUMMON_APOCRYPHA_FOLLOWERS:
        return (r_idx == MonsterRaceId::FOLLOWER_WARRIOR) || (r_idx == MonsterRaceId::FOLLOWER_MAGE);
    case SUMMON_APOCRYPHA_DRAGONS:
        return (monrace.cc_def.character == 'D') && (monrace.level >= 60) && (r_idx != MonsterRaceId::WYRM_COLOURS) && (r_idx != MonsterRaceId::ALDUIN);
    case SUMMON_VESPOID:
        return r_idx == MonsterRaceId::VESPOID;
    case SUMMON_ANTI_TIGERS: {
        auto is_match = one_in_(32) ? (monrace.cc_def.character == 'P') : false;
        is_match |= one_in_(48) ? (monrace.cc_def.character == 'd') : false;
        is_match |= one_in_(16) ? (monrace.cc_def.character == 'l') : false;
        is_match |= (r_idx == MonsterRaceId::STAR_VAMPIRE) || (r_idx == MonsterRaceId::SWALLOW) || (r_idx == MonsterRaceId::HAWK);
        is_match |= (r_idx == MonsterRaceId::LION) || (r_idx == MonsterRaceId::BUFFALO) || (r_idx == MonsterRaceId::FIGHTER) || (r_idx == MonsterRaceId::GOLDEN_EAGLE);
        is_match |= (r_idx == MonsterRaceId::SHALLOW_PUDDLE) || (r_idx == MonsterRaceId::DEEP_PUDDLE) || (r_idx == MonsterRaceId::SKY_WHALE);
        return is_match;
    }
    case SUMMON_DEAD_UNIQUE: {
        return monrace.kind_flags.has(MonsterKindType::UNIQUE) && monrace.max_num == 0;
    }
    default:
        return false;
    }
    }
}
