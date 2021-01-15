#include "player/player-status.h"
#include "artifact/fixed-art-types.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-item/cmd-magiceat.h"
#include "combat/attack-power-table.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-leaver.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena-info-table.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "mutation/mutation-calculator.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "pet/pet-util.h"
#include "player-info/avatar.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/mimic-info-table.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-personalities-types.h"
#include "player/player-personality.h"
#include "player/player-race-types.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/player-view.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-names-table.h"
#include "realm/realm-song-numbers.h"
#include "specific-object/bow.h"
#include "specific-object/torch.h"
#include "spell-realm/spells-hex.h"
#include "spell/range-calc.h"
#include "spell/spells-describer.h"
#include "spell/spells-execution.h"
#include "spell/spells-status.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "status/base-status.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/angband.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

static bool is_martial_arts_mode(player_type *creature_ptr);

static ACTION_SKILL_POWER calc_intra_vision(player_type *creature_ptr);
static ACTION_SKILL_POWER calc_stealth(player_type *creature_ptr);
static ACTION_SKILL_POWER calc_disarming(player_type *creature_ptr);
static ACTION_SKILL_POWER calc_device_ability(player_type *creature_ptr);
static ACTION_SKILL_POWER calc_saving_throw(player_type *creature_ptr);
static ACTION_SKILL_POWER calc_search(player_type *creature_ptr);
static ACTION_SKILL_POWER calc_search_freq(player_type *creature_ptr);
static ACTION_SKILL_POWER calc_to_hit_melee(player_type *creature_ptr);
static ACTION_SKILL_POWER calc_to_hit_shoot(player_type *creature_ptr);
static ACTION_SKILL_POWER calc_to_hit_throw(player_type *creature_ptr);
static ACTION_SKILL_POWER calc_skill_dig(player_type *creature_ptr);
static s16b calc_num_blow(player_type *creature_ptr, int i);
static s16b calc_strength_addition(player_type *creature_ptr);
static s16b calc_intelligence_addition(player_type *creature_ptr);
static s16b calc_wisdom_addition(player_type *creature_ptr);
static s16b calc_dexterity_addition(player_type *creature_ptr);
static s16b calc_constitution_addition(player_type *creature_ptr);
static s16b calc_charisma_addition(player_type *creature_ptr);
static s16b calc_to_magic_chance(player_type *creature_ptr);
static ARMOUR_CLASS calc_base_ac(player_type *creature_ptr);
static ARMOUR_CLASS calc_to_ac(player_type *creature_ptr, bool is_true_value);
static s16b calc_speed(player_type *creature_ptr);
static s16b calc_double_weapon_penalty(player_type *creature_ptr, INVENTORY_IDX slot);
static void calc_use_status(player_type *creature_ptr, int status);
static void calc_top_status(player_type *creature_ptr, int status);
static void calc_ind_status(player_type *creature_ptr, int status);
static s16b calc_riding_bow_penalty(player_type *creature_ptr);
static void put_equipment_warning(player_type *creature_ptr);

static s16b calc_to_damage(player_type *creature_ptr, INVENTORY_IDX slot, bool is_true_value);
static s16b calc_to_hit(player_type *creature_ptr, INVENTORY_IDX slot, bool is_true_value);

static s16b calc_to_hit_bow(player_type *creature_ptr, bool is_true_value);

static s16b calc_to_damage_misc(player_type *creature_ptr);
static s16b calc_to_hit_misc(player_type *creature_ptr);

static DICE_NUMBER calc_to_weapon_dice_num(player_type *creature_ptr, INVENTORY_IDX slot);
static DICE_NUMBER calc_to_weapon_dice_side(player_type *creature_ptr, INVENTORY_IDX slot);

static int get_default_hand(player_type *creature_ptr);

/*** Player information ***/

/*!
 * @brief プレイヤー用のクリーチャー構造体実体 / Static player info record
 */
player_type p_body;

/*!
 * @brief プレイヤー用のクリーチャー構造体参照ポインタ / Pointer to the player info
 */
player_type *p_ptr = &p_body;

/*!
 * @brief クリーチャーの抽象的善悪アライメントの表記を返す。 / Return alignment title
 * @param creature_ptr 算出するクリーチャーの参照ポインタ。
 * @return アライメントの表記を返す。
 */
concptr your_alignment(player_type *creature_ptr)
{
    if (creature_ptr->align > 150)
        return _("大善", "Lawful");
    else if (creature_ptr->align > 50)
        return _("中善", "Good");
    else if (creature_ptr->align > 10)
        return _("小善", "Neutral Good");
    else if (creature_ptr->align > -11)
        return _("中立", "Neutral");
    else if (creature_ptr->align > -51)
        return _("小悪", "Neutral Evil");
    else if (creature_ptr->align > -151)
        return _("中悪", "Evil");
    else
        return _("大悪", "Chaotic");
}

/*!
 * @brief 武器や各種スキル（騎乗以外）の抽象的表現ランクを返す。 /  Return proficiency level of weapons and misc. skills (except riding)
 * @param weapon_exp 経験値
 * @return ランク値
 */
int weapon_exp_level(int weapon_exp)
{
    if (weapon_exp < WEAPON_EXP_BEGINNER)
        return EXP_LEVEL_UNSKILLED;
    else if (weapon_exp < WEAPON_EXP_SKILLED)
        return EXP_LEVEL_BEGINNER;
    else if (weapon_exp < WEAPON_EXP_EXPERT)
        return EXP_LEVEL_SKILLED;
    else if (weapon_exp < WEAPON_EXP_MASTER)
        return EXP_LEVEL_EXPERT;
    else
        return EXP_LEVEL_MASTER;
}

/*!
 * @brief 騎乗スキルの抽象的ランクを返す。 / Return proficiency level of riding
 * @param weapon_exp 経験値
 * @return ランク値
 */
int riding_exp_level(int riding_exp)
{
    if (riding_exp < RIDING_EXP_BEGINNER)
        return EXP_LEVEL_UNSKILLED;
    else if (riding_exp < RIDING_EXP_SKILLED)
        return EXP_LEVEL_BEGINNER;
    else if (riding_exp < RIDING_EXP_EXPERT)
        return EXP_LEVEL_SKILLED;
    else if (riding_exp < RIDING_EXP_MASTER)
        return EXP_LEVEL_EXPERT;
    else
        return EXP_LEVEL_MASTER;
}

/*!
 * @brief クリーチャーの呪文レベルの抽象的ランクを返す。 / Return proficiency level of spells
 * @param spell_exp 経験値
 * @return ランク値
 */
int spell_exp_level(int spell_exp)
{
    if (spell_exp < SPELL_EXP_BEGINNER)
        return EXP_LEVEL_UNSKILLED;
    else if (spell_exp < SPELL_EXP_SKILLED)
        return EXP_LEVEL_BEGINNER;
    else if (spell_exp < SPELL_EXP_EXPERT)
        return EXP_LEVEL_SKILLED;
    else if (spell_exp < SPELL_EXP_MASTER)
        return EXP_LEVEL_EXPERT;
    else
        return EXP_LEVEL_MASTER;
}

/*!
 * @brief 遅延描画更新 / Delayed visual update
 * @details update_view(), update_lite(), update_mon_lite() においてのみ更新すること / Only used if update_view(), update_lite() or update_mon_lite() was called
 * @param player_ptr 主観となるプレイヤー構造体参照ポインタ
 * @todo 将来独自インターフェース実装にはz-term系に追い出すべきか？
 * @return なし
 */
static void delayed_visual_update(player_type *player_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = 0; i < floor_ptr->redraw_n; i++) {
        POSITION y = floor_ptr->redraw_y[i];
        POSITION x = floor_ptr->redraw_x[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        if (!(g_ptr->info & CAVE_REDRAW))
            continue;

        if (g_ptr->info & CAVE_NOTE)
            note_spot(player_ptr, y, x);

        lite_spot(player_ptr, y, x);
        if (g_ptr->m_idx)
            update_monster(player_ptr, g_ptr->m_idx, FALSE);

        g_ptr->info &= ~(CAVE_NOTE | CAVE_REDRAW);
    }

    floor_ptr->redraw_n = 0;
}

/*!
 * @brief 射撃武器がプレイヤーにとって重すぎるかどうかの判定 /
 * @param o_ptr 判定する射撃武器のアイテム情報参照ポインタ
 * @return 重すぎるならばTRUE
 */
static bool is_heavy_shoot(player_type *creature_ptr, object_type *o_ptr)
{
    int hold = adj_str_hold[creature_ptr->stat_ind[A_STR]];
    return (hold < o_ptr->weight / 10);
}

/*!
 * @brief 所持品総重量を計算する
 * @param creature_ptr 計算対象となるクリーチャーの参照ポインタ
 * @return 総重量
 */
WEIGHT calc_inventory_weight(player_type *creature_ptr)
{
    WEIGHT weight = 0;

    object_type *o_ptr;
    for (inventory_slot_type i = 0; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        weight += o_ptr->weight * o_ptr->number;
    }
    return weight;
}
/*!
 * @brief プレイヤーの全ステータスを更新する /
 * Calculate the players current "state", taking into account
 * not only race/class intrinsics, but also objects being worn
 * and temporary spell effects.
 * @return なし
 * @details
 * <pre>
 * See also calc_mana() and calc_hitpoints().
 *
 * Take note of the new "speed code", in particular, a very strong
 * player will start slowing down as soon as he reaches 150 pounds,
 * but not until he reaches 450 pounds will he be half as fast as
 * a normal kobold.  This both hurts and helps the player, hurts
 * because in the old days a player could just avoid 300 pounds,
 * and helps because now carrying 300 pounds is not very painful.
 *
 * The "weapon" and "bow" do *not* add to the bonuses to hit or to
 * damage, since that would affect non-combat things.  These values
 * are actually added in later, at the appropriate place.
 *
 * This function induces various "status" messages.
 * </pre>
 * @todo ここで計算していた各値は一部の状態変化メッセージ処理を除き、今後必要な時に適示計算する形に移行するためほぼすべて削られる。
 */
void calc_bonuses(player_type *creature_ptr)
{
    int empty_hands_status = empty_hands(creature_ptr, TRUE);
    object_type *o_ptr;

    /* Save the old vision stuff */
    BIT_FLAGS old_telepathy = creature_ptr->telepathy;
    BIT_FLAGS old_esp_animal = creature_ptr->esp_animal;
    BIT_FLAGS old_esp_undead = creature_ptr->esp_undead;
    BIT_FLAGS old_esp_demon = creature_ptr->esp_demon;
    BIT_FLAGS old_esp_orc = creature_ptr->esp_orc;
    BIT_FLAGS old_esp_troll = creature_ptr->esp_troll;
    BIT_FLAGS old_esp_giant = creature_ptr->esp_giant;
    BIT_FLAGS old_esp_dragon = creature_ptr->esp_dragon;
    BIT_FLAGS old_esp_human = creature_ptr->esp_human;
    BIT_FLAGS old_esp_evil = creature_ptr->esp_evil;
    BIT_FLAGS old_esp_good = creature_ptr->esp_good;
    BIT_FLAGS old_esp_nonliving = creature_ptr->esp_nonliving;
    BIT_FLAGS old_esp_unique = creature_ptr->esp_unique;
    BIT_FLAGS old_see_inv = creature_ptr->see_inv;
    BIT_FLAGS old_mighty_throw = creature_ptr->mighty_throw;
    s16b old_speed = creature_ptr->pspeed;

    ARMOUR_CLASS old_dis_ac = creature_ptr->dis_ac;
    ARMOUR_CLASS old_dis_to_a = creature_ptr->dis_to_a;

    creature_ptr->xtra_might = has_xtra_might(creature_ptr);
    creature_ptr->esp_evil = has_esp_evil(creature_ptr);
    creature_ptr->esp_animal = has_esp_animal(creature_ptr);
    creature_ptr->esp_undead = has_esp_undead(creature_ptr);
    creature_ptr->esp_demon = has_esp_demon(creature_ptr);
    creature_ptr->esp_orc = has_esp_orc(creature_ptr);
    creature_ptr->esp_troll = has_esp_troll(creature_ptr);
    creature_ptr->esp_giant = has_esp_giant(creature_ptr);
    creature_ptr->esp_dragon = has_esp_dragon(creature_ptr);
    creature_ptr->esp_human = has_esp_human(creature_ptr);
    creature_ptr->esp_good = has_esp_good(creature_ptr);
    creature_ptr->esp_nonliving = has_esp_nonliving(creature_ptr);
    creature_ptr->esp_unique = has_esp_unique(creature_ptr);
    creature_ptr->telepathy = has_esp_telepathy(creature_ptr);
    creature_ptr->bless_blade = has_bless_blade(creature_ptr);
    creature_ptr->easy_2weapon = has_easy2_weapon(creature_ptr);
    creature_ptr->down_saving = has_down_saving(creature_ptr);
    creature_ptr->yoiyami = has_no_ac(creature_ptr);
    creature_ptr->mighty_throw = has_mighty_throw(creature_ptr);
    creature_ptr->dec_mana = has_dec_mana(creature_ptr);
    creature_ptr->see_nocto = has_see_nocto(creature_ptr);
    creature_ptr->warning = has_warning(creature_ptr);
    creature_ptr->anti_magic = has_anti_magic(creature_ptr);
    creature_ptr->anti_tele = has_anti_tele(creature_ptr);
    creature_ptr->easy_spell = has_easy_spell(creature_ptr);
    creature_ptr->heavy_spell = has_heavy_spell(creature_ptr);
    creature_ptr->hold_exp = has_hold_exp(creature_ptr);
    creature_ptr->see_inv = has_see_inv(creature_ptr);
    creature_ptr->free_act = has_free_act(creature_ptr);
    creature_ptr->levitation = has_levitation(creature_ptr);
    has_can_swim(creature_ptr);
    creature_ptr->slow_digest = has_slow_digest(creature_ptr);
    creature_ptr->regenerate = has_regenerate(creature_ptr);
    has_curses(creature_ptr);
    creature_ptr->impact = has_impact(creature_ptr);
    has_extra_blow(creature_ptr);

    creature_ptr->lite = has_lite(creature_ptr);

    if (creature_ptr->special_defense & KAMAE_MASK) {
        if (!(empty_hands_status & EMPTY_HAND_RARM)) {
            set_action(creature_ptr, ACTION_NONE);
        }
    }

    creature_ptr->stat_add[A_STR] = calc_strength_addition(creature_ptr);
    creature_ptr->stat_add[A_INT] = calc_intelligence_addition(creature_ptr);
    creature_ptr->stat_add[A_WIS] = calc_wisdom_addition(creature_ptr);
    creature_ptr->stat_add[A_DEX] = calc_dexterity_addition(creature_ptr);
    creature_ptr->stat_add[A_CON] = calc_constitution_addition(creature_ptr);
    creature_ptr->stat_add[A_CHR] = calc_charisma_addition(creature_ptr);
    creature_ptr->to_m_chance = calc_to_magic_chance(creature_ptr);
    creature_ptr->ac = calc_base_ac(creature_ptr);
    creature_ptr->to_a = calc_to_ac(creature_ptr, TRUE);
    creature_ptr->dis_ac = calc_base_ac(creature_ptr);
    creature_ptr->dis_to_a = calc_to_ac(creature_ptr, FALSE);

    for (int i = 0; i < A_MAX; i++) {
        calc_top_status(creature_ptr, i);
        calc_use_status(creature_ptr, i);
        calc_ind_status(creature_ptr, i);
    }

    o_ptr = &creature_ptr->inventory_list[INVEN_BOW];
    if (o_ptr->k_idx) {
        creature_ptr->tval_ammo = (byte)bow_tval_ammo(o_ptr);
        if (o_ptr->k_idx && !is_heavy_shoot(creature_ptr, &creature_ptr->inventory_list[INVEN_BOW])) {
            creature_ptr->num_fire = calc_num_fire(creature_ptr, o_ptr);
        }
    }

    for (int i = 0; i < 2; i++) {
        creature_ptr->icky_wield[i] = has_icky_wield_weapon(creature_ptr, i);
        creature_ptr->riding_wield[i] = has_riding_wield_weapon(creature_ptr, i);
        creature_ptr->num_blow[i] = calc_num_blow(creature_ptr, i);
        creature_ptr->to_dd[i] = calc_to_weapon_dice_num(creature_ptr, INVEN_RARM + i);
        creature_ptr->to_ds[i] = calc_to_weapon_dice_side(creature_ptr, INVEN_RARM + i);
    }

    creature_ptr->pspeed = calc_speed(creature_ptr);
    creature_ptr->see_infra = calc_intra_vision(creature_ptr);
    creature_ptr->skill_stl = calc_stealth(creature_ptr);
    creature_ptr->skill_dis = calc_disarming(creature_ptr);
    creature_ptr->skill_dev = calc_device_ability(creature_ptr);
    creature_ptr->skill_sav = calc_saving_throw(creature_ptr);
    creature_ptr->skill_srh = calc_search(creature_ptr);
    creature_ptr->skill_fos = calc_search_freq(creature_ptr);
    creature_ptr->skill_thn = calc_to_hit_melee(creature_ptr);
    creature_ptr->skill_thb = calc_to_hit_shoot(creature_ptr);
    creature_ptr->skill_tht = calc_to_hit_throw(creature_ptr);
    creature_ptr->to_d[0] = calc_to_damage(creature_ptr, INVEN_RARM, TRUE);
    creature_ptr->to_d[1] = calc_to_damage(creature_ptr, INVEN_LARM, TRUE);
    creature_ptr->dis_to_d[0] = calc_to_damage(creature_ptr, INVEN_RARM, FALSE);
    creature_ptr->dis_to_d[1] = calc_to_damage(creature_ptr, INVEN_LARM, FALSE);
    creature_ptr->to_h[0] = calc_to_hit(creature_ptr, INVEN_RARM, TRUE);
    creature_ptr->to_h[1] = calc_to_hit(creature_ptr, INVEN_LARM, TRUE);
    creature_ptr->dis_to_h[0] = calc_to_hit(creature_ptr, INVEN_RARM, FALSE);
    creature_ptr->dis_to_h[1] = calc_to_hit(creature_ptr, INVEN_LARM, FALSE);
    creature_ptr->to_h_b = calc_to_hit_bow(creature_ptr, TRUE);
    creature_ptr->dis_to_h_b = calc_to_hit_bow(creature_ptr, FALSE);
    creature_ptr->to_d_m = calc_to_damage_misc(creature_ptr);
    creature_ptr->to_h_m = calc_to_hit_misc(creature_ptr);
    creature_ptr->skill_dig = calc_skill_dig(creature_ptr);

    if (old_mighty_throw != creature_ptr->mighty_throw) {
        creature_ptr->window |= PW_INVEN;
    }

    if (creature_ptr->telepathy != old_telepathy) {
        creature_ptr->update |= (PU_MONSTERS);
    }

    if ((creature_ptr->esp_animal != old_esp_animal) || (creature_ptr->esp_undead != old_esp_undead) || (creature_ptr->esp_demon != old_esp_demon)
        || (creature_ptr->esp_orc != old_esp_orc) || (creature_ptr->esp_troll != old_esp_troll) || (creature_ptr->esp_giant != old_esp_giant)
        || (creature_ptr->esp_dragon != old_esp_dragon) || (creature_ptr->esp_human != old_esp_human) || (creature_ptr->esp_evil != old_esp_evil)
        || (creature_ptr->esp_good != old_esp_good) || (creature_ptr->esp_nonliving != old_esp_nonliving) || (creature_ptr->esp_unique != old_esp_unique)) {
        creature_ptr->update |= (PU_MONSTERS);
    }

    if (creature_ptr->see_inv != old_see_inv) {
        creature_ptr->update |= (PU_MONSTERS);
    }

    if (creature_ptr->pspeed != old_speed) {
        creature_ptr->redraw |= (PR_SPEED);
    }

    if ((creature_ptr->dis_ac != old_dis_ac) || (creature_ptr->dis_to_a != old_dis_to_a)) {
        creature_ptr->redraw |= (PR_ARMOR);
        creature_ptr->window |= (PW_PLAYER);
    }

    if (current_world_ptr->character_xtra)
        return;

    put_equipment_warning(creature_ptr);
}

