/*!
 * @brief モンスターから直接攻撃を受けた時に、プレイヤーのオーラダメージで反撃する処理
 * @date 2020/05/31
 * @author Hourier
 */

#include "combat/aura-counterattack.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "monster-attack/monster-attack-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-damage.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "object-hook/hook-armor.h"
#include "player/player-status-flags.h"
#include "realm/realm-hex-numbers.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "effect/attribute-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

static void aura_fire_by_monster_attack(PlayerType *player_ptr, monap_type *monap_ptr)
{
    if (!has_sh_fire(player_ptr) || !monap_ptr->alive || player_ptr->is_dead)
        return;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if ((r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK) != 0) {
        if (is_original_ap_and_seen(player_ptr, monap_ptr->m_ptr))
            r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);

        return;
    }

    HIT_POINT dam = damroll(2, 6);
    dam = mon_damage_mod(player_ptr, monap_ptr->m_ptr, dam, false);
    msg_format(_("%^sは突然熱くなった！", "%^s is suddenly very hot!"), monap_ptr->m_name);
    MonsterDamageProcessor mdp(player_ptr, monap_ptr->m_idx, dam, &monap_ptr->fear, AttributeType::FIRE);
    if (mdp.mon_take_hit(_("は灰の山になった。", " turns into a pile of ash."))) {
        monap_ptr->blinked = false;
        monap_ptr->alive = false;
    }
}

static void aura_elec_by_monster_attack(PlayerType *player_ptr, monap_type *monap_ptr)
{
    if (!has_sh_elec(player_ptr) || !monap_ptr->alive || player_ptr->is_dead)
        return;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if ((r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK) != 0) {
        if (is_original_ap_and_seen(player_ptr, monap_ptr->m_ptr))
            r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);

        return;
    }

    HIT_POINT dam = damroll(2, 6);
    dam = mon_damage_mod(player_ptr, monap_ptr->m_ptr, dam, false);
    msg_format(_("%^sは電撃をくらった！", "%^s gets zapped!"), monap_ptr->m_name);
    MonsterDamageProcessor mdp(player_ptr, monap_ptr->m_idx, dam, &monap_ptr->fear, AttributeType::ELEC);
    if (mdp.mon_take_hit(_("は燃え殻の山になった。", " turns into a pile of cinders."))) {
        monap_ptr->blinked = false;
        monap_ptr->alive = false;
    }
}

static void aura_cold_by_monster_attack(PlayerType *player_ptr, monap_type *monap_ptr)
{
    if (!has_sh_cold(player_ptr) || !monap_ptr->alive || player_ptr->is_dead)
        return;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if ((r_ptr->flagsr & RFR_EFF_IM_COLD_MASK) != 0) {
        if (is_original_ap_and_seen(player_ptr, monap_ptr->m_ptr))
            r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);

        return;
    }

    HIT_POINT dam = damroll(2, 6);
    dam = mon_damage_mod(player_ptr, monap_ptr->m_ptr, dam, false);
    msg_format(_("%^sは冷気をくらった！", "%^s is very cold!"), monap_ptr->m_name);
    MonsterDamageProcessor mdp(player_ptr, monap_ptr->m_idx, dam, &monap_ptr->fear, AttributeType::COLD);
    if (mdp.mon_take_hit(_("は凍りついた。", " was frozen."))) {
        monap_ptr->blinked = false;
        monap_ptr->alive = false;
    }
}

static void aura_shards_by_monster_attack(PlayerType *player_ptr, monap_type *monap_ptr)
{
    if (!player_ptr->dustrobe || !monap_ptr->alive || player_ptr->is_dead)
        return;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if ((r_ptr->flagsr & RFR_EFF_RES_SHAR_MASK) != 0) {
        if (is_original_ap_and_seen(player_ptr, monap_ptr->m_ptr))
            r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_RES_SHAR_MASK);
    } else {
        HIT_POINT dam = damroll(2, 6);
        dam = mon_damage_mod(player_ptr, monap_ptr->m_ptr, dam, false);
        msg_format(_("%^sは鏡の破片をくらった！", "%^s gets sliced!"), monap_ptr->m_name);
        MonsterDamageProcessor mdp(player_ptr, monap_ptr->m_idx, dam, &monap_ptr->fear, AttributeType::SHARDS);
        if (mdp.mon_take_hit(_("はズタズタになった。", " is torn to pieces."))) {
            monap_ptr->blinked = false;
            monap_ptr->alive = false;
        }
    }

    if (player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].is_mirror())
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
}

