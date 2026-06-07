#include "autopick/autopick-entry.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-keys-table.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-ego.h"
#include "object-hook/hook-weapon.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "util/finalizer.h"
#include "util/string-processor.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <tl/optional.hpp>

namespace {
std::string_view ltrim_sv_one(std::string_view sv)
{
    if (sv.starts_with(' ')) {
        sv.remove_prefix(1);
    }

    return sv;
}

std::string_view ltrim_sv(std::string_view sv)
{
    sv.remove_prefix(std::min(sv.find_first_not_of(' '), sv.size()));
    return sv;
}

tl::optional<std::string_view> match_key(std::string_view sv, std::string_view key)
{
    if (!sv.starts_with(key)) {
        return tl::nullopt;
    }

    sv.remove_prefix(key.size());
    return ltrim_sv_one(sv);
}

struct ParsedNumericValue {
public:
    uint8_t value;
    std::string_view remaining_sv;
};

tl::optional<ParsedNumericValue> parse_numeric_value(std::string_view sv)
{
    const auto original_sv = ltrim_sv(sv);
    auto current_sv = original_sv;
    uint8_t parsed_value = 0;
    while (!current_sv.empty() && is_numeric(current_sv.front())) {
        parsed_value = 10 * parsed_value + (current_sv.front() - '0');
        current_sv.remove_prefix(1);
    }

    const auto num_digits = original_sv.length() - current_sv.length();
    if ((num_digits == 1) || (num_digits == 2)) {
        return ParsedNumericValue{ parsed_value, current_sv };
    }

    return tl::nullopt;
}

#ifdef JP
constexpr std::string_view kanji_colon = "：";
#endif
}

/*!
 * @brief A function to create new entry
 */