static void calc_alignment(player_type *creature_ptr)
{
    creature_ptr->align = 0;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    for (MONSTER_IDX m_idx = floor_ptr->m_max - 1; m_idx >= 1; m_idx--) {
        monster_type *m_ptr;
        monster_race *r_ptr;
        m_ptr = &floor_ptr->m_list[m_idx];
        if (!monster_is_valid(m_ptr))
            continue;
        r_ptr = &r_info[m_ptr->r_idx];

        if (!is_pet(m_ptr))
            continue;

        if (r_ptr->flags3 & RF3_GOOD)
            creature_ptr->align += r_ptr->level;
        if (r_ptr->flags3 & RF3_EVIL)
            creature_ptr->align -= r_ptr->level;
    }

    if (creature_ptr->mimic_form) {
        switch (creature_ptr->mimic_form) {
        case MIMIC_DEMON:
            creature_ptr->align -= 200;
            break;
        case MIMIC_DEMON_LORD:
            creature_ptr->align -= 200;
            break;
        }
    } else {
        switch (creature_ptr->prace) {
        case RACE_ARCHON:
            creature_ptr->align += 200;
            break;
        case RACE_BALROG:
            creature_ptr->align -= 200;
            break;
        }
    }

    for (int i = 0; i < 2; i++) {
        if (!has_melee_weapon(creature_ptr, INVEN_RARM + i))
            continue;
        if (creature_ptr->inventory_list[INVEN_RARM + i].name1 != ART_IRON_BALL)
            continue;
        creature_ptr->align -= 1000;
    }

    int j = 0;
    int neutral[2];
    for (int i = 0; i < 8; i++) {
        switch (creature_ptr->vir_types[i]) {
        case V_JUSTICE:
            creature_ptr->align += creature_ptr->virtues[i] * 2;
            break;
        case V_CHANCE:
            break;
        case V_NATURE:
        case V_HARMONY:
            neutral[j++] = i;
            break;
        case V_UNLIFE:
            creature_ptr->align -= creature_ptr->virtues[i];
            break;
        default:
            creature_ptr->align += creature_ptr->virtues[i];
            break;
        }
    }

    for (int i = 0; i < j; i++) {
        if (creature_ptr->align > 0) {
            creature_ptr->align -= creature_ptr->virtues[neutral[i]] / 2;
            if (creature_ptr->align < 0)
                creature_ptr->align = 0;
        } else if (creature_ptr->align < 0) {
            creature_ptr->align += creature_ptr->virtues[neutral[i]] / 2;
            if (creature_ptr->align > 0)
                creature_ptr->align = 0;
        }
    }
}

/*!
 * @brief プレイヤーの最大HPを計算する /
 * Calculate the players (maximal) hit points
 * Adjust current hitpoints if necessary
 * @return なし
 * @details
 */
static void calc_hitpoints(player_type *creature_ptr)
{
    int bonus = ((int)(adj_con_mhp[creature_ptr->stat_ind[A_CON]]) - 128) * creature_ptr->lev / 4;
    int mhp = creature_ptr->player_hp[creature_ptr->lev - 1];

    byte tmp_hitdie;
    if (creature_ptr->mimic_form) {
        if (creature_ptr->pclass == CLASS_SORCERER)
            tmp_hitdie = mimic_info[creature_ptr->mimic_form].r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
        else
            tmp_hitdie = mimic_info[creature_ptr->mimic_form].r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        mhp = mhp * tmp_hitdie / creature_ptr->hitdie;
    }

    if (creature_ptr->pclass == CLASS_SORCERER) {
        if (creature_ptr->lev < 30)
            mhp = (mhp * (45 + creature_ptr->lev) / 100);
        else
            mhp = (mhp * 75 / 100);
        bonus = (bonus * 65 / 100);
    }

    mhp += bonus;

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        mhp = mhp * (110 + (((creature_ptr->lev + 40) * (creature_ptr->lev + 40) - 1550) / 110)) / 100;
    }

    if (mhp < creature_ptr->lev + 1)
        mhp = creature_ptr->lev + 1;
    if (is_hero(creature_ptr))
        mhp += 10;
    if (is_shero(creature_ptr))
        mhp += 30;
    if (creature_ptr->tsuyoshi)
        mhp += 50;
    if (hex_spelling(creature_ptr, HEX_XTRA_MIGHT))
        mhp += 15;
    if (hex_spelling(creature_ptr, HEX_BUILDING))
        mhp += 60;
    if (creature_ptr->mhp == mhp)
        return;

    if (creature_ptr->chp >= mhp) {
        creature_ptr->chp = mhp;
        creature_ptr->chp_frac = 0;
    }

#ifdef JP
    if (creature_ptr->level_up_message && (mhp > creature_ptr->mhp)) {
        msg_format("最大ヒット・ポイントが %d 増加した！", (mhp - creature_ptr->mhp));
    }
#endif
    creature_ptr->mhp = mhp;

    creature_ptr->redraw |= PR_HP;
    creature_ptr->window |= PW_PLAYER;
}

/*!
 * @brief プレイヤーの現在学習可能な魔法数を計算し、増減に応じて魔法の忘却、再学習を処置する。 /
 * Calculate number of spells player should have, and forget,
 * or remember, spells until that number is properly reflected.
 * @return なし
 * @details
 * Note that this function induces various "status" messages,
 * which must be bypasses until the character is created.
 */
