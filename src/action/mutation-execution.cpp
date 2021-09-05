/*!
 * @file mutation-execution.cpp
 * @brief プレイヤーの変異能力実行定義
 */

#include "action/mutation-execution.h"
#include "cmd-item/cmd-throw.h"
#include "core/asking-player.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "game-option/play-record-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/write-diary.h"
#include "mind/mind-mage.h"
#include "mind/mind-warrior.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-techniques.h"
#include "object-enchant/item-feeling.h"
#include "player-info/self-info.h"
#include "player-status/player-energy.h"
#include "player/player-damage.h"
#include "player/player-status.h"
#include "racial/racial-vampire.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-sorcery.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 突然変異のレイシャル効果実装
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param power 発動させる突然変異レイシャルのID
 * @return レイシャルを実行した場合TRUE、キャンセルした場合FALSEを返す
 */
bool exe_mutation_power(player_type *creature_ptr, MUTA power)
{
    DIRECTION dir = 0;
    PLAYER_LEVEL lvl = creature_ptr->lev;
    switch (power) {
    case MUTA::SPIT_ACID:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        stop_mouth(creature_ptr);
        msg_print(_("酸を吐きかけた...", "You spit acid..."));
        fire_ball(creature_ptr, GF_ACID, dir, lvl, 1 + (lvl / 30));
        return true;
    case MUTA::BR_FIRE:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        stop_mouth(creature_ptr);
        msg_print(_("あなたは火炎のブレスを吐いた...", "You breathe fire..."));
        fire_breath(creature_ptr, GF_FIRE, dir, lvl * 2, 1 + (lvl / 20));
        return true;
    case MUTA::HYPN_GAZE:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        msg_print(_("あなたの目は幻惑的になった...", "Your eyes look mesmerizing..."));
        (void)charm_monster(creature_ptr, dir, lvl);
        return true;
    case MUTA::TELEKINES:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        msg_print(_("集中している...", "You concentrate..."));
        fetch_item(creature_ptr, dir, lvl * 10, true);
        return true;
    case MUTA::VTELEPORT:
        msg_print(_("集中している...", "You concentrate..."));
        teleport_player(creature_ptr, 10 + 4 * lvl, TELEPORT_SPONTANEOUS);
        return true;
    case MUTA::MIND_BLST:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        msg_print(_("集中している...", "You concentrate..."));
        fire_bolt(creature_ptr, GF_PSI, dir, damroll(3 + ((lvl - 1) / 5), 3));
        return true;
    case MUTA::RADIATION:
        msg_print(_("体から放射能が発生した！", "Radiation flows from your body!"));
        fire_ball(creature_ptr, GF_NUKE, 0, (lvl * 2), 3 + (lvl / 20));
        return true;
    case MUTA::VAMPIRISM:
        vampirism(creature_ptr);
        return true;
    case MUTA::SMELL_MET:
        stop_mouth(creature_ptr);
        (void)detect_treasure(creature_ptr, DETECT_RAD_DEFAULT);
        return true;
    case MUTA::SMELL_MON:
        stop_mouth(creature_ptr);
        (void)detect_monsters_normal(creature_ptr, DETECT_RAD_DEFAULT);
        return true;
    case MUTA::BLINK:
        teleport_player(creature_ptr, 10, TELEPORT_SPONTANEOUS);
        return true;
    case MUTA::EAT_ROCK:
        return eat_rock(creature_ptr);
    case MUTA::SWAP_POS:
        project_length = -1;
        if (!get_aim_dir(creature_ptr, &dir)) {
            project_length = 0;
            return false;
        }

        (void)teleport_swap(creature_ptr, dir);
        project_length = 0;
        return true;
    case MUTA::SHRIEK:
        stop_mouth(creature_ptr);
        (void)fire_ball(creature_ptr, GF_SOUND, 0, 2 * lvl, 8);
        (void)aggravate_monsters(creature_ptr, 0);
        return true;
    case MUTA::ILLUMINE:
        (void)lite_area(creature_ptr, damroll(2, (lvl / 2)), (lvl / 10) + 1);
        return true;
    case MUTA::DET_CURSE:
        for (int i = 0; i < INVEN_TOTAL; i++) {
            object_type *o_ptr = &creature_ptr->inventory_list[i];
            if ((o_ptr->k_idx == 0) || !o_ptr->is_cursed())
                continue;

            o_ptr->feeling = FEEL_CURSED;
        }

        return true;
    case MUTA::BERSERK:
        (void)berserk(creature_ptr, randint1(25) + 25);
        return true;
    case MUTA::POLYMORPH:
        if (!get_check(_("変身します。よろしいですか？", "You will polymorph your self. Are you sure? ")))
            return false;

        do_poly_self(creature_ptr);
        return true;
    case MUTA::MIDAS_TCH:
        return alchemy(creature_ptr);
    case MUTA::GROW_MOLD:
        for (DIRECTION i = 0; i < 8; i++)
            summon_specific(creature_ptr, -1, creature_ptr->y, creature_ptr->x, lvl, SUMMON_MOLD, PM_FORCE_PET);

        return true;
    case MUTA::RESIST: {
        int num = lvl / 10;
        TIME_EFFECT dur = randint1(20) + 20;
        if (randint0(5) < num) {
            (void)set_oppose_acid(creature_ptr, dur, false);
            num--;
        }

        if (randint0(4) < num) {
            (void)set_oppose_elec(creature_ptr, dur, false);
            num--;
        }

        if (randint0(3) < num) {
            (void)set_oppose_fire(creature_ptr, dur, false);
            num--;
        }

        if (randint0(2) < num) {
            (void)set_oppose_cold(creature_ptr, dur, false);
            num--;
        }

        if (num != 0) {
            (void)set_oppose_pois(creature_ptr, dur, false);
            num--;
        }

        return true;
    }
    case MUTA::EARTHQUAKE:
        (void)earthquake(creature_ptr, creature_ptr->y, creature_ptr->x, 10, 0);
        return true;
    case MUTA::EAT_MAGIC:
        return eat_magic(creature_ptr, creature_ptr->lev * 2);
    case MUTA::WEIGH_MAG:
        report_magics(creature_ptr);
        return true;
    case MUTA::STERILITY:
        msg_print(_("突然頭が痛くなった！", "You suddenly have a headache!"));
        take_hit(creature_ptr, DAMAGE_LOSELIFE, randint1(17) + 17, _("禁欲を強いた疲労", "the strain of forcing abstinence"));
        creature_ptr->current_floor_ptr->num_repro += MAX_REPRO;
        return true;
    case MUTA::HIT_AND_AWAY:
        return hit_and_away(creature_ptr);
    case MUTA::DAZZLE:
        stun_monsters(creature_ptr, lvl * 4);
        confuse_monsters(creature_ptr, lvl * 4);
        turn_monsters(creature_ptr, lvl * 4);
        return true;
    case MUTA::LASER_EYE:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        fire_beam(creature_ptr, GF_LITE, dir, 2 * lvl);
        return true;
    case MUTA::RECALL:
        return recall_player(creature_ptr, randint0(21) + 15);
    case MUTA::BANISH: {
        if (!get_direction(creature_ptr, &dir, false, false))
            return false;

        POSITION y = creature_ptr->y + ddy[dir];
        POSITION x = creature_ptr->x + ddx[dir];
        grid_type *g_ptr;
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

        if (!g_ptr->m_idx) {
            msg_print(_("邪悪な存在を感じとれません！", "You sense no evil there!"));
            return true;
        }

        monster_type *m_ptr;
        m_ptr = &creature_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
        monster_race *r_ptr;
        r_ptr = &r_info[m_ptr->r_idx];
        if ((r_ptr->flags3 & RF3_EVIL) && !(r_ptr->flags1 & RF1_QUESTOR) && !(r_ptr->flags1 & RF1_UNIQUE) && !creature_ptr->current_floor_ptr->inside_arena
            && !creature_ptr->current_floor_ptr->inside_quest && (r_ptr->level < randint1(creature_ptr->lev + 50)) && m_ptr->mflag2.has_not(MFLAG2::NOGENO)) {
            if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(creature_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
                exe_write_diary(creature_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_GENOCIDE, m_name);
            }

            delete_monster_idx(creature_ptr, g_ptr->m_idx);
            msg_print(_("その邪悪なモンスターは硫黄臭い煙とともに消え去った！", "The evil creature vanishes in a puff of sulfurous smoke!"));
            return true;
        }

        msg_print(_("祈りは効果がなかった！", "Your invocation is ineffectual!"));
        if (one_in_(13))
            m_ptr->mflag2.set(MFLAG2::NOGENO);

        return true;
    }
    case MUTA::COLD_TOUCH: {
        if (!get_direction(creature_ptr, &dir, false, false))
            return false;

        POSITION y = creature_ptr->y + ddy[dir];
        POSITION x = creature_ptr->x + ddx[dir];
        grid_type *g_ptr;
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
        if (!g_ptr->m_idx) {
            msg_print(_("あなたは何もない場所で手を振った。", "You wave your hands in the air."));
            return true;
        }

        fire_bolt(creature_ptr, GF_COLD, dir, 2 * lvl);
        return true;
    }
    case MUTA::LAUNCHER: {
        return ThrowCommand(creature_ptr).do_cmd_throw(2 + lvl / 40, false, -1);
    }
    default:
        PlayerEnergy(creature_ptr).reset_player_turn();
        msg_format(_("能力 %s は実装されていません。", "Power %s not implemented. Oops."), power);
        return true;
    }
}