bool autopick_new_entry(autopick_type *entry, std::string_view line_input, bool allow_default)
{
    if ((line_input.length() > 1) && (line_input[1] == ':')) {
        switch (line_input[0]) {
        case '?':
        case '%':
        case 'A':
        case 'P':
        case 'C':
            return false;
        default:
            break;
        }
    }

    entry->flags[0] = 0;
    entry->flags[1] = 0;
    entry->dice = 0;
    entry->bonus = 0;

    EnumClassFlagGroup<AutopickMethod> act = { AutopickMethod::AUTOPICK, AutopickMethod::DISPLAY };
    std::string_view line = line_input;
    while (!line.empty()) {
        if (act.has(AutopickMethod::AUTOPICK)) {
            const auto c = line.front();
            if (c == '!' || c == '~' || c == ';') {
                act.reset(AutopickMethod::AUTOPICK);
                if (c == '!') {
                    act.set(AutopickMethod::AUTODESTROY);
                } else if (c == '~') {
                    act.set(AutopickMethod::NOT_AUTOPICK);
                } else {
                    act.set(AutopickMethod::QUERY_AUTOPICK);
                }

                line.remove_prefix(1);
                continue;
            }
        }

        if (act.has(AutopickMethod::DISPLAY) && (line.front() == '(')) {
            act.reset(AutopickMethod::DISPLAY);
            line.remove_prefix(1);
            continue;
        }

        break;
    }

    std::string inscription;
    std::stringstream ss;
    while (!line.empty()) {
        auto c = line.front();
        line.remove_prefix(1);
#ifdef JP
        if (iskanji(c) && !line.empty()) {
            ss << c << line.front();
            line.remove_prefix(1);
            continue;
        }
#endif
        if (c == '#') {
            inscription = line;
            break;
        }

        if (isupper(c)) {
            c = (char)tolower(c);
        }

        ss << c;
    }

    const auto buf = ss.str();
    if (buf.empty() && (!allow_default || !inscription.empty())) {
        return false;
    }

    std::string_view sv = buf;
    std::string_view old_sv;
    do {
        sv = ltrim_sv(sv);
        if (sv.empty()) {
            break;
        }

        old_sv = sv;
        static const std::vector<std::pair<std::string_view, int>> adjectives = {
            { autopick_keys.at(AutopickKey::ALL), FLG_ALL },
            { autopick_keys.at(AutopickKey::COLLECTING), FLG_COLLECTING },
            { autopick_keys.at(AutopickKey::UNAWARE), FLG_UNAWARE },
            { autopick_keys.at(AutopickKey::UNIDENTIFIED), FLG_UNIDENTIFIED },
            { autopick_keys.at(AutopickKey::IDENTIFIED), FLG_IDENTIFIED },
            { autopick_keys.at(AutopickKey::STAR_IDENTIFIED), FLG_STAR_IDENTIFIED },
            { autopick_keys.at(AutopickKey::BOOSTED), FLG_BOOSTED },
            { autopick_keys.at(AutopickKey::WORTHLESS), FLG_WORTHLESS },
            { autopick_keys.at(AutopickKey::EGO), FLG_EGO },
            { autopick_keys.at(AutopickKey::GOOD), FLG_GOOD },
            { autopick_keys.at(AutopickKey::NAMELESS), FLG_NAMELESS },
            { autopick_keys.at(AutopickKey::AVERAGE), FLG_AVERAGE },
            { autopick_keys.at(AutopickKey::RARE), FLG_RARE },
            { autopick_keys.at(AutopickKey::COMMON), FLG_COMMON },
            { autopick_keys.at(AutopickKey::WANTED), FLG_WANTED },
            { autopick_keys.at(AutopickKey::UNIQUE), FLG_UNIQUE },
            { autopick_keys.at(AutopickKey::HUMAN), FLG_HUMAN },
            { autopick_keys.at(AutopickKey::UNREADABLE), FLG_UNREADABLE },
            { autopick_keys.at(AutopickKey::REALM1), FLG_REALM1 },
            { autopick_keys.at(AutopickKey::REALM2), FLG_REALM2 },
            { autopick_keys.at(AutopickKey::FIRST), FLG_FIRST },
            { autopick_keys.at(AutopickKey::SECOND), FLG_SECOND },
            { autopick_keys.at(AutopickKey::THIRD), FLG_THIRD },
            { autopick_keys.at(AutopickKey::FOURTH), FLG_FOURTH },
        };
        for (const auto &[key, flag] : adjectives) {
            if (const auto sv_opt = match_key(sv, key); sv_opt) {
                entry->add(flag);
                sv = *sv_opt;
                if (sv.empty()) {
                    break;
                }
            }
        }

        if (sv.empty()) {
            break;
        }

        /*** Weapons whose dd*ds is more than nn ***/
        if (const auto sv_opt = match_key(sv, autopick_keys.at(AutopickKey::MORE_THAN)); sv_opt) {
            if (const auto parsed_val_opt = parse_numeric_value(*sv_opt)) {
                entry->dice = parsed_val_opt->value;
                std::string_view remaining_sv = parsed_val_opt->remaining_sv;
                if (const auto dice_key_sv_opt = match_key(remaining_sv, autopick_keys.at(AutopickKey::DICE)); dice_key_sv_opt) {
                    remaining_sv = *dice_key_sv_opt;
                }

                entry->add(FLG_MORE_DICE);
                sv = remaining_sv;
                if (sv.empty()) {
                    break;
                }
            }
        }

        /*** Items whose magical bonus is more than n ***/
        if (const auto sv_opt = match_key(sv, autopick_keys.at(AutopickKey::MORE_BONUS)); sv_opt) {
            if (const auto parsed_val_opt = parse_numeric_value(*sv_opt)) {
                entry->bonus = parsed_val_opt->value;
                std::string_view current_parse_sv = parsed_val_opt->remaining_sv;
#ifdef JP
                if (const auto bonus2_sv_opt = match_key(current_parse_sv, autopick_keys.at(AutopickKey::MORE_BONUS2)); bonus2_sv_opt) {
                    current_parse_sv = *bonus2_sv_opt;
                }
#else
                current_parse_sv = ltrim_sv_one(current_parse_sv);
#endif
                entry->add(FLG_MORE_BONUS);
                sv = current_parse_sv;
                if (sv.empty()) {
                    break;
                }
            }
        }
    } while (old_sv != sv);

    const auto sv_backup = sv;
    tl::optional<int> previous_flag;
    if (const auto sv_opt = match_key(sv, autopick_keys.at(AutopickKey::ARTIFACT)); sv_opt) {
        entry->add(FLG_ARTIFACT);
        previous_flag = FLG_ARTIFACT;
        sv = *sv_opt;
    }

    static const std::vector<std::pair<std::string_view, int>> nouns = {
        { autopick_keys.at(AutopickKey::ITEMS), FLG_ITEMS },
        { autopick_keys.at(AutopickKey::WEAPONS), FLG_WEAPONS },
        { autopick_keys.at(AutopickKey::FAVORITE_WEAPONS), FLG_FAVORITE_WEAPONS },
        { autopick_keys.at(AutopickKey::ARMORS), FLG_ARMORS },
        { autopick_keys.at(AutopickKey::MISSILES), FLG_MISSILES },
        { autopick_keys.at(AutopickKey::DEVICES), FLG_DEVICES },
        { autopick_keys.at(AutopickKey::LIGHTS), FLG_LIGHTS },
        { autopick_keys.at(AutopickKey::JUNKS), FLG_JUNKS },
        { autopick_keys.at(AutopickKey::CORPSES), FLG_CORPSES },
        { autopick_keys.at(AutopickKey::SPELLBOOKS), FLG_SPELLBOOKS },
        { autopick_keys.at(AutopickKey::HAFTED), FLG_HAFTED },
        { autopick_keys.at(AutopickKey::SHIELDS), FLG_SHIELDS },
        { autopick_keys.at(AutopickKey::BOWS), FLG_BOWS },
        { autopick_keys.at(AutopickKey::RINGS), FLG_RINGS },
        { autopick_keys.at(AutopickKey::AMULETS), FLG_AMULETS },
        { autopick_keys.at(AutopickKey::SUITS), FLG_SUITS },
        { autopick_keys.at(AutopickKey::CLOAKS), FLG_CLOAKS },
        { autopick_keys.at(AutopickKey::HELMS), FLG_HELMS },
        { autopick_keys.at(AutopickKey::GLOVES), FLG_GLOVES },
        { autopick_keys.at(AutopickKey::BOOTS), FLG_BOOTS },
    };

    for (const auto &[key, flag] : nouns) {
        if (const auto sv_opt = match_key(sv, key); sv_opt) {
            entry->add(flag);
            previous_flag = flag;
            sv = *sv_opt;
            break;
        }
    }

    const auto finalizer = util::make_finalizer([&]() {
        entry->name = sv;
        entry->action = act;
        entry->insc = std::move(inscription);
    });
    if (sv.starts_with(':')) {
        sv.remove_prefix(1);
        sv = ltrim_sv(sv);
        return true;
    }

#ifdef JP
    if (sv.starts_with(kanji_colon)) {
        sv.remove_prefix(kanji_colon.length());
        sv = ltrim_sv(sv);
        return true;
    }
#endif

    if (!previous_flag) {
        if (sv.empty()) {
            entry->add(FLG_ITEMS);
            previous_flag = FLG_ITEMS;
        }

        return true;
    }

    if (sv.empty()) {
        return true;
    }

    entry->remove(*previous_flag);
    sv = sv_backup;
    return true;
}