static void calc_spells(player_type *creature_ptr)
{
    if (!mp_ptr->spell_book)
        return;
    if (!current_world_ptr->character_generated)
        return;
    if (current_world_ptr->character_xtra)
        return;
    if ((creature_ptr->pclass == CLASS_SORCERER) || (creature_ptr->pclass == CLASS_RED_MAGE)) {
        creature_ptr->new_spells = 0;
        return;
    }

    concptr p = spell_category_name(mp_ptr->spell_book);
    int levels = creature_ptr->lev - mp_ptr->spell_first + 1;
    if (levels < 0)
        levels = 0;

    int num_allowed = (adj_mag_study[creature_ptr->stat_ind[mp_ptr->spell_stat]] * levels / 2);
    int bonus = 0;
    if ((creature_ptr->pclass != CLASS_SAMURAI) && (mp_ptr->spell_book != TV_LIFE_BOOK)) {
        bonus = 4;
    }

    if (creature_ptr->pclass == CLASS_SAMURAI) {
        num_allowed = 32;
    } else if (creature_ptr->realm2 == REALM_NONE) {
        num_allowed = (num_allowed + 1) / 2;
        if (num_allowed > (32 + bonus))
            num_allowed = 32 + bonus;
    } else if ((creature_ptr->pclass == CLASS_MAGE) || (creature_ptr->pclass == CLASS_PRIEST)) {
        if (num_allowed > (96 + bonus))
            num_allowed = 96 + bonus;
    } else {
        if (num_allowed > (80 + bonus))
            num_allowed = 80 + bonus;
    }

    int num_boukyaku = 0;
    for (int j = 0; j < 64; j++) {
        if ((j < 32) ? (creature_ptr->spell_forgotten1 & (1L << j)) : (creature_ptr->spell_forgotten2 & (1L << (j - 32)))) {
            num_boukyaku++;
        }
    }

    creature_ptr->new_spells = num_allowed + creature_ptr->add_spells + num_boukyaku - creature_ptr->learned_spells;
    for (int i = 63; i >= 0; i--) {
        if (!creature_ptr->spell_learned1 && !creature_ptr->spell_learned2)
            break;

        int j = creature_ptr->spell_order[i];
        if (j >= 99)
            continue;

        const magic_type *s_ptr;
        if (!is_magic((j < 32) ? creature_ptr->realm1 : creature_ptr->realm2)) {
            if (j < 32)
                s_ptr = &technic_info[creature_ptr->realm1 - MIN_TECHNIC][j];
            else
                s_ptr = &technic_info[creature_ptr->realm2 - MIN_TECHNIC][j % 32];
        } else if (j < 32)
            s_ptr = &mp_ptr->info[creature_ptr->realm1 - 1][j];
        else
            s_ptr = &mp_ptr->info[creature_ptr->realm2 - 1][j % 32];

        if (s_ptr->slevel <= creature_ptr->lev)
            continue;

        bool is_spell_learned = (j < 32) ? (creature_ptr->spell_learned1 & (1L << j)) : (creature_ptr->spell_learned2 & (1L << (j - 32)));
        if (!is_spell_learned)
            continue;

        REALM_IDX which;
        if (j < 32) {
            creature_ptr->spell_forgotten1 |= (1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_forgotten2 |= (1L << (j - 32));
            which = creature_ptr->realm2;
        }

        if (j < 32) {
            creature_ptr->spell_learned1 &= ~(1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_learned2 &= ~(1L << (j - 32));
            which = creature_ptr->realm2;
        }

#ifdef JP
        msg_format("%sの%sを忘れてしまった。", exe_spell(creature_ptr, which, j % 32, SPELL_NAME), p);
#else
        msg_format("You have forgotten the %s of %s.", p, exe_spell(creature_ptr, which, j % 32, SPELL_NAME));
#endif
        creature_ptr->new_spells++;
    }

    /* Forget spells if we know too many spells */
    for (int i = 63; i >= 0; i--) {
        if (creature_ptr->new_spells >= 0)
            break;
        if (!creature_ptr->spell_learned1 && !creature_ptr->spell_learned2)
            break;

        int j = creature_ptr->spell_order[i];
        if (j >= 99)
            continue;

        bool is_spell_learned = (j < 32) ? (creature_ptr->spell_learned1 & (1L << j)) : (creature_ptr->spell_learned2 & (1L << (j - 32)));
        if (!is_spell_learned)
            continue;

        REALM_IDX which;
        if (j < 32) {
            creature_ptr->spell_forgotten1 |= (1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_forgotten2 |= (1L << (j - 32));
            which = creature_ptr->realm2;
        }

        if (j < 32) {
            creature_ptr->spell_learned1 &= ~(1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_learned2 &= ~(1L << (j - 32));
            which = creature_ptr->realm2;
        }

#ifdef JP
        msg_format("%sの%sを忘れてしまった。", exe_spell(creature_ptr, which, j % 32, SPELL_NAME), p);
#else
        msg_format("You have forgotten the %s of %s.", p, exe_spell(creature_ptr, which, j % 32, SPELL_NAME));
#endif
        creature_ptr->new_spells++;
    }

    /* Check for spells to remember */
    for (int i = 0; i < 64; i++) {
        if (creature_ptr->new_spells <= 0)
            break;
        if (!creature_ptr->spell_forgotten1 && !creature_ptr->spell_forgotten2)
            break;
        int j = creature_ptr->spell_order[i];
        if (j >= 99)
            break;

        const magic_type *s_ptr;
        if (!is_magic((j < 32) ? creature_ptr->realm1 : creature_ptr->realm2)) {
            if (j < 32)
                s_ptr = &technic_info[creature_ptr->realm1 - MIN_TECHNIC][j];
            else
                s_ptr = &technic_info[creature_ptr->realm2 - MIN_TECHNIC][j % 32];
        } else if (j < 32)
            s_ptr = &mp_ptr->info[creature_ptr->realm1 - 1][j];
        else
            s_ptr = &mp_ptr->info[creature_ptr->realm2 - 1][j % 32];

        if (s_ptr->slevel > creature_ptr->lev)
            continue;

        bool is_spell_learned = (j < 32) ? (creature_ptr->spell_forgotten1 & (1L << j)) : (creature_ptr->spell_forgotten2 & (1L << (j - 32)));
        if (!is_spell_learned)
            continue;

        REALM_IDX which;
        if (j < 32) {
            creature_ptr->spell_forgotten1 &= ~(1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_forgotten2 &= ~(1L << (j - 32));
            which = creature_ptr->realm2;
        }

        if (j < 32) {
            creature_ptr->spell_learned1 |= (1L << j);
            which = creature_ptr->realm1;
        } else {
            creature_ptr->spell_learned2 |= (1L << (j - 32));
            which = creature_ptr->realm2;
        }

#ifdef JP
        msg_format("%sの%sを思い出した。", exe_spell(creature_ptr, which, j % 32, SPELL_NAME), p);
#else
        msg_format("You have remembered the %s of %s.", p, exe_spell(creature_ptr, which, j % 32, SPELL_NAME));
#endif
        creature_ptr->new_spells--;
    }

    if (creature_ptr->realm2 == REALM_NONE) {
        int k = 0;
        for (int j = 0; j < 32; j++) {
            const magic_type *s_ptr;
            if (!is_magic(creature_ptr->realm1))
                s_ptr = &technic_info[creature_ptr->realm1 - MIN_TECHNIC][j];
            else
                s_ptr = &mp_ptr->info[creature_ptr->realm1 - 1][j];

            if (s_ptr->slevel > creature_ptr->lev)
                continue;

            if (creature_ptr->spell_learned1 & (1L << j)) {
                continue;
            }

            k++;
        }

        if (k > 32)
            k = 32;
        if ((creature_ptr->new_spells > k) && ((mp_ptr->spell_book == TV_LIFE_BOOK) || (mp_ptr->spell_book == TV_HISSATSU_BOOK))) {
            creature_ptr->new_spells = (s16b)k;
        }
    }

    if (creature_ptr->new_spells < 0)
        creature_ptr->new_spells = 0;

    if (creature_ptr->old_spells == creature_ptr->new_spells)
        return;

    if (creature_ptr->new_spells) {
#ifdef JP
        if (creature_ptr->new_spells < 10) {
            msg_format("あと %d つの%sを学べる。", creature_ptr->new_spells, p);
        } else {
            msg_format("あと %d 個の%sを学べる。", creature_ptr->new_spells, p);
        }
#else
        msg_format("You can learn %d more %s%s.", creature_ptr->new_spells, p, (creature_ptr->new_spells != 1) ? "s" : "");
#endif
    }

    creature_ptr->old_spells = creature_ptr->new_spells;
    creature_ptr->redraw |= PR_STUDY;
    creature_ptr->window |= PW_OBJECT;
}

/*!
 * @brief プレイヤーの最大MPを計算する /
 * Calculate maximum mana.  You do not need to know any spells.
 * Note that mana is lowered by heavy (or inappropriate) armor.
 * @return なし
 * @details
 * This function induces status messages.
 */
static void calc_mana(player_type *creature_ptr)
{
    if (!mp_ptr->spell_book)
        return;

    int levels;
    if ((creature_ptr->pclass == CLASS_MINDCRAFTER) || (creature_ptr->pclass == CLASS_MIRROR_MASTER) || (creature_ptr->pclass == CLASS_BLUE_MAGE)) {
        levels = creature_ptr->lev;
    } else {
        if (mp_ptr->spell_first > creature_ptr->lev) {
            creature_ptr->msp = 0;
            creature_ptr->redraw |= (PR_MANA);
            return;
        }

        levels = (creature_ptr->lev - mp_ptr->spell_first) + 1;
    }

    int msp;
    if (creature_ptr->pclass == CLASS_SAMURAI) {
        msp = (adj_mag_mana[creature_ptr->stat_ind[mp_ptr->spell_stat]] + 10) * 2;
        if (msp)
            msp += (msp * rp_ptr->r_adj[mp_ptr->spell_stat] / 20);
    } else {
        msp = adj_mag_mana[creature_ptr->stat_ind[mp_ptr->spell_stat]] * (levels + 3) / 4;
        if (msp)
            msp++;
        if (msp)
            msp += (msp * rp_ptr->r_adj[mp_ptr->spell_stat] / 20);
        if (msp && (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN))
            msp += msp / 2;
        if (msp && (creature_ptr->pclass == CLASS_HIGH_MAGE))
            msp += msp / 4;
        if (msp && (creature_ptr->pclass == CLASS_SORCERER))
            msp += msp * (25 + creature_ptr->lev) / 100;
    }

    if (mp_ptr->spell_xtra & MAGIC_GLOVE_REDUCE_MANA) {
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        creature_ptr->cumber_glove = FALSE;
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[INVEN_HANDS];
        object_flags(creature_ptr, o_ptr, flgs);
        if (o_ptr->k_idx && !(has_flag(flgs, TR_FREE_ACT)) && !(has_flag(flgs, TR_DEC_MANA)) && !(has_flag(flgs, TR_EASY_SPELL))
            && !((has_flag(flgs, TR_MAGIC_MASTERY)) && (o_ptr->pval > 0)) && !((has_flag(flgs, TR_DEX)) && (o_ptr->pval > 0))) {
            creature_ptr->cumber_glove = TRUE;
            msp = (3 * msp) / 4;
        }
    }

    creature_ptr->cumber_armor = FALSE;

    int cur_wgt = 0;
    if (creature_ptr->inventory_list[INVEN_RARM].tval > TV_SWORD)
        cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight;
    if (creature_ptr->inventory_list[INVEN_LARM].tval > TV_SWORD)
        cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight;
    cur_wgt += creature_ptr->inventory_list[INVEN_BODY].weight;
    cur_wgt += creature_ptr->inventory_list[INVEN_HEAD].weight;
    cur_wgt += creature_ptr->inventory_list[INVEN_OUTER].weight;
    cur_wgt += creature_ptr->inventory_list[INVEN_HANDS].weight;
    cur_wgt += creature_ptr->inventory_list[INVEN_FEET].weight;

    switch (creature_ptr->pclass) {
    case CLASS_MAGE:
    case CLASS_HIGH_MAGE:
    case CLASS_BLUE_MAGE:
    case CLASS_MONK:
    case CLASS_FORCETRAINER:
    case CLASS_SORCERER: {
        if (creature_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight;
        if (creature_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight;
        break;
    }
    case CLASS_PRIEST:
    case CLASS_BARD:
    case CLASS_TOURIST: {
        if (creature_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight * 2 / 3;
        if (creature_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight * 2 / 3;
        break;
    }
    case CLASS_MINDCRAFTER:
    case CLASS_BEASTMASTER:
    case CLASS_MIRROR_MASTER: {
        if (creature_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight / 2;
        if (creature_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight / 2;
        break;
    }
    case CLASS_ROGUE:
    case CLASS_RANGER:
    case CLASS_RED_MAGE:
    case CLASS_WARRIOR_MAGE: {
        if (creature_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight / 3;
        if (creature_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight / 3;
        break;
    }
    case CLASS_PALADIN:
    case CLASS_CHAOS_WARRIOR: {
        if (creature_ptr->inventory_list[INVEN_RARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_RARM].weight / 5;
        if (creature_ptr->inventory_list[INVEN_LARM].tval <= TV_SWORD)
            cur_wgt += creature_ptr->inventory_list[INVEN_LARM].weight / 5;
        break;
    }
    default: {
        break;
    }
    }

    int max_wgt = mp_ptr->spell_weight;
    if ((cur_wgt - max_wgt) > 0) {
        creature_ptr->cumber_armor = TRUE;
        switch (creature_ptr->pclass) {
        case CLASS_MAGE:
        case CLASS_HIGH_MAGE:
        case CLASS_BLUE_MAGE: {
            msp -= msp * (cur_wgt - max_wgt) / 600;
            break;
        }
        case CLASS_PRIEST:
        case CLASS_MINDCRAFTER:
        case CLASS_BEASTMASTER:
        case CLASS_BARD:
        case CLASS_FORCETRAINER:
        case CLASS_TOURIST:
        case CLASS_MIRROR_MASTER: {
            msp -= msp * (cur_wgt - max_wgt) / 800;
            break;
        }
        case CLASS_SORCERER: {
            msp -= msp * (cur_wgt - max_wgt) / 900;
            break;
        }
        case CLASS_ROGUE:
        case CLASS_RANGER:
        case CLASS_MONK:
        case CLASS_RED_MAGE: {
            msp -= msp * (cur_wgt - max_wgt) / 1000;
            break;
        }
        case CLASS_PALADIN:
        case CLASS_CHAOS_WARRIOR:
        case CLASS_WARRIOR_MAGE: {
            msp -= msp * (cur_wgt - max_wgt) / 1200;
            break;
        }
        case CLASS_SAMURAI: {
            creature_ptr->cumber_armor = FALSE;
            break;
        }
        default: {
            msp -= msp * (cur_wgt - max_wgt) / 800;
            break;
        }
        }
    }

    if (msp < 0)
        msp = 0;

    if (creature_ptr->msp != msp) {
        if ((creature_ptr->csp >= msp) && (creature_ptr->pclass != CLASS_SAMURAI)) {
            creature_ptr->csp = msp;
            creature_ptr->csp_frac = 0;
        }

#ifdef JP
        if (creature_ptr->level_up_message && (msp > creature_ptr->msp)) {
            msg_format("最大マジック・ポイントが %d 増加した！", (msp - creature_ptr->msp));
        }
#endif
        creature_ptr->msp = msp;
        creature_ptr->redraw |= (PR_MANA);
        creature_ptr->window |= (PW_PLAYER | PW_SPELL);
    }

    if (current_world_ptr->character_xtra)
        return;

    if (creature_ptr->old_cumber_glove != creature_ptr->cumber_glove) {
        if (creature_ptr->cumber_glove)
            msg_print(_("手が覆われて呪文が唱えにくい感じがする。", "Your covered hands feel unsuitable for spellcasting."));
        else
            msg_print(_("この手の状態なら、ぐっと呪文が唱えやすい感じだ。", "Your hands feel more suitable for spellcasting."));

        creature_ptr->old_cumber_glove = creature_ptr->cumber_glove;
    }

    if (creature_ptr->old_cumber_armor == creature_ptr->cumber_armor)
        return;

    if (creature_ptr->cumber_armor)
        msg_print(_("装備の重さで動きが鈍くなってしまっている。", "The weight of your equipment encumbers your movement."));
    else
        msg_print(_("ぐっと楽に体を動かせるようになった。", "You feel able to move more freely."));

    creature_ptr->old_cumber_armor = creature_ptr->cumber_armor;
}

/*!
 * @brief 装備中の射撃武器の威力倍率を返す /
 * calcurate the fire rate of target object
 * @param o_ptr 計算する射撃武器のアイテム情報参照ポインタ
 * @return 射撃倍率の値(100で1.00倍)
 */
s16b calc_num_fire(player_type *creature_ptr, object_type *o_ptr)
{
    int extra_shots = 0;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *q_ptr;
        q_ptr = &creature_ptr->inventory_list[i];
        if (!q_ptr->k_idx)
            continue;

        if (i == INVEN_BOW)
            continue;

        object_flags(creature_ptr, q_ptr, flgs);
        if (has_flag(flgs, TR_XTRA_SHOTS))
            extra_shots++;
    }

    object_flags(creature_ptr, o_ptr, flgs);
    if (has_flag(flgs, TR_XTRA_SHOTS))
        extra_shots++;

    int num = 0;
    if (o_ptr->k_idx == 0 || is_heavy_shoot(creature_ptr, o_ptr))
        return (s16b)num;

    num = 100;
    num += (extra_shots * 100);

    tval_type tval_ammo = bow_tval_ammo(o_ptr);
    if ((creature_ptr->pclass == CLASS_RANGER) && (tval_ammo == TV_ARROW)) {
        num += (creature_ptr->lev * 4);
    }

    if ((creature_ptr->pclass == CLASS_CAVALRY) && (tval_ammo == TV_ARROW)) {
        num += (creature_ptr->lev * 3);
    }

    if (creature_ptr->pclass == CLASS_ARCHER) {
        if (tval_ammo == TV_ARROW)
            num += ((creature_ptr->lev * 5) + 50);
        else if ((tval_ammo == TV_BOLT) || (tval_ammo == TV_SHOT))
            num += (creature_ptr->lev * 4);
    }

    if (creature_ptr->pclass == CLASS_WARRIOR && (tval_ammo <= TV_BOLT) && (tval_ammo >= TV_SHOT)) {
        num += (creature_ptr->lev * 2);
    }

    if ((creature_ptr->pclass == CLASS_ROGUE) && (tval_ammo == TV_SHOT)) {
        num += (creature_ptr->lev * 4);
    }

    return (s16b)num;
}

/*!
 * @brief 赤外線視力計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 赤外線視力
 * @details
 * * 種族による加算
 * * 変異MUT3_INFRAVISによる加算(+3)
 * * 魔法効果tim_infraによる加算(+3)
 * * 装備がTR_INFRAフラグ持ちなら加算(+pval*1)
 */
static ACTION_SKILL_POWER calc_intra_vision(player_type *creature_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];

    pow = tmp_rp_ptr->infra;

    if (creature_ptr->muta3 & MUT3_INFRAVIS) {
        pow += 3;
    }

    if (creature_ptr->tim_infra) {
        pow += 3;
    }

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_INFRA))
            pow += o_ptr->pval;
    }

    return pow;
}

/*!
 * @brief 隠密能力計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 隠密能力
 * @details
 * * 初期値1
 * * 種族/職業/性格による加算
 * * 職業による追加加算
 * * 装備による修正(TR_STEALTHがあれば+pval*1)
 * * 変異MUT3_XTRA_NOISで減算(-3)
 * * 変異MUT3_MOTIONで加算(+1)
 * * 呪術を唱えていると減算(-(詠唱数+1))
 * * セクシーギャルでない影フェアリーがTRC_AGGRAVATE持ちの時、別処理でTRC_AGGRAVATEを無効にする代わりに減算(-3か3未満なら(現在値+2)/2)
 * * 狂戦士化で減算(-7)
 * * 忍者がheavy_armorならば減算(-レベル/10)
 * * 忍者がheavy_armorでなく適正な武器を持っていれば加算(+レベル/10)
 * * 隠密の歌で加算(+99)
 * * 最大30、最低0に補正
 */
static ACTION_SKILL_POWER calc_stealth(player_type *creature_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    pow = 1 + tmp_rp_ptr->r_stl + c_ptr->c_stl + a_ptr->a_stl;
    pow += (c_ptr->x_stl * creature_ptr->lev / 10);

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_STEALTH))
            pow += o_ptr->pval;
    }

    if (creature_ptr->muta3 & MUT3_XTRA_NOIS) {
        pow -= 3;
    }
    if (creature_ptr->muta3 & MUT3_MOTION) {
        pow += 1;
    }
    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling_any(creature_ptr))
            pow -= (1 + casting_hex_num(creature_ptr));
    }
    if ((is_specific_player_race(creature_ptr, RACE_S_FAIRY)) && (creature_ptr->pseikaku != PERSONALITY_SEXY) && (creature_ptr->cursed & TRC_AGGRAVATE)) {
        pow = MIN(pow - 3, (pow + 2) / 2);
    }

    if (is_shero(creature_ptr)) {
        pow -= 7;
    }

    if (creature_ptr->pclass == CLASS_NINJA && heavy_armor(creature_ptr)) {
        pow -= (creature_ptr->lev) / 10;
    } else if ((!creature_ptr->inventory_list[INVEN_RARM].k_idx || has_right_hand_weapon(creature_ptr))
        && (!creature_ptr->inventory_list[INVEN_LARM].k_idx || has_left_hand_weapon(creature_ptr))) {
        pow += (creature_ptr->lev) / 10;
    }

    if (is_time_limit_stealth(creature_ptr))
        pow += 99;

    if (pow > 30)
        pow = 30;
    if (pow < 0)
        pow = 0;

    return pow;
}

/*!
 * @brief 解除能力計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 解除能力
 * @details
 * * 種族/職業/性格による加算
 * * 職業と性格とレベルによる追加加算
 * * 器用さに応じたadj_dex_disテーブルによる加算
 * * 知力に応じたadj_int_disテーブルによる加算
 */
static ACTION_SKILL_POWER calc_disarming(player_type *creature_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    pow = tmp_rp_ptr->r_dis + c_ptr->c_dis + a_ptr->a_dis;
    pow += ((cp_ptr->x_dis * creature_ptr->lev / 10) + (ap_ptr->a_dis * creature_ptr->lev / 50));
    pow += adj_dex_dis[creature_ptr->stat_ind[A_DEX]];
    pow += adj_int_dis[creature_ptr->stat_ind[A_INT]];
    return pow;
}

/*!
 * @brief 魔法防御計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 魔法防御
 * @details
 * * 種族/職業/性格による加算
 * * 職業と性格とレベルによる追加加算
 * * 装備による加算(TR_MAGIC_MASTERYを持っていたら+pval*8)
 * * 知力に応じたadj_int_devテーブルによる加算
 * * 狂戦士化による減算(-20)
 */
static ACTION_SKILL_POWER calc_device_ability(player_type *creature_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    pow = tmp_rp_ptr->r_dev + c_ptr->c_dev + a_ptr->a_dev;
    pow += ((c_ptr->x_dev * creature_ptr->lev / 10) + (ap_ptr->a_dev * creature_ptr->lev / 50));

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_MAGIC_MASTERY))
            pow += 8 * o_ptr->pval;
    }

    pow += adj_int_dev[creature_ptr->stat_ind[A_INT]];

    if (is_shero(creature_ptr)) {
        pow -= 20;
    }
    return pow;
}

/*!
 * @brief 魔法防御計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 魔法防御
 * @details
 * * 種族/職業/性格による加算
 * * 職業と性格とレベルによる追加加算
 * * 変異MUT3_MAGIC_RESによる加算(15 + レベル / 5)
 * * 賢さによるadj_wis_savテーブル加算
 * * 狂戦士化による減算(-30)
 * * 反魔法持ちで大なり上書き(90+レベル未満ならその値に上書き)
 * * クターのつぶれ状態なら(10に上書き)
 * * 生命の「究極の耐性」や regist_magic,magicdef持ちなら大なり上書き(95+レベル未満ならその値に上書き)
 * * 呪いのdown_savingがかかっているなら半減
 */
static ACTION_SKILL_POWER calc_saving_throw(player_type *creature_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    pow = tmp_rp_ptr->r_sav + c_ptr->c_sav + a_ptr->a_sav;
    pow += ((cp_ptr->x_sav * creature_ptr->lev / 10) + (ap_ptr->a_sav * creature_ptr->lev / 50));

    if (creature_ptr->muta3 & MUT3_MAGIC_RES)
        pow += (15 + (creature_ptr->lev / 5));

    pow += adj_wis_sav[creature_ptr->stat_ind[A_WIS]];

    if (is_shero(creature_ptr))
        pow -= 30;

    if (creature_ptr->anti_magic && (pow < (90 + creature_ptr->lev)))
        pow = 90 + creature_ptr->lev;

    if (creature_ptr->tsubureru)
        pow = 10;

    if ((creature_ptr->ult_res || creature_ptr->resist_magic || creature_ptr->magicdef) && (pow < (95 + creature_ptr->lev)))
        pow = 95 + creature_ptr->lev;

    if (creature_ptr->down_saving)
        pow /= 2;

    return pow;
}

/*!
 * @brief 探索深度計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 探索深度
 * @details
 * * 種族/職業/性格による加算
 * * 職業とレベルによる追加加算
 * * 各装備による加算(TR_SEARCHがあれば+pval*5)
 * * 狂戦士化による減算(-15)
 * * 変異(MUT3_XTRA_EYES)による加算(+15)
 */
static ACTION_SKILL_POWER calc_search(player_type *creature_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    pow = tmp_rp_ptr->r_srh + c_ptr->c_srh + a_ptr->a_srh;
    pow += (c_ptr->x_srh * creature_ptr->lev / 10);

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_SEARCH))
            pow += (o_ptr->pval * 5);
    }

    if (creature_ptr->muta3 & MUT3_XTRA_EYES) {
        pow += 15;
    }

    if (is_shero(creature_ptr)) {
        pow -= 15;
    }

    return pow;
}

