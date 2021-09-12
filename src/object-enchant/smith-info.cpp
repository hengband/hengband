#include "object-enchant/smith-info.h"
#include "object-enchant/smith-types.h"
#include "object-enchant/tr-types.h"
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

bool BasicSmithInfo::can_give_smith_effect(const object_type *o_ptr) const
{
    /*!
     * @note 固定orランダムアーティファクトもしくはすでに鍛冶済みでないかを最初にチェックし、
     * 残る具体的な絞り込みは BasicSmithInfo::can_give_smith_effect_impl およびその派生クラスで
     * オーバーライドした関数にて行う
     */
    if (o_ptr->is_artifact() || o_ptr->is_smith()) {
        return false;
    }

    return this->can_give_smith_effect_impl(o_ptr);
}

bool BasicSmithInfo::can_give_smith_effect_impl(const object_type *o_ptr) const
{
    if (this->category == SmithCategory::WEAPON_ATTR || this->category == SmithCategory::SLAYING) {
        return o_ptr->is_melee_ammo();
    }

    return o_ptr->is_weapon_armour_ammo();
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

    if (o_ptr->to_h < max_val) {
        o_ptr->to_h++;
    }
    if (o_ptr->to_d < max_val) {
        o_ptr->to_d++;
    }

    return true;
}

bool EnchantWeaponSmithInfo::can_give_smith_effect(const object_type *o_ptr) const
{
    return o_ptr->allow_enchant_melee_weapon();
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

bool EnchantArmourSmithInfo::can_give_smith_effect(const object_type *o_ptr) const
{
    return o_ptr->is_armour();
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

bool SustainSmithInfo::can_give_smith_effect(const object_type *o_ptr) const
{
    return o_ptr->is_weapon_armour_ammo();
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

bool SlayingGlovesSmithInfo::can_give_smith_effect_impl(const object_type *o_ptr) const
{
    return o_ptr->tval == TV_GLOVES;
}
