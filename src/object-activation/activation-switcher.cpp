/*!
 * @brief プレイヤーの発動コマンド実装
 * @date 2018/09/07
 * @author deskull
 */

#include "object-activation/activation-switcher.h"
#include "artifact/random-art-effects.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object-activation/activation-bolt-ball.h"
#include "object-activation/activation-breath.h"
#include "object-activation/activation-charm.h"
#include "object-activation/activation-genocide.h"
#include "object-activation/activation-others.h"
#include "object-activation/activation-resistance.h"
#include "object-activation/activation-teleport.h"
#include "object-enchant/activation-info-table.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "specific-object/blade-turner.h"
#include "specific-object/bloody-moon.h"
#include "specific-object/death-crimson.h"
#include "specific-object/muramasa.h"
#include "specific-object/ring-of-power.h"
#include "specific-object/stone-of-lore.h"
#include "specific-object/toragoroshi.h"
#include "spell-realm/spells-sorcery.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool switch_activation(PlayerType *player_ptr, object_type **o_ptr_ptr, const activation_type *const act_ptr, concptr name)
{
    object_type *o_ptr = (*o_ptr_ptr);

    switch (act_ptr->index) {
    case RandomArtActType::SUNLIGHT:
        return activate_sunlight(player_ptr);
    case RandomArtActType::BO_MISS_1:
        return activate_missile_1(player_ptr);
    case RandomArtActType::BA_POIS_1:
        return activate_ball_pois_1(player_ptr);
    case RandomArtActType::BO_ELEC_1:
        return activate_bolt_elec_1(player_ptr);
    case RandomArtActType::BO_ACID_1:
        return activate_bolt_acid_1(player_ptr);
    case RandomArtActType::BO_COLD_1:
        return activate_bolt_cold_1(player_ptr);
    case RandomArtActType::BO_FIRE_1:
        return activate_bolt_fire_1(player_ptr);
    case RandomArtActType::BA_COLD_1:
        return activate_ball_cold_1(player_ptr);
    case RandomArtActType::BA_COLD_2:
        return activate_ball_cold_2(player_ptr);
    case RandomArtActType::BA_COLD_3:
        return activate_ball_cold_2(player_ptr);
    case RandomArtActType::BA_FIRE_1:
        return activate_ball_fire_1(player_ptr);
    case RandomArtActType::BA_FIRE_2:
        return activate_ball_fire_2(player_ptr, name);
    case RandomArtActType::BA_FIRE_3:
        return activate_ball_fire_3(player_ptr);
    case RandomArtActType::BA_FIRE_4:
        return activate_ball_fire_4(player_ptr);
    case RandomArtActType::BA_ELEC_2:
        return activate_ball_elec_2(player_ptr);
    case RandomArtActType::BA_ELEC_3:
        return activate_ball_elec_3(player_ptr);
    case RandomArtActType::BA_ACID_1:
        return activate_ball_acid_1(player_ptr);
    case RandomArtActType::BA_NUKE_1:
        return activate_ball_nuke_1(player_ptr);
    case RandomArtActType::HYPODYNAMIA_1:
        return activate_bolt_hypodynamia_1(player_ptr, name);
    case RandomArtActType::HYPODYNAMIA_2:
        return activate_bolt_hypodynamia_2(player_ptr);
    case RandomArtActType::DRAIN_1:
        return activate_bolt_drain_1(player_ptr);
    case RandomArtActType::BO_MISS_2:
        return activate_missile_2(player_ptr);
    case RandomArtActType::WHIRLWIND:
        return activate_whirlwind(player_ptr);
    case RandomArtActType::DRAIN_2:
        return activate_bolt_drain_2(player_ptr);
    case RandomArtActType::CALL_CHAOS:
        return activate_call_chaos(player_ptr);
    case RandomArtActType::ROCKET:
        return activate_rocket(player_ptr);
    case RandomArtActType::DISP_EVIL:
        return activate_dispel_evil(player_ptr);
    case RandomArtActType::BA_MISS_3:
        return activate_missile_3(player_ptr);
    case RandomArtActType::DISP_GOOD:
        return activate_dispel_good(player_ptr);
    case RandomArtActType::BO_MANA:
        return activate_bolt_mana(player_ptr, name);
    case RandomArtActType::BA_WATER:
        return activate_ball_water(player_ptr, name);
    case RandomArtActType::BA_DARK:
        return activate_ball_dark(player_ptr, name);
    case RandomArtActType::BA_MANA:
        return activate_ball_mana(player_ptr, name);
    case RandomArtActType::PESTICIDE:
        return activate_pesticide(player_ptr);
    case RandomArtActType::BLINDING_LIGHT:
        return activate_blinding_light(player_ptr, name);
    case RandomArtActType::BIZARRE:
        return activate_ring_of_power(player_ptr, name);
    case RandomArtActType::CAST_BA_STAR:
        return activate_ball_lite(player_ptr, name);
    case RandomArtActType::BLADETURNER:
        return activate_bladeturner(player_ptr);
    case RandomArtActType::BR_FIRE:
        return activate_breath_fire(player_ptr, o_ptr);
    case RandomArtActType::BR_COLD:
        return activate_breath_cold(player_ptr, o_ptr);
    case RandomArtActType::BR_DRAGON:
        return activate_dragon_breath(player_ptr, o_ptr);
    case RandomArtActType::TREE_CREATION:
        return activate_tree_creation(player_ptr, o_ptr, name);
    case RandomArtActType::ANIM_DEAD:
        return activate_animate_dead(player_ptr, o_ptr);
    case RandomArtActType::CONFUSE:
        return activate_confusion(player_ptr);
    case RandomArtActType::SLEEP:
        return activate_sleep(player_ptr);
    case RandomArtActType::QUAKE:
        return activate_earthquake(player_ptr);
    case RandomArtActType::TERROR:
        return activate_terror(player_ptr);
    case RandomArtActType::TELE_AWAY:
        return activate_teleport_away(player_ptr);
    case RandomArtActType::BANISH_EVIL:
        return activate_banish_evil(player_ptr);
    case RandomArtActType::GENOCIDE:
        return activate_genocide(player_ptr);
    case RandomArtActType::MASS_GENO:
        return activate_mass_genocide(player_ptr);
    case RandomArtActType::SCARE_AREA:
        return activate_scare(player_ptr);
    case RandomArtActType::AGGRAVATE:
        return activate_aggravation(player_ptr, o_ptr, name);
    case RandomArtActType::CHARM_ANIMAL:
        return activate_charm_animal(player_ptr);
    case RandomArtActType::CHARM_UNDEAD:
        return activate_charm_undead(player_ptr);
    case RandomArtActType::CHARM_OTHER:
        return activate_charm_other(player_ptr);
    case RandomArtActType::CHARM_ANIMALS:
        return activate_charm_animals(player_ptr);
    case RandomArtActType::CHARM_OTHERS:
        return activate_charm_others(player_ptr);
    case RandomArtActType::SUMMON_ANIMAL:
        (void)summon_specific(player_ptr, -1, player_ptr->y, player_ptr->x, player_ptr->lev, SUMMON_ANIMAL_RANGER, PM_ALLOW_GROUP | PM_FORCE_PET);
        return true;
    case RandomArtActType::SUMMON_PHANTOM:
        msg_print(_("幻霊を召喚した。", "You summon a phantasmal servant."));
        (void)summon_specific(player_ptr, -1, player_ptr->y, player_ptr->x, player_ptr->current_floor_ptr->dun_level, SUMMON_PHANTOM, PM_ALLOW_GROUP | PM_FORCE_PET);
        return true;
    case RandomArtActType::SUMMON_ELEMENTAL:
        return cast_summon_elemental(player_ptr, (player_ptr->lev * 3) / 2);
    case RandomArtActType::SUMMON_DEMON:
        cast_summon_demon(player_ptr, (player_ptr->lev * 3) / 2);
        return true;
    case RandomArtActType::SUMMON_UNDEAD:
        return cast_summon_undead(player_ptr, (player_ptr->lev * 3) / 2);
    case RandomArtActType::SUMMON_HOUND:
        return cast_summon_hound(player_ptr, (player_ptr->lev * 3) / 2);
    case RandomArtActType::SUMMON_DAWN:
        msg_print(_("暁の師団を召喚した。", "You summon the Legion of the Dawn."));
        (void)summon_specific(player_ptr, -1, player_ptr->y, player_ptr->x, player_ptr->current_floor_ptr->dun_level, SUMMON_DAWN, PM_ALLOW_GROUP | PM_FORCE_PET);
        return true;
    case RandomArtActType::SUMMON_OCTOPUS:
        return cast_summon_octopus(player_ptr);
    case RandomArtActType::CHOIR_SINGS:
        msg_print(_("天国の歌が聞こえる...", "A heavenly choir sings..."));
        (void)cure_critical_wounds(player_ptr, 777);
        (void)set_hero(player_ptr, randint1(25) + 25, false);
        return true;
    case RandomArtActType::CURE_LW:
        return activate_cure_lw(player_ptr);
    case RandomArtActType::CURE_MW:
        msg_print(_("深紫色の光を発している...", "It radiates deep purple..."));
        (void)cure_serious_wounds(player_ptr, 4, 8);
        return true;
    case RandomArtActType::CURE_POISON: {
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        BadStatusSetter bss(player_ptr);
        (void)bss.afraidness(0);
        (void)bss.poison(0);
        return true;
    }
    case RandomArtActType::REST_EXP:
        msg_print(_("深紅に輝いている...", "It glows a deep red..."));
        restore_level(player_ptr);
        return true;
    case RandomArtActType::REST_ALL:
        msg_print(_("濃緑色に輝いている...", "It glows a deep green..."));
        (void)restore_all_status(player_ptr);
        (void)restore_level(player_ptr);
        return true;
    case RandomArtActType::CURE_700:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        msg_print(_("体内に暖かい鼓動が感じられる...", "You feel a warm tingling inside..."));
        (void)cure_critical_wounds(player_ptr, 700);
        return true;
    case RandomArtActType::CURE_1000:
        msg_print(_("白く明るく輝いている...", "It glows a bright white..."));
        msg_print(_("ひじょうに気分がよい...", "You feel much better..."));
        (void)cure_critical_wounds(player_ptr, 1000);
        return true;
    case RandomArtActType::CURING:
        msg_format(_("%sの優しさに癒される...", "the %s cures you affectionately ..."), name);
        true_healing(player_ptr, 0);
        return true;
    case RandomArtActType::CURE_MANA_FULL:
        msg_format(_("%sが青白く光った．．．", "The %s glows palely..."), name);
        restore_mana(player_ptr, true);
        return true;
    case RandomArtActType::ESP:
        (void)set_tim_esp(player_ptr, randint1(30) + 25, false);
        return true;
    case RandomArtActType::BERSERK:
        (void)berserk(player_ptr, randint1(25) + 25);
        return true;
    case RandomArtActType::PROT_EVIL:
        msg_format(_("%sから鋭い音が流れ出た...", "The %s lets out a shrill wail..."), name);
        (void)set_protevil(player_ptr, randint1(25) + player_ptr->lev * 3, false);
        return true;
    case RandomArtActType::RESIST_ALL:
        return activate_resistance_elements(player_ptr);
    case RandomArtActType::SPEED:
        msg_print(_("明るく緑色に輝いている...", "It glows bright green..."));
        (void)set_fast(player_ptr, randint1(20) + 20, false);
        return true;
    case RandomArtActType::XTRA_SPEED:
        msg_print(_("明るく輝いている...", "It glows brightly..."));
        (void)set_fast(player_ptr, randint1(75) + 75, false);
        return true;
    case RandomArtActType::WRAITH:
        set_wraith_form(player_ptr, randint1(player_ptr->lev / 2) + (player_ptr->lev / 2), false);
        return true;
    case RandomArtActType::INVULN:
        (void)set_invuln(player_ptr, randint1(8) + 8, false);
        return true;
    case RandomArtActType::HERO:
        (void)heroism(player_ptr, 25);
        return true;
    case RandomArtActType::HERO_SPEED:
        (void)set_fast(player_ptr, randint1(50) + 50, false);
        (void)heroism(player_ptr, 50);
        return true;
    case RandomArtActType::ACID_BALL_AND_RESISTANCE:
        return activate_acid_ball_and_resistance(player_ptr, name);
    case RandomArtActType::FIRE_BALL_AND_RESISTANCE:
        return activate_fire_ball_and_resistance(player_ptr, name);
    case RandomArtActType::COLD_BALL_AND_RESISTANCE:
        return activate_cold_ball_and_resistance(player_ptr, name);
    case RandomArtActType::ELEC_BALL_AND_RESISTANCE:
        return activate_elec_ball_and_resistance(player_ptr, name);
    case RandomArtActType::POIS_BALL_AND_RESISTANCE:
        return activate_pois_ball_and_resistance(player_ptr, name);
    case RandomArtActType::RESIST_ACID:
        return activate_resistance_acid(player_ptr, name);
    case RandomArtActType::RESIST_FIRE:
        return activate_resistance_fire(player_ptr, name);
    case RandomArtActType::RESIST_COLD:
        return activate_resistance_cold(player_ptr, name);
    case RandomArtActType::RESIST_ELEC:
        return activate_resistance_elec(player_ptr, name);
    case RandomArtActType::RESIST_POIS:
        return activate_resistance_pois(player_ptr, name);
    case RandomArtActType::LIGHT:
        return activate_light(player_ptr, name);
    case RandomArtActType::MAP_LIGHT:
        return activate_map_light(player_ptr);
    case RandomArtActType::DETECT_ALL:
        return activate_all_detection(player_ptr);
    case RandomArtActType::DETECT_XTRA:
        return activate_extra_detection(player_ptr);
    case RandomArtActType::ID_FULL:
        return activate_fully_identification(player_ptr);
    case RandomArtActType::ID_PLAIN:
        return activate_identification(player_ptr);
    case RandomArtActType::RUNE_EXPLO:
        return activate_exploding_rune(player_ptr);
    case RandomArtActType::RUNE_PROT:
        return activate_protection_rune(player_ptr);
    case RandomArtActType::SATIATE:
        (void)set_food(player_ptr, PY_FOOD_MAX - 1);
        return true;
    case RandomArtActType::DEST_DOOR:
        return activate_door_destroy(player_ptr);
    case RandomArtActType::STONE_MUD:
        return activate_stone_mud(player_ptr);
    case RandomArtActType::RECHARGE:
        return activate_recharge(player_ptr);
    case RandomArtActType::ALCHEMY:
        msg_print(_("明るい黄色に輝いている...", "It glows bright yellow..."));
        (void)alchemy(player_ptr);
        return true;
    case RandomArtActType::DIM_DOOR:
        return activate_dimension_door(player_ptr);
    case RandomArtActType::TELEPORT:
        return activate_teleport(player_ptr);
    case RandomArtActType::RECALL:
        return activate_recall(player_ptr);
    case RandomArtActType::JUDGE:
        return activate_judgement(player_ptr, name);
    case RandomArtActType::TELEKINESIS:
        return activate_telekinesis(player_ptr, name);
    case RandomArtActType::DETECT_UNIQUE:
        return activate_unique_detection(player_ptr);
    case RandomArtActType::ESCAPE:
        return activate_escape(player_ptr);
    case RandomArtActType::DISP_CURSE_XTRA:
        return activate_dispel_curse(player_ptr, name);
    case RandomArtActType::BRAND_FIRE_BOLTS:
        msg_format(_("%sが深紅に輝いた...", "Your %s glows deep red..."), name);
        brand_bolts(player_ptr);
        return true;
    case RandomArtActType::RECHARGE_XTRA:
        return activate_recharge_extra(player_ptr, name);
    case RandomArtActType::LORE:
        return StoneOfLore(player_ptr).perilous_secrets();
    case RandomArtActType::SHIKOFUMI:
        return activate_shikofumi(player_ptr);
    case RandomArtActType::PHASE_DOOR:
        return activate_phase_door(player_ptr);
    case RandomArtActType::DETECT_ALL_MONS:
        return activate_all_monsters_detection(player_ptr);
    case RandomArtActType::ULTIMATE_RESIST:
        return activate_ultimate_resistance(player_ptr);
    case RandomArtActType::ELBERETH:
        return activate_protection_elbereth(player_ptr);
    case RandomArtActType::DETECT_TREASURE:
        return activate_detect_treasure(player_ptr);
    case RandomArtActType::CAST_OFF:
        (void)cosmic_cast_off(player_ptr, o_ptr_ptr);
        return true;
    case RandomArtActType::FALLING_STAR:
        return activate_toragoroshi(player_ptr);
    case RandomArtActType::GRAND_CROSS:
        return activate_grand_cross(player_ptr);
    case RandomArtActType::TELEPORT_LEVEL:
        return activate_teleport_level(player_ptr);
    case RandomArtActType::STRAIN_HASTE:
        msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
        take_hit(player_ptr, DAMAGE_LOSELIFE, damroll(3, 8), _("加速した疲労", "the strain of haste"));
        (void)set_fast(player_ptr, player_ptr->fast + 25 + randint1(25), false);
        return true;
    case RandomArtActType::FISHING:
        return fishing(player_ptr);
    case RandomArtActType::INROU:
        mitokohmon(player_ptr);
        return true;
    case RandomArtActType::MURAMASA:
        return activate_muramasa(player_ptr, o_ptr);
    case RandomArtActType::BLOODY_MOON:
        return activate_bloody_moon(player_ptr, o_ptr);
    case RandomArtActType::CRIMSON:
        return activate_crimson(player_ptr, o_ptr);
    default:
        msg_format(_("Unknown activation effect: %d.", "Unknown activation effect: %d."), act_ptr->index);
        return false;
    }
}