/*!
 * @brief 探索頻度計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 探索頻度
 * @details
 * * 種族/職業/性格による加算
 * * 職業とレベルによる追加加算
 * * 各装備による加算(TR_SEARCHがあれば+pval*5)
 * * 狂戦士化による減算(-15)
 * * 変異(MUT3_XTRA_EYES)による加算(+15)
 */
static ACTION_SKILL_POWER calc_search_freq(player_type *creature_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race *tmp_rp_ptr;

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    pow = tmp_rp_ptr->r_fos + c_ptr->c_fos + a_ptr->a_fos;
    pow += (c_ptr->x_fos * creature_ptr->lev / 10);

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_SEARCH))
            pow += (o_ptr->pval * 5);
    }

    if (is_shero(creature_ptr)) {
        pow -= 15;
    }

    if (creature_ptr->muta3 & MUT3_XTRA_EYES) {
        pow += 15;
    }

    return pow;
}

/*!
 * @brief 打撃命中能力計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 打撃命中能力
 * @details
 * * 種族/職業/性格による加算とレベルによる追加加算
 */
static ACTION_SKILL_POWER calc_to_hit_melee(player_type *creature_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race *tmp_rp_ptr;
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];

    pow = tmp_rp_ptr->r_thn + c_ptr->c_thn + a_ptr->a_thn;
    pow += ((c_ptr->x_thn * creature_ptr->lev / 10) + (a_ptr->a_thn * creature_ptr->lev / 50));
    return pow;
}

/*!
 * @brief 射撃命中能力計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 射撃命中能力
 * @details
 * * 種族/職業/性格による加算とレベルによる追加加算
 */
static ACTION_SKILL_POWER calc_to_hit_shoot(player_type *creature_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race *tmp_rp_ptr;
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];

    pow = tmp_rp_ptr->r_thb + c_ptr->c_thb + a_ptr->a_thb;
    pow += ((c_ptr->x_thb * creature_ptr->lev / 10) + (a_ptr->a_thb * creature_ptr->lev / 50));
    return pow;
}

/*!
 * @brief 投擲命中能力計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 投擲命中能力
 * @details
 * * 種族/職業/性格による加算とレベルによる追加加算
 * * 狂戦士による減算(-20)
 */
static ACTION_SKILL_POWER calc_to_hit_throw(player_type *creature_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race *tmp_rp_ptr;
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];

    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];

    pow = tmp_rp_ptr->r_thb + c_ptr->c_thb + a_ptr->a_thb;
    pow += ((c_ptr->x_thb * creature_ptr->lev / 10) + (a_ptr->a_thb * creature_ptr->lev / 50));

    if (is_shero(creature_ptr)) {
        pow -= 20;
    }

    return pow;
}

/*!
 * @brief 掘削能力計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 掘削能力値
 * @details
 * * エントが素手の場合のプラス修正
 * * 狂戦士化時のプラス修正
 * * 腕力によるテーブルプラス修正
 * * 職業狂戦士のプラス修正
 * * 装備の特性によるプラス修正
 * * 武器重量によるプラス修正
 * * 最終算出値に1を保証
 */
static ACTION_SKILL_POWER calc_skill_dig(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    ACTION_SKILL_POWER pow;

    pow = 0;

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_ENT && !creature_ptr->inventory_list[INVEN_RARM].k_idx) {
        pow += creature_ptr->lev * 10;
    }

    if (is_shero(creature_ptr))
        pow += 30;

    pow += adj_str_dig[creature_ptr->stat_ind[A_STR]];

    if (creature_ptr->pclass == CLASS_BERSERKER)
        pow += (100 + creature_ptr->lev * 8);

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_TUNNEL))
            pow += (o_ptr->pval * 20);
    }

    for (int i = 0; i < 2; i++) {
        o_ptr = &creature_ptr->inventory_list[INVEN_RARM + i];
        if (has_melee_weapon(creature_ptr, INVEN_RARM + i) && !creature_ptr->heavy_wield[i]) {
            pow += (o_ptr->weight / 10);
        }
    }

    if (is_shero(creature_ptr)) {
        pow += 30;
    }

    if (pow < 1)
        pow = 1;

    return pow;
}

static bool is_martial_arts_mode(player_type *creature_ptr)
{
    return ((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER) || (creature_ptr->pclass == CLASS_BERSERKER))
        && (empty_hands(creature_ptr, TRUE) & EMPTY_HAND_RARM) && !has_left_hand_weapon(creature_ptr);
}

static s16b calc_num_blow(player_type *creature_ptr, int i)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    s16b num_blow = 0;

    o_ptr = &creature_ptr->inventory_list[INVEN_RARM + i];
    object_flags(creature_ptr, o_ptr, flgs);
    creature_ptr->heavy_wield[i] = FALSE;
    if (!has_melee_weapon(creature_ptr, INVEN_RARM + i)) {
        num_blow = 1;
    } else {
        if (calc_weapon_weight_limit(creature_ptr) < o_ptr->weight / 10) {
            creature_ptr->heavy_wield[i] = TRUE;
        }

        if (o_ptr->k_idx && !creature_ptr->heavy_wield[i]) {
            int str_index, dex_index;
            int num = 0, wgt = 0, mul = 0, div = 0;

            num = class_info[creature_ptr->pclass].num;
            wgt = class_info[creature_ptr->pclass].wgt;
            mul = class_info[creature_ptr->pclass].mul;

            if (creature_ptr->pclass == CLASS_CAVALRY && (creature_ptr->riding) && (has_flag(flgs, TR_RIDING))) {
                num = 5;
                wgt = 70;
                mul = 4;
            }

            if (hex_spelling(creature_ptr, HEX_XTRA_MIGHT) || hex_spelling(creature_ptr, HEX_BUILDING)) {
                num++;
                wgt /= 2;
                mul += 2;
            }

            div = ((o_ptr->weight < wgt) ? wgt : o_ptr->weight);
            str_index = (adj_str_blow[creature_ptr->stat_ind[A_STR]] * mul / div);

            if (has_two_handed_weapons(creature_ptr) && !has_disable_two_handed_bonus(creature_ptr, 0))
                str_index++;
            if (creature_ptr->pclass == CLASS_NINJA)
                str_index = MAX(0, str_index - 1);
            if (str_index > 11)
                str_index = 11;

            dex_index = (adj_dex_blow[creature_ptr->stat_ind[A_DEX]]);
            if (dex_index > 11)
                dex_index = 11;

            num_blow = blows_table[str_index][dex_index];
            if (num_blow > num)
                num_blow = (s16b)num;

            num_blow += (s16b)creature_ptr->extra_blows[i];
            if (creature_ptr->pclass == CLASS_WARRIOR)
                num_blow += (creature_ptr->lev / 40);
            else if (creature_ptr->pclass == CLASS_BERSERKER)
                num_blow += (creature_ptr->lev / 23);
            else if ((creature_ptr->pclass == CLASS_ROGUE) && (o_ptr->weight < 50) && (creature_ptr->stat_ind[A_DEX] >= 30))
                num_blow++;

            if (creature_ptr->special_defense & KATA_FUUJIN)
                num_blow -= 1;

            if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE))
                num_blow = 1;

            if (num_blow < 1)
                num_blow = 1;
        }
    }

    if (i != 0)
        return num_blow;
    /* Different calculation for monks with empty hands */
    if (is_martial_arts_mode(creature_ptr)) {
        int blow_base = creature_ptr->lev + adj_dex_blow[creature_ptr->stat_ind[A_DEX]];
        num_blow = 0;

        if (creature_ptr->pclass == CLASS_FORCETRAINER) {
            if (blow_base > 18)
                num_blow++;
            if (blow_base > 31)
                num_blow++;
            if (blow_base > 44)
                num_blow++;
            if (blow_base > 58)
                num_blow++;

            MAGIC_NUM1 current_ki = get_current_ki(creature_ptr);
            if (current_ki != i) {
                creature_ptr->to_d[i] += current_ki / 5;
                creature_ptr->dis_to_d[i] += current_ki / 5;
            }
        } else {
            if (blow_base > 12)
                num_blow++;
            if (blow_base > 22)
                num_blow++;
            if (blow_base > 31)
                num_blow++;
            if (blow_base > 39)
                num_blow++;
            if (blow_base > 46)
                num_blow++;
            if (blow_base > 53)
                num_blow++;
            if (blow_base > 59)
                num_blow++;
        }

        if (heavy_armor(creature_ptr) && (creature_ptr->pclass != CLASS_BERSERKER))
            num_blow /= 2;

        if (creature_ptr->special_defense & KAMAE_GENBU) {
            creature_ptr->to_a += (creature_ptr->lev * creature_ptr->lev) / 50;
            creature_ptr->dis_to_a += (creature_ptr->lev * creature_ptr->lev) / 50;
            num_blow -= 2;
            if ((creature_ptr->pclass == CLASS_MONK) && (creature_ptr->lev > 42))
                num_blow--;
            if (num_blow < 0)
                num_blow = 0;
        } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
            creature_ptr->to_h[i] -= (creature_ptr->lev / 3);
            creature_ptr->to_d[i] -= (creature_ptr->lev / 6);

            creature_ptr->dis_to_h[i] -= (creature_ptr->lev / 3);
            creature_ptr->dis_to_d[i] -= (creature_ptr->lev / 6);
            num_blow /= 2;
        }

        num_blow += 1 + creature_ptr->extra_blows[0];
    }

    if (has_not_ninja_weapon(creature_ptr, i)) {
        num_blow /= 2;
        if (num_blow < 1)
            num_blow = 1;
    }

    return num_blow;
}

/*!
 * @brief 腕力補正計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 腕力補正値
 * @details
 * * 種族/職業/性格修正
 * * エントは別途レベル26,41,46到達ごとに加算(+1)
 * * 装備がTR_STRフラグを持っていれば加算(+pval*1)
 * * 呪術の腕力強化で加算(+4)
 * * 呪術の肉体強化で加算(+4)
 * * 降鬼陣で加算(+5)
 * * 白虎の構えで加算(+2)
 * * 朱雀の構えで減算(-2)
 * * 変異MUT3_HYPER_STRで加算(+4)
 * * 変異MUT3_PUNYで減算(-4)
 * * ネオ・つよしスペシャル中で加算(+4)
 */
static s16b calc_strength_addition(player_type *creature_ptr)
{
    s16b pow;
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    pow = tmp_rp_ptr->r_adj[A_STR] + c_ptr->c_adj[A_STR] + a_ptr->a_adj[A_STR];

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_ENT) {
        if (creature_ptr->lev > 25)
            pow++;
        if (creature_ptr->lev > 40)
            pow++;
        if (creature_ptr->lev > 45)
            pow++;
    }

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_STR)) {
            pow += o_ptr->pval;
        }
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_XTRA_MIGHT)) {
            pow += 4;
        }
        if (hex_spelling(creature_ptr, HEX_BUILDING)) {
            pow += 4;
        }
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        pow += 5;
    }

    if (creature_ptr->special_defense & KAMAE_BYAKKO) {
        pow += 2;
    } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
        pow -= 2;
    }

    if (creature_ptr->muta3) {

        if (creature_ptr->muta3 & MUT3_HYPER_STR) {
            pow += 4;
        }

        if (creature_ptr->muta3 & MUT3_PUNY) {
            pow -= 4;
        }
    }

    if (creature_ptr->tsuyoshi) {
        pow += 4;
    }
    return pow;
}

/*!
 * @brief 知力補正計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 知力補正値
 * @details
 * * 種族/職業/性格修正
 * * 装備がTR_INTフラグを持っていれば加算(+pval*1)
 * * 呪術の腕力強化で加算(+4)
 * * 呪術の肉体強化で加算(+4)
 * * 降鬼陣で加算(+5)
 * * 玄武の構えで減算(-1)
 * * 朱雀の構えで加算(+1)
 * * 変異MUT3_HYPER_INTで加算(+4)
 * * 変異MUT3_MORONICで減算(-4)
 */
s16b calc_intelligence_addition(player_type *creature_ptr)
{
    s16b pow;
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    pow = tmp_rp_ptr->r_adj[A_INT] + c_ptr->c_adj[A_INT] + a_ptr->a_adj[A_INT];

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_INT)) {
            pow += o_ptr->pval;
        }
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        pow += 5;
    }

    if (creature_ptr->special_defense & KAMAE_GENBU) {
        pow -= 1;
    } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
        pow += 1;
    }

    if (creature_ptr->muta3) {
        if (creature_ptr->muta3 & MUT3_HYPER_INT) {
            pow += 4;
        }

        if (creature_ptr->muta3 & MUT3_MORONIC) {
            pow -= 4;
        }
    }

    return pow;
}

/*!
 * @brief 賢さ補正計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 賢さ補正値
 * @details
 * * 種族/職業/性格修正
 * * 装備がTR_WISフラグを持っていれば加算(+pval*1)
 * * 呪術の腕力強化で加算(+4)
 * * 呪術の肉体強化で加算(+4)
 * * 降鬼陣で加算(+5)
 * * 玄武の構えで減算(-1)
 * * 朱雀の構えで加算(+1)
 * * 変異MUT3_HYPER_INTで加算(+4)
 * * 変異MUT3_MORONICで減算(-4)
 */
static s16b calc_wisdom_addition(player_type *creature_ptr)
{
    s16b pow;
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    pow = tmp_rp_ptr->r_adj[A_WIS] + c_ptr->c_adj[A_WIS] + a_ptr->a_adj[A_WIS];

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_WIS)) {
            pow += o_ptr->pval;
        }
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        pow += 5;
    }

    if (creature_ptr->special_defense & KAMAE_GENBU) {
        pow -= 1;
    } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
        pow += 1;
    }

    if (creature_ptr->muta3) {

        if (creature_ptr->muta3 & MUT3_HYPER_INT) {
            pow += 4;
        }

        if (creature_ptr->muta3 & MUT3_MORONIC) {
            pow -= 4;
        }
    }

    return pow;
}

/*!
 * @brief 器用さ補正計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 器用さ補正値
 * @details
 * * 種族/職業/性格修正
 * * エントは別途レベル26,41,46到達ごとに減算(-1)
 * * 装備がTR_DEXフラグを持っていれば加算(+pval*1)
 * * 呪術の肉体強化で加算(+4)
 * * 降鬼陣で加算(+5)
 * * 白虎の構えで加算(+2)
 * * 玄武の構えで減算(-2)
 * * 朱雀の構えで加算(+2)
 * * 変異MUT3_IRON_SKINで減算(-1)
 * * 変異MUT3_LIMBERで加算(+3)
 * * 変異MUT3_ARTHRITISで減算(-3)
 */
static s16b calc_dexterity_addition(player_type *creature_ptr)
{
    s16b pow;
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    pow = tmp_rp_ptr->r_adj[A_DEX] + c_ptr->c_adj[A_DEX] + a_ptr->a_adj[A_DEX];

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_ENT) {
        if (creature_ptr->lev > 25)
            pow--;
        if (creature_ptr->lev > 40)
            pow--;
        if (creature_ptr->lev > 45)
            pow--;
    }

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_DEX)) {
            pow += o_ptr->pval;
        }
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_BUILDING)) {
            pow += 4;
        }
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        pow += 5;
    }

    if (creature_ptr->special_defense & KAMAE_BYAKKO) {
        pow += 2;
    } else if (creature_ptr->special_defense & KAMAE_GENBU) {
        pow -= 2;
    } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
        pow += 2;
    }

    if (creature_ptr->muta3 & MUT3_IRON_SKIN) {
        pow -= 1;
    }

    if (creature_ptr->muta3 & MUT3_LIMBER) {
        pow += 3;
    }

    if (creature_ptr->muta3 & MUT3_ARTHRITIS) {
        pow -= 3;
    }

    return pow;
}

/*!
 * @brief 耐久力補正計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 耐久力補正値
 * @details
 * * 種族/職業/性格修正
 * * エントは別途レベル26,41,46到達ごとに加算(+1)
 * * 装備がTR_CONフラグを持っていれば加算(+pval*1)
 * * 呪術の肉体強化で加算(+4)
 * * 降鬼陣で加算(+5)
 * * 白虎の構えで減算(-3)
 * * 玄武の構えで加算(+3)
 * * 朱雀の構えで減算(-2)
 * * 変異MUT3_RESILIENTで加算(+4)
 * * 変異MUT3_ALBINOで減算(-4)
 * * 変異MUT3_XTRA_FATで加算(+2)
 * * 変異MUT3_FLESH_ROTで減算(-2)
 * * ネオ・つよしスペシャル中で加算(+4)
 */
static s16b calc_constitution_addition(player_type *creature_ptr)
{
    s16b pow;
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    pow = tmp_rp_ptr->r_adj[A_CON] + c_ptr->c_adj[A_CON] + a_ptr->a_adj[A_CON];

    if (!creature_ptr->mimic_form && creature_ptr->prace == RACE_ENT) {
        if (creature_ptr->lev > 25)
            pow++;
        if (creature_ptr->lev > 40)
            pow++;
        if (creature_ptr->lev > 45)
            pow++;
    }

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_CON))
            pow += o_ptr->pval;
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_BUILDING)) {
            pow += 4;
        }
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        pow += 5;
    }

    if (creature_ptr->special_defense & KAMAE_BYAKKO) {
        pow -= 3;
    } else if (creature_ptr->special_defense & KAMAE_GENBU) {
        pow += 3;
    } else if (creature_ptr->special_defense & KAMAE_SUZAKU) {
        pow -= 2;
    }

    if (creature_ptr->muta3) {
        if (creature_ptr->muta3 & MUT3_RESILIENT) {
            pow += 4;
        }

        if (creature_ptr->muta3 & MUT3_ALBINO) {
            pow -= 4;
        }

        if (creature_ptr->muta3 & MUT3_XTRA_FAT) {
            pow += 2;
        }

        if (creature_ptr->muta3 & MUT3_FLESH_ROT) {
            pow -= 2;
        }
    }

    if (creature_ptr->tsuyoshi) {
        pow += 4;
    }

    return pow;
}