/*!
 * @brief Get auto-picker entry from o_ptr.
 */
void autopick_entry_from_object(PlayerType *player_ptr, autopick_type *entry, const ItemEntity *o_ptr)
{
    /* Assume that object name is to be added */
    bool name = true;
    entry->name.clear();
    entry->insc = o_ptr->inscription.value_or("");
    entry->action = { AutopickMethod::AUTOPICK, AutopickMethod::DISPLAY };
    entry->flags[0] = entry->flags[1] = 0L;
    entry->dice = 0;

    // エゴ銘が邪魔かもしれないので、デフォルトで「^」は付けない.
    // We can always use the ^ mark in English.
    bool is_hat_added = _(false, true);
    if (!o_ptr->is_aware()) {
        entry->add(FLG_UNAWARE);
        is_hat_added = true;
    } else if (!o_ptr->is_known()) {
        if (o_ptr->has_not_identification_flag(IdentificationFlag::SENSE)) {
            entry->add(FLG_UNIDENTIFIED);
            is_hat_added = true;
        } else {
            switch (o_ptr->feeling) {
            case FEEL_AVERAGE:
            case FEEL_GOOD:
                entry->add(FLG_NAMELESS);
                is_hat_added = true;
                break;

            case FEEL_BROKEN:
            case FEEL_CURSED:
                entry->add(FLG_NAMELESS);
                entry->add(FLG_WORTHLESS);
                is_hat_added = true;
                break;

            case FEEL_TERRIBLE:
            case FEEL_WORTHLESS:
                entry->add(FLG_WORTHLESS);
                break;

            case FEEL_EXCELLENT:
                entry->add(FLG_EGO);
                break;

            case FEEL_UNCURSED:
                break;

            default:
                break;
            }
        }
    } else {
        if (o_ptr->is_ego()) {
            if (o_ptr->is_weapon_armour_ammo()) {
                auto &ego = o_ptr->get_ego();
#ifdef JP
                /* エゴ銘には「^」マークが使える */
                entry->name = "^";
                entry->name.append(ego.name);
#else
                /* We omit the basename and cannot use the ^ mark */
                entry->name = ego.name;
#endif
                name = false;
                if (!o_ptr->is_rare()) {
                    entry->add(FLG_COMMON);
                }
            }

            entry->add(FLG_EGO);
        } else if (o_ptr->is_fixed_or_random_artifact()) {
            entry->add(FLG_ARTIFACT);
        } else {
            if (o_ptr->is_equipment()) {
                entry->add(FLG_NAMELESS);
            }

            is_hat_added = true;
        }
    }

    if (o_ptr->is_melee_weapon()) {
        const auto &baseitem = o_ptr->get_baseitem();
        if (o_ptr->damage_dice != baseitem.damage_dice) {
            entry->add(FLG_BOOSTED);
        }
    }

    if (o_ptr->is_bounty()) {
        entry->remove(FLG_WORTHLESS);
        entry->add(FLG_WANTED);
    }

    const auto &bi_key = o_ptr->bi_key;
    const auto tval = bi_key.tval();
    if ((tval == ItemKindType::MONSTER_REMAINS) || (tval == ItemKindType::STATUE)) {
        if (o_ptr->get_monrace().kind_flags.has(MonsterKindType::UNIQUE)) {
            entry->add(FLG_UNIQUE);
        }
    }

    if (tval == ItemKindType::MONSTER_REMAINS) {
        if (o_ptr->get_monrace().is_human()) {
            entry->add(FLG_HUMAN);
        }
    }

    if (o_ptr->is_spell_book() && !check_book_realm(player_ptr, bi_key)) {
        entry->add(FLG_UNREADABLE);
        if (tval != ItemKindType::ARCANE_BOOK) {
            name = false;
        }
    }

    PlayerClass pc(player_ptr);
    auto realm_except_class = pc.equals(PlayerClassType::SORCERER) || pc.equals(PlayerClassType::RED_MAGE);

    PlayerRealm pr(player_ptr);
    if ((pr.realm1().get_book() == tval) && !realm_except_class) {
        entry->add(FLG_REALM1);
        name = false;
    }

    if ((pr.realm2().get_book() == tval) && !realm_except_class) {
        entry->add(FLG_REALM2);
        name = false;
    }

    const auto sval = bi_key.sval();
    if (o_ptr->is_spell_book() && (sval == 0)) {
        entry->add(FLG_FIRST);
    }
    if (o_ptr->is_spell_book() && (sval == 1)) {
        entry->add(FLG_SECOND);
    }
    if (o_ptr->is_spell_book() && (sval == 2)) {
        entry->add(FLG_THIRD);
    }
    if (o_ptr->is_spell_book() && (sval == 3)) {
        entry->add(FLG_FOURTH);
    }

    if (o_ptr->is_ammo()) {
        entry->add(FLG_MISSILES);
    } else if (tval == ItemKindType::SCROLL || o_ptr->is_wand_staff() || o_ptr->is_wand_rod()) {
        entry->add(FLG_DEVICES);
    } else if (tval == ItemKindType::LITE) {
        entry->add(FLG_LIGHTS);
    } else if (o_ptr->is_junk()) {
        entry->add(FLG_JUNKS);
    } else if (tval == ItemKindType::MONSTER_REMAINS) {
        entry->add(FLG_CORPSES);
    } else if (o_ptr->is_spell_book()) {
        entry->add(FLG_SPELLBOOKS);
    } else if (o_ptr->is_melee_weapon()) {
        entry->add(FLG_WEAPONS);
    } else if (tval == ItemKindType::SHIELD) {
        entry->add(FLG_SHIELDS);
    } else if (tval == ItemKindType::BOW) {
        entry->add(FLG_BOWS);
    } else if (tval == ItemKindType::RING) {
        entry->add(FLG_RINGS);
    } else if (tval == ItemKindType::AMULET) {
        entry->add(FLG_AMULETS);
    } else if (o_ptr->is_armour()) {
        entry->add(FLG_SUITS);
    } else if (tval == ItemKindType::CLOAK) {
        entry->add(FLG_CLOAKS);
    } else if (tval == ItemKindType::HELM) {
        entry->add(FLG_HELMS);
    } else if (tval == ItemKindType::GLOVES) {
        entry->add(FLG_GLOVES);
    } else if (tval == ItemKindType::BOOTS) {
        entry->add(FLG_BOOTS);
    }

    if (!name) {
        entry->name = str_tolower(entry->name);
        return;
    }

    const auto item_name = describe_flavor(player_ptr, *o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL | OD_NAME_ONLY));

    /*
     * If necessary, add a '^' which indicates the
     * beginning of line.
     */
    entry->name = str_tolower(std::string(is_hat_added ? "^" : "").append(item_name));
}

