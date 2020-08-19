/*!
 * @brief プレイヤーの発動コマンド実装
 * @date 2018/09/07
 * @author deskull
 */

#include "object-activation/activation-switcher.h"
#include "action/action-limited.h"
#include "action/activation-execution.h"
#include "art-definition/art-bow-types.h"
#include "art-definition/art-sword-types.h"
#include "art-definition/art-weapon-types.h"
#include "art-definition/random-art-effects.h"
#include "artifact/artifact-info.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "core/hp-mp-processor.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/special-options.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "object-activation/activation-bolt-ball.h"
#include "object-activation/activation-breath.h"
#include "object-activation/activation-charm.h"
#include "object-activation/activation-resistance.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/dragon-breaths-table.h"
#include "object-enchant/object-ego.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-magic.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "player-attack/player-attack.h"
#include "player/attack-defense-types.h"
#include "player/avatar.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-race-types.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "specific-object/death-crimson.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-sorcery.h"
#include "spell/process-effect.h"
#include "spell/spell-types.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "status/temporary-resistance.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "world/world.h"

bool activate_sunlight(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    msg_print(_("太陽光線が放たれた。", "A line of sunlight appears."));
    (void)lite_line(user_ptr, dir, damroll(6, 8));
    return TRUE;
}

bool activate_ring_of_power(player_type *user_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%sは漆黒に輝いた...", "The %s glows intensely black..."), name);
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    ring_of_power(user_ptr, dir);
    return TRUE;
}

bool activate_bladeturner(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
    fire_breath(user_ptr, GF_MISSILE, dir, 300, 4);
    msg_print(_("鎧が様々な色に輝いた...", "Your armor glows many colours..."));
    (void)set_afraid(user_ptr, 0);
    (void)set_hero(user_ptr, randint1(50) + 50, FALSE);
    (void)hp_player(user_ptr, 10);
    (void)set_blessed(user_ptr, randint1(50) + 50, FALSE);
    (void)set_oppose_acid(user_ptr, randint1(50) + 50, FALSE);
    (void)set_oppose_elec(user_ptr, randint1(50) + 50, FALSE);
    (void)set_oppose_fire(user_ptr, randint1(50) + 50, FALSE);
    (void)set_oppose_cold(user_ptr, randint1(50) + 50, FALSE);
    (void)set_oppose_pois(user_ptr, randint1(50) + 50, FALSE);
    return TRUE;
}

bool activate_confusion(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    confuse_monster(user_ptr, dir, 20);
    return TRUE;
}

bool activate_teleport_away(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_beam(user_ptr, GF_AWAY_ALL, dir, user_ptr->lev);
    return TRUE;
}

bool activate_banish_evil(player_type *user_ptr)
{
    if (banish_evil(user_ptr, 100))
        msg_print(_("アーティファクトの力が邪悪を打ち払った！", "The power of the artifact banishes evil!"));

    return TRUE;
}

bool activate_scare(player_type *user_ptr)
{
    if (music_singing_any(user_ptr))
        stop_singing(user_ptr);

    if (hex_spelling_any(user_ptr))
        stop_hex_spell_all(user_ptr);

    msg_print(_("あなたは力強い突風を吹き鳴らした。周囲の敵が震え上っている!", "You wind a mighty blast; your enemies tremble!"));
    (void)turn_monsters(user_ptr, (3 * user_ptr->lev / 2) + 10);
    return TRUE;
}

bool activate_aggravation(player_type *user_ptr, object_type *o_ptr, concptr name)
{
    if (o_ptr->name1 == ART_HYOUSIGI)
        msg_print(_("拍子木を打った。", "You beat your wooden clappers."));
    else
        msg_format(_("%sは不快な物音を立てた。", "The %s sounds an unpleasant noise."), name);

    aggravate_monsters(user_ptr, 0);
    return TRUE;
}