/*!
 * @brief 魅力補正計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 魅力補正値
 * @details
 * * 種族/職業/性格修正
 * * 装備がTR_CHRフラグを持っていれば加算(+pval*1)
 * * 呪術の肉体強化で加算(+4)
 * * 降鬼陣で加算(+5)
 * * 変異MUT3_FLESH_ROTで減算(-1)
 * * 変異MUT3_SILLY_VOIで減算(-4)
 * * 変異MUT3_BLANK_FACで減算(-1)
 * * 変異MUT3_WART_SKINで減算(-2)
 * * 変異MUT3_SCALESで減算(-1)
 * * 変異MUT3_ILL_NORMで0固定(後で個体値のみ上書きを行う)
 */
static s16b calc_charisma_addition(player_type *creature_ptr)
{
    s16b pow;
    const player_race *tmp_rp_ptr;
    if (creature_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[creature_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[creature_ptr->prace];
    const player_class *c_ptr = &class_info[creature_ptr->pclass];
    const player_personality *a_ptr = &personality_info[creature_ptr->pseikaku];
    pow = tmp_rp_ptr->r_adj[A_CHR] + c_ptr->c_adj[A_CHR] + a_ptr->a_adj[A_CHR];

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_CHR))
            pow += o_ptr->pval;
    }

    if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        pow += 5;
    }

    if (creature_ptr->muta3) {
        if (creature_ptr->muta3 & MUT3_FLESH_ROT) {
            pow -= 1;
        }
        if (creature_ptr->muta3 & MUT3_SILLY_VOI) {
            pow -= 4;
        }
        if (creature_ptr->muta3 & MUT3_BLANK_FAC) {
            pow -= 1;
        }
        if (creature_ptr->muta3 & MUT3_WART_SKIN) {
            pow -= 2;
        }
        if (creature_ptr->muta3 & MUT3_SCALES) {
            pow -= 1;
        }
        if (creature_ptr->muta3 & MUT3_ILL_NORM) {
            pow = 0;
        }
    }

    return pow;
}

/*!
 * @brief 魔法失敗値計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 魔法失敗値
 * @details
 * * 性格なまけものなら加算(+10)
 * * 性格きれものなら減算(-3)
 * * 性格ちからじまんとがまんづよいなら加算(+1)
 * * 性格チャージマンなら加算(+5)
 * * 装備品にTRC_LOW_MAGICがあるなら加算(軽い呪いなら+3/重い呪いなら+10)
 */
static s16b calc_to_magic_chance(player_type *creature_ptr)
{
    s16b chance = 0;

    if (creature_ptr->pseikaku == PERSONALITY_LAZY)
        chance += 10;
    if (creature_ptr->pseikaku == PERSONALITY_SHREWD)
        chance -= 3;
    if ((creature_ptr->pseikaku == PERSONALITY_PATIENT) || (creature_ptr->pseikaku == PERSONALITY_MIGHTY))
        chance++;
    if (creature_ptr->pseikaku == PERSONALITY_CHARGEMAN)
        chance += 5;

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        object_flags(creature_ptr, o_ptr, flgs);
        if (o_ptr->curse_flags & TRC_LOW_MAGIC) {
            if (o_ptr->curse_flags & TRC_HEAVY_CURSE) {
                chance += 10;
            } else {
                chance += 3;
            }
        }
    }
    return chance;
}

static ARMOUR_CLASS calc_base_ac(player_type *creature_ptr)
{
    ARMOUR_CLASS ac = 0;
    if (creature_ptr->yoiyami)
        return 0;

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        ac += o_ptr->ac;
    }

    if (object_is_armour(creature_ptr, &creature_ptr->inventory_list[INVEN_RARM])
        || object_is_armour(creature_ptr, &creature_ptr->inventory_list[INVEN_LARM])) {
        ac += creature_ptr->skill_exp[GINOU_SHIELD] * (1 + creature_ptr->lev / 22) / 2000;
    }

    return ac;
}

static ARMOUR_CLASS calc_to_ac(player_type *creature_ptr, bool is_true_value)
{
    ARMOUR_CLASS ac = 0;
    if (creature_ptr->yoiyami)
        return 0;

    ac += ((int)(adj_dex_ta[creature_ptr->stat_ind[A_DEX]]) - 128);

    if (creature_ptr->mimic_form) {
        switch (creature_ptr->mimic_form) {
        case MIMIC_DEMON:
            ac += 10;
            break;
        case MIMIC_DEMON_LORD:
            ac += 20;
            break;
        case MIMIC_VAMPIRE:
            ac += 10;
        }
    }

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        ac += 10 + creature_ptr->lev / 2;
    }
    if (creature_ptr->pclass == CLASS_SORCERER) {
        ac -= 50;
    }

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        if (is_true_value || object_is_known(o_ptr))
            ac += o_ptr->to_a;

        if (o_ptr->curse_flags & TRC_LOW_AC) {
            if (o_ptr->curse_flags & TRC_HEAVY_CURSE) {
                if (is_true_value || object_is_fully_known(o_ptr))
                    ac -= 30;
            } else {
                if (is_true_value || object_is_fully_known(o_ptr))
                    ac -= 10;
            }
        }

        if ((i == INVEN_LARM) && (o_ptr->tval == TV_SWORD) && ((o_ptr->sval == SV_MAIN_GAUCHE) || (o_ptr->sval == SV_WAKIZASHI))) {
            ac += 5;
        }
    }

    if (is_specific_player_race(creature_ptr, RACE_GOLEM) || is_specific_player_race(creature_ptr, RACE_ANDROID)) {
        ac += 10 + (creature_ptr->lev * 2 / 5);
    }

    if ((creature_ptr->inventory_list[INVEN_RARM].name1 == ART_QUICKTHORN) && (creature_ptr->inventory_list[INVEN_LARM].name1 == ART_TINYTHORN)) {
        ac += 10;
    }

    if ((creature_ptr->inventory_list[INVEN_RARM].name1 == ART_MUSASI_KATANA) && (creature_ptr->inventory_list[INVEN_LARM].name1 == ART_MUSASI_WAKIZASI)) {
        ac += 10;
    }

    if (creature_ptr->muta3 & MUT3_WART_SKIN) {
        ac += 5;
    }

    if (creature_ptr->muta3 & MUT3_SCALES) {
        ac += 10;
    }

    if (creature_ptr->muta3 & MUT3_IRON_SKIN) {
        ac += 25;
    }

    if (((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER)) && !heavy_armor(creature_ptr)) {
        if (!(creature_ptr->inventory_list[INVEN_BODY].k_idx)) {
            ac += (creature_ptr->lev * 3) / 2;
        }
        if (!(creature_ptr->inventory_list[INVEN_OUTER].k_idx) && (creature_ptr->lev > 15)) {
            ac += ((creature_ptr->lev - 13) / 3);
        }
        if (!(creature_ptr->inventory_list[INVEN_LARM].k_idx) && (creature_ptr->lev > 10)) {
            ac += ((creature_ptr->lev - 8) / 3);
        }
        if (!(creature_ptr->inventory_list[INVEN_HEAD].k_idx) && (creature_ptr->lev > 4)) {
            ac += (creature_ptr->lev - 2) / 3;
        }
        if (!(creature_ptr->inventory_list[INVEN_HANDS].k_idx)) {
            ac += (creature_ptr->lev / 2);
        }
        if (!(creature_ptr->inventory_list[INVEN_FEET].k_idx)) {
            ac += (creature_ptr->lev / 3);
        }
    }

    if (creature_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(creature_ptr, HEX_ICE_ARMOR)) {
            ac += 30;
        }

        for (inventory_slot_type i = INVEN_RARM; i <= INVEN_FEET; i++) {
            object_type *o_ptr = &creature_ptr->inventory_list[i];
            if (!o_ptr->k_idx)
                continue;
            if (!object_is_armour(creature_ptr, o_ptr))
                continue;
            if (!object_is_cursed(o_ptr))
                continue;
            if (o_ptr->curse_flags & TRC_CURSED)
                ac += 5;
            if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
                ac += 7;
            if (o_ptr->curse_flags & TRC_PERMA_CURSE)
                ac += 13;
        }
    }

    if (creature_ptr->special_defense & KAMAE_BYAKKO) {
        ac -= 40;
    } else if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        ac -= 50;
    } else if (creature_ptr->special_defense & KATA_KOUKIJIN) {
        ac -= 50;
    }

    if (creature_ptr->ult_res || (creature_ptr->special_defense & KATA_MUSOU)) {
        ac += 100;
    } else if (creature_ptr->tsubureru || creature_ptr->shield || creature_ptr->magicdef) {
        ac += 50;
    }

    if (is_blessed(creature_ptr)) {
        ac += 5;
    }

    if (is_shero(creature_ptr)) {
        ac -= 10;
    }

    if (creature_ptr->pclass == CLASS_NINJA) {
        if ((!creature_ptr->inventory_list[INVEN_RARM].k_idx || has_right_hand_weapon(creature_ptr))
            && (!creature_ptr->inventory_list[INVEN_LARM].k_idx || has_left_hand_weapon(creature_ptr))) {
            ac += creature_ptr->lev / 2 + 5;
        }
    }

    return ac;
}

/*!
 * @brief 速度計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @return 速度値
 * @details
 * * 基礎値110(+-0に対応)
 * * 騎乗していない場合以下の処理
 * ** クラッコンと妖精に加算(+レベル/10)
 * ** 悪魔変化/吸血鬼変化で加算(+3)
 * ** 魔王変化で加算(+5)
 * ** 装備品にTR_SPEEDがあれば加算(+pval+1
 * ** 忍者の装備が重ければ減算(-レベル/10)
 * ** 忍者の装備が適正ならば加算(+3)さらにクラッコン、妖精、いかさま以外なら加算(+レベル/10)
 * ** 錬気術師で装備が重くなくクラッコン、妖精、いかさま以外なら加算(+レベル/10)
 * ** 狂戦士なら加算(+3),レベル20/30/40/50ごとに+1
 * ** いかさまでクラッコン/妖精以外なら加算(+5+レベル/10)
 * ** 加速状態中なら加算(+10)
 * ** 原則状態中なら減算(-10)
 * ** 呪術「衝撃のクローク」で加算(+3)
 * ** 食い過ぎなら減算(-10)
 * ** 朱雀の構えなら加算(+10)
 * ** 変異MUT3_XTRA_FATなら減算(-2)
 * ** 変異MUT3_XTRA_LEGなら加算(+3)
 * ** 変異MUT3_SHORT_LEGなら減算(-3)
 * ** マーフォークがFF_WATER地形にいれば加算(+2+レベル/10)
 * ** そうでなく浮遊を持っていないなら減算(-2)
 * ** 棘セット装備中ならば加算(+7)
 * * 騎乗中ならばモンスターの加速に準拠、ただし騎乗技能値とモンスターレベルによるキャップ処理あり
 * * 探索中なら減算(-10)
 * * 光速移動中かこの時点で+99を超えていたら+99にキャップ
 * * -99未満なら-99にキャップ
 */
static s16b calc_speed(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    feature_type *f_ptr = &f_info[floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat];

    s16b pow = 110;

    int j = calc_inventory_weight(creature_ptr);
    int count;

    if (!creature_ptr->riding) {
        if (is_specific_player_race(creature_ptr, RACE_KLACKON) || is_specific_player_race(creature_ptr, RACE_SPRITE))
            pow += (creature_ptr->lev) / 10;

        if (creature_ptr->mimic_form) {
            switch (creature_ptr->mimic_form) {
            case MIMIC_DEMON:
                pow += 3;
                break;
            case MIMIC_DEMON_LORD:
                pow += 5;
                break;
            case MIMIC_VAMPIRE:
                pow += 3;
                break;
            }
        }

        for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
            object_type *o_ptr = &creature_ptr->inventory_list[i];
            BIT_FLAGS flgs[TR_FLAG_SIZE];
            object_flags(creature_ptr, o_ptr, flgs);

            if (!o_ptr->k_idx)
                continue;
            if (has_flag(flgs, TR_SPEED))
                pow += o_ptr->pval;
        }

        if (creature_ptr->pclass == CLASS_NINJA) {
            if (heavy_armor(creature_ptr)) {
                pow -= (creature_ptr->lev) / 10;
            } else if ((!creature_ptr->inventory_list[INVEN_RARM].k_idx || has_right_hand_weapon(creature_ptr))
                && (!creature_ptr->inventory_list[INVEN_LARM].k_idx || has_left_hand_weapon(creature_ptr))) {
                pow += 3;
                if (!(is_specific_player_race(creature_ptr, RACE_KLACKON) || is_specific_player_race(creature_ptr, RACE_SPRITE)
                        || (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN)))
                    pow += (creature_ptr->lev) / 10;
            }
        }

        if (creature_ptr->pclass == CLASS_FORCETRAINER && !(heavy_armor(creature_ptr))) {
            if (!(is_specific_player_race(creature_ptr, RACE_KLACKON) || is_specific_player_race(creature_ptr, RACE_SPRITE)
                    || (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN)))
                pow += (creature_ptr->lev) / 10;
        }

        if (creature_ptr->pclass == CLASS_BERSERKER) {
            pow += 2;
            if (creature_ptr->lev > 29)
                pow++;
            if (creature_ptr->lev > 39)
                pow++;
            if (creature_ptr->lev > 44)
                pow++;
            if (creature_ptr->lev > 49)
                pow++;
        }

        if (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN && creature_ptr->prace != RACE_KLACKON && creature_ptr->prace != RACE_SPRITE) {
            pow += (creature_ptr->lev) / 10 + 5;
        }

        if (is_fast(creature_ptr)) {
            pow += 10;
        }

        if (creature_ptr->slow) {
            pow -= 10;
        }

        if (creature_ptr->realm1 == REALM_HEX) {
            if (hex_spelling(creature_ptr, HEX_SHOCK_CLOAK)) {
                pow += 3;
            }
        }

        if (creature_ptr->food >= PY_FOOD_MAX)
            pow -= 10;

        if (creature_ptr->special_defense & KAMAE_SUZAKU)
            pow += 10;

        if (creature_ptr->muta3) {

            if (creature_ptr->muta3 & MUT3_XTRA_FAT) {
                pow -= 2;
            }

            if (creature_ptr->muta3 & MUT3_XTRA_LEGS) {
                pow += 3;
            }

            if (creature_ptr->muta3 & MUT3_SHORT_LEG) {
                pow -= 3;
            }
        }

        if (creature_ptr->prace == RACE_MERFOLK) {
            if (has_flag(f_ptr->flags, FF_WATER)) {
                pow += (2 + creature_ptr->lev / 10);
            } else if (!creature_ptr->levitation) {
                pow -= 2;
            }
        }

        if (has_melee_weapon(creature_ptr, INVEN_RARM) && has_melee_weapon(creature_ptr, INVEN_LARM)) {
            if ((creature_ptr->inventory_list[INVEN_RARM].name1 == ART_QUICKTHORN) && (creature_ptr->inventory_list[INVEN_LARM].name1 == ART_TINYTHORN)) {
                pow += 7;
            }
        }

        count = (int)calc_weight_limit(creature_ptr);
        if (j > count)
            pow -= ((j - count) / (count / 5));

    } else {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        SPEED speed = riding_m_ptr->mspeed;

        if (riding_m_ptr->mspeed > 110) {
            pow = 110 + (s16b)((speed - 110) * (creature_ptr->skill_exp[GINOU_RIDING] * 3 + creature_ptr->lev * 160L - 10000L) / (22000L));
            if (pow < 110)
                pow = 110;
        } else {
            pow = speed;
        }

        pow += (creature_ptr->skill_exp[GINOU_RIDING] + creature_ptr->lev * 160L) / 3200;

        if (monster_fast_remaining(riding_m_ptr))
            pow += 10;
        if (monster_slow_remaining(riding_m_ptr))
            pow -= 10;

        if (creature_ptr->skill_exp[GINOU_RIDING] < RIDING_EXP_SKILLED)
            j += (creature_ptr->wt * 3 * (RIDING_EXP_SKILLED - creature_ptr->skill_exp[GINOU_RIDING])) / RIDING_EXP_SKILLED;

        count = 1500 + riding_r_ptr->level * 25;
        if (j > count)
            pow -= ((j - count) / (count / 5));
    }

    if (creature_ptr->action == ACTION_SEARCH)
        pow -= 10;

    /* Maximum speed is (+99). (internally it's 110 + 99) */
    /* Temporary lightspeed forces to be maximum speed */
    if ((creature_ptr->lightspeed && !creature_ptr->riding) || (pow > 209)) {
        pow = 209;
    }

    /* Minimum speed is (-99). (internally it's 110 - 99) */
    if (pow < 11)
        pow = 11;

    return pow;
}

