#include "mspell/summon-checker.h"
#include "player-base/player-race.h"
#include "spell/summon-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

/*!
 * @brief 指定されたモンスター種族がsummon_specific_typeで指定された召喚条件に合うかどうかを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 召喚条件が一致するならtrue
 * @details
 */
bool check_summon_specific(PlayerType *player_ptr, MonraceId summoner_id, MonraceId summoned_id, summon_type type)
{
    const auto &monraces = MonraceList::get_instance();
    const auto &monrace = monraces.get_monrace(summoned_id);
    switch (type) {
    case SUMMON_ANT:
        return monrace.symbol_char_is_any_of("a");
    case SUMMON_SPIDER:
        return monrace.symbol_char_is_any_of("S");
    case SUMMON_HOUND:
        return monrace.symbol_char_is_any_of("CZ");
    case SUMMON_HYDRA:
        return monrace.symbol_char_is_any_of("M");
    case SUMMON_ANGEL:
        return monrace.symbol_char_is_any_of("A") && (monrace.kind_flags.has(MonsterKindType::EVIL) || monrace.kind_flags.has(MonsterKindType::GOOD));
    case SUMMON_DEMON:
        return monrace.kind_flags.has(MonsterKindType::DEMON);
    case SUMMON_UNDEAD:
        return monrace.kind_flags.has(MonsterKindType::UNDEAD);
    case SUMMON_DRAGON:
        return monrace.kind_flags.has(MonsterKindType::DRAGON);
    case SUMMON_HI_UNDEAD:
        return monrace.symbol_char_is_any_of("LVW");
    case SUMMON_HI_DRAGON:
        return monrace.symbol_char_is_any_of("D");
    case SUMMON_HI_DEMON:
        return monrace.symbol_char_is_any_of("UHB") && monrace.kind_flags.has(MonsterKindType::DEMON);
    case SUMMON_AMBERITES:
        return monrace.kind_flags.has(MonsterKindType::AMBERITE);
    case SUMMON_UNIQUE:
        return monrace.kind_flags.has(MonsterKindType::UNIQUE);
    case SUMMON_MOLD:
        return monrace.symbol_char_is_any_of("m");
    case SUMMON_BAT:
        return monrace.symbol_char_is_any_of("b");
    case SUMMON_QUYLTHULG:
        return monrace.symbol_char_is_any_of("Q");
    case SUMMON_COIN_MIMIC:
        return monrace.symbol_char_is_any_of("$");
    case SUMMON_MIMIC:
        return monrace.symbol_char_is_any_of("!?=$|");
    case SUMMON_GOLEM:
        return monrace.symbol_char_is_any_of("g");
    case SUMMON_CYBER:
        return monrace.symbol_char_is_any_of("U") && monrace.ability_flags.has(MonsterAbilityType::ROCKET);
    case SUMMON_KIN: {
        const auto is_monster = MonraceList::is_valid(summoner_id);
        const auto monster_kin_character = monraces.get_monrace(summoner_id).symbol_definition.character;
        const auto player_kin_character = PlayerRace(player_ptr).get_summon_symbol();
        const auto summon_kin_type = is_monster ? monster_kin_character : player_kin_character;
        return (monrace.symbol_definition.character == summon_kin_type) && (summoned_id != MonraceId::HAGURE);
    }
    case SUMMON_DAWN:
        return summoned_id == MonraceId::DAWN;
    case SUMMON_ANIMAL:
        return monrace.kind_flags.has(MonsterKindType::ANIMAL);
    case SUMMON_ANIMAL_RANGER: {
        auto is_match = monrace.kind_flags.has(MonsterKindType::ANIMAL);
        is_match &= monrace.symbol_char_is_any_of("abcflqrwBCHIJKMRS");
        is_match &= monrace.kind_flags.has_not(MonsterKindType::DRAGON);
        is_match &= monrace.kind_flags.has_not(MonsterKindType::EVIL);
        is_match &= monrace.kind_flags.has_not(MonsterKindType::UNDEAD);
        is_match &= monrace.kind_flags.has_not(MonsterKindType::DEMON);
        is_match &= monrace.misc_flags.has_not(MonsterMiscType::MULTIPLY);
        is_match &= monrace.ability_flags.none();
        return is_match;
    }
    case SUMMON_SMALL_MOAI:
        return summoned_id == MonraceId::SMALL_MOAI;
    case SUMMON_PYRAMID:
        return one_in_(16) ? monrace.symbol_char_is_any_of("z") : summoned_id == MonraceId::SCARAB;
    case SUMMON_PHANTOM:
        return (summoned_id == MonraceId::PHANTOM_B) || (summoned_id == MonraceId::PHANTOM_W);
    case SUMMON_BLUE_HORROR:
        return summoned_id == MonraceId::BLUE_HORROR;
    case SUMMON_TOTEM_MOAI:
        return summoned_id == MonraceId::TOTEM_MOAI;
    case SUMMON_LIVING:
        return monrace.has_living_flag();
    case SUMMON_HI_DRAGON_LIVING:
        return monrace.symbol_char_is_any_of("D") && monrace.has_living_flag();
    case SUMMON_ELEMENTAL:
        return monrace.symbol_char_is_any_of("E");
    case SUMMON_VORTEX:
        return monrace.symbol_char_is_any_of("v");
    case SUMMON_HYBRID:
        return monrace.symbol_char_is_any_of("H");
    case SUMMON_BIRD:
        return monrace.symbol_char_is_any_of("B");
    case SUMMON_KAMIKAZE:
        return monrace.is_explodable();
    case SUMMON_KAMIKAZE_LIVING: {
        return monrace.is_explodable() && monrace.has_living_flag();
    case SUMMON_MANES:
        return summoned_id == MonraceId::MANES;
    case SUMMON_LOUSE:
        return summoned_id == MonraceId::LOUSE;
    case SUMMON_GUARDIANS:
        return monrace.misc_flags.has(MonsterMiscType::GUARDIAN);
    case SUMMON_KNIGHTS: {
        auto is_match = summoned_id == MonraceId::NOV_PALADIN;
        is_match |= summoned_id == MonraceId::NOV_PALADIN_G;
        is_match |= summoned_id == MonraceId::PALADIN;
        is_match |= summoned_id == MonraceId::W_KNIGHT;
        is_match |= summoned_id == MonraceId::ULTRA_PALADIN;
        is_match |= summoned_id == MonraceId::KNI_TEMPLAR;
        return is_match;
    }
    case SUMMON_EAGLES: {
        auto is_match = monrace.symbol_char_is_any_of("B");
        is_match &= monrace.wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN);
        is_match &= monrace.wilderness_flags.has(MonsterWildernessType::WILD_ONLY);
        return is_match;
    }
    case SUMMON_PIRANHAS:
        return summoned_id == MonraceId::PIRANHA;
    case SUMMON_ARMAGE_GOOD:
        return monrace.symbol_char_is_any_of("A") && monrace.kind_flags.has(MonsterKindType::GOOD);
    case SUMMON_ARMAGE_EVIL:
        return monrace.kind_flags.has(MonsterKindType::DEMON) || (monrace.symbol_char_is_any_of("A") && monrace.kind_flags.has(MonsterKindType::EVIL));
    case SUMMON_APOCRYPHA_FOLLOWERS:
        return (summoned_id == MonraceId::FOLLOWER_WARRIOR) || (summoned_id == MonraceId::FOLLOWER_MAGE);
    case SUMMON_APOCRYPHA_DRAGONS:
        return monrace.symbol_char_is_any_of("D") && (monrace.level >= 60) && (summoned_id != MonraceId::WYRM_COLOURS) && (summoned_id != MonraceId::ALDUIN);
    case SUMMON_VESPOID:
        return summoned_id == MonraceId::VESPOID;
    case SUMMON_ANTI_TIGERS: {
        auto is_match = one_in_(32) ? monrace.symbol_char_is_any_of("P") : false;
        is_match |= one_in_(48) ? monrace.symbol_char_is_any_of("d") : false;
        is_match |= one_in_(16) ? monrace.symbol_char_is_any_of("l") : false;
        is_match |= (summoned_id == MonraceId::STAR_VAMPIRE) || (summoned_id == MonraceId::SWALLOW) || (summoned_id == MonraceId::HAWK);
        is_match |= (summoned_id == MonraceId::LION) || (summoned_id == MonraceId::BUFFALO) || (summoned_id == MonraceId::FIGHTER) || (summoned_id == MonraceId::GOLDEN_EAGLE);
        is_match |= (summoned_id == MonraceId::SHALLOW_PUDDLE) || (summoned_id == MonraceId::DEEP_PUDDLE) || (summoned_id == MonraceId::SKY_WHALE);
        return is_match;
    }
    case SUMMON_DEAD_UNIQUE: {
        return monrace.is_dead_unique();
    }
    default:
        return false;
    }
    }
}
