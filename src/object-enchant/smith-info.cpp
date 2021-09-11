#include "object-enchant/smith-info.h"
#include "object/object-flags.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

ISmithInfo::ISmithInfo(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption)
    : effect(effect)
    , name(name)
    , category(category)
    , need_essences(std::move(need_essences))
    , consumption(consumption)
{
}

TrFlags ISmithInfo::tr_flags() const
{
    return {};
}

std::optional<random_art_activation_type> ISmithInfo::activation_index() const
{
    return std::nullopt;
}

BasicSmithInfo::BasicSmithInfo(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption, TrFlags add_flags)
    : ISmithInfo(effect, name, category, std::move(need_essences), consumption)
    , add_flags(add_flags)
{
}

bool BasicSmithInfo::add_essence(player_type *, object_type *o_ptr, int) const
{
    o_ptr->xtra3 = static_cast<decltype(o_ptr->xtra3)>(effect);

    return true;
}

void BasicSmithInfo::erase_essence(object_type *o_ptr) const
{
    o_ptr->xtra3 = 0;
    auto flgs = object_flags(o_ptr);
    if (flgs.has_none_of(TR_PVAL_FLAG_MASK))
        o_ptr->pval = 0;
}

TrFlags BasicSmithInfo::tr_flags() const
{
    return this->add_flags;
}

ActivationSmithInfo::ActivationSmithInfo(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption, TrFlags add_flags, random_art_activation_type act_idx)
    : BasicSmithInfo(effect, name, category, std::move(need_essences), consumption, std::move(add_flags))
    , act_idx(act_idx)
{
}

TrFlags ActivationSmithInfo::tr_flags() const
{
    return BasicSmithInfo::tr_flags().set(TR_ACTIVATE);
}

std::optional<random_art_activation_type> ActivationSmithInfo::activation_index() const
{
    return this->act_idx;
}

EnchantWeaponSmithInfo::EnchantWeaponSmithInfo(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption)
    : ISmithInfo(effect, name, category, std::move(need_essences), consumption)
{
}

bool EnchantWeaponSmithInfo::add_essence(player_type *player_ptr, object_type *o_ptr, int) const
{
    const auto max_val = player_ptr->lev / 5 + 5;
    if ((o_ptr->to_h >= max_val) && (o_ptr->to_d >= max_val)) {
        return false;
    }

    o_ptr->to_h = static_cast<HIT_PROB>(std::min(o_ptr->to_h + 1, max_val));
    o_ptr->to_d = static_cast<HIT_POINT>(std::min(o_ptr->to_d + 1, max_val));

    return true;
}

EnchantArmourSmithInfo::EnchantArmourSmithInfo(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption)
    : ISmithInfo(effect, name, category, std::move(need_essences), consumption)
{
}

bool EnchantArmourSmithInfo::add_essence(player_type *player_ptr, object_type *o_ptr, int) const
{
    const auto max_val = player_ptr->lev / 5 + 5;
    if (o_ptr->to_a >= max_val) {
        return false;
    }

    o_ptr->to_a++;

    return true;
}

SustainSmithInfo::SustainSmithInfo(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption)
    : ISmithInfo(effect, name, category, std::move(need_essences), consumption)
{
}

bool SustainSmithInfo::add_essence(player_type *, object_type *o_ptr, int) const
{
    o_ptr->art_flags.set(TR_IGNORE_ACID);
    o_ptr->art_flags.set(TR_IGNORE_ELEC);
    o_ptr->art_flags.set(TR_IGNORE_FIRE);
    o_ptr->art_flags.set(TR_IGNORE_COLD);

    return true;
}

SlayingGlovesSmithInfo::SlayingGlovesSmithInfo(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption)
    : BasicSmithInfo(effect, name, category, std::move(need_essences), consumption, {})
{
}

bool SlayingGlovesSmithInfo::add_essence(player_type *player_ptr, object_type *o_ptr, int number) const
{
    BasicSmithInfo::add_essence(player_ptr, o_ptr, number);

    HIT_PROB get_to_h = ((number + 1) / 2 + randint0(number / 2 + 1));
    HIT_POINT get_to_d = ((number + 1) / 2 + randint0(number / 2 + 1));
    o_ptr->xtra4 = (get_to_h << 8) + get_to_d;
    o_ptr->to_h += get_to_h;
    o_ptr->to_d += get_to_d;

    return true;
}

void SlayingGlovesSmithInfo::erase_essence(object_type *o_ptr) const
{
    BasicSmithInfo::erase_essence(o_ptr);

    o_ptr->to_h -= (o_ptr->xtra4 >> 8);
    o_ptr->to_d -= (o_ptr->xtra4 & 0x000f);
    o_ptr->xtra4 = 0;
    if (o_ptr->to_h < 0)
        o_ptr->to_h = 0;
    if (o_ptr->to_d < 0)
        o_ptr->to_d = 0;
}