static std::string shape_autopick_key(std::string_view key)
{
#ifdef JP
    return std::string(key);
#else
    std::stringstream ss;
    ss << key << ' ';
    return ss.str();
#endif
}

/*!
 * @brief Reconstruct preference line from entry
 */
std::string autopick_line_from_entry(const autopick_type &entry)
{
    std::stringstream ss;
    if (entry.action.has_not(AutopickMethod::DISPLAY)) {
        ss << '(';
    }

    if (entry.action.has(AutopickMethod::QUERY_AUTOPICK)) {
        ss << ';';
    }

    if (entry.action.has(AutopickMethod::AUTODESTROY)) {
        ss << '!';
    }

    if (entry.action.has(AutopickMethod::NOT_AUTOPICK)) {
        ss << '~';
    }

    if (entry.has(FLG_ALL)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::ALL));
    }
    if (entry.has(FLG_COLLECTING)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::COLLECTING));
    }
    if (entry.has(FLG_UNAWARE)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::UNAWARE));
    }
    if (entry.has(FLG_UNIDENTIFIED)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::UNIDENTIFIED));
    }
    if (entry.has(FLG_IDENTIFIED)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::IDENTIFIED));
    }
    if (entry.has(FLG_STAR_IDENTIFIED)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::STAR_IDENTIFIED));
    }
    if (entry.has(FLG_BOOSTED)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::BOOSTED));
    }

    if (entry.has(FLG_MORE_DICE)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::MORE_THAN));
        ss << entry.dice;
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::DICE));
    }

    if (entry.has(FLG_MORE_BONUS)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::MORE_BONUS));
        ss << entry.bonus;
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::MORE_BONUS2));
    }

    if (entry.has(FLG_UNREADABLE)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::UNREADABLE));
    }

    if (entry.has(FLG_REALM1)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::REALM1));
    }

    if (entry.has(FLG_REALM2)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::REALM2));
    }

    if (entry.has(FLG_FIRST)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::FIRST));
    }

    if (entry.has(FLG_SECOND)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::SECOND));
    }

    if (entry.has(FLG_THIRD)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::THIRD));
    }

    if (entry.has(FLG_FOURTH)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::FOURTH));
    }

    if (entry.has(FLG_WANTED)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::WANTED));
    }

    if (entry.has(FLG_UNIQUE)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::UNIQUE));
    }

    if (entry.has(FLG_HUMAN)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::HUMAN));
    }

    if (entry.has(FLG_WORTHLESS)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::WORTHLESS));
    }

    if (entry.has(FLG_GOOD)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::GOOD));
    }

    if (entry.has(FLG_NAMELESS)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::NAMELESS));
    }

    if (entry.has(FLG_AVERAGE)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::AVERAGE));
    }

    if (entry.has(FLG_RARE)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::RARE));
    }

    if (entry.has(FLG_COMMON)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::COMMON));
    }

    if (entry.has(FLG_EGO)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::EGO));
    }

    if (entry.has(FLG_ARTIFACT)) {
        ss << shape_autopick_key(autopick_keys.at(AutopickKey::ARTIFACT));
    }

    auto should_separate = true;
    if (entry.has(FLG_ITEMS)) {
        ss << autopick_keys.at(AutopickKey::ITEMS);
    } else if (entry.has(FLG_WEAPONS)) {
        ss << autopick_keys.at(AutopickKey::WEAPONS);
    } else if (entry.has(FLG_FAVORITE_WEAPONS)) {
        ss << autopick_keys.at(AutopickKey::FAVORITE_WEAPONS);
    } else if (entry.has(FLG_ARMORS)) {
        ss << autopick_keys.at(AutopickKey::ARMORS);
    } else if (entry.has(FLG_MISSILES)) {
        ss << autopick_keys.at(AutopickKey::MISSILES);
    } else if (entry.has(FLG_DEVICES)) {
        ss << autopick_keys.at(AutopickKey::DEVICES);
    } else if (entry.has(FLG_LIGHTS)) {
        ss << autopick_keys.at(AutopickKey::LIGHTS);
    } else if (entry.has(FLG_JUNKS)) {
        ss << autopick_keys.at(AutopickKey::JUNKS);
    } else if (entry.has(FLG_CORPSES)) {
        ss << autopick_keys.at(AutopickKey::CORPSES);
    } else if (entry.has(FLG_SPELLBOOKS)) {
        ss << autopick_keys.at(AutopickKey::SPELLBOOKS);
    } else if (entry.has(FLG_HAFTED)) {
        ss << autopick_keys.at(AutopickKey::HAFTED);
    } else if (entry.has(FLG_SHIELDS)) {
        ss << autopick_keys.at(AutopickKey::SHIELDS);
    } else if (entry.has(FLG_BOWS)) {
        ss << autopick_keys.at(AutopickKey::BOWS);
    } else if (entry.has(FLG_RINGS)) {
        ss << autopick_keys.at(AutopickKey::RINGS);
    } else if (entry.has(FLG_AMULETS)) {
        ss << autopick_keys.at(AutopickKey::AMULETS);
    } else if (entry.has(FLG_SUITS)) {
        ss << autopick_keys.at(AutopickKey::SUITS);
    } else if (entry.has(FLG_CLOAKS)) {
        ss << autopick_keys.at(AutopickKey::CLOAKS);
    } else if (entry.has(FLG_HELMS)) {
        ss << autopick_keys.at(AutopickKey::HELMS);
    } else if (entry.has(FLG_GLOVES)) {
        ss << autopick_keys.at(AutopickKey::GLOVES);
    } else if (entry.has(FLG_BOOTS)) {
        ss << autopick_keys.at(AutopickKey::BOOTS);
    } else if (!entry.has(FLG_ARTIFACT)) {
        should_separate = false;
    }

    if (!entry.name.empty()) {
        if (should_separate) {
            ss << ":";
        }

        ss << entry.name;
    }

    if (entry.insc.empty()) {
        return ss.str();
    }

    ss << '#' << entry.insc;
    return ss.str();
}

/*!
 * @brief Choose an item and get auto-picker entry from it.
 */
bool entry_from_choosed_object(PlayerType *player_ptr, autopick_type *entry)
{
    constexpr auto q = _("どのアイテムを登録しますか? ", "Enter which item? ");
    constexpr auto s = _("アイテムを持っていない。", "You have nothing to enter.");
    const auto &[item, _] = choose_item(player_ptr, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP);
    if (!item) {
        return false;
    }

    autopick_entry_from_object(player_ptr, entry, item.get());
    return true;
}