/*!
 * @brief 二刀流ペナルティ量計算
 * @param creature_ptr 計算するクリーチャーの参照ポインタ
 * @param slot ペナルティ量を計算する武器スロット
 * @return 二刀流ペナルティ量
 * @details
 * * 二刀流にしていなければ0
 * * 棘セットによる軽減
 * * 源氏エゴによる軽減
 * * マンゴーシュ/脇差を左に装備した場合の軽減
 * * 武蔵セットによる軽減
 * * 竿上武器による増加
 */
s16b calc_double_weapon_penalty(player_type *creature_ptr, INVENTORY_IDX slot)
{
    int penalty = 0;
    if (has_melee_weapon(creature_ptr, INVEN_RARM) && has_melee_weapon(creature_ptr, INVEN_LARM)) {
        penalty = ((100 - creature_ptr->skill_exp[GINOU_NITOURYU] / 160) - (130 - creature_ptr->inventory_list[slot].weight) / 8);
        if ((creature_ptr->inventory_list[INVEN_RARM].name1 == ART_QUICKTHORN) && (creature_ptr->inventory_list[INVEN_LARM].name1 == ART_TINYTHORN)) {
            penalty = penalty / 2 - 5;
        }
        if (creature_ptr->easy_2weapon) {
            if (penalty > 0)
                penalty /= 2;
        } else if ((creature_ptr->inventory_list[INVEN_LARM].tval == TV_SWORD)
            && ((creature_ptr->inventory_list[INVEN_LARM].sval == SV_MAIN_GAUCHE) || (creature_ptr->inventory_list[INVEN_LARM].sval == SV_WAKIZASHI))) {
            penalty = MAX(0, penalty - 10);
        }
        if ((creature_ptr->inventory_list[INVEN_RARM].name1 == ART_MUSASI_KATANA) && (creature_ptr->inventory_list[INVEN_LARM].name1 == ART_MUSASI_WAKIZASI)) {
            penalty = MIN(0, penalty);
        } else {
            if ((creature_ptr->inventory_list[INVEN_RARM].name1 == ART_MUSASI_KATANA) && (penalty > 0))
                penalty /= 2;
            if ((creature_ptr->inventory_list[INVEN_LARM].name1 == ART_MUSASI_WAKIZASI) && (penalty > 0))
                penalty /= 2;
        }

        if (creature_ptr->inventory_list[slot].tval == TV_POLEARM)
            penalty += 10;
    }
    return (s16b)penalty;
}

static void calc_ind_status(player_type *creature_ptr, int status)
{
    int ind;
    if (creature_ptr->stat_use[status] <= 18)
        ind = (creature_ptr->stat_use[status] - 3);
    else if (creature_ptr->stat_use[status] <= 18 + 219)
        ind = (15 + (creature_ptr->stat_use[status] - 18) / 10);
    else
        ind = (37);

    if (creature_ptr->stat_ind[status] == ind)
        return;

    creature_ptr->stat_ind[status] = (s16b)ind;
    if (status == A_CON) {
        creature_ptr->update |= (PU_HP);
    } else if (status == A_INT) {
        if (mp_ptr->spell_stat == A_INT) {
            creature_ptr->update |= (PU_MANA | PU_SPELLS);
        }
    } else if (status == A_WIS) {
        if (mp_ptr->spell_stat == A_WIS) {
            creature_ptr->update |= (PU_MANA | PU_SPELLS);
        }
    } else if (status == A_CHR) {
        if (mp_ptr->spell_stat == A_CHR) {
            creature_ptr->update |= (PU_MANA | PU_SPELLS);
        }
    }

    creature_ptr->window |= (PW_PLAYER);
}

static void calc_use_status(player_type *creature_ptr, int status)
{
    int use = modify_stat_value(creature_ptr->stat_cur[status], creature_ptr->stat_add[status]);

    if ((status == A_CHR) && (creature_ptr->muta3 & MUT3_ILL_NORM)) {
        /* 10 to 18/90 charisma, guaranteed, based on level */
        if (use < 8 + 2 * creature_ptr->lev) {
            use = 8 + 2 * creature_ptr->lev;
        }
    }

    if (creature_ptr->stat_use[status] != use) {
        creature_ptr->stat_use[status] = (s16b)use;
        creature_ptr->redraw |= (PR_STATS);
        creature_ptr->window |= (PW_PLAYER);
    }
}

static void calc_top_status(player_type *creature_ptr, int status)
{
    int top = modify_stat_value(creature_ptr->stat_max[status], creature_ptr->stat_add[status]);

    if (creature_ptr->stat_top[status] != top) {
        creature_ptr->stat_top[status] = (s16b)top;
        creature_ptr->redraw |= (PR_STATS);
        creature_ptr->window |= (PW_PLAYER);
    }
}

static s16b calc_riding_bow_penalty(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (!creature_ptr->riding)
        return 0;

    s16b penalty = 0;

    creature_ptr->riding_ryoute = FALSE;

    if (has_two_handed_weapons(creature_ptr) || (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_NONE))
        creature_ptr->riding_ryoute = TRUE;
    else if (creature_ptr->pet_extra_flags & PF_TWO_HANDS) {
        switch (creature_ptr->pclass) {
        case CLASS_MONK:
        case CLASS_FORCETRAINER:
        case CLASS_BERSERKER:
            if ((empty_hands(creature_ptr, FALSE) != EMPTY_HAND_NONE) && !has_melee_weapon(creature_ptr, INVEN_RARM)
                && !has_melee_weapon(creature_ptr, INVEN_LARM))
                creature_ptr->riding_ryoute = TRUE;
            break;
        }
    }

    if ((creature_ptr->pclass == CLASS_BEASTMASTER) || (creature_ptr->pclass == CLASS_CAVALRY)) {
        if (creature_ptr->tval_ammo != TV_ARROW)
            penalty = 5;
    } else {
        penalty = r_info[floor_ptr->m_list[creature_ptr->riding].r_idx].level - creature_ptr->skill_exp[GINOU_RIDING] / 80;
        penalty += 30;
        if (penalty < 30)
            penalty = 30;
    }

    if (creature_ptr->tval_ammo == TV_BOLT)
        penalty *= 2;

    return penalty;
}

void put_equipment_warning(player_type *creature_ptr)
{
    bool heavy_shoot = is_heavy_shoot(creature_ptr, &creature_ptr->inventory_list[INVEN_BOW]);
    if (creature_ptr->old_heavy_shoot != heavy_shoot) {
        if (heavy_shoot) {
            msg_print(_("こんな重い弓を装備しているのは大変だ。", "You have trouble wielding such a heavy bow."));
        } else if (creature_ptr->inventory_list[INVEN_BOW].k_idx) {
            msg_print(_("この弓なら装備していても辛くない。", "You have no trouble wielding your bow."));
        } else {
            msg_print(_("重い弓を装備からはずして体が楽になった。", "You feel relieved to put down your heavy bow."));
        }
        creature_ptr->old_heavy_shoot = heavy_shoot;
    }

    for (int i = 0; i < 2; i++) {
        if (creature_ptr->old_heavy_wield[i] != creature_ptr->heavy_wield[i]) {
            if (creature_ptr->heavy_wield[i]) {
                msg_print(_("こんな重い武器を装備しているのは大変だ。", "You have trouble wielding such a heavy weapon."));
            } else if (has_melee_weapon(creature_ptr, INVEN_RARM + i)) {
                msg_print(_("これなら装備していても辛くない。", "You have no trouble wielding your weapon."));
            } else if (creature_ptr->heavy_wield[1 - i]) {
                msg_print(_("まだ武器が重い。", "You have still trouble wielding a heavy weapon."));
            } else {
                msg_print(_("重い武器を装備からはずして体が楽になった。", "You feel relieved to put down your heavy weapon."));
            }

            creature_ptr->old_heavy_wield[i] = creature_ptr->heavy_wield[i];
        }

        if (creature_ptr->old_riding_wield[i] != creature_ptr->riding_wield[i]) {
            if (creature_ptr->riding_wield[i]) {
                msg_print(_("この武器は乗馬中に使うにはむかないようだ。", "This weapon is not suitable for use while riding."));
            } else if (!creature_ptr->riding) {
                msg_print(_("この武器は徒歩で使いやすい。", "This weapon is suitable for use on foot."));
            } else if (has_melee_weapon(creature_ptr, INVEN_RARM + i)) {
                msg_print(_("これなら乗馬中にぴったりだ。", "This weapon is suitable for use while riding."));
            }

            creature_ptr->old_riding_wield[i] = creature_ptr->riding_wield[i];
        }

        if (creature_ptr->old_icky_wield[i] == creature_ptr->icky_wield[i])
            continue;

        if (creature_ptr->icky_wield[i]) {
            msg_print(_("今の装備はどうも自分にふさわしくない気がする。", "You do not feel comfortable with your weapon."));
            if (current_world_ptr->is_loading_now) {
                chg_virtue(creature_ptr, V_FAITH, -1);
            }
        } else if (has_melee_weapon(creature_ptr, INVEN_RARM + i)) {
            msg_print(_("今の装備は自分にふさわしい気がする。", "You feel comfortable with your weapon."));
        } else {
            msg_print(_("装備をはずしたら随分と気が楽になった。", "You feel more comfortable after removing your weapon."));
        }

        creature_ptr->old_icky_wield[i] = creature_ptr->icky_wield[i];
    }

    if (creature_ptr->riding && (creature_ptr->old_riding_ryoute != creature_ptr->riding_ryoute)) {
        if (creature_ptr->riding_ryoute) {
#ifdef JP
            msg_format("%s馬を操れない。", (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_NONE) ? "両手がふさがっていて" : "");
#else
            msg_print("You are using both hand for fighting, and you can't control the pet you're riding.");
#endif
        } else {
#ifdef JP
            msg_format("%s馬を操れるようになった。", (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_NONE) ? "手が空いて" : "");
#else
            msg_print("You began to control the pet you're riding with one hand.");
#endif
        }

        creature_ptr->old_riding_ryoute = creature_ptr->riding_ryoute;
    }

    if (((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER) || (creature_ptr->pclass == CLASS_NINJA))
        && (heavy_armor(creature_ptr) != creature_ptr->monk_notify_aux)) {
        if (heavy_armor(creature_ptr)) {
            msg_print(_("装備が重くてバランスを取れない。", "The weight of your armor disrupts your balance."));
            if (current_world_ptr->is_loading_now) {
                chg_virtue(creature_ptr, V_HARMONY, -1);
            }
        } else {
            msg_print(_("バランスがとれるようになった。", "You regain your balance."));
        }

        creature_ptr->monk_notify_aux = heavy_armor(creature_ptr);
    }
}

static s16b calc_to_damage(player_type *creature_ptr, INVENTORY_IDX slot, bool is_true_value)
{
    object_type *o_ptr = &creature_ptr->inventory_list[slot];
    int id = slot - INVEN_RARM;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(creature_ptr, o_ptr, flgs);

    s16b damage = 0;
    damage += ((int)(adj_str_td[creature_ptr->stat_ind[A_STR]]) - 128);

    if (is_shero(creature_ptr)) {
        damage += 3 + (creature_ptr->lev / 5);
    }

    if (creature_ptr->stun > 50) {
        damage -= 20;
    } else if (creature_ptr->stun) {
        damage -= 5;
    }

    if ((creature_ptr->pclass == CLASS_PRIEST) && (!(has_flag(flgs, TR_BLESSED))) && ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM))) {
        damage -= 2;
    } else if (creature_ptr->pclass == CLASS_BERSERKER) {
        damage += creature_ptr->lev / 6;
        if (((id == 0) && !has_left_hand_weapon(creature_ptr)) || has_two_handed_weapons(creature_ptr)) {
            damage += creature_ptr->lev / 6;
        }
    } else if (creature_ptr->pclass == CLASS_SORCERER) {
        if (!((o_ptr->tval == TV_HAFTED) && ((o_ptr->sval == SV_WIZSTAFF) || (o_ptr->sval == SV_NAMAKE_HAMMER)))) {
            damage -= 200;
        } else {
            damage -= 10;
        }
    }

    if ((creature_ptr->realm1 == REALM_HEX) && object_is_cursed(o_ptr)) {
        if (hex_spelling(creature_ptr, HEX_RUNESWORD)) {
            if (o_ptr->curse_flags & (TRC_CURSED)) {
                damage += 5;
            }
            if (o_ptr->curse_flags & (TRC_HEAVY_CURSE)) {
                damage += 7;
            }
            if (o_ptr->curse_flags & (TRC_PERMA_CURSE)) {
                damage += 13;
            }
        }
    }

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        int bonus_to_d = 0;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx || o_ptr->tval == TV_CAPTURE || (i == INVEN_RARM && has_melee_weapon(creature_ptr, i))
            || (i == INVEN_LARM && has_melee_weapon(creature_ptr, i)) || i == INVEN_BOW)
            continue;

        if (!object_is_known(o_ptr) && !is_true_value)
            continue;
        bonus_to_d = o_ptr->to_d;

        if (creature_ptr->pclass == CLASS_NINJA) {
            if (o_ptr->to_d > 0)
                bonus_to_d = (o_ptr->to_d + 1) / 2;
        }

        if ((i == INVEN_LEFT || i == INVEN_RIGHT) && !has_two_handed_weapons(creature_ptr)) {
            damage += (s16b)bonus_to_d;
        } else if (has_right_hand_weapon(creature_ptr) && has_left_hand_weapon(creature_ptr)) {
            if (id == 0)
                damage += (bonus_to_d > 0) ? (bonus_to_d + 1) / 2 : bonus_to_d;
            if (id == 1)
                damage += (bonus_to_d > 0) ? bonus_to_d / 2 : bonus_to_d;
        } else if (id == get_default_hand(creature_ptr)) {
            damage += (s16b)bonus_to_d;
        }
    }

    if (get_default_hand(creature_ptr) == id) {
        if ((is_martial_arts_mode(creature_ptr) && empty_hands(creature_ptr, FALSE) == (EMPTY_HAND_RARM | EMPTY_HAND_LARM))
            || !has_disable_two_handed_bonus(creature_ptr, 0)) {
            int bonus_to_d = 0;
            bonus_to_d = ((int)(adj_str_td[creature_ptr->stat_ind[A_STR]]) - 128) / 2;
            damage += MAX(bonus_to_d, 1);
        }
    }

    if (is_martial_arts_mode(creature_ptr) && (!heavy_armor(creature_ptr) || creature_ptr->pclass != CLASS_BERSERKER)) {
        damage += (creature_ptr->lev / 6);
    }

    return damage;
}

/*!
 * @brief 武器の命中修正を計算する。 / Calculate hit bonus from a wielded weapon.
 * @details
 * 'slot' MUST be INVEN_RARM or INVEM_LARM.
 */
