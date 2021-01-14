#include "player-info/weapon-effect-info.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "player-info/self-info-util.h"
#include "util/bit-flags-calculator.h"

static void set_weapon_bless_info(self_info_type *si_ptr)
{
    if (has_flag(si_ptr->flags, TR_BLESSED))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͐_�̏j�����󂯂Ă���B", "Your weapon has been blessed by the gods.");

    if (has_flag(si_ptr->flags, TR_CHAOTIC))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓��O���X�̒��̑��������B", "Your weapon is branded with the Sign of Logrus.");

    if (has_flag(si_ptr->flags, TR_IMPACT))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͑Ō��Œn�k�𔭐����邱�Ƃ��ł���B", "The impact of your weapon can cause earthquakes.");

    if (has_flag(si_ptr->flags, TR_VORPAL))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͔��ɉs���B", "Your weapon is very sharp.");

    if (has_flag(si_ptr->flags, TR_VAMPIRIC))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͓G���琶���͂��z������B", "Your weapon drains life from your foes.");
}

static void set_brand_attack_info(self_info_type *si_ptr)
{
    if (has_flag(si_ptr->flags, TR_BRAND_ACID))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͓G��n�����B", "Your weapon melts your foes.");

    if (has_flag(si_ptr->flags, TR_BRAND_ELEC))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͓G�����d������B", "Your weapon shocks your foes.");

    if (has_flag(si_ptr->flags, TR_BRAND_FIRE))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͓G��R�₷�B", "Your weapon burns your foes.");

    if (has_flag(si_ptr->flags, TR_BRAND_COLD))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͓G�𓀂点��B", "Your weapon freezes your foes.");

    if (has_flag(si_ptr->flags, TR_BRAND_POIS))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͓G��łŐN���B", "Your weapon poisons your foes.");
}

static void set_slay_info(self_info_type *si_ptr)
{
    if (has_flag(si_ptr->flags, TR_KILL_ANIMAL))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͓����̓V�G�ł���B", "Your weapon is a great bane of animals.");
    else if (has_flag(si_ptr->flags, TR_SLAY_ANIMAL))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͓����ɑ΂��ċ����͂𔭊�����B", "Your weapon strikes at animals with extra force.");

    if (has_flag(si_ptr->flags, TR_KILL_EVIL))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͎׈��Ȃ鑶�݂̓V�G�ł���B", "Your weapon is a great bane of evil.");
    else if (has_flag(si_ptr->flags, TR_SLAY_EVIL))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͎׈��Ȃ鑶�݂ɑ΂��ċ����͂𔭊�����B", "Your weapon strikes at evil with extra force.");

    if (has_flag(si_ptr->flags, TR_KILL_HUMAN))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͐l�Ԃ̓V�G�ł���B", "Your weapon is a great bane of humans.");
    else if (has_flag(si_ptr->flags, TR_SLAY_HUMAN))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͐l�Ԃɑ΂��ē��ɋ����͂𔭊�����B", "Your weapon is especially deadly against humans.");

    if (has_flag(si_ptr->flags, TR_KILL_UNDEAD))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓A���f�b�h�̓V�G�ł���B", "Your weapon is a great bane of undead.");
    else if (has_flag(si_ptr->flags, TR_SLAY_UNDEAD))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓A���f�b�h�ɑ΂��Đ_���Ȃ�͂𔭊�����B", "Your weapon strikes at undead with holy wrath.");

    if (has_flag(si_ptr->flags, TR_KILL_DEMON))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓f�[�����̓V�G�ł���B", "Your weapon is a great bane of demons.");
    else if (has_flag(si_ptr->flags, TR_SLAY_DEMON))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓f�[�����ɑ΂��Đ_���Ȃ�͂𔭊�����B", "Your weapon strikes at demons with holy wrath.");

    if (has_flag(si_ptr->flags, TR_KILL_ORC))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓I�[�N�̓V�G�ł���B", "Your weapon is a great bane of orcs.");
    else if (has_flag(si_ptr->flags, TR_SLAY_ORC))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓I�[�N�ɑ΂��ē��ɋ����͂𔭊�����B", "Your weapon is especially deadly against orcs.");

    if (has_flag(si_ptr->flags, TR_KILL_TROLL))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓g�����̓V�G�ł���B", "Your weapon is a great bane of trolls.");
    else if (has_flag(si_ptr->flags, TR_SLAY_TROLL))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓g�����ɑ΂��ē��ɋ����͂𔭊�����B", "Your weapon is especially deadly against trolls.");

    if (has_flag(si_ptr->flags, TR_KILL_GIANT))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͋��l�̓V�G�ł���B", "Your weapon is a great bane of giants.");
    else if (has_flag(si_ptr->flags, TR_SLAY_GIANT))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͋��l�ɑ΂��ē��ɋ����͂𔭊�����B", "Your weapon is especially deadly against giants.");

    if (has_flag(si_ptr->flags, TR_KILL_DRAGON))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓h���S���̓V�G�ł���B", "Your weapon is a great bane of dragons.");
    else if (has_flag(si_ptr->flags, TR_SLAY_DRAGON))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���̓h���S���ɑ΂��ē��ɋ����͂𔭊�����B", "Your weapon is especially deadly against dragons.");
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
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕����MP���g���čU������B", "Your weapon causes greate damages using your MP.");

    if (has_flag(si_ptr->flags, TR_THROW))
        si_ptr->info[si_ptr->line++] = _("���Ȃ��̕���͓����₷���B", "Your weapon can be thrown well.");
}
