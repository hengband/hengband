/*!
 * @brief プレイヤーの発動コマンド実装
 * @date 2018/09/07
 * @author deskull
 */

#include "object-activation/activation-switcher.h"
#include "action/action-limited.h"
#include "action/activation-execution.h"
#include "artifact/artifact-info.h"
#include "art-definition/art-bow-types.h"
#include "art-definition/art-sword-types.h"
#include "art-definition/art-weapon-types.h"
#include "art-definition/random-art-effects.h"
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
#include "object-activation/activation-breath.h"
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

/*!
 * @brief アイテムの発動効果を処理する。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動実行の是非を返す。
 */
bool activate_artifact(player_type *user_ptr, object_type *o_ptr)
{
    PLAYER_LEVEL plev = user_ptr->lev;
    DIRECTION dir;
    concptr name = k_name + k_info[o_ptr->k_idx].name;
    const activation_type *const act_ptr = find_activation_info(user_ptr, o_ptr);
    if (!act_ptr) {
        msg_print("Activation information is not found.");
        return FALSE;
    }

    switch (act_ptr->index) {
    case ACT_SUNLIGHT:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        msg_print(_("太陽光線が放たれた。", "A line of sunlight appears."));
        (void)lite_line(user_ptr, dir, damroll(6, 8));
        break;
    case ACT_BO_MISS_1:
        msg_print(_("それは眩しいくらいに明るく輝いている...", "It glows extremely brightly..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_bolt(user_ptr, GF_MISSILE, dir, damroll(2, 6));
        break;
    case ACT_BA_POIS_1:
        msg_print(_("それは濃緑色に脈動している...", "It throbs deep green..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_POIS, dir, 12, 3);
        break;
    case ACT_BO_ELEC_1:
        msg_print(_("それは火花に覆われた...", "It is covered in sparks..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_bolt(user_ptr, GF_ELEC, dir, damroll(4, 8));
        break;
    case ACT_BO_ACID_1:
        msg_print(_("それは酸に覆われた...", "It is covered in acid..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_bolt(user_ptr, GF_ACID, dir, damroll(5, 8));
        break;
    case ACT_BO_COLD_1:
        msg_print(_("それは霜に覆われた...", "It is covered in frost..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_bolt(user_ptr, GF_COLD, dir, damroll(6, 8));
        break;
    case ACT_BO_FIRE_1:
        msg_print(_("それは炎に覆われた...", "It is covered in fire..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_bolt(user_ptr, GF_FIRE, dir, damroll(9, 8));
        break;
    case ACT_BA_COLD_1:
        msg_print(_("それは霜に覆われた...", "It is covered in frost..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_COLD, dir, 48, 2);
        break;
    case ACT_BA_COLD_2:
        msg_print(_("それは青く激しく輝いた...", "It glows an intense blue..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_COLD, dir, 100, 2);
        break;
    case ACT_BA_COLD_3:
        msg_print(_("明るく白色に輝いている...", "It glows bright white..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_COLD, dir, 400, 3);
        break;
    case ACT_BA_FIRE_1:
        msg_print(_("それは赤く激しく輝いた...", "It glows an intense red..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_FIRE, dir, 72, 2);
        break;
    case ACT_BA_FIRE_2:
        msg_format(_("%sから炎が吹き出した...", "The %s rages in fire..."), name);
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_FIRE, dir, 120, 3);
        break;
    case ACT_BA_FIRE_3:
        msg_print(_("深赤色に輝いている...", "It glows deep red..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_FIRE, dir, 300, 3);
        break;
    case ACT_BA_FIRE_4:
        msg_print(_("それは赤く激しく輝いた...", "It glows an intense red..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_FIRE, dir, 100, 2);
        break;
    case ACT_BA_ELEC_2:
        msg_print(_("電気がパチパチ音を立てた...", "It crackles with electricity..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_ELEC, dir, 100, 3);
        break;
    case ACT_BA_ELEC_3:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_ELEC, dir, 500, 3);
        break;
    case ACT_BA_ACID_1:
        msg_print(_("それは黒く激しく輝いた...", "It glows an intense black..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_ACID, dir, 100, 2);
        break;
    case ACT_BA_NUKE_1:
        msg_print(_("それは緑に激しく輝いた...", "It glows an intense green..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_NUKE, dir, 100, 2);
        break;
    case ACT_HYPODYNAMIA_1:
        msg_format(_("あなたは%sに敵を締め殺すよう命じた。", "You order the %s to strangle your opponent."), name);
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        hypodynamic_bolt(user_ptr, dir, 100);
        break;
    case ACT_HYPODYNAMIA_2:
        msg_print(_("黒く輝いている...", "It glows black..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        hypodynamic_bolt(user_ptr, dir, 120);
        break;
    case ACT_DRAIN_1:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        for (int dummy = 0; dummy < 3; dummy++) {
            if (hypodynamic_bolt(user_ptr, dir, 50))
                hp_player(user_ptr, 50);
        }

        break;
    case ACT_BO_MISS_2:
        msg_print(_("魔法のトゲが現れた...", "It grows magical spikes..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_bolt(user_ptr, GF_ARROW, dir, 150);
        break;
    case ACT_WHIRLWIND:
        massacre(user_ptr);
        break;
    case ACT_DRAIN_2:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        for (int dummy = 0; dummy < 3; dummy++)
            if (hypodynamic_bolt(user_ptr, dir, 100))
                hp_player(user_ptr, 100);

        break;
    case ACT_CALL_CHAOS:
        msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
        call_chaos(user_ptr);
        break;
    case ACT_ROCKET:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        msg_print(_("ロケットを発射した！", "You launch a rocket!"));
        fire_ball(user_ptr, GF_ROCKET, dir, 250 + plev * 3, 2);
        break;
    case ACT_DISP_EVIL:
        msg_print(_("神聖な雰囲気が充満した...", "It floods the area with goodness..."));
        dispel_evil(user_ptr, user_ptr->lev * 5);
        break;
    case ACT_BA_MISS_3:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
        fire_breath(user_ptr, GF_MISSILE, dir, 300, 4);
        break;
    case ACT_DISP_GOOD:
        msg_print(_("邪悪な雰囲気が充満した...", "It floods the area with evil..."));
        dispel_good(user_ptr, user_ptr->lev * 5);
        break;
    case ACT_BO_MANA:
        msg_format(_("%sに魔法のトゲが現れた...", "The %s grows magical spikes..."), name);
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_bolt(user_ptr, GF_ARROW, dir, 150);
        break;
    case ACT_BA_WATER:
        msg_format(_("%sが深い青色に鼓動している...", "The %s throbs deep blue..."), name);
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;
        
        fire_ball(user_ptr, GF_WATER, dir, 200, 3);
        break;
    case ACT_BA_DARK:
        msg_format(_("%sが深い闇に覆われた...", "The %s is coverd in pitch-darkness..."), name);
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_DARK, dir, 250, 4);
        break;
    case ACT_BA_MANA:
        msg_format(_("%sが青白く光った．．．", "The %s glows pale..."), name);
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_MANA, dir, 250, 4);
        break;
    case ACT_PESTICIDE:
        msg_print(_("あなたは害虫を一掃した。", "You exterminate small life."));
        (void)dispel_monsters(user_ptr, 4);
        break;
    case ACT_BLINDING_LIGHT:
        msg_format(_("%sが眩しい光で輝いた...", "The %s gleams with blinding light..."), name);
        fire_ball(user_ptr, GF_LITE, 0, 300, 6);
        confuse_monsters(user_ptr, 3 * user_ptr->lev / 2);
        break;
    case ACT_BIZARRE:
        msg_format(_("%sは漆黒に輝いた...", "The %s glows intensely black..."), name);
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        ring_of_power(user_ptr, dir);
        break;
    case ACT_CAST_BA_STAR: {
        HIT_POINT num = damroll(5, 3);
        POSITION y = 0, x = 0;
        int attempts;
        msg_format(_("%sが稲妻で覆われた...", "The %s is surrounded by lightning..."), name);
        for (int k = 0; k < num; k++) {
            attempts = 1000;

            while (attempts--) {
                scatter(user_ptr, &y, &x, user_ptr->y, user_ptr->x, 4, 0);
                if (!cave_have_flag_bold(user_ptr->current_floor_ptr, y, x, FF_PROJECT))
                    continue;
                if (!player_bold(user_ptr, y, x))
                    break;
            }

            project(user_ptr, 0, 3, y, x, 150, GF_ELEC, (PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
        }

        break;
    }
    case ACT_BLADETURNER:
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
        break;
    case ACT_BR_FIRE:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_breath(user_ptr, GF_FIRE, dir, 200, 2);
        if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES))
            (void)set_oppose_fire(user_ptr, randint1(20) + 20, FALSE);

        break;
    case ACT_BR_COLD:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_breath(user_ptr, GF_COLD, dir, 200, 2);
        if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE))
            (void)set_oppose_cold(user_ptr, randint1(20) + 20, FALSE);

        break;
    case ACT_BR_DRAGON:
        if (!activate_dragon_breath(user_ptr, o_ptr))
            return FALSE;

        break;
    case ACT_CONFUSE:
        msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        confuse_monster(user_ptr, dir, 20);
        break;
    case ACT_SLEEP:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        sleep_monsters_touch(user_ptr);
        break;
    case ACT_QUAKE:
        earthquake(user_ptr, user_ptr->y, user_ptr->x, 5, 0);
        break;
    case ACT_TERROR:
        turn_monsters(user_ptr, 40 + user_ptr->lev);
        break;
    case ACT_TELE_AWAY:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        (void)fire_beam(user_ptr, GF_AWAY_ALL, dir, plev);
        break;
    case ACT_BANISH_EVIL:
        if (banish_evil(user_ptr, 100))
            msg_print(_("アーティファクトの力が邪悪を打ち払った！", "The power of the artifact banishes evil!"));

        break;
    case ACT_GENOCIDE:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        (void)symbol_genocide(user_ptr, 200, TRUE);
        break;
    case ACT_MASS_GENO:
        msg_print(_("ひどく鋭い音が流れ出た...", "It lets out a long, shrill note..."));
        (void)mass_genocide(user_ptr, 200, TRUE);
        break;
    case ACT_SCARE_AREA:
        if (music_singing_any(user_ptr))
            stop_singing(user_ptr);

        if (hex_spelling_any(user_ptr))
            stop_hex_spell_all(user_ptr);

        msg_print(_("あなたは力強い突風を吹き鳴らした。周囲の敵が震え上っている!", "You wind a mighty blast; your enemies tremble!"));
        (void)turn_monsters(user_ptr, (3 * user_ptr->lev / 2) + 10);
        break;
    case ACT_AGGRAVATE:
        if (o_ptr->name1 == ART_HYOUSIGI)
            msg_print(_("拍子木を打った。", "You beat your wooden clappers."));
        else
            msg_format(_("%sは不快な物音を立てた。", "The %s sounds an unpleasant noise."), name);

        aggravate_monsters(user_ptr, 0);
        break;
    case ACT_CHARM_ANIMAL:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        (void)charm_animal(user_ptr, dir, plev);
        break;
    case ACT_CHARM_UNDEAD:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        (void)control_one_undead(user_ptr, dir, plev);
        break;
    case ACT_CHARM_OTHER:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        (void)charm_monster(user_ptr, dir, plev * 2);
        break;
    case ACT_CHARM_ANIMALS:
        (void)charm_animals(user_ptr, plev * 2);
        break;
    case ACT_CHARM_OTHERS:
        charm_monsters(user_ptr, plev * 2);
        break;
    case ACT_SUMMON_ANIMAL:
        (void)summon_specific(user_ptr, -1, user_ptr->y, user_ptr->x, plev, SUMMON_ANIMAL_RANGER, (PM_ALLOW_GROUP | PM_FORCE_PET));
        break;
    case ACT_SUMMON_PHANTOM:
        msg_print(_("幻霊を召喚した。", "You summon a phantasmal servant."));
        (void)summon_specific(user_ptr, -1, user_ptr->y, user_ptr->x, user_ptr->current_floor_ptr->dun_level, SUMMON_PHANTOM, (PM_ALLOW_GROUP | PM_FORCE_PET));
        break;
    case ACT_SUMMON_ELEMENTAL:
        if (!cast_summon_elemental(user_ptr, (plev * 3) / 2))
            return FALSE;

        break;
    case ACT_SUMMON_DEMON:
        cast_summon_demon(user_ptr, (plev * 3) / 2);
        break;
    case ACT_SUMMON_UNDEAD:
        if (!cast_summon_undead(user_ptr, (plev * 3) / 2))
            return FALSE;

        break;
    case ACT_SUMMON_HOUND:
        if (!cast_summon_hound(user_ptr, (plev * 3) / 2))
            return FALSE;

        break;
    case ACT_SUMMON_DAWN:
        msg_print(_("暁の師団を召喚した。", "You summon the Legion of the Dawn."));
        (void)summon_specific(user_ptr, -1, user_ptr->y, user_ptr->x, user_ptr->current_floor_ptr->dun_level, SUMMON_DAWN, (PM_ALLOW_GROUP | PM_FORCE_PET));
        break;
    case ACT_SUMMON_OCTOPUS:
        if (!cast_summon_octopus(user_ptr))
            return FALSE;

        break;
    case ACT_CHOIR_SINGS:
        msg_print(_("天国の歌が聞こえる...", "A heavenly choir sings..."));
        (void)cure_critical_wounds(user_ptr, 777);
        (void)set_hero(user_ptr, randint1(25) + 25, FALSE);
        break;
    case ACT_CURE_LW:
        (void)set_afraid(user_ptr, 0);
        (void)hp_player(user_ptr, 30);
        break;
    case ACT_CURE_MW:
        msg_print(_("深紫色の光を発している...", "It radiates deep purple..."));
        (void)cure_serious_wounds(user_ptr, 4, 8);
        break;
    case ACT_CURE_POISON:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        (void)set_afraid(user_ptr, 0);
        (void)set_poisoned(user_ptr, 0);
        break;
    case ACT_REST_EXP:
        msg_print(_("深紅に輝いている...", "It glows a deep red..."));
        restore_level(user_ptr);
        break;
    case ACT_REST_ALL:
        msg_print(_("濃緑色に輝いている...", "It glows a deep green..."));
        (void)restore_all_status(user_ptr);
        (void)restore_level(user_ptr);
        break;
    case ACT_CURE_700:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        msg_print(_("体内に暖かい鼓動が感じられる...", "You feel a warm tingling inside..."));
        (void)cure_critical_wounds(user_ptr, 700);
        break;
    case ACT_CURE_1000:
        msg_print(_("白く明るく輝いている...", "It glows a bright white..."));
        msg_print(_("ひじょうに気分がよい...", "You feel much better..."));
        (void)cure_critical_wounds(user_ptr, 1000);
        break;
    case ACT_CURING:
        msg_format(_("%sの優しさに癒される...", "the %s cures you affectionately ..."), name);
        true_healing(user_ptr, 0);
        break;
    case ACT_CURE_MANA_FULL:
        msg_format(_("%sが青白く光った．．．", "The %s glows pale..."), name);
        restore_mana(user_ptr, TRUE);
        break;
    case ACT_ESP:
        (void)set_tim_esp(user_ptr, randint1(30) + 25, FALSE);
        break;
    case ACT_BERSERK:
        (void)berserk(user_ptr, randint1(25) + 25);
        break;
    case ACT_PROT_EVIL:
        msg_format(_("%sから鋭い音が流れ出た...", "The %s lets out a shrill wail..."), name);
        int k = 3 * user_ptr->lev;
        (void)set_protevil(user_ptr, randint1(25) + k, FALSE);
        break;
    case ACT_RESIST_ALL:
        msg_print(_("様々な色に輝いている...", "It glows many colours..."));
        (void)set_oppose_acid(user_ptr, randint1(40) + 40, FALSE);
        (void)set_oppose_elec(user_ptr, randint1(40) + 40, FALSE);
        (void)set_oppose_fire(user_ptr, randint1(40) + 40, FALSE);
        (void)set_oppose_cold(user_ptr, randint1(40) + 40, FALSE);
        (void)set_oppose_pois(user_ptr, randint1(40) + 40, FALSE);
        break;
    case ACT_SPEED:
        msg_print(_("明るく緑色に輝いている...", "It glows bright green..."));
        (void)set_fast(user_ptr, randint1(20) + 20, FALSE);
        break;
    case ACT_XTRA_SPEED:
        msg_print(_("明るく輝いている...", "It glows brightly..."));
        (void)set_fast(user_ptr, randint1(75) + 75, FALSE);
        break;
    case ACT_WRAITH:
        set_wraith_form(user_ptr, randint1(plev / 2) + (plev / 2), FALSE);
        break;
    case ACT_INVULN:
        (void)set_invuln(user_ptr, randint1(8) + 8, FALSE);
        break;
    case ACT_HERO:
        (void)heroism(user_ptr, 25);
        break;
    case ACT_HERO_SPEED:
        (void)set_fast(user_ptr, randint1(50) + 50, FALSE);
        (void)heroism(user_ptr, 50);
        break;
    case ACT_RESIST_ACID:
        msg_format(_("%sが黒く輝いた...", "The %s grows black."), name);
        if ((o_ptr->tval != TV_RING) || (o_ptr->sval != SV_RING_ACID))
            break;
        
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_ACID, dir, 100, 2);
        (void)set_oppose_acid(user_ptr, randint1(20) + 20, FALSE);
        break;
    case ACT_RESIST_FIRE:
        msg_format(_("%sが赤く輝いた...", "The %s grows red."), name);
        if ((o_ptr->tval != TV_RING) || (o_ptr->sval != SV_RING_FLAMES))
            break;

        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_FIRE, dir, 100, 2);
        (void)set_oppose_fire(user_ptr, randint1(20) + 20, FALSE);
        break;
    case ACT_RESIST_COLD:
        msg_format(_("%sが白く輝いた...", "The %s grows white."), name);
        if ((o_ptr->tval != TV_RING) || (o_ptr->sval != SV_RING_ICE))
            break;
    
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_COLD, dir, 100, 2);
        (void)set_oppose_cold(user_ptr, randint1(20) + 20, FALSE);
        break;
    case ACT_RESIST_ELEC:
        msg_format(_("%sが青く輝いた...", "The %s grows blue."), name);
        if ((o_ptr->tval != TV_RING) || (o_ptr->sval != SV_RING_ELEC))
            break;
        
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        fire_ball(user_ptr, GF_ELEC, dir, 100, 2);
        (void)set_oppose_elec(user_ptr, randint1(20) + 20, FALSE);
        break;
    case ACT_RESIST_POIS:
        msg_format(_("%sが緑に輝いた...", "The %s grows green."), name);
        (void)set_oppose_pois(user_ptr, randint1(20) + 20, FALSE);
        break;
    case ACT_LIGHT:
        msg_format(_("%sから澄んだ光があふれ出た...", "The %s wells with clear light..."), name);
        lite_area(user_ptr, damroll(2, 15), 3);
        break;
    case ACT_MAP_LIGHT:
        msg_print(_("眩しく輝いた...", "It shines brightly..."));
        map_area(user_ptr, DETECT_RAD_MAP);
        lite_area(user_ptr, damroll(2, 15), 3);
        break;
    case ACT_DETECT_ALL:
        msg_print(_("白く明るく輝いている...", "It glows bright white..."));
        msg_print(_("心にイメージが浮かんできた...", "An image forms in your mind..."));
        detect_all(user_ptr, DETECT_RAD_DEFAULT);
        break;
    case ACT_DETECT_XTRA:
        msg_print(_("明るく輝いている...", "It glows brightly..."));
        detect_all(user_ptr, DETECT_RAD_DEFAULT);
        probing(user_ptr);
        identify_fully(user_ptr, FALSE, 0);
        break;
    case ACT_ID_FULL:
        msg_print(_("黄色く輝いている...", "It glows yellow..."));
        identify_fully(user_ptr, FALSE, 0);
        break;
    case ACT_ID_PLAIN:
        if (!ident_spell(user_ptr, FALSE, 0))
            return FALSE;

        break;
    case ACT_RUNE_EXPLO:
        msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
        explosive_rune(user_ptr, user_ptr->y, user_ptr->x);
        break;
    case ACT_RUNE_PROT:
        msg_print(_("ブルーに明るく輝いている...", "It glows light blue..."));
        warding_glyph(user_ptr);
        break;
    case ACT_SATIATE:
        (void)set_food(user_ptr, PY_FOOD_MAX - 1);
        break;
    case ACT_DEST_DOOR:
        msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
        destroy_doors_touch(user_ptr);
        break;
    case ACT_STONE_MUD:
        msg_print(_("鼓動している...", "It pulsates..."));
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        wall_to_mud(user_ptr, dir, 20 + randint1(30));
        break;
    case ACT_RECHARGE:
        recharge(user_ptr, 130);
        break;
    case ACT_ALCHEMY:
        msg_print(_("明るい黄色に輝いている...", "It glows bright yellow..."));
        (void)alchemy(user_ptr);
        break;
    case ACT_DIM_DOOR:
        msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
        if (!dimension_door(user_ptr))
            return FALSE;

        break;
    case ACT_TELEPORT:
        msg_print(_("周りの空間が歪んでいる...", "It twists space around you..."));
        teleport_player(user_ptr, 100, TELEPORT_SPONTANEOUS);
        break;
    case ACT_RECALL:
        msg_print(_("やわらかな白色に輝いている...", "It glows soft white..."));
        if (!recall_player(user_ptr, randint0(21) + 15))
            return FALSE;

        break;
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

        break;
    case ACT_TELEKINESIS:
        if (!get_aim_dir(user_ptr, &dir))
            return FALSE;

        msg_format(_("%sを伸ばした。", "You stretched your %s."), name);
        fetch_item(user_ptr, dir, 500, TRUE);
        break;
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

        break;
    }
    case ACT_ESCAPE:
        switch (randint1(13)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            teleport_player(user_ptr, 10, TELEPORT_SPONTANEOUS);
            break;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            teleport_player(user_ptr, 222, TELEPORT_SPONTANEOUS);
            break;
        case 11:
        case 12:
            (void)stair_creation(user_ptr);
            break;
        default:
            if (!get_check(_("この階を去りますか？", "Leave this level? ")))
                break;
            
            if (autosave_l)
                do_cmd_save_game(user_ptr, TRUE);

            user_ptr->leaving = TRUE;
            break;
        }
    case ACT_DISP_CURSE_XTRA:
        msg_format(_("%sが真実を照らし出す...", "The %s exhibits the truth..."), name);
        (void)remove_all_curse(user_ptr);
        (void)probing(user_ptr);
        break;
    case ACT_BRAND_FIRE_BOLTS:
        msg_format(_("%sが深紅に輝いた...", "Your %s glows deep red..."), name);
        brand_bolts(user_ptr);
        break;
    case ACT_RECHARGE_XTRA:
        msg_format(_("%sが白く輝いた．．．", "The %s gleams with blinding light..."), name);
        if (!recharge(user_ptr, 1000))
            return FALSE;

        break;
    case ACT_LORE:
        msg_print(_("石が隠された秘密を写し出した．．．", "The stone reveals hidden mysteries..."));
        if (!perilous_secrets(user_ptr))
            return FALSE;
        
        break;
    case ACT_SHIKOFUMI:
        msg_print(_("力強く四股を踏んだ。", "You stamp. (as if you are in a ring.)"));
        (void)set_afraid(user_ptr, 0);
        (void)set_hero(user_ptr, randint1(20) + 20, FALSE);
        dispel_evil(user_ptr, user_ptr->lev * 3);
        break;
    case ACT_PHASE_DOOR:
        teleport_player(user_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case ACT_DETECT_ALL_MONS:
        (void)detect_monsters_invis(user_ptr, 255);
        (void)detect_monsters_normal(user_ptr, 255);
        break;
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
        break;
    }
    case ACT_CAST_OFF:
        cosmic_cast_off(user_ptr, o_ptr);
        break;
    case ACT_FALLING_STAR:
        msg_print(_("あなたは妖刀に魅入られた…", "You are enchanted by cursed blade..."));
        msg_print(_("「狂ほしく 血のごとき 月はのぼれり 秘めおきし 魔剣 いずこぞや」", "'Behold the blade arts.'"));
        massacre(user_ptr);
        break;
    case ACT_GRAND_CROSS:
        msg_print(_("「闇に還れ！」", "You say, 'Return to darkness!'"));
        project(user_ptr, 0, 8, user_ptr->y, user_ptr->x, (randint1(100) + 200) * 2, GF_HOLY_FIRE, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID, -1);
        break;
    case ACT_TELEPORT_LEVEL:
        if (!get_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)")))
            return FALSE;

        teleport_level(user_ptr, 0);
        break;
    case ACT_STRAIN_HASTE: {
        msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
        take_hit(user_ptr, DAMAGE_LOSELIFE, damroll(3, 8), _("加速した疲労", "the strain of haste"), -1);
        int t = 25 + randint1(25);
        (void)set_fast(user_ptr, user_ptr->fast + t, FALSE);
        break;
    }
    case ACT_FISHING:
        if (!fishing(user_ptr))
            return FALSE;

        break;
    case ACT_INROU:
        mitokohmon(user_ptr);
        break;
    case ACT_MURAMASA:
        if (o_ptr->name1 != ART_MURAMASA)
            return FALSE;

        if (!get_check(_("本当に使いますか？", "Are you sure?!")))
            break;

        msg_print(_("村正が震えた．．．", "The Muramasa pulsates..."));
        do_inc_stat(user_ptr, A_STR);
        if (one_in_(2)) {
            msg_print(_("村正は壊れた！", "The Muramasa is destroyed!"));
            curse_weapon_object(user_ptr, TRUE, o_ptr);
        }

        break;
    case ACT_BLOODY_MOON:
        if (o_ptr->name1 != ART_BLOOD)
            return FALSE;

        msg_print(_("鎌が明るく輝いた...", "Your scythe glows brightly!"));
        get_bloody_moon_flags(o_ptr);
        if (user_ptr->prace == RACE_ANDROID)
            calc_android_exp(user_ptr);

        user_ptr->update |= (PU_BONUS | PU_HP);
        break;
    case ACT_CRIMSON:
        if (o_ptr->name1 != ART_CRIMSON)
            return FALSE;

        msg_print(_("せっかくだから『クリムゾン』をぶっぱなすぜ！", "I'll fire CRIMSON! SEKKAKUDAKARA!"));
        if (!fire_crimson(user_ptr))
            return FALSE;

        break;
    default:
        msg_format(_("Unknown activation effect: %d.", "Unknown activation effect: %d."), act_ptr->index);
        return FALSE;
    }

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