static s16b calc_to_hit(player_type *creature_ptr, INVENTORY_IDX slot, bool is_true_value)
{
    s16b hit = 0;

    /* Base bonuses */
    hit += ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    hit += ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128);

    /* Temporary bonuses */
    if (is_blessed(creature_ptr)) {
        hit += 10;
    }

    if (is_hero(creature_ptr)) {
        hit += 12;
    }

    if (is_shero(creature_ptr)) {
        hit += 12;
    }

    if (creature_ptr->stun > 50) {
        hit -= 20;
    } else if (creature_ptr->stun) {
        hit -= 5;
    }

    /* Default hand bonuses */
    int id = slot - INVEN_RARM;
    int default_hand = get_default_hand(creature_ptr);
    if (default_hand == id) {
        /* Add trained bonus of empty hands' combat when having no weapon and riding */
        if ((!has_right_hand_weapon(creature_ptr) && (empty_hands(creature_ptr, TRUE) & EMPTY_HAND_LARM))
            || (!has_left_hand_weapon(creature_ptr) && (empty_hands(creature_ptr, TRUE) & EMPTY_HAND_RARM))) {
            hit += (creature_ptr->skill_exp[GINOU_SUDE] - WEAPON_EXP_BEGINNER) / 200;
        }

        if ((is_martial_arts_mode(creature_ptr) && empty_hands(creature_ptr, FALSE) == (EMPTY_HAND_RARM | EMPTY_HAND_LARM))
            || !has_disable_two_handed_bonus(creature_ptr, 0)) {
            int bonus_to_h = 0;
            bonus_to_h = ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128) + ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
            hit += MAX(bonus_to_h, 1);
        }
    }

    /* Bonuses and penalties by weapon */
    if (has_melee_weapon(creature_ptr, slot)) {
        object_type *o_ptr = &creature_ptr->inventory_list[slot];
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        object_flags(creature_ptr, o_ptr, flgs);

        tval_type tval = o_ptr->tval - TV_WEAPON_BEGIN;
        OBJECT_SUBTYPE_VALUE sval = o_ptr->sval;

        /* Traind bonuses */
        hit += (creature_ptr->weapon_exp[tval][sval] - WEAPON_EXP_BEGINNER) / 200;

        /* Weight penalty */
        if (calc_weapon_weight_limit(creature_ptr) < o_ptr->weight / 10) {
            hit += 2 * (calc_weapon_weight_limit(creature_ptr) - o_ptr->weight / 10);
        }

        /* Low melee penalty */
        if ((object_is_fully_known(o_ptr) || is_true_value) && o_ptr->curse_flags & TRC_LOW_MELEE) {
            if (o_ptr->curse_flags & TRC_HEAVY_CURSE) {
                hit -= 15;
            } else {
                hit -= 5;
            }
        }

        /* Riding bonus and penalty */
        if (creature_ptr->riding) {
            if ((o_ptr->tval == TV_POLEARM) && ((o_ptr->sval == SV_LANCE) || (o_ptr->sval == SV_HEAVY_LANCE))) {
                hit += 15;
            }
        }

        if (creature_ptr->riding != 0 && !(o_ptr->tval == TV_POLEARM) && ((o_ptr->sval == SV_LANCE) || (o_ptr->sval == SV_HEAVY_LANCE))
            && !has_flag(flgs, TR_RIDING)) {

            int penalty;
            if ((creature_ptr->pclass == CLASS_BEASTMASTER) || (creature_ptr->pclass == CLASS_CAVALRY)) {
                penalty = 5;
            } else {
                penalty = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level - creature_ptr->skill_exp[GINOU_RIDING] / 80;
                penalty += 30;
                if (penalty < 30)
                    penalty = 30;
            }
            hit -= (s16b)penalty;
        }

        /* Class penalties */
        if ((creature_ptr->pclass == CLASS_PRIEST) && (!(has_flag(flgs, TR_BLESSED))) && ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM))) {
            hit -= 2;
        } else if (creature_ptr->pclass == CLASS_BERSERKER) {
            hit += creature_ptr->lev / 5;
            if (((id == 0) && !has_left_hand_weapon(creature_ptr)) || has_two_handed_weapons(creature_ptr)) {
                hit += creature_ptr->lev / 5;
            }
        } else if (creature_ptr->pclass == CLASS_SORCERER) {
            if (!((o_ptr->tval == TV_HAFTED) && ((o_ptr->sval == SV_WIZSTAFF) || (o_ptr->sval == SV_NAMAKE_HAMMER)))) {
                hit -= 200;
            } else {
                hit -= 30;
            }
        }

        if (has_not_ninja_weapon(creature_ptr, id) || has_not_monk_weapon(creature_ptr, id)) {
            hit -= 40;
        }

        /* Hex realm bonuses */
        if ((creature_ptr->realm1 == REALM_HEX) && object_is_cursed(o_ptr)) {
            if (o_ptr->curse_flags & (TRC_CURSED)) {
                hit += 5;
            }
            if (o_ptr->curse_flags & (TRC_HEAVY_CURSE)) {
                hit += 7;
            }
            if (o_ptr->curse_flags & (TRC_PERMA_CURSE)) {
                hit += 13;
            }
            if (o_ptr->curse_flags & (TRC_TY_CURSE)) {
                hit += 5;
            }
        }
    }

    /* Bonuses from inventory */
    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];

        /* Ignore empty hands, handed weapons, bows and capture balls */
        if (!o_ptr->k_idx
            || o_ptr->tval == TV_CAPTURE
            || (i == INVEN_RARM && has_melee_weapon(creature_ptr, i))
            || (i == INVEN_LARM && has_melee_weapon(creature_ptr, i))
            || i == INVEN_BOW)
            continue;

        /* Fake value does not include unknown objects' value */
        if (is_true_value || !object_is_known(o_ptr))
            continue;

        int bonus_to_h = o_ptr->to_h;

        /* When wields only a weapon */
        if (creature_ptr->pclass == CLASS_NINJA) {
            if (o_ptr->to_h > 0)
                bonus_to_h = (o_ptr->to_h + 1) / 2;
        }

        if ((i == INVEN_LEFT || i == INVEN_RIGHT) && !has_two_handed_weapons(creature_ptr)) {
            hit += (s16b)bonus_to_h;
            continue;
        }

        /* When wields two weapons on each hand */
        if (has_right_hand_weapon(creature_ptr) && has_left_hand_weapon(creature_ptr)) {
            if (default_hand == 0)
                hit += (bonus_to_h > 0) ? (bonus_to_h + 1) / 2 : bonus_to_h;
            if (default_hand == 1)
                hit += (bonus_to_h > 0) ? bonus_to_h / 2 : bonus_to_h;
            continue;
        }

        if (default_hand == id)
            hit += (s16b)bonus_to_h;
    }

    /* Martial arts bonus */
    if (is_martial_arts_mode(creature_ptr) && (!heavy_armor(creature_ptr) || creature_ptr->pclass != CLASS_BERSERKER)) {
        hit += (creature_ptr->lev / 3);
    }

    /* Two handed combat penalty */
    hit -= calc_double_weapon_penalty(creature_ptr, slot);

    return hit;
}

static s16b calc_to_hit_bow(player_type *creature_ptr, bool is_true_value)
{
    s16b pow = 0;

    pow += ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    pow += ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128);

    {
        object_type *o_ptr;
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[INVEN_BOW];
        if (o_ptr->k_idx) {
            object_flags(creature_ptr, o_ptr, flgs);

            if (o_ptr->curse_flags & TRC_LOW_MELEE) {
                if (o_ptr->curse_flags & TRC_HEAVY_CURSE) {
                    pow -= 15;
                } else {
                    pow -= 5;
                }
            }
        }
    }

    if (creature_ptr->stun > 50) {
        pow -= 20;
    } else if (creature_ptr->stun) {
        pow -= 5;
    }

    if (is_blessed(creature_ptr)) {
        pow += 10;
    }

    if (is_hero(creature_ptr)) {
        pow += 12;
    }

    if (is_shero(creature_ptr)) {
        pow -= 12;
    }

    object_type *o_ptr = &creature_ptr->inventory_list[INVEN_BOW];

    if (is_heavy_shoot(creature_ptr, o_ptr)) {
        pow += 2 * (calc_weapon_weight_limit(creature_ptr) - o_ptr->weight / 10);
    }

    if (o_ptr->k_idx) {
        if (o_ptr->k_idx && !is_heavy_shoot(creature_ptr, &creature_ptr->inventory_list[INVEN_BOW])) {
            if ((creature_ptr->pclass == CLASS_SNIPER) && (creature_ptr->tval_ammo == TV_BOLT)) {
                pow += (10 + (creature_ptr->lev / 5));
            }
        }
    }

    // 武器以外の装備による修正
    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        int bonus_to_h;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx || o_ptr->tval == TV_CAPTURE || (i == INVEN_RARM && has_melee_weapon(creature_ptr, i))
            || (i == INVEN_LARM && has_melee_weapon(creature_ptr, i)) || i == INVEN_BOW)
            continue;

        bonus_to_h = o_ptr->to_h;

        if (creature_ptr->pclass == CLASS_NINJA) {
            if (o_ptr->to_h > 0)
                bonus_to_h = (o_ptr->to_h + 1) / 2;
        }

        if (is_true_value || object_is_known(o_ptr))
            pow += (s16b)bonus_to_h;
    }

    pow -= calc_riding_bow_penalty(creature_ptr);

    return pow;
}

static s16b calc_to_damage_misc(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    s16b to_dam = 0;

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        int bonus_to_d = o_ptr->to_d;
        if (creature_ptr->pclass == CLASS_NINJA) {
            if (o_ptr->to_d > 0)
                bonus_to_d = (o_ptr->to_d + 1) / 2;
        }
        to_dam += (s16b)bonus_to_d;
    }

    if (is_shero(creature_ptr)) {
        to_dam += 3 + (creature_ptr->lev / 5);
    }

    if (creature_ptr->stun > 50) {
        to_dam -= 20;
    } else if (creature_ptr->stun) {
        to_dam -= 5;
    }

    to_dam += ((int)(adj_str_td[creature_ptr->stat_ind[A_STR]]) - 128);
    return to_dam;
}

static s16b calc_to_hit_misc(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    s16b to_hit = 0;

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        int bonus_to_h = o_ptr->to_h;
        if (creature_ptr->pclass == CLASS_NINJA) {
            if (o_ptr->to_h > 0)
                bonus_to_h = (o_ptr->to_h + 1) / 2;
        }
        to_hit += (s16b)bonus_to_h;
    }

    if (is_blessed(creature_ptr)) {
        to_hit += 10;
    }

    if (is_hero(creature_ptr)) {
        to_hit += 12;
    }

    if (is_shero(creature_ptr)) {
        to_hit += 12;
    }

    if (creature_ptr->stun > 50) {
        to_hit -= 20;
    } else if (creature_ptr->stun) {
        to_hit -= 5;
    }

    to_hit += ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    to_hit += ((int)(adj_str_th[creature_ptr->stat_ind[A_STR]]) - 128);

    return to_hit;
}

static DICE_NUMBER calc_to_weapon_dice_num(player_type *creature_ptr, INVENTORY_IDX slot)
{
    object_type *o_ptr = &creature_ptr->inventory_list[slot];
    DICE_NUMBER dn = 0;

    if (creature_ptr->riding) {

        if ((o_ptr->tval == TV_POLEARM) && ((o_ptr->sval == SV_LANCE) || (o_ptr->sval == SV_HEAVY_LANCE))) {
            dn += 2;
        }
    }

    return dn;
}

static DICE_NUMBER calc_to_weapon_dice_side(player_type *creature_ptr, INVENTORY_IDX slot)
{
    (void)creature_ptr; // unused
    (void)slot; // unused
    return 0;
}

/*!
 * @brief プレイヤーの所持重量制限を計算する /
 * Computes current weight limit.
 * @return 制限重量(ポンド)
 */
WEIGHT calc_weight_limit(player_type *creature_ptr)
{
    WEIGHT i = (WEIGHT)adj_str_wgt[creature_ptr->stat_ind[A_STR]] * 50;
    if (creature_ptr->pclass == CLASS_BERSERKER)
        i = i * 3 / 2;
    return i;
}

/*!
 * @brief プレイヤーが現在右手/左手に武器を持っているか判定する /
 * @param i 判定する手のID(右手:INVEN_RARM 左手:INVEN_LARM)
 * @return 持っているならばTRUE
 */
bool has_melee_weapon(player_type *creature_ptr, int slot)
{
    return ((creature_ptr->inventory_list[slot].k_idx) && object_is_melee_weapon(&creature_ptr->inventory_list[slot]));
}

/*!
 * @brief プレイヤーの現在開いている手の状態を返す
 * @param riding_control 乗馬中により片手を必要としている状態ならばTRUEを返す。
 * @return 開いている手のビットフラグ
 */
BIT_FLAGS16 empty_hands(player_type *creature_ptr, bool riding_control)
{
    BIT_FLAGS16 status = EMPTY_HAND_NONE;
    if (!creature_ptr->inventory_list[INVEN_RARM].k_idx)
        status |= EMPTY_HAND_RARM;
    if (!creature_ptr->inventory_list[INVEN_LARM].k_idx)
        status |= EMPTY_HAND_LARM;

    if (riding_control && (status != EMPTY_HAND_NONE) && creature_ptr->riding && !(creature_ptr->pet_extra_flags & PF_TWO_HANDS)) {
        if (status & EMPTY_HAND_LARM)
            status &= ~(EMPTY_HAND_LARM);
        else if (status & EMPTY_HAND_RARM)
            status &= ~(EMPTY_HAND_RARM);
    }

    return status;
}

/*!
 * @brief プレイヤーが防具重量制限のある職業時にペナルティを受ける状態にあるかどうかを返す。
 * @return ペナルティが適用されるならばTRUE。
 */
bool heavy_armor(player_type *creature_ptr)
{
    if ((creature_ptr->pclass != CLASS_MONK) && (creature_ptr->pclass != CLASS_FORCETRAINER) && (creature_ptr->pclass != CLASS_NINJA))
        return FALSE;

    WEIGHT monk_arm_wgt = 0;
    if (creature_ptr->inventory_list[INVEN_RARM].tval > TV_SWORD)
        monk_arm_wgt += creature_ptr->inventory_list[INVEN_RARM].weight;
    if (creature_ptr->inventory_list[INVEN_LARM].tval > TV_SWORD)
        monk_arm_wgt += creature_ptr->inventory_list[INVEN_LARM].weight;
    monk_arm_wgt += creature_ptr->inventory_list[INVEN_BODY].weight;
    monk_arm_wgt += creature_ptr->inventory_list[INVEN_HEAD].weight;
    monk_arm_wgt += creature_ptr->inventory_list[INVEN_OUTER].weight;
    monk_arm_wgt += creature_ptr->inventory_list[INVEN_HANDS].weight;
    monk_arm_wgt += creature_ptr->inventory_list[INVEN_FEET].weight;

    return (monk_arm_wgt > (100 + (creature_ptr->lev * 4)));
}

/*!
 * @brief update のフラグに応じた更新をまとめて行う / Handle "update"
 * @return なし
 * @details 更新処理の対象はプレイヤーの能力修正/光源寿命/HP/MP/魔法の学習状態、他多数の外界の状態判定。
 */
void update_creature(player_type *creature_ptr)
{
    if (!creature_ptr->update)
        return;

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (creature_ptr->update & (PU_AUTODESTROY)) {
        creature_ptr->update &= ~(PU_AUTODESTROY);
        autopick_delayed_alter(creature_ptr);
    }

    if (creature_ptr->update & (PU_COMBINE)) {
        creature_ptr->update &= ~(PU_COMBINE);
        combine_pack(creature_ptr);
    }

    if (creature_ptr->update & (PU_REORDER)) {
        creature_ptr->update &= ~(PU_REORDER);
        reorder_pack(creature_ptr);
    }

    if (creature_ptr->update & (PU_BONUS)) {
        creature_ptr->update &= ~(PU_BONUS);
        calc_alignment(creature_ptr);
        calc_bonuses(creature_ptr);
    }

    if (creature_ptr->update & (PU_TORCH)) {
        creature_ptr->update &= ~(PU_TORCH);
        calc_lite_radius(creature_ptr);
    }

    if (creature_ptr->update & (PU_HP)) {
        creature_ptr->update &= ~(PU_HP);
        calc_hitpoints(creature_ptr);
    }

    if (creature_ptr->update & (PU_MANA)) {
        creature_ptr->update &= ~(PU_MANA);
        calc_mana(creature_ptr);
    }

    if (creature_ptr->update & (PU_SPELLS)) {
        creature_ptr->update &= ~(PU_SPELLS);
        calc_spells(creature_ptr);
    }

    if (!current_world_ptr->character_generated)
        return;
    if (current_world_ptr->character_icky)
        return;
    if (creature_ptr->update & (PU_UN_LITE)) {
        creature_ptr->update &= ~(PU_UN_LITE);
        forget_lite(floor_ptr);
    }

    if (creature_ptr->update & (PU_UN_VIEW)) {
        creature_ptr->update &= ~(PU_UN_VIEW);
        forget_view(floor_ptr);
    }

    if (creature_ptr->update & (PU_VIEW)) {
        creature_ptr->update &= ~(PU_VIEW);
        update_view(creature_ptr);
    }

    if (creature_ptr->update & (PU_LITE)) {
        creature_ptr->update &= ~(PU_LITE);
        update_lite(creature_ptr);
    }

    if (creature_ptr->update & (PU_FLOW)) {
        creature_ptr->update &= ~(PU_FLOW);
        update_flow(creature_ptr);
    }

    if (creature_ptr->update & (PU_DISTANCE)) {
        creature_ptr->update &= ~(PU_DISTANCE);

        update_monsters(creature_ptr, TRUE);
    }

    if (creature_ptr->update & (PU_MON_LITE)) {
        creature_ptr->update &= ~(PU_MON_LITE);
        update_mon_lite(creature_ptr);
    }

    if (creature_ptr->update & (PU_DELAY_VIS)) {
        creature_ptr->update &= ~(PU_DELAY_VIS);
        delayed_visual_update(creature_ptr);
    }

    if (creature_ptr->update & (PU_MONSTERS)) {
        creature_ptr->update &= ~(PU_MONSTERS);
        update_monsters(creature_ptr, FALSE);
    }
}

/*!
 * @brief プレイヤーが魔道書を一冊も持っていないかを判定する
 * @return 魔道書を一冊も持っていないならTRUEを返す
 */
bool player_has_no_spellbooks(player_type *creature_ptr)
{
    object_type *o_ptr;
    for (int i = 0; i < INVEN_PACK; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (o_ptr->k_idx && check_book_realm(creature_ptr, o_ptr->tval, o_ptr->sval))
            return FALSE;
    }

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    for (int i = floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].o_idx; i; i = o_ptr->next_o_idx) {
        o_ptr = &floor_ptr->o_list[i];
        if (o_ptr->k_idx && (o_ptr->marked & OM_FOUND) && check_book_realm(creature_ptr, o_ptr->tval, o_ptr->sval))
            return FALSE;
    }

    return TRUE;
}

void take_turn(player_type *creature_ptr, PERCENTAGE need_cost) { creature_ptr->energy_use = (ENERGY)need_cost; }

void free_turn(player_type *creature_ptr) { creature_ptr->energy_use = 0; }

/*!
 * @brief プレイヤーを指定座標に配置する / Place the player in the dungeon XXX XXX
 * @param x 配置先X座標
 * @param y 配置先Y座標
 * @return 配置に成功したらTRUE
 */
bool player_place(player_type *creature_ptr, POSITION y, POSITION x)
{
    if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx != 0)
        return FALSE;

    /* Save player location */
    creature_ptr->y = y;
    creature_ptr->x = x;
    return TRUE;
}

/*!
 * @brief 種族アンバライトが出血時パターンの上に乗った際のペナルティ処理
 * @return なし
 */
