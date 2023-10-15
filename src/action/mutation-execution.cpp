/*!
 * @file mutation-execution.cpp
 * @brief プレイヤーの変異能力実行定義
 */

#include "action/mutation-execution.h"
#include "cmd-item/cmd-throw.h"
#include "core/asking-player.h"
#include "dungeon/quest.h"
#include "effect/attribute-types.h"
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
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 突然変異のレイシャル効果実装
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param power 発動させる突然変異レイシャルのID
 * @return レイシャルを実行した場合TRUE、キャンセルした場合FALSEを返す
 */
bool exe_mutation_power(PlayerType *player_ptr, PlayerMutationType power)
{
    DIRECTION dir = 0;
    PLAYER_LEVEL lvl = player_ptr->lev;
    auto &floor = *player_ptr->current_floor_ptr;
    switch (power) {
    case PlayerMutationType::SPIT_ACID:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        stop_mouth(player_ptr);
        msg_print(_("酸を吐きかけた...", "You spit acid..."));
        fire_ball(player_ptr, AttributeType::ACID, dir, lvl, 1 + (lvl / 30));
        return true;
    case PlayerMutationType::BR_FIRE:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        stop_mouth(player_ptr);
        msg_print(_("あなたは火炎のブレスを吐いた...", "You breathe fire..."));
        fire_breath(player_ptr, AttributeType::FIRE, dir, lvl * 2, 1 + (lvl / 20));
        return true;
    case PlayerMutationType::HYPN_GAZE:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        msg_print(_("あなたの目は幻惑的になった...", "Your eyes look mesmerizing..."));
        (void)charm_monster(player_ptr, dir, lvl);
        return true;
    case PlayerMutationType::TELEKINES:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        msg_print(_("集中している...", "You concentrate..."));
        fetch_item(player_ptr, dir, lvl * 10, true);
        return true;
    case PlayerMutationType::VTELEPORT:
        msg_print(_("集中している...", "You concentrate..."));
        teleport_player(player_ptr, 10 + 4 * lvl, TELEPORT_SPONTANEOUS);
        return true;
    case PlayerMutationType::MIND_BLST:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        msg_print(_("集中している...", "You concentrate..."));
        fire_bolt(player_ptr, AttributeType::PSI, dir, damroll(3 + ((lvl - 1) / 5), 3));
        return true;
    case PlayerMutationType::RADIATION:
        msg_print(_("体から放射能が発生した！", "Radiation flows from your body!"));
        fire_ball(player_ptr, AttributeType::NUKE, 0, (lvl * 2), 3 + (lvl / 20));
        return true;
    case PlayerMutationType::VAMPIRISM:
        vampirism(player_ptr);
        return true;
    case PlayerMutationType::SMELL_MET:
        stop_mouth(player_ptr);
        (void)detect_treasure(player_ptr, DETECT_RAD_DEFAULT);
        return true;
    case PlayerMutationType::SMELL_MON:
        stop_mouth(player_ptr);
        (void)detect_monsters_normal(player_ptr, DETECT_RAD_DEFAULT);
        return true;
    case PlayerMutationType::BLINK:
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
        return true;
    case PlayerMutationType::EAT_ROCK:
        return eat_rock(player_ptr);
    case PlayerMutationType::SWAP_POS:
        project_length = -1;
        if (!get_aim_dir(player_ptr, &dir)) {
            project_length = 0;
            return false;
        }

        (void)teleport_swap(player_ptr, dir);
        project_length = 0;
        return true;
    case PlayerMutationType::SHRIEK:
        stop_mouth(player_ptr);
        (void)fire_ball(player_ptr, AttributeType::SOUND, 0, 2 * lvl, 8);
        (void)aggravate_monsters(player_ptr, 0);
        return true;
    case PlayerMutationType::ILLUMINE:
        (void)lite_area(player_ptr, damroll(2, (lvl / 2)), (lvl / 10) + 1);
        return true;
    case PlayerMutationType::DET_CURSE:
        for (int i = 0; i < INVEN_TOTAL; i++) {
            auto *o_ptr = &player_ptr->inventory_list[i];
            if (!o_ptr->is_valid() || !o_ptr->is_cursed()) {
                continue;
            }

            o_ptr->feeling = FEEL_CURSED;
        }

        return true;
    case PlayerMutationType::BERSERK:
        (void)berserk(player_ptr, randint1(25) + 25);
        return true;
    case PlayerMutationType::POLYMORPH:
        if (!input_check(_("変身します。よろしいですか？", "You will polymorph your self. Are you sure? "))) {
            return false;
        }

        do_poly_self(player_ptr);
        return true;
    case PlayerMutationType::MIDAS_TCH:
        return alchemy(player_ptr);
    case PlayerMutationType::GROW_MOLD:
        for (DIRECTION i = 0; i < 8; i++) {
            summon_specific(player_ptr, -1, player_ptr->y, player_ptr->x, lvl, SUMMON_MOLD, PM_FORCE_PET);
        }

        return true;
    case PlayerMutationType::RESIST: {
        int num = lvl / 10;
        TIME_EFFECT dur = randint1(20) + 20;
        if (randint0(5) < num) {
            (void)set_oppose_acid(player_ptr, dur, false);
            num--;
        }

        if (randint0(4) < num) {
            (void)set_oppose_elec(player_ptr, dur, false);
            num--;
        }

        if (randint0(3) < num) {
            (void)set_oppose_fire(player_ptr, dur, false);
            num--;
        }

        if (randint0(2) < num) {
            (void)set_oppose_cold(player_ptr, dur, false);
            num--;
        }

        if (num != 0) {
            (void)set_oppose_pois(player_ptr, dur, false);
            num--;
        }

        return true;
    }
    case PlayerMutationType::EARTHQUAKE:
        (void)earthquake(player_ptr, player_ptr->y, player_ptr->x, 10, 0);
        return true;
    case PlayerMutationType::EAT_MAGIC:
        return eat_magic(player_ptr, player_ptr->lev * 2);
    case PlayerMutationType::WEIGH_MAG:
        report_magics(player_ptr);
        return true;
    case PlayerMutationType::STERILITY:
        msg_print(_("突然頭が痛くなった！", "You suddenly have a headache!"));
        take_hit(player_ptr, DAMAGE_LOSELIFE, randint1(17) + 17, _("禁欲を強いた疲労", "the strain of forcing abstinence"));
        floor.num_repro += MAX_REPRODUCTION;
        return true;
    case PlayerMutationType::HIT_AND_AWAY:
        return hit_and_away(player_ptr);
    case PlayerMutationType::DAZZLE:
        stun_monsters(player_ptr, lvl * 4);
        confuse_monsters(player_ptr, lvl * 4);
        turn_monsters(player_ptr, lvl * 4);
        return true;
    case PlayerMutationType::LASER_EYE:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        fire_beam(player_ptr, AttributeType::LITE, dir, 2 * lvl);
        return true;
    case PlayerMutationType::RECALL:
        return recall_player(player_ptr, randint0(21) + 15);
    case PlayerMutationType::BANISH: {
        if (!get_direction(player_ptr, &dir)) {
            return false;
        }

        const auto y = player_ptr->y + ddy[dir];
        const auto x = player_ptr->x + ddx[dir];
        const auto &grid = floor.grid_array[y][x];
        if (!grid.m_idx) {
            msg_print(_("邪悪な存在を感じとれません！", "You sense no evil there!"));
            return true;
        }

        auto &monster = floor.m_list[grid.m_idx];
        const auto &monrace = monster.get_monrace();
        auto can_banish = monrace.kind_flags.has(MonsterKindType::EVIL);
        can_banish &= none_bits(monrace.flags1, RF1_QUESTOR);
        can_banish &= monrace.kind_flags.has_not(MonsterKindType::UNIQUE);
        can_banish &= !floor.inside_arena;
        can_banish &= !floor.is_in_quest();
        can_banish &= (monrace.level < randint1(player_ptr->lev + 50));
        can_banish &= monster.mflag2.has_not(MonsterConstantFlagType::NOGENO);
        if (can_banish) {
            if (record_named_pet && monster.is_named_pet()) {
                const auto m_name = monster_desc(player_ptr, &monster, MD_INDEF_VISIBLE);
                exe_write_diary(player_ptr, DiaryKind::NAMED_PET, RECORD_NAMED_PET_GENOCIDE, m_name);
            }

            delete_monster_idx(player_ptr, grid.m_idx);
            msg_print(_("その邪悪なモンスターは硫黄臭い煙とともに消え去った！", "The evil creature vanishes in a puff of sulfurous smoke!"));
            return true;
        }

        msg_print(_("祈りは効果がなかった！", "Your invocation is ineffectual!"));
        if (one_in_(13)) {
            monster.mflag2.set(MonsterConstantFlagType::NOGENO);
        }

        return true;
    }
    case PlayerMutationType::COLD_TOUCH: {
        if (!get_direction(player_ptr, &dir)) {
            return false;
        }

        const auto y = player_ptr->y + ddy[dir];
        const auto x = player_ptr->x + ddx[dir];
        auto &grid = floor.grid_array[y][x];
        if (!grid.m_idx) {
            msg_print(_("あなたは何もない場所で手を振った。", "You wave your hands in the air."));
            return true;
        }

        fire_bolt(player_ptr, AttributeType::COLD, dir, 2 * lvl);
        return true;
    }
    case PlayerMutationType::LAUNCHER:
        return ThrowCommand(player_ptr).do_cmd_throw(2 + lvl / 40, false, -1);
    default:
        PlayerEnergy(player_ptr).reset_player_turn();
        msg_format(_("能力 PlayerMutationType::%d は実装されていません。", "Power PlayerMutationType::%d not implemented. Oops."), enum2i(power));
        return true;
    }
}