bool switch_activation(player_type *user_ptr, object_type *o_ptr, const activation_type *const act_ptr, concptr name)
{
    DIRECTION dir;
    switch (act_ptr->index) {
    case ACT_SUNLIGHT:
        return activate_sunlight(user_ptr);
    case ACT_BO_MISS_1:
        return activate_missile_1(user_ptr);
    case ACT_BA_POIS_1:
        return activate_ball_pois_1(user_ptr);
    case ACT_BO_ELEC_1:
        return activate_bolt_elec_1(user_ptr);
    case ACT_BO_ACID_1:
        return activate_bolt_acid_1(user_ptr);
    case ACT_BO_COLD_1:
        return activate_bolt_cold_1(user_ptr);
    case ACT_BO_FIRE_1:
        return activate_bolt_fire_1(user_ptr);
    case ACT_BA_COLD_1:
        return activate_ball_cold_1(user_ptr);
    case ACT_BA_COLD_2:
        return activate_ball_cold_2(user_ptr);
    case ACT_BA_COLD_3:
        return activate_ball_cold_2(user_ptr);
    case ACT_BA_FIRE_1:
        return activate_ball_fire_1(user_ptr);
    case ACT_BA_FIRE_2:
        return activate_ball_fire_2(user_ptr, name);
    case ACT_BA_FIRE_3:
        return activate_ball_fire_3(user_ptr);
    case ACT_BA_FIRE_4:
        return activate_ball_fire_4(user_ptr);
    case ACT_BA_ELEC_2:
        return activate_ball_elec_2(user_ptr);
    case ACT_BA_ELEC_3:
        return activate_ball_elec_3(user_ptr);
    case ACT_BA_ACID_1:
        return activate_ball_acid_1(user_ptr);
    case ACT_BA_NUKE_1:
        return activate_ball_nuke_1(user_ptr);
    case ACT_HYPODYNAMIA_1:
        return activate_bolt_hypodynamia_1(user_ptr, name);
    case ACT_HYPODYNAMIA_2:
        return activate_bolt_hypodynamia_2(user_ptr);
    case ACT_DRAIN_1:
        return activate_bolt_drain_1(user_ptr);
    case ACT_BO_MISS_2:
        return activate_missile_2(user_ptr);
    case ACT_WHIRLWIND:
        massacre(user_ptr);
        return TRUE;
    case ACT_DRAIN_2:
        return activate_bolt_drain_2(user_ptr);
    case ACT_CALL_CHAOS:
        msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
        call_chaos(user_ptr);
        return TRUE;
    case ACT_ROCKET:
        return activate_rocket(user_ptr);
    case ACT_DISP_EVIL:
        msg_print(_("神聖な雰囲気が充満した...", "It floods the area with goodness..."));
        dispel_evil(user_ptr, user_ptr->lev * 5);
        return TRUE;
    case ACT_BA_MISS_3:
        return activate_missile_3(user_ptr);
    case ACT_DISP_GOOD:
        msg_print(_("邪悪な雰囲気が充満した...", "It floods the area with evil..."));
        dispel_good(user_ptr, user_ptr->lev * 5);
        return TRUE;
    case ACT_BO_MANA:
        return activate_bolt_mana(user_ptr, name);
    case ACT_BA_WATER:
        return activate_ball_water(user_ptr, name);
    case ACT_BA_DARK:
        return activate_ball_dark(user_ptr, name);
    case ACT_BA_MANA:
        return activate_ball_mana(user_ptr, name);
    case ACT_PESTICIDE:
        msg_print(_("あなたは害虫を一掃した。", "You exterminate small life."));
        (void)dispel_monsters(user_ptr, 4);
        return TRUE;
    case ACT_BLINDING_LIGHT:
        msg_format(_("%sが眩しい光で輝いた...", "The %s gleams with blinding light..."), name);
        (void)fire_ball(user_ptr, GF_LITE, 0, 300, 6);
        confuse_monsters(user_ptr, 3 * user_ptr->lev / 2);
        return TRUE;
    case ACT_BIZARRE:
        return activate_ring_of_power(user_ptr, name);
    case ACT_CAST_BA_STAR:
        return activate_ball_lite(user_ptr, name);
    case ACT_BLADETURNER:
        return activate_bladeturner(user_ptr);
    case ACT_BR_FIRE:
        return activate_breath_fire(user_ptr, o_ptr);
    case ACT_BR_COLD:
        return activate_breath_cold(user_ptr, o_ptr);
    case ACT_BR_DRAGON:
        return activate_dragon_breath(user_ptr, o_ptr);
    case ACT_CONFUSE:
        return activate_confusion(user_ptr);
    case ACT_SLEEP:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        sleep_monsters_touch(user_ptr);
        return TRUE;
    case ACT_QUAKE:
        earthquake(user_ptr, user_ptr->y, user_ptr->x, 5, 0);
        return TRUE;
    case ACT_TERROR:
        turn_monsters(user_ptr, 40 + user_ptr->lev);
        return TRUE;
    case ACT_TELE_AWAY:
        return activate_teleport_away(user_ptr);
    case ACT_BANISH_EVIL:
        return activate_banish_evil(user_ptr);
    case ACT_GENOCIDE:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        (void)symbol_genocide(user_ptr, 200, TRUE);
        return TRUE;
    case ACT_MASS_GENO:
        msg_print(_("ひどく鋭い音が流れ出た...", "It lets out a long, shrill note..."));
        (void)mass_genocide(user_ptr, 200, TRUE);
        return TRUE;
    case ACT_SCARE_AREA:
        return activate_scare(user_ptr);
    case ACT_AGGRAVATE:
        return activate_aggravation(user_ptr, o_ptr, name);
    case ACT_CHARM_ANIMAL:
        return activate_charm_animal(user_ptr);
    case ACT_CHARM_UNDEAD:
        return activate_charm_undead(user_ptr);
    case ACT_CHARM_OTHER:
        return activate_charm_other(user_ptr);
    case ACT_CHARM_ANIMALS:
        return activate_charm_animals(user_ptr);
    case ACT_CHARM_OTHERS:
        return activate_charm_others(user_ptr);
    case ACT_SUMMON_ANIMAL:
        (void)summon_specific(user_ptr, -1, user_ptr->y, user_ptr->x, user_ptr->lev, SUMMON_ANIMAL_RANGER, (PM_ALLOW_GROUP | PM_FORCE_PET));
        return TRUE;
    case ACT_SUMMON_PHANTOM:
        msg_print(_("幻霊を召喚した。", "You summon a phantasmal servant."));
        (void)summon_specific(user_ptr, -1, user_ptr->y, user_ptr->x, user_ptr->current_floor_ptr->dun_level, SUMMON_PHANTOM, (PM_ALLOW_GROUP | PM_FORCE_PET));
        return TRUE;
    case ACT_SUMMON_ELEMENTAL:
        return cast_summon_elemental(user_ptr, (user_ptr->lev * 3) / 2);
    case ACT_SUMMON_DEMON:
        cast_summon_demon(user_ptr, (user_ptr->lev * 3) / 2);
        return TRUE;
    case ACT_SUMMON_UNDEAD:
        return cast_summon_undead(user_ptr, (user_ptr->lev * 3) / 2);
    case ACT_SUMMON_HOUND:
        return cast_summon_hound(user_ptr, (user_ptr->lev * 3) / 2);
    case ACT_SUMMON_DAWN:
        msg_print(_("暁の師団を召喚した。", "You summon the Legion of the Dawn."));
        (void)summon_specific(user_ptr, -1, user_ptr->y, user_ptr->x, user_ptr->current_floor_ptr->dun_level, SUMMON_DAWN, (PM_ALLOW_GROUP | PM_FORCE_PET));
        return TRUE;
    case ACT_SUMMON_OCTOPUS:
        return cast_summon_octopus(user_ptr);
    case ACT_CHOIR_SINGS:
        msg_print(_("天国の歌が聞こえる...", "A heavenly choir sings..."));
        (void)cure_critical_wounds(user_ptr, 777);
        (void)set_hero(user_ptr, randint1(25) + 25, FALSE);
        return TRUE;
    case ACT_CURE_LW:
        (void)set_afraid(user_ptr, 0);
        (void)hp_player(user_ptr, 30);
        return TRUE;
    case ACT_CURE_MW:
        msg_print(_("深紫色の光を発している...", "It radiates deep purple..."));
        (void)cure_serious_wounds(user_ptr, 4, 8);
        return TRUE;
    case ACT_CURE_POISON:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        (void)set_afraid(user_ptr, 0);
        (void)set_poisoned(user_ptr, 0);
        return TRUE;
    case ACT_REST_EXP:
        msg_print(_("深紅に輝いている...", "It glows a deep red..."));
        restore_level(user_ptr);
        return TRUE;
    case ACT_REST_ALL:
        msg_print(_("濃緑色に輝いている...", "It glows a deep green..."));
        (void)restore_all_status(user_ptr);
        (void)restore_level(user_ptr);
        return TRUE;
    case ACT_CURE_700:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        msg_print(_("体内に暖かい鼓動が感じられる...", "You feel a warm tingling inside..."));
        (void)cure_critical_wounds(user_ptr, 700);
        return TRUE;
    case ACT_CURE_1000:
        msg_print(_("白く明るく輝いている...", "It glows a bright white..."));
        msg_print(_("ひじょうに気分がよい...", "You feel much better..."));
        (void)cure_critical_wounds(user_ptr, 1000);
        return TRUE;
    case ACT_CURING:
        msg_format(_("%sの優しさに癒される...", "the %s cures you affectionately ..."), name);
        true_healing(user_ptr, 0);
        return TRUE;
    case ACT_CURE_MANA_FULL:
        msg_format(_("%sが青白く光った．．．", "The %s glows pale..."), name);
        restore_mana(user_ptr, TRUE);
        return TRUE;
    case ACT_ESP:
        (void)set_tim_esp(user_ptr, randint1(30) + 25, FALSE);
        return TRUE;
    case ACT_BERSERK:
        (void)berserk(user_ptr, randint1(25) + 25);
        return TRUE;
    case ACT_PROT_EVIL:
        msg_format(_("%sから鋭い音が流れ出た...", "The %s lets out a shrill wail..."), name);
        (void)set_protevil(user_ptr, randint1(25) + user_ptr->lev * 3, FALSE);
        return TRUE;
    case ACT_RESIST_ALL:
        return activate_resistance_elements(user_ptr);
    case ACT_SPEED:
        msg_print(_("明るく緑色に輝いている...", "It glows bright green..."));
        (void)set_fast(user_ptr, randint1(20) + 20, FALSE);
        return TRUE;
    case ACT_XTRA_SPEED:
        msg_print(_("明るく輝いている...", "It glows brightly..."));
        (void)set_fast(user_ptr, randint1(75) + 75, FALSE);
        return TRUE;
    case ACT_WRAITH:
        set_wraith_form(user_ptr, randint1(user_ptr->lev / 2) + (user_ptr->lev / 2), FALSE);
        return TRUE;
    case ACT_INVULN:
        (void)set_invuln(user_ptr, randint1(8) + 8, FALSE);
        return TRUE;
    case ACT_HERO:
        (void)heroism(user_ptr, 25);
        return TRUE;
    case ACT_HERO_SPEED:
        (void)set_fast(user_ptr, randint1(50) + 50, FALSE);
        (void)heroism(user_ptr, 50);
        return TRUE;
    case ACT_RESIST_ACID:
        return activate_resistance_acid(user_ptr, o_ptr, name);
    case ACT_RESIST_FIRE:
        return activate_resistance_fire(user_ptr, o_ptr, name);
    case ACT_RESIST_COLD:
        return activate_resistance_cold(user_ptr, o_ptr, name);
    case ACT_RESIST_ELEC:
        return activate_resistance_elec(user_ptr, o_ptr, name);
    case ACT_RESIST_POIS:
        return activate_resistance_pois(user_ptr, name);
    case ACT_LIGHT:
        msg_format(_("%sから澄んだ光があふれ出た...", "The %s wells with clear light..."), name);
        lite_area(user_ptr, damroll(2, 15), 3);
        return TRUE;
    case ACT_MAP_LIGHT:
        msg_print(_("眩しく輝いた...", "It shines brightly..."));
        map_area(user_ptr, DETECT_RAD_MAP);
        lite_area(user_ptr, damroll(2, 15), 3);
        return TRUE;
    case ACT_DETECT_ALL:
        msg_print(_("白く明るく輝いている...", "It glows bright white..."));
        msg_print(_("心にイメージが浮かんできた...", "An image forms in your mind..."));
        detect_all(user_ptr, DETECT_RAD_DEFAULT);
        return TRUE;
    case ACT_DETECT_XTRA:
        msg_print(_("明るく輝いている...", "It glows brightly..."));
        detect_all(user_ptr, DETECT_RAD_DEFAULT);
        probing(user_ptr);
        identify_fully(user_ptr, FALSE, 0);
        return TRUE;
    case ACT_ID_FULL:
        msg_print(_("黄色く輝いている...", "It glows yellow..."));
        identify_fully(user_ptr, FALSE, 0);
        return TRUE;
    case ACT_ID_PLAIN:
        return ident_spell(user_ptr, FALSE, 0);
    case ACT_RUNE_EXPLO:
        msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
        explosive_rune(user_ptr, user_ptr->y, user_ptr->x);
        return TRUE;
    case ACT_RUNE_PROT:
        msg_print(_("ブルーに明るく輝いている...", "It glows light blue..."));
        warding_glyph(user_ptr);
        return TRUE;
    case ACT_SATIATE:
        (void)set_food(user_ptr, PY_FOOD_MAX - 1);
        return TRUE;
    case ACT_DEST_DOOR:
        msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
        destroy_doors_touch(user_ptr);
        return TRUE;
    case ACT_STONE_MUD:
        msg_print(_("鼓動している...", "It pulsates..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        wall_to_mud(user_ptr, dir, 20 + randint1(30));
        return TRUE;
    case ACT_RECHARGE:
        recharge(user_ptr, 130);
        return TRUE;
    case ACT_ALCHEMY:
        msg_print(_("明るい黄色に輝いている...", "It glows bright yellow..."));
        (void)alchemy(user_ptr);
        return TRUE;
    case ACT_DIM_DOOR:
        msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
        return dimension_door(user_ptr);
    case ACT_TELEPORT:
        msg_print(_("周りの空間が歪んでいる...", "It twists space around you..."));
        teleport_player(user_ptr, 100, TELEPORT_SPONTANEOUS);
        return TRUE;
    case ACT_RECALL:
        msg_print(_("やわらかな白色に輝いている...", "It glows soft white..."));
        return recall_player(user_ptr, randint0(21) + 15);
    case ACT_JUDGE:
        msg_format(_("%sは赤く明るく光った！", "The %s flashes bright red!"), name);
        chg_virtue(user_ptr, V_KNOWLEDGE, 1);
        chg_virtue(user_ptr, V_ENLIGHTEN, 1);
        wiz_lite(user_ptr, FALSE);

        msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
        take_hit(user_ptr, DAMAGE_LOSELIFE, damroll(3, 8), _("審判の宝石", "the Jewel of Judgement"), -1);

        (void)detect_traps(user_ptr, DETECT_RAD_DEFAULT, TRUE);
        (void)detect_doors(user_ptr, DETECT_RAD_DEFAULT);
        (void)detect_stairs(user_ptr, DETECT_RAD_DEFAULT);

        if (get_check(_("帰還の力を使いますか？", "Activate recall? ")))
            (void)recall_player(user_ptr, randint0(21) + 15);

        return TRUE;
    case ACT_TELEKINESIS:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        msg_format(_("%sを伸ばした。", "You stretched your %s."), name);
        fetch_item(user_ptr, dir, 500, TRUE);
        return TRUE;
    case ACT_DETECT_UNIQUE: {
        monster_type *m_ptr;
        monster_race *r_ptr;
        msg_print(_("奇妙な場所が頭の中に浮かんだ．．．", "Some strange places show up in your mind. And you see ..."));
        for (int i = user_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
            m_ptr = &user_ptr->current_floor_ptr->m_list[i];
            if (!monster_is_valid(m_ptr))
                continue;

            r_ptr = &r_info[m_ptr->r_idx];
            if (r_ptr->flags1 & RF1_UNIQUE)
                msg_format(_("%s． ", "%s. "), r_name + r_ptr->name);
        }

        return TRUE;
    }
    case ACT_ESCAPE:
        switch (randint1(13)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            teleport_player(user_ptr, 10, TELEPORT_SPONTANEOUS);
            return TRUE;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            teleport_player(user_ptr, 222, TELEPORT_SPONTANEOUS);
            return TRUE;
        case 11:
        case 12:
            (void)stair_creation(user_ptr);
            return TRUE;
        default:
            if (!get_check(_("この階を去りますか？", "Leave this level? ")))
                return TRUE;

            if (autosave_l)
                do_cmd_save_game(user_ptr, TRUE);

            user_ptr->leaving = TRUE;
            return TRUE;
        }
    case ACT_DISP_CURSE_XTRA:
        msg_format(_("%sが真実を照らし出す...", "The %s exhibits the truth..."), name);
        (void)remove_all_curse(user_ptr);
        (void)probing(user_ptr);
        return TRUE;
    case ACT_BRAND_FIRE_BOLTS:
        msg_format(_("%sが深紅に輝いた...", "Your %s glows deep red..."), name);
        brand_bolts(user_ptr);
        return TRUE;
    case ACT_RECHARGE_XTRA:
        msg_format(_("%sが白く輝いた．．．", "The %s gleams with blinding light..."), name);
        return recharge(user_ptr, 1000);
    case ACT_LORE:
        msg_print(_("石が隠された秘密を写し出した．．．", "The stone reveals hidden mysteries..."));
        return perilous_secrets(user_ptr);
    case ACT_SHIKOFUMI:
        msg_print(_("力強く四股を踏んだ。", "You stamp. (as if you are in a ring.)"));
        (void)set_afraid(user_ptr, 0);
        (void)set_hero(user_ptr, randint1(20) + 20, FALSE);
        (void)dispel_evil(user_ptr, user_ptr->lev * 3);
        return TRUE;
    case ACT_PHASE_DOOR:
        teleport_player(user_ptr, 10, TELEPORT_SPONTANEOUS);
        return TRUE;
    case ACT_DETECT_ALL_MONS:
        (void)detect_monsters_invis(user_ptr, 255);
        (void)detect_monsters_normal(user_ptr, 255);
        return TRUE;
    case ACT_ULTIMATE_RESIST: {
        TIME_EFFECT v = randint1(25) + 25;
        (void)set_afraid(user_ptr, 0);
        (void)set_hero(user_ptr, v, FALSE);
        (void)hp_player(user_ptr, 10);
        (void)set_blessed(user_ptr, v, FALSE);
        (void)set_oppose_acid(user_ptr, v, FALSE);
        (void)set_oppose_elec(user_ptr, v, FALSE);
        (void)set_oppose_fire(user_ptr, v, FALSE);
        (void)set_oppose_cold(user_ptr, v, FALSE);
        (void)set_oppose_pois(user_ptr, v, FALSE);
        (void)set_ultimate_res(user_ptr, v, FALSE);
        return TRUE;
    }
    case ACT_CAST_OFF:
        cosmic_cast_off(user_ptr, o_ptr);
        return TRUE;
    case ACT_FALLING_STAR:
        msg_print(_("あなたは妖刀に魅入られた…", "You are enchanted by cursed blade..."));
        msg_print(_("「狂ほしく 血のごとき 月はのぼれり 秘めおきし 魔剣 いずこぞや」", "'Behold the blade arts.'"));
        massacre(user_ptr);
        return TRUE;
    case ACT_GRAND_CROSS:
        msg_print(_("「闇に還れ！」", "You say, 'Return to darkness!'"));
        project(user_ptr, 0, 8, user_ptr->y, user_ptr->x, (randint1(100) + 200) * 2, GF_HOLY_FIRE, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID, -1);
        return TRUE;
    case ACT_TELEPORT_LEVEL:
        if (!get_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)")))
            return FALSE;

        teleport_level(user_ptr, 0);
        return TRUE;
    case ACT_STRAIN_HASTE:
        msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
        take_hit(user_ptr, DAMAGE_LOSELIFE, damroll(3, 8), _("加速した疲労", "the strain of haste"), -1);
        (void)set_fast(user_ptr, user_ptr->fast + 25 + randint1(25), FALSE);
        return TRUE;
    case ACT_FISHING:
        return fishing(user_ptr);
    case ACT_INROU:
        mitokohmon(user_ptr);
        return TRUE;
    case ACT_MURAMASA:
        if (o_ptr->name1 != ART_MURAMASA)
            return FALSE;

        if (!get_check(_("本当に使いますか？", "Are you sure?!")))
            return TRUE;

        msg_print(_("村正が震えた．．．", "The Muramasa pulsates..."));
        do_inc_stat(user_ptr, A_STR);
        if (one_in_(2)) {
            msg_print(_("村正は壊れた！", "The Muramasa is destroyed!"));
            curse_weapon_object(user_ptr, TRUE, o_ptr);
        }

        return TRUE;
    case ACT_BLOODY_MOON:
        if (o_ptr->name1 != ART_BLOOD)
            return FALSE;

        msg_print(_("鎌が明るく輝いた...", "Your scythe glows brightly!"));
        get_bloody_moon_flags(o_ptr);
        if (user_ptr->prace == RACE_ANDROID)
            calc_android_exp(user_ptr);

        user_ptr->update |= (PU_BONUS | PU_HP);
        return TRUE;
    case ACT_CRIMSON:
        if (o_ptr->name1 != ART_CRIMSON)
            return FALSE;

        msg_print(_("せっかくだから『クリムゾン』をぶっぱなすぜ！", "I'll fire CRIMSON! SEKKAKUDAKARA!"));
        return fire_crimson(user_ptr);
    default:
        msg_format(_("Unknown activation effect: %d.", "Unknown activation effect: %d."), act_ptr->index);
        return FALSE;
    }
}

/*!
 * @brief アイテムの発動効果を処理する。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動実行の是非を返す。
 */
bool activate_artifact(player_type *user_ptr, object_type *o_ptr)
{
    concptr name = k_name + k_info[o_ptr->k_idx].name;
    const activation_type *const act_ptr = find_activation_info(user_ptr, o_ptr);
    if (!act_ptr) {
        msg_print("Activation information is not found.");
        return FALSE;
    }

    if (!switch_activation(user_ptr, o_ptr, act_ptr, name))
        return FALSE;

    if (act_ptr->timeout.constant >= 0) {
        o_ptr->timeout = (s16b)act_ptr->timeout.constant;
        if (act_ptr->timeout.dice > 0)
            o_ptr->timeout += randint1(act_ptr->timeout.dice);

        return TRUE;
    }

    switch (act_ptr->index) {
    case ACT_BR_FIRE:
        o_ptr->timeout = ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) ? 200 : 250;
        return TRUE;
    case ACT_BR_COLD:
        o_ptr->timeout = ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) ? 200 : 250;
        return TRUE;
    case ACT_TERROR:
        o_ptr->timeout = 3 * (user_ptr->lev + 10);
        return TRUE;
    case ACT_MURAMASA:
        return TRUE;
    default:
        msg_format("Special timeout is not implemented: %d.", act_ptr->index);
        return FALSE;
    }
}