void wreck_the_pattern(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    int pattern_type = f_info[floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat].subtype;
    if (pattern_type == PATTERN_TILE_WRECKED)
        return;

    msg_print(_("パターンを血で汚してしまった！", "You bleed on the Pattern!"));
    msg_print(_("何か恐ろしい事が起こった！", "Something terrible happens!"));

    if (!is_invuln(creature_ptr))
        take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(10, 8), _("パターン損壊", "corrupting the Pattern"), -1);

    int to_ruin = randint1(45) + 35;
    while (to_ruin--) {
        POSITION r_y, r_x;
        scatter(creature_ptr, &r_y, &r_x, creature_ptr->y, creature_ptr->x, 4, PROJECT_NONE);

        if (pattern_tile(floor_ptr, r_y, r_x) && (f_info[floor_ptr->grid_array[r_y][r_x].feat].subtype != PATTERN_TILE_WRECKED)) {
            cave_set_feat(creature_ptr, r_y, r_x, feat_pattern_corrupted);
        }
    }

    cave_set_feat(creature_ptr, creature_ptr->y, creature_ptr->x, feat_pattern_corrupted);
}

/*!
 * @brief プレイヤーの経験値について整合性のためのチェックと調整を行う /
 * Advance experience levels and print experience
 * @return なし
 */
void check_experience(player_type *creature_ptr)
{
    if (creature_ptr->exp < 0)
        creature_ptr->exp = 0;
    if (creature_ptr->max_exp < 0)
        creature_ptr->max_exp = 0;
    if (creature_ptr->max_max_exp < 0)
        creature_ptr->max_max_exp = 0;

    if (creature_ptr->exp > PY_MAX_EXP)
        creature_ptr->exp = PY_MAX_EXP;
    if (creature_ptr->max_exp > PY_MAX_EXP)
        creature_ptr->max_exp = PY_MAX_EXP;
    if (creature_ptr->max_max_exp > PY_MAX_EXP)
        creature_ptr->max_max_exp = PY_MAX_EXP;

    if (creature_ptr->exp > creature_ptr->max_exp)
        creature_ptr->max_exp = creature_ptr->exp;
    if (creature_ptr->max_exp > creature_ptr->max_max_exp)
        creature_ptr->max_max_exp = creature_ptr->max_exp;

    creature_ptr->redraw |= (PR_EXP);
    handle_stuff(creature_ptr);

    bool android = (creature_ptr->prace == RACE_ANDROID ? TRUE : FALSE);
    PLAYER_LEVEL old_lev = creature_ptr->lev;
    while ((creature_ptr->lev > 1) && (creature_ptr->exp < ((android ? player_exp_a : player_exp)[creature_ptr->lev - 2] * creature_ptr->expfact / 100L))) {
        creature_ptr->lev--;
        creature_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
        creature_ptr->redraw |= (PR_LEV | PR_TITLE);
        creature_ptr->window |= (PW_PLAYER);
        handle_stuff(creature_ptr);
    }

    bool level_reward = FALSE;
    bool level_mutation = FALSE;
    bool level_inc_stat = FALSE;
    while ((creature_ptr->lev < PY_MAX_LEVEL)
        && (creature_ptr->exp >= ((android ? player_exp_a : player_exp)[creature_ptr->lev - 1] * creature_ptr->expfact / 100L))) {
        creature_ptr->lev++;
        if (creature_ptr->lev > creature_ptr->max_plv) {
            creature_ptr->max_plv = creature_ptr->lev;

            if ((creature_ptr->pclass == CLASS_CHAOS_WARRIOR) || (creature_ptr->muta2 & MUT2_CHAOS_GIFT)) {
                level_reward = TRUE;
            }
            if (creature_ptr->prace == RACE_BEASTMAN) {
                if (one_in_(5))
                    level_mutation = TRUE;
            }
            level_inc_stat = TRUE;

            exe_write_diary(creature_ptr, DIARY_LEVELUP, creature_ptr->lev, NULL);
        }

        sound(SOUND_LEVEL);
        msg_format(_("レベル %d にようこそ。", "Welcome to level %d."), creature_ptr->lev);
        creature_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
        creature_ptr->redraw |= (PR_LEV | PR_TITLE | PR_EXP);
        creature_ptr->window |= (PW_PLAYER | PW_SPELL | PW_INVEN);
        creature_ptr->level_up_message = TRUE;
        handle_stuff(creature_ptr);

        creature_ptr->level_up_message = FALSE;
        if (level_inc_stat) {
            if (!(creature_ptr->max_plv % 10)) {
                int choice;
                screen_save();
                while (TRUE) {
                    int n;
                    char tmp[32];

                    cnv_stat(creature_ptr->stat_max[0], tmp);
                    prt(format(_("        a) 腕力 (現在値 %s)", "        a) Str (cur %s)"), tmp), 2, 14);
                    cnv_stat(creature_ptr->stat_max[1], tmp);
                    prt(format(_("        b) 知能 (現在値 %s)", "        b) Int (cur %s)"), tmp), 3, 14);
                    cnv_stat(creature_ptr->stat_max[2], tmp);
                    prt(format(_("        c) 賢さ (現在値 %s)", "        c) Wis (cur %s)"), tmp), 4, 14);
                    cnv_stat(creature_ptr->stat_max[3], tmp);
                    prt(format(_("        d) 器用 (現在値 %s)", "        d) Dex (cur %s)"), tmp), 5, 14);
                    cnv_stat(creature_ptr->stat_max[4], tmp);
                    prt(format(_("        e) 耐久 (現在値 %s)", "        e) Con (cur %s)"), tmp), 6, 14);
                    cnv_stat(creature_ptr->stat_max[5], tmp);
                    prt(format(_("        f) 魅力 (現在値 %s)", "        f) Chr (cur %s)"), tmp), 7, 14);

                    prt("", 8, 14);
                    prt(_("        どの能力値を上げますか？", "        Which stat do you want to raise?"), 1, 14);

                    while (TRUE) {
                        choice = inkey();
                        if ((choice >= 'a') && (choice <= 'f'))
                            break;
                    }
                    for (n = 0; n < A_MAX; n++)
                        if (n != choice - 'a')
                            prt("", n + 2, 14);
                    if (get_check(_("よろしいですか？", "Are you sure? ")))
                        break;
                }
                do_inc_stat(creature_ptr, choice - 'a');
                screen_load();
            } else if (!(creature_ptr->max_plv % 2))
                do_inc_stat(creature_ptr, randint0(6));
        }

        if (level_mutation) {
            msg_print(_("あなたは変わった気がする...", "You feel different..."));
            (void)gain_mutation(creature_ptr, 0);
            level_mutation = FALSE;
        }

        /*
         * 報酬でレベルが上ると再帰的に check_experience(creature_ptr) が
         * 呼ばれるので順番を最後にする。
         */
        if (level_reward) {
            gain_level_reward(creature_ptr, 0);
            level_reward = FALSE;
        }

        creature_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
        creature_ptr->redraw |= (PR_LEV | PR_TITLE);
        creature_ptr->window |= (PW_PLAYER | PW_SPELL);
        handle_stuff(creature_ptr);
    }

    if (old_lev != creature_ptr->lev)
        autopick_load_pref(creature_ptr, FALSE);
}

/*!
 * @brief 現在の修正後能力値を3～17及び18/xxx形式に変換する / Converts stat num into a six-char (right justified) string
 * @param val 能力値
 * @param out_val 出力先文字列ポインタ
 * @return なし
 */
void cnv_stat(int val, char *out_val)
{
    if (val <= 18) {
        sprintf(out_val, "    %2d", val);
        return;
    }

    int bonus = (val - 18);
    if (bonus >= 220) {
        sprintf(out_val, "18/%3s", "***");
    } else if (bonus >= 100) {
        sprintf(out_val, "18/%03d", bonus);
    } else {
        sprintf(out_val, " 18/%02d", bonus);
    }
}

/*!
 * @brief 能力値現在値から3～17及び18/xxx様式に基づく加減算を行う。
 * Modify a stat value by a "modifier", return new value
 * @param value 現在値
 * @param amount 加減算値
 * @return 加減算後の値
 * @details
 * <pre>
 * Stats go up: 3,4,...,17,18,18/10,18/20,...,18/220
 * Or even: 18/13, 18/23, 18/33, ..., 18/220
 * Stats go down: 18/220, 18/210,..., 18/10, 18, 17, ..., 3
 * Or even: 18/13, 18/03, 18, 17, ..., 3
 * </pre>
 */
s16b modify_stat_value(int value, int amount)
{
    if (amount > 0) {
        for (int i = 0; i < amount; i++) {
            if (value < 18)
                value++;
            else
                value += 10;
        }
    } else if (amount < 0) {
        for (int i = 0; i < (0 - amount); i++) {
            if (value >= 18 + 10)
                value -= 10;
            else if (value > 18)
                value = 18;
            else if (value > 3)
                value--;
        }
    }

    return (s16b)value;
}

/*!
 * @brief スコアを計算する /
 * Hack -- Calculates the total number of points earned		-JWT-
 * @return なし
 * @details
 */
long calc_score(player_type *creature_ptr)
{
    int arena_win = MIN(creature_ptr->arena_number, MAX_ARENA_MONS);

    int mult = 100;
    if (!preserve_mode)
        mult += 10;
    if (!autoroller)
        mult += 10;
    if (!smart_learn)
        mult -= 20;
    if (smart_cheat)
        mult += 30;
    if (ironman_shops)
        mult += 50;
    if (ironman_small_levels)
        mult += 10;
    if (ironman_empty_levels)
        mult += 20;
    if (!powerup_home)
        mult += 50;
    if (ironman_rooms)
        mult += 100;
    if (ironman_nightmare)
        mult += 100;

    if (mult < 5)
        mult = 5;

    DEPTH max_dl = 0;
    for (int i = 0; i < current_world_ptr->max_d_idx; i++)
        if (max_dlv[i] > max_dl)
            max_dl = max_dlv[i];

    u32b point_l = (creature_ptr->max_max_exp + (100 * max_dl));
    u32b point_h = point_l / 0x10000L;
    point_l = point_l % 0x10000L;
    point_h *= mult;
    point_l *= mult;
    point_h += point_l / 0x10000L;
    point_l %= 0x10000L;

    point_l += ((point_h % 100) << 16);
    point_h /= 100;
    point_l /= 100;

    u32b point = (point_h << 16) + (point_l);
    if (creature_ptr->arena_number >= 0)
        point += (arena_win * arena_win * (arena_win > 29 ? 1000 : 100));

    if (ironman_downward)
        point *= 2;
    if (creature_ptr->pclass == CLASS_BERSERKER) {
        if (creature_ptr->prace == RACE_SPECTRE)
            point = point / 5;
    }

    if ((creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) && point) {
        point = 1;
        if (current_world_ptr->total_winner)
            point = 2;
    }

    if (easy_band)
        point = (0 - point);

    return point;
}

/*!
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 祝福状態ならばTRUE
 */
bool is_blessed(player_type *creature_ptr)
{
    return creature_ptr->blessed || music_singing(creature_ptr, MUSIC_BLESS) || hex_spelling(creature_ptr, HEX_BLESS);
}

bool is_tim_esp(player_type *creature_ptr)
{
    return creature_ptr->tim_esp || music_singing(creature_ptr, MUSIC_MIND) || (creature_ptr->concent >= CONCENT_TELE_THRESHOLD);
}

bool is_tim_stealth(player_type *creature_ptr) { return creature_ptr->tim_stealth || music_singing(creature_ptr, MUSIC_STEALTH); }

bool is_time_limit_esp(player_type *creature_ptr)
{
    return creature_ptr->tim_esp || music_singing(creature_ptr, MUSIC_MIND) || (creature_ptr->concent >= CONCENT_TELE_THRESHOLD);
}

bool is_time_limit_stealth(player_type *creature_ptr) { return creature_ptr->tim_stealth || music_singing(creature_ptr, MUSIC_STEALTH); }

bool can_two_hands_wielding(player_type *creature_ptr) { return !creature_ptr->riding || (creature_ptr->pet_extra_flags & PF_TWO_HANDS); }

/*!
 * @brief 歌の停止を処理する / Stop singing if the player is a Bard
 * @return なし
 */
void stop_singing(player_type *creature_ptr)
{
    if (creature_ptr->pclass != CLASS_BARD)
        return;

    /* Are there interupted song? */
    if (INTERUPTING_SONG_EFFECT(creature_ptr)) {
        /* Forget interupted song */
        INTERUPTING_SONG_EFFECT(creature_ptr) = MUSIC_NONE;
        return;
    }

    /* The player is singing? */
    if (!SINGING_SONG_EFFECT(creature_ptr))
        return;

    /* Hack -- if called from set_action(), avoid recursive loop */
    if (creature_ptr->action == ACTION_SING)
        set_action(creature_ptr, ACTION_NONE);

    /* Message text of each song or etc. */
    exe_spell(creature_ptr, REALM_MUSIC, SINGING_SONG_ID(creature_ptr), SPELL_STOP);

    SINGING_SONG_EFFECT(creature_ptr) = MUSIC_NONE;
    SINGING_SONG_ID(creature_ptr) = 0;
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->redraw |= (PR_STATUS);
}

/*!
 * @brief 口を使う継続的な処理を中断する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void stop_mouth(player_type *caster_ptr)
{
    if (music_singing_any(caster_ptr))
        stop_singing(caster_ptr);
    if (hex_spelling_any(caster_ptr))
        stop_hex_spell_all(caster_ptr);
}

/*!
 * @brief ペットの維持コスト計算
 * @return 維持コスト(%)
 */
PERCENTAGE calculate_upkeep(player_type *creature_ptr)
{
    MONSTER_IDX m_idx;
    bool has_a_unique = FALSE;
    DEPTH total_friend_levels = 0;

    total_friends = 0;

    for (m_idx = creature_ptr->current_floor_ptr->m_max - 1; m_idx >= 1; m_idx--) {
        monster_type *m_ptr;
        monster_race *r_ptr;

        m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
        if (!monster_is_valid(m_ptr))
            continue;
        r_ptr = &r_info[m_ptr->r_idx];

        if (is_pet(m_ptr)) {
            total_friends++;
            if (r_ptr->flags1 & RF1_UNIQUE) {
                if (creature_ptr->pclass == CLASS_CAVALRY) {
                    if (creature_ptr->riding == m_idx)
                        total_friend_levels += (r_ptr->level + 5) * 2;
                    else if (!has_a_unique && (r_info[m_ptr->r_idx].flags7 & RF7_RIDING))
                        total_friend_levels += (r_ptr->level + 5) * 7 / 2;
                    else
                        total_friend_levels += (r_ptr->level + 5) * 10;
                    has_a_unique = TRUE;
                } else
                    total_friend_levels += (r_ptr->level + 5) * 10;
            } else
                total_friend_levels += r_ptr->level;
        }
    }

    if (total_friends) {
        int upkeep_factor;
        upkeep_factor = (total_friend_levels - (creature_ptr->lev * 80 / (cp_ptr->pet_upkeep_div)));
        if (upkeep_factor < 0)
            upkeep_factor = 0;
        if (upkeep_factor > 1000)
            upkeep_factor = 1000;
        return upkeep_factor;
    } else
        return 0;
}

bool music_singing(player_type *caster_ptr, int music_songs) { return (caster_ptr->pclass == CLASS_BARD) && (caster_ptr->magic_num1[0] == music_songs); }

bool is_fast(player_type *creature_ptr) { return creature_ptr->fast || music_singing(creature_ptr, MUSIC_SPEED) || music_singing(creature_ptr, MUSIC_SHERO); }

bool is_invuln(player_type *creature_ptr) { return creature_ptr->invuln || music_singing(creature_ptr, MUSIC_INVULN); }

bool is_hero(player_type *creature_ptr) { return creature_ptr->hero || music_singing(creature_ptr, MUSIC_HERO) || music_singing(creature_ptr, MUSIC_SHERO); }

bool is_shero(player_type *creature_ptr) { return creature_ptr->shero || creature_ptr->pclass == CLASS_BERSERKER; }

bool is_echizen(player_type *creature_ptr)
{
    return (creature_ptr->pseikaku == PERSONALITY_COMBAT) || (creature_ptr->inventory_list[INVEN_BOW].name1 == ART_CRIMSON);
}

int calc_weapon_weight_limit(player_type *creature_ptr)
{
    int weight = adj_str_hold[creature_ptr->stat_ind[A_STR]];

    if (has_two_handed_weapons(creature_ptr))
        weight *= 2;

    return weight;
}

static int get_default_hand(player_type *creature_ptr)
{
    int default_hand = 0;

    if (has_melee_weapon(creature_ptr, INVEN_LARM)) {
        if (!has_right_hand_weapon(creature_ptr))
            default_hand = 1;
    }

    if (can_two_hands_wielding(creature_ptr)) {
        if (has_right_hand_weapon(creature_ptr) && (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_LARM)
            && object_allow_two_hands_wielding(&creature_ptr->inventory_list[INVEN_RARM])) {
        } else if (has_left_hand_weapon(creature_ptr) && (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_RARM)
            && object_allow_two_hands_wielding(&creature_ptr->inventory_list[INVEN_LARM])) {
        } else {
            default_hand = 1;
        }
    }

    return default_hand;
}
