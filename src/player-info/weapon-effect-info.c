#include "player-info/weapon-effect-info.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "player-info/self-info-util.h"
#include "util/bit-flags-calculator.h"

static void set_weapon_bless_info(self_info_type *si_ptr)
{
    if (has_flag(si_ptr->flags, TR_BLESSED))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は神の祝福を受けている。", "Your weapon has been blessed by the gods.");

    if (has_flag(si_ptr->flags, TR_CHAOTIC))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はログルスの徴の属性をもつ。", "Your weapon is branded with the Sign of Logrus.");

    if (has_flag(si_ptr->flags, TR_IMPACT))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は打撃で地震を発生することができる。", "The impact of your weapon can cause earthquakes.");

    if (has_flag(si_ptr->flags, TR_VORPAL))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は非常に鋭い。", "Your weapon is very sharp.");

    if (has_flag(si_ptr->flags, TR_VAMPIRIC))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は敵から生命力を吸収する。", "Your weapon drains life from your foes.");
}

static void set_brand_attack_info(self_info_type *si_ptr)
{
    if (has_flag(si_ptr->flags, TR_BRAND_ACID))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は敵を溶かす。", "Your weapon melts your foes.");

    if (has_flag(si_ptr->flags, TR_BRAND_ELEC))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は敵を感電させる。", "Your weapon shocks your foes.");

    if (has_flag(si_ptr->flags, TR_BRAND_FIRE))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は敵を燃やす。", "Your weapon burns your foes.");

    if (has_flag(si_ptr->flags, TR_BRAND_COLD))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は敵を凍らせる。", "Your weapon freezes your foes.");

    if (has_flag(si_ptr->flags, TR_BRAND_POIS))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は敵を毒で侵す。", "Your weapon poisons your foes.");
}

static void set_slay_info(self_info_type *si_ptr)
{
    if (has_flag(si_ptr->flags, TR_KILL_ANIMAL))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は動物の天敵である。", "Your weapon is a great bane of animals.");
    else if (has_flag(si_ptr->flags, TR_SLAY_ANIMAL))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は動物に対して強い力を発揮する。", "Your weapon strikes at animals with extra force.");

    if (has_flag(si_ptr->flags, TR_KILL_EVIL))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は邪悪なる存在の天敵である。", "Your weapon is a great bane of evil.");
    else if (has_flag(si_ptr->flags, TR_SLAY_EVIL))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は邪悪なる存在に対して強い力を発揮する。", "Your weapon strikes at evil with extra force.");

    if (has_flag(si_ptr->flags, TR_KILL_HUMAN))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は人間の天敵である。", "Your weapon is a great bane of humans.");
    else if (has_flag(si_ptr->flags, TR_SLAY_HUMAN))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は人間に対して特に強い力を発揮する。", "Your weapon is especially deadly against humans.");

    if (has_flag(si_ptr->flags, TR_KILL_UNDEAD))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はアンデッドの天敵である。", "Your weapon is a great bane of undead.");
    else if (has_flag(si_ptr->flags, TR_SLAY_UNDEAD))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はアンデッドに対して神聖なる力を発揮する。", "Your weapon strikes at undead with holy wrath.");

    if (has_flag(si_ptr->flags, TR_KILL_DEMON))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はデーモンの天敵である。", "Your weapon is a great bane of demons.");
    else if (has_flag(si_ptr->flags, TR_SLAY_DEMON))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はデーモンに対して神聖なる力を発揮する。", "Your weapon strikes at demons with holy wrath.");

    if (has_flag(si_ptr->flags, TR_KILL_ORC))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はオークの天敵である。", "Your weapon is a great bane of orcs.");
    else if (has_flag(si_ptr->flags, TR_SLAY_ORC))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はオークに対して特に強い力を発揮する。", "Your weapon is especially deadly against orcs.");

    if (has_flag(si_ptr->flags, TR_KILL_TROLL))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はトロルの天敵である。", "Your weapon is a great bane of trolls.");
    else if (has_flag(si_ptr->flags, TR_SLAY_TROLL))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はトロルに対して特に強い力を発揮する。", "Your weapon is especially deadly against trolls.");

    if (has_flag(si_ptr->flags, TR_KILL_GIANT))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は巨人の天敵である。", "Your weapon is a great bane of giants.");
    else if (has_flag(si_ptr->flags, TR_SLAY_GIANT))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は巨人に対して特に強い力を発揮する。", "Your weapon is especially deadly against giants.");

    if (has_flag(si_ptr->flags, TR_KILL_DRAGON))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はドラゴンの天敵である。", "Your weapon is a great bane of dragons.");
    else if (has_flag(si_ptr->flags, TR_SLAY_DRAGON))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はドラゴンに対して特に強い力を発揮する。", "Your weapon is especially deadly against dragons.");
}

void set_weapon_effect_info(player_type *creature_ptr, self_info_type *si_ptr)
{
    object_type *o_ptr = &creature_ptr->inventory_list[INVEN_RARM];
    if (o_ptr->k_idx == 0)
        return;

    set_weapon_bless_info(si_ptr);
    set_brand_attack_info(si_ptr);
    set_slay_info(si_ptr);
    if (has_flag(si_ptr->flags, TR_FORCE_WEAPON))
        si_ptr->info[si_ptr->line++] = _("あなたの武器はMPを使って攻撃する。", "Your weapon causes greate damages using your MP.");

    if (has_flag(si_ptr->flags, TR_THROW))
        si_ptr->info[si_ptr->line++] = _("あなたの武器は投げやすい。", "Your weapon can be thrown well.");
}