static void aura_holy_by_monster_attack(PlayerType *player_ptr, monap_type *monap_ptr)
{
    if (!player_ptr->tim_sh_holy || !monap_ptr->alive || player_ptr->is_dead)
        return;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if ((r_ptr->flags3 & RF3_EVIL) == 0)
        return;

    if ((r_ptr->flagsr & RFR_RES_ALL) != 0) {
        if (is_original_ap_and_seen(player_ptr, monap_ptr->m_ptr))
            r_ptr->r_flagsr |= RFR_RES_ALL;

        return;
    }

    HIT_POINT dam = damroll(2, 6);
    dam = mon_damage_mod(player_ptr, monap_ptr->m_ptr, dam, false);
    msg_format(_("%^sは聖なるオーラで傷ついた！", "%^s is injured by holy power!"), monap_ptr->m_name);
    MonsterDamageProcessor mdp(player_ptr, monap_ptr->m_idx, dam, &monap_ptr->fear, AttributeType::HOLY_FIRE);
    if (mdp.mon_take_hit(_("は倒れた。", " is destroyed."))) {
        monap_ptr->blinked = false;
        monap_ptr->alive = false;
    }

    if (is_original_ap_and_seen(player_ptr, monap_ptr->m_ptr))
        r_ptr->r_flags3 |= RF3_EVIL;
}

static void aura_force_by_monster_attack(PlayerType *player_ptr, monap_type *monap_ptr)
{
    if (!player_ptr->tim_sh_touki || !monap_ptr->alive || player_ptr->is_dead)
        return;

    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if ((r_ptr->flagsr & RFR_RES_ALL) != 0) {
        if (is_original_ap_and_seen(player_ptr, monap_ptr->m_ptr))
            r_ptr->r_flagsr |= RFR_RES_ALL;

        return;
    }

    HIT_POINT dam = damroll(2, 6);
    dam = mon_damage_mod(player_ptr, monap_ptr->m_ptr, dam, false);
    msg_format(_("%^sが鋭い闘気のオーラで傷ついた！", "%^s is injured by the Force"), monap_ptr->m_name);
    MonsterDamageProcessor mdp(player_ptr, monap_ptr->m_idx, dam, &monap_ptr->fear, AttributeType::MANA);
    if (mdp.mon_take_hit(_("は倒れた。", " is destroyed."))) {
        monap_ptr->blinked = false;
        monap_ptr->alive = false;
    }
}

static void aura_shadow_by_monster_attack(PlayerType *player_ptr, monap_type *monap_ptr)
{
    if (!SpellHex(player_ptr).is_spelling_specific(HEX_SHADOW_CLOAK) || !monap_ptr->alive || player_ptr->is_dead)
        return;

    HIT_POINT dam = 1;
    object_type *o_armed_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND];
    monster_race *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (((r_ptr->flagsr & RFR_RES_ALL) != 0) || ((r_ptr->flagsr & RFR_RES_DARK) != 0)) {
        if (is_original_ap_and_seen(player_ptr, monap_ptr->m_ptr))
            r_ptr->r_flagsr |= (RFR_RES_ALL | RFR_RES_DARK);

        return;
    }

    if (o_armed_ptr->k_idx) {
        int basedam = ((o_armed_ptr->dd + player_ptr->to_dd[0]) * (o_armed_ptr->ds + player_ptr->to_ds[0] + 1));
        dam = basedam / 2 + o_armed_ptr->to_d + player_ptr->to_d[0];
    }

    o_armed_ptr = &player_ptr->inventory_list[INVEN_BODY];
    if ((o_armed_ptr->k_idx) && o_armed_ptr->is_cursed())
        dam *= 2;

    dam = mon_damage_mod(player_ptr, monap_ptr->m_ptr, dam, false);
    msg_format(_("影のオーラが%^sに反撃した！", "Enveloping shadows attack %^s."), monap_ptr->m_name);
    MonsterDamageProcessor mdp(player_ptr, monap_ptr->m_idx, dam, &monap_ptr->fear, AttributeType::DARK);
    if (mdp.mon_take_hit(_("は倒れた。", " is destroyed."))) {
        monap_ptr->blinked = false;
        monap_ptr->alive = false;
        return;
    }
    
    struct attribute_table {
        inventory_slot_type slot;
        AttributeType type;
    };

    const int TABLE_SIZE = 4;
    attribute_table table[TABLE_SIZE] = { { INVEN_HEAD, AttributeType::OLD_CONF }, { INVEN_SUB_HAND, AttributeType::OLD_SLEEP }, 
                            { INVEN_ARMS, AttributeType::TURN_ALL }, { INVEN_FEET, AttributeType::OLD_SLOW } };

    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

    /* Some cursed armours gives an extra effect */
    for (int j = 0; j < TABLE_SIZE; j++) {
        o_armed_ptr = &player_ptr->inventory_list[table[j].slot];
        if ((o_armed_ptr->k_idx) && o_armed_ptr->is_cursed() && o_armed_ptr->is_armour())
            project(player_ptr, 0, 0, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, (player_ptr->lev * 2), table[j].type, flg);
    }
}

void process_aura_counterattack(PlayerType *player_ptr, monap_type *monap_ptr)
{
    if (!monap_ptr->touched)
        return;

    aura_fire_by_monster_attack(player_ptr, monap_ptr);
    aura_elec_by_monster_attack(player_ptr, monap_ptr);
    aura_cold_by_monster_attack(player_ptr, monap_ptr);
    aura_shards_by_monster_attack(player_ptr, monap_ptr);
    aura_holy_by_monster_attack(player_ptr, monap_ptr);
    aura_force_by_monster_attack(player_ptr, monap_ptr);
    aura_shadow_by_monster_attack(player_ptr, monap_ptr);
}
