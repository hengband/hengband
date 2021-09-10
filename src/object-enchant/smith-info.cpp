#include "object-enchant/smith-info.h"
#include "object/object-flags.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

smith_info_base::smith_info_base(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption)
    : effect(effect)
    , name(name)
    , category(category)
    , need_essences(std::move(need_essences))
    , consumption(consumption)
{
}

TrFlags smith_info_base::tr_flags() const
{
    return {};
}

std::optional<random_art_activation_type> smith_info_base::activation_index() const
{
    return std::nullopt;
}

basic_smith_info::basic_smith_info(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption, TrFlags add_flags)
    : smith_info_base(effect, name, category, std::move(need_essences), consumption)
    , add_flags(add_flags)
{
}

bool basic_smith_info::add_essence(player_type *, object_type *o_ptr, int) const
{
    o_ptr->xtra3 = static_cast<decltype(o_ptr->xtra3)>(effect);

    return true;
}

void basic_smith_info::erase_essence(object_type *o_ptr) const
{
    o_ptr->xtra3 = 0;
    auto flgs = object_flags(o_ptr);
    if (flgs.has_none_of(TR_PVAL_FLAG_MASK))
        o_ptr->pval = 0;
}

TrFlags basic_smith_info::tr_flags() const
{
    return this->add_flags;
}

activation_smith_info::activation_smith_info(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption, TrFlags add_flags, random_art_activation_type act_idx)
    : basic_smith_info(effect, name, category, std::move(need_essences), consumption, std::move(add_flags))
    , act_idx(act_idx)
{
}

TrFlags activation_smith_info::tr_flags() const
{
    return basic_smith_info::tr_flags().set(TR_ACTIVATE);
}

std::optional<random_art_activation_type> activation_smith_info::activation_index() const
{
    return this->act_idx;
}

enchant_weapon_smith_info::enchant_weapon_smith_info(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption)
    : smith_info_base(effect, name, category, std::move(need_essences), consumption)
{
}

bool enchant_weapon_smith_info::add_essence(player_type *player_ptr, object_type *o_ptr, int) const
{
    const auto max_val = player_ptr->lev / 5 + 5;
    if ((o_ptr->to_h >= max_val) && (o_ptr->to_d >= max_val)) {
        return false;
    }

    o_ptr->to_h = static_cast<HIT_PROB>(std::min(o_ptr->to_h + 1, max_val));
    o_ptr->to_d = static_cast<HIT_POINT>(std::min(o_ptr->to_d + 1, max_val));

    return true;
}

enchant_armour_smith_info::enchant_armour_smith_info(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption)
    : smith_info_base(effect, name, category, std::move(need_essences), consumption)
{
}

bool enchant_armour_smith_info::add_essence(player_type *player_ptr, object_type *o_ptr, int) const
{
    const auto max_val = player_ptr->lev / 5 + 5;
    if (o_ptr->to_a >= max_val) {
        return false;
    }

    o_ptr->to_a++;

    return true;
}

sustain_smith_info::sustain_smith_info(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption)
    : smith_info_base(effect, name, category, std::move(need_essences), consumption)
{
}

bool sustain_smith_info::add_essence(player_type *, object_type *o_ptr, int) const
{
    o_ptr->art_flags.set(TR_IGNORE_ACID);
    o_ptr->art_flags.set(TR_IGNORE_ELEC);
    o_ptr->art_flags.set(TR_IGNORE_FIRE);
    o_ptr->art_flags.set(TR_IGNORE_COLD);

    return true;
}

slaying_gloves_smith_info::slaying_gloves_smith_info(SmithEffect effect, concptr name, SmithCategory category, std::vector<SmithEssence> need_essences, int consumption)
    : basic_smith_info(effect, name, category, std::move(need_essences), consumption, {})
{
}

bool slaying_gloves_smith_info::add_essence(player_type *player_ptr, object_type *o_ptr, int number) const
{
    basic_smith_info::add_essence(player_ptr, o_ptr, number);

    HIT_PROB get_to_h = ((number + 1) / 2 + randint0(number / 2 + 1));
    HIT_POINT get_to_d = ((number + 1) / 2 + randint0(number / 2 + 1));
    o_ptr->xtra4 = (get_to_h << 8) + get_to_d;
    o_ptr->to_h += get_to_h;
    o_ptr->to_d += get_to_d;

    return true;
}

void slaying_gloves_smith_info::erase_essence(object_type *o_ptr) const
{
    basic_smith_info::erase_essence(o_ptr);

    o_ptr->to_h -= (o_ptr->xtra4 >> 8);
    o_ptr->to_d -= (o_ptr->xtra4 & 0x000f);
    o_ptr->xtra4 = 0;
    if (o_ptr->to_h < 0)
        o_ptr->to_h = 0;
    if (o_ptr->to_d < 0)
        o_ptr->to_d = 0;
}
