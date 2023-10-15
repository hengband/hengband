#include "player/player-status.h"
#include "artifact/fixed-art-types.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-item/cmd-magiceat.h"
#include "combat/attack-power-table.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-leaver.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
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
#include "monster-race/race-flags7.h"
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
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "pet/pet-util.h"
#include "player-ability/player-charisma.h"
#include "player-ability/player-constitution.h"
#include "player-ability/player-dexterity.h"
#include "player-ability/player-intelligence.h"
#include "player-ability/player-strength.h"
#include "player-ability/player-wisdom.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/alignment.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player-info/mimic-info-table.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player-status/player-basic-statistics.h"
#include "player-status/player-hand-types.h"
#include "player-status/player-infravision.h"
#include "player-status/player-speed.h"
#include "player-status/player-stealth.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/patron.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-personality-types.h"
#include "player/player-personality.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/player-view.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-names-table.h"
#include "realm/realm-song-numbers.h"
#include "specific-object/torch.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/range-calc.h"
#include "spell/spells-describer.h"
#include "spell/spells-execution.h"
#include "spell/spells-status.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "status/base-status.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "term/screen-processor.h"
#include "timed-effect/player-acceleration.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

static const int extra_magic_glove_reduce_mana = 1;

static bool is_martial_arts_mode(PlayerType *player_ptr);

static ACTION_SKILL_POWER calc_disarming(PlayerType *player_ptr);
static ACTION_SKILL_POWER calc_device_ability(PlayerType *player_ptr);
static ACTION_SKILL_POWER calc_saving_throw(PlayerType *player_ptr);
static ACTION_SKILL_POWER calc_search(PlayerType *player_ptr);
static ACTION_SKILL_POWER calc_search_freq(PlayerType *player_ptr);
static ACTION_SKILL_POWER calc_to_hit_melee(PlayerType *player_ptr);
static ACTION_SKILL_POWER calc_to_hit_shoot(PlayerType *player_ptr);
static ACTION_SKILL_POWER calc_to_hit_throw(PlayerType *player_ptr);
static ACTION_SKILL_POWER calc_skill_dig(PlayerType *player_ptr);
static bool is_heavy_wield(PlayerType *player_ptr, int i);
static int16_t calc_num_blow(PlayerType *player_ptr, int i);
static int16_t calc_to_magic_chance(PlayerType *player_ptr);
static ARMOUR_CLASS calc_base_ac(PlayerType *player_ptr);
static ARMOUR_CLASS calc_to_ac(PlayerType *player_ptr, bool is_real_value);
static int16_t calc_double_weapon_penalty(PlayerType *player_ptr, INVENTORY_IDX slot);
static bool is_riding_two_hands(PlayerType *player_ptr);
static int16_t calc_riding_bow_penalty(PlayerType *player_ptr);
static void put_equipment_warning(PlayerType *player_ptr);

static short calc_to_damage(PlayerType *player_ptr, INVENTORY_IDX slot, bool is_real_value);
static int16_t calc_to_hit(PlayerType *player_ptr, INVENTORY_IDX slot, bool is_real_value);

static int16_t calc_to_hit_bow(PlayerType *player_ptr, bool is_real_value);

static int16_t calc_to_damage_misc(PlayerType *player_ptr);
static int16_t calc_to_hit_misc(PlayerType *player_ptr);

static DICE_NUMBER calc_to_weapon_dice_num(PlayerType *player_ptr, INVENTORY_IDX slot);
static player_hand main_attack_hand(PlayerType *player_ptr);

/*** Player information ***/

/*!
 * @brief 遅延描画更新 / Delayed visual update
 * @details update_view(), update_lite(), update_mon_lite() においてのみ更新すること / Only used if update_view(), update_lite() or update_mon_lite() was called
 * @param player_ptr 主観となるプレイヤー構造体参照ポインタ
 * @todo 将来独自インターフェース実装にはz-term系に追い出すべきか？
 */
static void delayed_visual_update(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = 0; i < floor_ptr->redraw_n; i++) {
        POSITION y = floor_ptr->redraw_y[i];
        POSITION x = floor_ptr->redraw_x[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        if (none_bits(g_ptr->info, CAVE_REDRAW)) {
            continue;
        }

        if (any_bits(g_ptr->info, CAVE_NOTE)) {
            note_spot(player_ptr, y, x);
        }

        lite_spot(player_ptr, y, x);
        if (g_ptr->m_idx) {
            update_monster(player_ptr, g_ptr->m_idx, false);
        }

        reset_bits(g_ptr->info, (CAVE_NOTE | CAVE_REDRAW));
    }

    floor_ptr->redraw_n = 0;
}

/*!
 * @brief 射撃武器がプレイヤーにとって重すぎるかどうかの判定 /
 * @param o_ptr 判定する射撃武器のアイテム情報参照ポインタ
 * @return 重すぎるならばTRUE
 */
static bool is_heavy_shoot(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    return calc_bow_weight_limit(player_ptr) < (o_ptr->weight / 10);
}

/*!
 * @brief 所持品総重量を計算する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 総重量
 */
WEIGHT calc_inventory_weight(PlayerType *player_ptr)
{
    WEIGHT weight = 0;

    ItemEntity *o_ptr;
    for (int i = 0; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }
        weight += o_ptr->weight * o_ptr->number;
    }
    return weight;
}

static void update_ability_scores(PlayerType *player_ptr)
{
    PlayerStrength player_str(player_ptr);
    PlayerIntelligence player_int(player_ptr);
    PlayerWisdom player_wis(player_ptr);
    PlayerDexterity player_dex(player_ptr);
    PlayerConstitution player_con(player_ptr);
    PlayerCharisma player_chr(player_ptr);
    PlayerBasicStatistics *player_stats[] = { &player_str, &player_int, &player_wis, &player_dex, &player_con, &player_chr };
    for (auto i = 0; i < A_MAX; ++i) {
        player_ptr->stat_add[i] = player_stats[i]->modification_value();
        player_stats[i]->update_value();
    }
}

/*!
 * @brief プレイヤーの全ステータスを更新する /
 * Calculate the players current "state", taking into account
 * not only race/class intrinsics, but also objects being worn
 * and temporary spell effects.
 * @details
 * <pre>
 * See also update_max_mana() and update_max_hitpoints().
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
static void update_bonuses(PlayerType *player_ptr)
{
    auto empty_hands_status = empty_hands(player_ptr, true);
    ItemEntity *o_ptr;

    /* Save the old vision stuff */
    BIT_FLAGS old_telepathy = player_ptr->telepathy;
    BIT_FLAGS old_esp_animal = player_ptr->esp_animal;
    BIT_FLAGS old_esp_undead = player_ptr->esp_undead;
    BIT_FLAGS old_esp_demon = player_ptr->esp_demon;
    BIT_FLAGS old_esp_orc = player_ptr->esp_orc;
    BIT_FLAGS old_esp_troll = player_ptr->esp_troll;
    BIT_FLAGS old_esp_giant = player_ptr->esp_giant;
    BIT_FLAGS old_esp_dragon = player_ptr->esp_dragon;
    BIT_FLAGS old_esp_human = player_ptr->esp_human;
    BIT_FLAGS old_esp_evil = player_ptr->esp_evil;
    BIT_FLAGS old_esp_good = player_ptr->esp_good;
    BIT_FLAGS old_esp_nonliving = player_ptr->esp_nonliving;
    BIT_FLAGS old_esp_unique = player_ptr->esp_unique;
    BIT_FLAGS old_see_inv = player_ptr->see_inv;
    BIT_FLAGS old_mighty_throw = player_ptr->mighty_throw;
    int16_t old_speed = player_ptr->pspeed;

    ARMOUR_CLASS old_dis_ac = player_ptr->dis_ac;
    ARMOUR_CLASS old_dis_to_a = player_ptr->dis_to_a;

    player_ptr->xtra_might = has_xtra_might(player_ptr);
    player_ptr->esp_evil = has_esp_evil(player_ptr);
    player_ptr->esp_animal = has_esp_animal(player_ptr);
    player_ptr->esp_undead = has_esp_undead(player_ptr);
    player_ptr->esp_demon = has_esp_demon(player_ptr);
    player_ptr->esp_orc = has_esp_orc(player_ptr);
    player_ptr->esp_troll = has_esp_troll(player_ptr);
    player_ptr->esp_giant = has_esp_giant(player_ptr);
    player_ptr->esp_dragon = has_esp_dragon(player_ptr);
    player_ptr->esp_human = has_esp_human(player_ptr);
    player_ptr->esp_good = has_esp_good(player_ptr);
    player_ptr->esp_nonliving = has_esp_nonliving(player_ptr);
    player_ptr->esp_unique = has_esp_unique(player_ptr);
    player_ptr->telepathy = has_esp_telepathy(player_ptr);
    player_ptr->bless_blade = has_bless_blade(player_ptr);
    player_ptr->easy_2weapon = has_easy2_weapon(player_ptr);
    player_ptr->down_saving = has_down_saving(player_ptr);
    player_ptr->yoiyami = has_no_ac(player_ptr);
    player_ptr->mighty_throw = has_mighty_throw(player_ptr);
    player_ptr->dec_mana = has_dec_mana(player_ptr);
    player_ptr->see_nocto = has_see_nocto(player_ptr);
    player_ptr->warning = has_warning(player_ptr);
    player_ptr->anti_magic = has_anti_magic(player_ptr);
    player_ptr->anti_tele = has_anti_tele(player_ptr);
    player_ptr->easy_spell = has_easy_spell(player_ptr);
    player_ptr->heavy_spell = has_heavy_spell(player_ptr);
    player_ptr->hold_exp = has_hold_exp(player_ptr);
    player_ptr->see_inv = has_see_inv(player_ptr);
    player_ptr->free_act = has_free_act(player_ptr);
    player_ptr->levitation = has_levitation(player_ptr);
    player_ptr->can_swim = has_can_swim(player_ptr);
    player_ptr->slow_digest = has_slow_digest(player_ptr);
    player_ptr->regenerate = has_regenerate(player_ptr);
    update_curses(player_ptr);
    player_ptr->impact = has_impact(player_ptr);
    player_ptr->earthquake = has_earthquake(player_ptr);
    update_extra_blows(player_ptr);

    player_ptr->lite = has_lite(player_ptr);

    if (!PlayerClass(player_ptr).monk_stance_is(MonkStanceType::NONE)) {
        if (none_bits(empty_hands_status, EMPTY_HAND_MAIN)) {
            set_action(player_ptr, ACTION_NONE);
        }
    }

    update_ability_scores(player_ptr);
    o_ptr = &player_ptr->inventory_list[INVEN_BOW];
    if (o_ptr->is_valid()) {
        player_ptr->tval_ammo = o_ptr->get_arrow_kind();
        player_ptr->num_fire = calc_num_fire(player_ptr, o_ptr);
    }

    for (int i = 0; i < 2; i++) {
        player_ptr->is_icky_wield[i] = is_wielding_icky_weapon(player_ptr, i);
        player_ptr->is_icky_riding_wield[i] = is_wielding_icky_riding_weapon(player_ptr, i);
        player_ptr->heavy_wield[i] = is_heavy_wield(player_ptr, i);
        player_ptr->num_blow[i] = calc_num_blow(player_ptr, i);
        player_ptr->to_dd[i] = calc_to_weapon_dice_num(player_ptr, INVEN_MAIN_HAND + i);
        player_ptr->to_ds[i] = 0;
    }

    player_ptr->pspeed = PlayerSpeed(player_ptr).get_value();
    player_ptr->see_infra = PlayerInfravision(player_ptr).get_value();
    player_ptr->skill_stl = PlayerStealth(player_ptr).get_value();
    player_ptr->skill_dis = calc_disarming(player_ptr);
    player_ptr->skill_dev = calc_device_ability(player_ptr);
    player_ptr->skill_sav = calc_saving_throw(player_ptr);
    player_ptr->skill_srh = calc_search(player_ptr);
    player_ptr->skill_fos = calc_search_freq(player_ptr);
    player_ptr->skill_thn = calc_to_hit_melee(player_ptr);
    player_ptr->skill_thb = calc_to_hit_shoot(player_ptr);
    player_ptr->skill_tht = calc_to_hit_throw(player_ptr);
    player_ptr->riding_ryoute = is_riding_two_hands(player_ptr);
    player_ptr->to_d[0] = calc_to_damage(player_ptr, INVEN_MAIN_HAND, true);
    player_ptr->to_d[1] = calc_to_damage(player_ptr, INVEN_SUB_HAND, true);
    player_ptr->dis_to_d[0] = calc_to_damage(player_ptr, INVEN_MAIN_HAND, false);
    player_ptr->dis_to_d[1] = calc_to_damage(player_ptr, INVEN_SUB_HAND, false);
    player_ptr->to_h[0] = calc_to_hit(player_ptr, INVEN_MAIN_HAND, true);
    player_ptr->to_h[1] = calc_to_hit(player_ptr, INVEN_SUB_HAND, true);
    player_ptr->dis_to_h[0] = calc_to_hit(player_ptr, INVEN_MAIN_HAND, false);
    player_ptr->dis_to_h[1] = calc_to_hit(player_ptr, INVEN_SUB_HAND, false);
    player_ptr->to_h_b = calc_to_hit_bow(player_ptr, true);
    player_ptr->dis_to_h_b = calc_to_hit_bow(player_ptr, false);
    player_ptr->to_d_m = calc_to_damage_misc(player_ptr);
    player_ptr->to_h_m = calc_to_hit_misc(player_ptr);
    player_ptr->skill_dig = calc_skill_dig(player_ptr);
    player_ptr->to_m_chance = calc_to_magic_chance(player_ptr);
    player_ptr->ac = calc_base_ac(player_ptr);
    player_ptr->to_a = calc_to_ac(player_ptr, true);
    player_ptr->dis_ac = calc_base_ac(player_ptr);
    player_ptr->dis_to_a = calc_to_ac(player_ptr, false);

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (old_mighty_throw != player_ptr->mighty_throw) {
        rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
    }

    if (player_ptr->telepathy != old_telepathy) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    }

    auto is_esp_updated = player_ptr->esp_animal != old_esp_animal;
    is_esp_updated |= player_ptr->esp_undead != old_esp_undead;
    is_esp_updated |= player_ptr->esp_demon != old_esp_demon;
    is_esp_updated |= player_ptr->esp_orc != old_esp_orc;
    is_esp_updated |= player_ptr->esp_troll != old_esp_troll;
    is_esp_updated |= player_ptr->esp_giant != old_esp_giant;
    is_esp_updated |= player_ptr->esp_dragon != old_esp_dragon;
    is_esp_updated |= player_ptr->esp_human != old_esp_human;
    is_esp_updated |= player_ptr->esp_evil != old_esp_evil;
    is_esp_updated |= player_ptr->esp_good != old_esp_good;
    is_esp_updated |= player_ptr->esp_nonliving != old_esp_nonliving;
    is_esp_updated |= player_ptr->esp_unique != old_esp_unique;
    if (is_esp_updated) {
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    }

    if (player_ptr->see_inv != old_see_inv) {
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    }

    if (player_ptr->pspeed != old_speed) {
        rfu.set_flag(MainWindowRedrawingFlag::SPEED);
    }

    if ((player_ptr->dis_ac != old_dis_ac) || (player_ptr->dis_to_a != old_dis_to_a)) {
        rfu.set_flag(MainWindowRedrawingFlag::AC);
        rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
    }

    if (w_ptr->character_xtra) {
        return;
    }

    put_equipment_warning(player_ptr);
    check_no_flowed(player_ptr);
}

/*!
 * @brief プレイヤーの最大HPを更新する /
 * Update the players maximal hit points
 * Adjust current hitpoints if necessary
 * @details
 */
static void update_max_hitpoints(PlayerType *player_ptr)
{
    int bonus = ((int)(adj_con_mhp[player_ptr->stat_index[A_CON]]) - 128) * player_ptr->lev / 4;
    int mhp = player_ptr->player_hp[player_ptr->lev - 1];

    byte tmp_hitdie;
    PlayerClass pc(player_ptr);
    auto is_sorcerer = pc.equals(PlayerClassType::SORCERER);
    if (player_ptr->mimic_form != MimicKindType::NONE) {
        auto r_mhp = mimic_info.at(player_ptr->mimic_form).r_mhp;
        if (is_sorcerer) {
            tmp_hitdie = r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
        } else {
            tmp_hitdie = r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
        }

        mhp = mhp * tmp_hitdie / player_ptr->hitdie;
    }

    if (is_sorcerer) {
        if (player_ptr->lev < 30) {
            mhp = (mhp * (45 + player_ptr->lev) / 100);
        } else {
            mhp = (mhp * 75 / 100);
        }
        bonus = (bonus * 65 / 100);
    }

    mhp += bonus;

    if (pc.equals(PlayerClassType::BERSERKER)) {
        mhp = mhp * (110 + (((player_ptr->lev + 40) * (player_ptr->lev + 40) - 1550) / 110)) / 100;
    }

    if (mhp < player_ptr->lev + 1) {
        mhp = player_ptr->lev + 1;
    }
    if (is_hero(player_ptr)) {
        mhp += 10;
    }
    if (is_shero(player_ptr)) {
        mhp += 30;
    }
    if (player_ptr->tsuyoshi) {
        mhp += 50;
    }
    if (SpellHex(player_ptr).is_spelling_specific(HEX_XTRA_MIGHT)) {
        mhp += 15;
    }
    if (SpellHex(player_ptr).is_spelling_specific(HEX_BUILDING)) {
        mhp += 60;
    }
    if (player_ptr->mhp == mhp) {
        return;
    }

    if (player_ptr->chp >= mhp) {
        player_ptr->chp = mhp;
        player_ptr->chp_frac = 0;
    }

#ifdef JP
    if (player_ptr->level_up_message && (mhp > player_ptr->mhp)) {
        msg_format("最大ヒット・ポイントが %d 増加した！", (mhp - player_ptr->mhp));
    }
#endif
    player_ptr->mhp = mhp;

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::HP);
    rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
}

/*!
 * @brief プレイヤーの現在学習可能な魔法数を計算し、増減に応じて魔法の忘却、再学習を処置する。 /
 * Calculate number of spells player should have, and forget,
 * or remember, spells until that number is properly reflected.
 * @details
 * Note that this function induces various "status" messages,
 * which must be bypasses until the character is created.
 */
static void update_num_of_spells(PlayerType *player_ptr)
{
    if ((mp_ptr->spell_book == ItemKindType::NONE) || !w_ptr->character_generated || w_ptr->character_xtra) {
        return;
    }

    PlayerClass pc(player_ptr);
    if (pc.is_every_magic()) {
        player_ptr->new_spells = 0;
        return;
    }

    const auto spell_category = spell_category_name(mp_ptr->spell_book);
    int levels = player_ptr->lev - mp_ptr->spell_first + 1;
    if (levels < 0) {
        levels = 0;
    }

    int num_allowed = (adj_mag_study[player_ptr->stat_index[mp_ptr->spell_stat]] * levels / 2);
    int bonus = 0;
    if (!pc.equals(PlayerClassType::SAMURAI) && (mp_ptr->spell_book != ItemKindType::LIFE_BOOK)) {
        bonus = 4;
    }

    if (pc.equals(PlayerClassType::SAMURAI)) {
        num_allowed = 32;
    } else if (player_ptr->realm2 == REALM_NONE) {
        num_allowed = (num_allowed + 1) / 2;
        if (num_allowed > (32 + bonus)) {
            num_allowed = 32 + bonus;
        }
    } else if (pc.equals(PlayerClassType::MAGE) || pc.equals(PlayerClassType::PRIEST)) {
        if (num_allowed > (96 + bonus)) {
            num_allowed = 96 + bonus;
        }
    } else {
        if (num_allowed > (80 + bonus)) {
            num_allowed = 80 + bonus;
        }
    }

    int num_boukyaku = 0;
    for (int j = 0; j < 64; j++) {
        if ((j < 32) ? any_bits(player_ptr->spell_forgotten1, (1UL << j)) : any_bits(player_ptr->spell_forgotten2, (1UL << (j - 32)))) {
            num_boukyaku++;
        }
    }

    player_ptr->new_spells = num_allowed + player_ptr->add_spells + num_boukyaku - player_ptr->learned_spells;
    for (int i = 63; i >= 0; i--) {
        if (!player_ptr->spell_learned1 && !player_ptr->spell_learned2) {
            break;
        }

        int j = player_ptr->spell_order[i];
        if (j >= 99) {
            continue;
        }

        const magic_type *s_ptr;
        if (!is_magic((j < 32) ? player_ptr->realm1 : player_ptr->realm2)) {
            if (j < 32) {
                s_ptr = &technic_info[player_ptr->realm1 - MIN_TECHNIC][j];
            } else {
                s_ptr = &technic_info[player_ptr->realm2 - MIN_TECHNIC][j % 32];
            }
        } else if (j < 32) {
            s_ptr = &mp_ptr->info[player_ptr->realm1 - 1][j];
        } else {
            s_ptr = &mp_ptr->info[player_ptr->realm2 - 1][j % 32];
        }

        if (s_ptr->slevel <= player_ptr->lev) {
            continue;
        }

        bool is_spell_learned = (j < 32) ? any_bits(player_ptr->spell_learned1, (1UL << j)) : any_bits(player_ptr->spell_learned2, (1UL << (j - 32)));
        if (!is_spell_learned) {
            continue;
        }

        int16_t which;
        if (j < 32) {
            set_bits(player_ptr->spell_forgotten1, (1UL << j));
            which = player_ptr->realm1;
        } else {
            set_bits(player_ptr->spell_forgotten2, (1UL << (j - 32)));
            which = player_ptr->realm2;
        }

        if (j < 32) {
            reset_bits(player_ptr->spell_learned1, (1UL << j));
            which = player_ptr->realm1;
        } else {
            reset_bits(player_ptr->spell_learned2, (1UL << (j - 32)));
            which = player_ptr->realm2;
        }

        const auto spell_name = exe_spell(player_ptr, which, j % 32, SpellProcessType::NAME);
#ifdef JP
        msg_format("%sの%sを忘れてしまった。", spell_name->data(), spell_category.data());
#else
        msg_format("You have forgotten the %s of %s.", spell_category.data(), spell_name->data());
#endif
        player_ptr->new_spells++;
    }

    /* Forget spells if we know too many spells */
    for (int i = 63; i >= 0; i--) {
        if (player_ptr->new_spells >= 0) {
            break;
        }
        if (!player_ptr->spell_learned1 && !player_ptr->spell_learned2) {
            break;
        }

        int j = player_ptr->spell_order[i];
        if (j >= 99) {
            continue;
        }

        bool is_spell_learned = (j < 32) ? any_bits(player_ptr->spell_learned1, (1UL << j)) : any_bits(player_ptr->spell_learned2, (1UL << (j - 32)));
        if (!is_spell_learned) {
            continue;
        }

        int16_t which;
        if (j < 32) {
            set_bits(player_ptr->spell_forgotten1, (1UL << j));
            which = player_ptr->realm1;
        } else {
            set_bits(player_ptr->spell_forgotten2, (1UL << (j - 32)));
            which = player_ptr->realm2;
        }

        if (j < 32) {
            reset_bits(player_ptr->spell_learned1, (1UL << j));
            which = player_ptr->realm1;
        } else {
            reset_bits(player_ptr->spell_learned2, (1UL << (j - 32)));
            which = player_ptr->realm2;
        }

        const auto spell_name = exe_spell(player_ptr, which, j % 32, SpellProcessType::NAME);
#ifdef JP
        msg_format("%sの%sを忘れてしまった。", spell_name->data(), spell_category.data());
#else
        msg_format("You have forgotten the %s of %s.", spell_category.data(), spell_name->data());
#endif
        player_ptr->new_spells++;
    }

    /* Check for spells to remember */
    for (int i = 0; i < 64; i++) {
        if (player_ptr->new_spells <= 0) {
            break;
        }
        if (!player_ptr->spell_forgotten1 && !player_ptr->spell_forgotten2) {
            break;
        }
        int j = player_ptr->spell_order[i];
        if (j >= 99) {
            break;
        }

        const magic_type *s_ptr;
        if (!is_magic((j < 32) ? player_ptr->realm1 : player_ptr->realm2)) {
            if (j < 32) {
                s_ptr = &technic_info[player_ptr->realm1 - MIN_TECHNIC][j];
            } else {
                s_ptr = &technic_info[player_ptr->realm2 - MIN_TECHNIC][j % 32];
            }
        } else if (j < 32) {
            s_ptr = &mp_ptr->info[player_ptr->realm1 - 1][j];
        } else {
            s_ptr = &mp_ptr->info[player_ptr->realm2 - 1][j % 32];
        }

        if (s_ptr->slevel > player_ptr->lev) {
            continue;
        }

        bool is_spell_learned = (j < 32) ? any_bits(player_ptr->spell_forgotten1, (1UL << j)) : any_bits(player_ptr->spell_forgotten2, (1UL << (j - 32)));
        if (!is_spell_learned) {
            continue;
        }

        int16_t which;
        if (j < 32) {
            reset_bits(player_ptr->spell_forgotten1, (1UL << j));
            which = player_ptr->realm1;
        } else {
            reset_bits(player_ptr->spell_forgotten2, (1UL << (j - 32)));
            which = player_ptr->realm2;
        }

        if (j < 32) {
            set_bits(player_ptr->spell_learned1, (1UL << j));
            which = player_ptr->realm1;
        } else {
            set_bits(player_ptr->spell_learned2, (1UL << (j - 32)));
            which = player_ptr->realm2;
        }

        const auto spell_name = exe_spell(player_ptr, which, j % 32, SpellProcessType::NAME);
#ifdef JP
        msg_format("%sの%sを思い出した。", spell_name->data(), spell_category.data());
#else
        msg_format("You have remembered the %s of %s.", spell_category.data(), spell_name->data());
#endif
        player_ptr->new_spells--;
    }

    if (player_ptr->realm2 == REALM_NONE) {
        int k = 0;
        for (int j = 0; j < 32; j++) {
            const magic_type *s_ptr;
            if (!is_magic(player_ptr->realm1)) {
                s_ptr = &technic_info[player_ptr->realm1 - MIN_TECHNIC][j];
            } else {
                s_ptr = &mp_ptr->info[player_ptr->realm1 - 1][j];
            }

            if (s_ptr->slevel > player_ptr->lev) {
                continue;
            }

            if (any_bits(player_ptr->spell_learned1, (1UL << j))) {
                continue;
            }

            k++;
        }

        if (k > 32) {
            k = 32;
        }
        if ((player_ptr->new_spells > k) && ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) || (mp_ptr->spell_book == ItemKindType::HISSATSU_BOOK))) {
            player_ptr->new_spells = (int16_t)k;
        }
    }

    if (player_ptr->new_spells < 0) {
        player_ptr->new_spells = 0;
    }

    if (player_ptr->old_spells == player_ptr->new_spells) {
        return;
    }

    if (player_ptr->new_spells) {
#ifdef JP
        if (player_ptr->new_spells < 10) {
            msg_format("あと %d つの%sを学べる。", player_ptr->new_spells, spell_category.data());
        } else {
            msg_format("あと %d 個の%sを学べる。", player_ptr->new_spells, spell_category.data());
        }
#else
        msg_format("You can learn %d more %s%s.", player_ptr->new_spells, spell_category.data(), (player_ptr->new_spells != 1) ? "s" : "");
#endif
    }

    player_ptr->old_spells = player_ptr->new_spells;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::STUDY);
    rfu.set_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGE);
}

/*!
 * @brief プレイヤーの最大MPを更新する /
 * Update maximum mana.  You do not need to know any spells.
 * Note that mana is lowered by heavy (or inappropriate) armor.
 * @details
 * This function induces status messages.
 */
static void update_max_mana(PlayerType *player_ptr)
{
    if ((mp_ptr->spell_book == ItemKindType::NONE) && mp_ptr->spell_first == SPELL_FIRST_NO_SPELL) {
        return;
    }

    int levels;
    PlayerClass pc(player_ptr);
    auto use_direct_level = pc.equals(PlayerClassType::MINDCRAFTER);
    use_direct_level |= pc.equals(PlayerClassType::MIRROR_MASTER);
    use_direct_level |= pc.equals(PlayerClassType::BLUE_MAGE);
    use_direct_level |= pc.equals(PlayerClassType::ELEMENTALIST);
    if (use_direct_level) {
        levels = player_ptr->lev;
    } else {
        if (mp_ptr->spell_first > player_ptr->lev) {
            player_ptr->msp = 0;
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
            return;
        }

        levels = (player_ptr->lev - mp_ptr->spell_first) + 1;
    }

    int msp;
    if (pc.equals(PlayerClassType::SAMURAI)) {
        msp = (adj_mag_mana[player_ptr->stat_index[mp_ptr->spell_stat]] + 10) * 2;
        if (msp) {
            msp += (msp * rp_ptr->r_adj[mp_ptr->spell_stat] / 20);
        }
    } else {
        msp = adj_mag_mana[player_ptr->stat_index[mp_ptr->spell_stat]] * (levels + 3) / 4;
        if (msp) {
            msp++;
        }
        if (msp) {
            msp += (msp * rp_ptr->r_adj[mp_ptr->spell_stat] / 20);
        }
        if (msp && (player_ptr->ppersonality == PERSONALITY_MUNCHKIN)) {
            msp += msp / 2;
        }
        if (msp && pc.equals(PlayerClassType::HIGH_MAGE)) {
            msp += msp / 4;
        }
        if (msp && pc.equals(PlayerClassType::SORCERER)) {
            msp += msp * (25 + player_ptr->lev) / 100;
        }
    }

    if (any_bits(mp_ptr->spell_xtra, extra_magic_glove_reduce_mana)) {
        player_ptr->cumber_glove = false;
        const auto *o_ptr = &player_ptr->inventory_list[INVEN_ARMS];
        const auto flags = o_ptr->get_flags();
        auto should_mp_decrease = o_ptr->is_valid();
        should_mp_decrease &= flags.has_not(TR_FREE_ACT);
        should_mp_decrease &= flags.has_not(TR_DEC_MANA);
        should_mp_decrease &= flags.has_not(TR_EASY_SPELL);
        should_mp_decrease &= flags.has_not(TR_MAGIC_MASTERY) || (o_ptr->pval <= 0);
        should_mp_decrease &= flags.has_not(TR_DEX) || (o_ptr->pval <= 0);
        if (should_mp_decrease) {
            player_ptr->cumber_glove = true;
            msp = (3 * msp) / 4;
        }
    }

    player_ptr->cumber_armor = false;

    auto cur_wgt = 0;
    const auto &item_main_hand = player_ptr->inventory_list[INVEN_MAIN_HAND];
    const auto tval_main = item_main_hand.bi_key.tval();
    if (tval_main > ItemKindType::SWORD) {
        cur_wgt += item_main_hand.weight;
    }

    const auto &item_sub_hand = player_ptr->inventory_list[INVEN_SUB_HAND];
    const auto tval_sub = item_sub_hand.bi_key.tval();
    if (item_sub_hand.bi_key.tval() > ItemKindType::SWORD) {
        cur_wgt += item_sub_hand.weight;
    }

    cur_wgt += player_ptr->inventory_list[INVEN_BODY].weight;
    cur_wgt += player_ptr->inventory_list[INVEN_HEAD].weight;
    cur_wgt += player_ptr->inventory_list[INVEN_OUTER].weight;
    cur_wgt += player_ptr->inventory_list[INVEN_ARMS].weight;
    cur_wgt += player_ptr->inventory_list[INVEN_FEET].weight;

    switch (player_ptr->pclass) {
    case PlayerClassType::MAGE:
    case PlayerClassType::HIGH_MAGE:
    case PlayerClassType::BLUE_MAGE:
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
    case PlayerClassType::SORCERER:
    case PlayerClassType::ELEMENTALIST:
        if (tval_main <= ItemKindType::SWORD) {
            cur_wgt += item_main_hand.weight;
        }

        if (tval_sub <= ItemKindType::SWORD) {
            cur_wgt += item_sub_hand.weight;
        }

        break;
    case PlayerClassType::PRIEST:
    case PlayerClassType::BARD:
    case PlayerClassType::TOURIST:
        if (tval_main <= ItemKindType::SWORD) {
            cur_wgt += item_main_hand.weight * 2 / 3;
        }

        if (tval_sub <= ItemKindType::SWORD) {
            cur_wgt += item_sub_hand.weight * 2 / 3;
        }

        break;
    case PlayerClassType::MINDCRAFTER:
    case PlayerClassType::BEASTMASTER:
    case PlayerClassType::MIRROR_MASTER:
        if (tval_main <= ItemKindType::SWORD) {
            cur_wgt += item_main_hand.weight / 2;
        }

        if (tval_sub <= ItemKindType::SWORD) {
            cur_wgt += item_sub_hand.weight / 2;
        }

        break;
    case PlayerClassType::ROGUE:
    case PlayerClassType::RANGER:
    case PlayerClassType::RED_MAGE:
    case PlayerClassType::WARRIOR_MAGE:
        if (tval_main <= ItemKindType::SWORD) {
            cur_wgt += item_main_hand.weight / 3;
        }

        if (tval_sub <= ItemKindType::SWORD) {
            cur_wgt += item_sub_hand.weight / 3;
        }

        break;
    case PlayerClassType::PALADIN:
    case PlayerClassType::CHAOS_WARRIOR:
        if (tval_main <= ItemKindType::SWORD) {
            cur_wgt += item_main_hand.weight / 5;
        }

        if (tval_sub <= ItemKindType::SWORD) {
            cur_wgt += item_sub_hand.weight / 5;
        }

        break;
    default:
        break;
    }

    int max_wgt = mp_ptr->spell_weight;
    if ((cur_wgt - max_wgt) > 0) {
        player_ptr->cumber_armor = true;
        switch (player_ptr->pclass) {
        case PlayerClassType::MAGE:
        case PlayerClassType::HIGH_MAGE:
        case PlayerClassType::BLUE_MAGE:
        case PlayerClassType::ELEMENTALIST: {
            msp -= msp * (cur_wgt - max_wgt) / 600;
            break;
        }
        case PlayerClassType::PRIEST:
        case PlayerClassType::MINDCRAFTER:
        case PlayerClassType::BEASTMASTER:
        case PlayerClassType::BARD:
        case PlayerClassType::FORCETRAINER:
        case PlayerClassType::TOURIST:
        case PlayerClassType::MIRROR_MASTER: {
            msp -= msp * (cur_wgt - max_wgt) / 800;
            break;
        }
        case PlayerClassType::SORCERER: {
            msp -= msp * (cur_wgt - max_wgt) / 900;
            break;
        }
        case PlayerClassType::ROGUE:
        case PlayerClassType::RANGER:
        case PlayerClassType::MONK:
        case PlayerClassType::RED_MAGE: {
            msp -= msp * (cur_wgt - max_wgt) / 1000;
            break;
        }
        case PlayerClassType::PALADIN:
        case PlayerClassType::CHAOS_WARRIOR:
        case PlayerClassType::WARRIOR_MAGE: {
            msp -= msp * (cur_wgt - max_wgt) / 1200;
            break;
        }
        case PlayerClassType::SAMURAI: {
            player_ptr->cumber_armor = false;
            break;
        }
        default: {
            msp -= msp * (cur_wgt - max_wgt) / 800;
            break;
        }
        }
    }

    if (msp < 0) {
        msp = 0;
    }

    if (player_ptr->msp != msp) {
        if ((player_ptr->csp >= msp) && !pc.equals(PlayerClassType::SAMURAI)) {
            player_ptr->csp = msp;
            player_ptr->csp_frac = 0;
        }

#ifdef JP
        if (player_ptr->level_up_message && (msp > player_ptr->msp)) {
            msg_format("最大マジック・ポイントが %d 増加した！", (msp - player_ptr->msp));
        }
#endif
        player_ptr->msp = msp;
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::MP);
        static constexpr auto flags = {
            SubWindowRedrawingFlag::PLAYER,
            SubWindowRedrawingFlag::SPELL,
        };
        rfu.set_flags(flags);
    }

    if (w_ptr->character_xtra) {
        return;
    }

    if (player_ptr->old_cumber_glove != player_ptr->cumber_glove) {
        if (player_ptr->cumber_glove) {
            msg_print(_("手が覆われて呪文が唱えにくい感じがする。", "Your covered hands feel unsuitable for spellcasting."));
        } else {
            msg_print(_("この手の状態なら、ぐっと呪文が唱えやすい感じだ。", "Your hands feel more suitable for spellcasting."));
        }

        player_ptr->old_cumber_glove = player_ptr->cumber_glove;
    }

    if (player_ptr->old_cumber_armor == player_ptr->cumber_armor) {
        return;
    }

    if (player_ptr->cumber_armor) {
        msg_print(_("装備の重さで動きが鈍くなってしまっている。", "The weight of your equipment encumbers your movement."));
    } else {
        msg_print(_("ぐっと楽に体を動かせるようになった。", "You feel able to move more freely."));
    }

    player_ptr->old_cumber_armor = player_ptr->cumber_armor;
}

/*!
 * @brief 装備中の射撃武器の威力倍率を返す /
 * calcurate the fire rate of target object
 * @param o_ptr 計算する射撃武器のアイテム情報参照ポインタ
 * @return 射撃倍率の値(100で1.00倍)
 */
short calc_num_fire(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    int extra_shots = 0;

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *q_ptr;
        q_ptr = &player_ptr->inventory_list[i];
        if (!q_ptr->is_valid()) {
            continue;
        }

        if (i == INVEN_BOW) {
            continue;
        }

        if (q_ptr->get_flags().has(TR_XTRA_SHOTS)) {
            extra_shots++;
        }
    }

    if (o_ptr->get_flags().has(TR_XTRA_SHOTS)) {
        extra_shots++;
    }

    int num = 0;
    if (!o_ptr->is_valid()) {
        return (int16_t)num;
    }

    num = 100;
    num += (extra_shots * 100);

    if (is_heavy_shoot(player_ptr, o_ptr)) {
        return (int16_t)num;
    }

    const auto tval_ammo = o_ptr->get_arrow_kind();
    PlayerClass pc(player_ptr);
    if (pc.equals(PlayerClassType::RANGER) && (tval_ammo == ItemKindType::ARROW)) {
        num += (player_ptr->lev * 4);
    }

    if (pc.equals(PlayerClassType::CAVALRY) && (tval_ammo == ItemKindType::ARROW)) {
        num += (player_ptr->lev * 3);
    }

    if (pc.equals(PlayerClassType::ARCHER)) {
        if (tval_ammo == ItemKindType::ARROW) {
            num += ((player_ptr->lev * 5) + 50);
        } else if ((tval_ammo == ItemKindType::BOLT) || (tval_ammo == ItemKindType::SHOT)) {
            num += (player_ptr->lev * 4);
        }
    }

    if (pc.equals(PlayerClassType::WARRIOR) && (tval_ammo <= ItemKindType::BOLT) && (tval_ammo >= ItemKindType::SHOT)) {
        num += (player_ptr->lev * 2);
    }

    if (pc.equals(PlayerClassType::ROGUE) && (tval_ammo == ItemKindType::SHOT)) {
        num += (player_ptr->lev * 4);
    }

    return (int16_t)num;
}

/*!
 * @brief 解除能力計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 解除能力
 * @details
 * * 種族/職業/性格による加算
 * * 職業と性格とレベルによる追加加算
 * * 器用さに応じたadj_dex_disテーブルによる加算
 * * 知力に応じたadj_int_disテーブルによる加算
 */
static ACTION_SKILL_POWER calc_disarming(PlayerType *player_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_rp_ptr;

    if (player_ptr->mimic_form != MimicKindType::NONE) {
        tmp_rp_ptr = &mimic_info.at(player_ptr->mimic_form);
    } else {
        tmp_rp_ptr = &race_info[enum2i(player_ptr->prace)];
    }

    const player_class_info *c_ptr = &class_info[enum2i(player_ptr->pclass)];
    const player_personality *a_ptr = &personality_info[player_ptr->ppersonality];

    pow = tmp_rp_ptr->r_dis + c_ptr->c_dis + a_ptr->a_dis;
    pow += ((cp_ptr->x_dis * player_ptr->lev / 10) + (ap_ptr->a_dis * player_ptr->lev / 50));
    pow += adj_dex_dis[player_ptr->stat_index[A_DEX]];
    pow += adj_int_dis[player_ptr->stat_index[A_INT]];
    return pow;
}

/*!
 * @brief 魔道具使用能力計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 魔道具使用能力
 * @details
 * * 種族/職業/性格による加算
 * * 職業と性格とレベルによる追加加算
 * * 装備による加算(TR_MAGIC_MASTERYを持っていたら+pval*8)
 * * 知力に応じたadj_int_devテーブルによる加算
 * * 狂戦士化による減算(-20)
 */
static ACTION_SKILL_POWER calc_device_ability(PlayerType *player_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_rp_ptr;

    if (player_ptr->mimic_form != MimicKindType::NONE) {
        tmp_rp_ptr = &mimic_info.at(player_ptr->mimic_form);
    } else {
        tmp_rp_ptr = &race_info[enum2i(player_ptr->prace)];
    }

    const player_class_info *c_ptr = &class_info[enum2i(player_ptr->pclass)];
    const player_personality *a_ptr = &personality_info[player_ptr->ppersonality];

    pow = tmp_rp_ptr->r_dev + c_ptr->c_dev + a_ptr->a_dev;
    pow += ((c_ptr->x_dev * player_ptr->lev / 10) + (ap_ptr->a_dev * player_ptr->lev / 50));

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->get_flags().has(TR_MAGIC_MASTERY)) {
            pow += 8 * o_ptr->pval;
        }
    }

    pow += adj_int_dev[player_ptr->stat_index[A_INT]];

    if (is_shero(player_ptr)) {
        pow -= 20;
    }
    return pow;
}

/*!
 * @brief 魔法防御計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 魔法防御
 * @details
 * * 種族/職業/性格による加算
 * * 職業と性格とレベルによる追加加算
 * * 変異MUT3_MAGIC_RESによる加算(15 + レベル / 5)
 * * 呪力耐性の装備による加算(30)
 * * 祝福された装備による加算(5 + レベル / 10)
 * * 賢さによるadj_wis_savテーブル加算
 * * 呪力弱点の装備による減算(-10)
 * * 呪力弱点の装備が強力に呪われているときさらに減算(-20)
 * * 狂戦士化による減算(-30)
 * * 反魔法持ちで大なり上書き(90+レベル未満ならその値に上書き)
 * * クターのつぶれ状態なら(10に上書き)
 * * 生命の「究極の耐性」や regist_magic,magicdef持ちなら大なり上書き(95+レベル未満ならその値に上書き)
 * * 呪いのdown_savingがかかっているなら半減
 */
static ACTION_SKILL_POWER calc_saving_throw(PlayerType *player_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_rp_ptr;

    if (player_ptr->mimic_form != MimicKindType::NONE) {
        tmp_rp_ptr = &mimic_info.at(player_ptr->mimic_form);
    } else {
        tmp_rp_ptr = &race_info[enum2i(player_ptr->prace)];
    }

    const player_class_info *c_ptr = &class_info[enum2i(player_ptr->pclass)];
    const player_personality *a_ptr = &personality_info[player_ptr->ppersonality];

    pow = tmp_rp_ptr->r_sav + c_ptr->c_sav + a_ptr->a_sav;
    pow += ((cp_ptr->x_sav * player_ptr->lev / 10) + (ap_ptr->a_sav * player_ptr->lev / 50));

    if (player_ptr->muta.has(PlayerMutationType::MAGIC_RES)) {
        pow += (15 + (player_ptr->lev / 5));
    }

    if (has_resist_curse(player_ptr)) {
        pow += 30;
    }

    if (player_ptr->bless_blade) {
        pow += 6 + (player_ptr->lev - 1) / 10;
    }

    pow += adj_wis_sav[player_ptr->stat_index[A_WIS]];

    if (has_vuln_curse(player_ptr)) {
        pow -= 10;
    }

    if (has_heavy_vuln_curse(player_ptr)) {
        pow -= 20;
    }

    if (is_shero(player_ptr)) {
        pow -= 30;
    }

    if (player_ptr->anti_magic && (pow < (90 + player_ptr->lev))) {
        pow = 90 + player_ptr->lev;
    }

    if (player_ptr->tsubureru) {
        pow = 10;
    }

    if ((player_ptr->ult_res || player_ptr->resist_magic || player_ptr->magicdef) && (pow < (95 + player_ptr->lev))) {
        pow = 95 + player_ptr->lev;
    }

    if (player_ptr->down_saving) {
        pow /= 2;
    }

    return pow;
}

/*!
 * @brief 探索深度計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 探索深度
 * @details
 * * 種族/職業/性格による加算
 * * 職業とレベルによる追加加算
 * * 各装備による加算(TR_SEARCHがあれば+pval*5)
 * * 狂戦士化による減算(-15)
 * * 変異(MUT3_XTRA_EYES)による加算(+15)
 */
static ACTION_SKILL_POWER calc_search(PlayerType *player_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_rp_ptr;

    if (player_ptr->mimic_form != MimicKindType::NONE) {
        tmp_rp_ptr = &mimic_info.at(player_ptr->mimic_form);
    } else {
        tmp_rp_ptr = &race_info[enum2i(player_ptr->prace)];
    }

    const player_class_info *c_ptr = &class_info[enum2i(player_ptr->pclass)];
    const player_personality *a_ptr = &personality_info[player_ptr->ppersonality];

    pow = tmp_rp_ptr->r_srh + c_ptr->c_srh + a_ptr->a_srh;
    pow += (c_ptr->x_srh * player_ptr->lev / 10);

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->get_flags().has(TR_SEARCH)) {
            pow += (o_ptr->pval * 5);
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::XTRA_EYES)) {
        pow += 15;
    }

    if (is_shero(player_ptr)) {
        pow -= 15;
    }

    return pow;
}

/*!
 * @brief 探索頻度計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 探索頻度
 * @details
 * * 種族/職業/性格による加算
 * * 職業とレベルによる追加加算
 * * 各装備による加算(TR_SEARCHがあれば+pval*5)
 * * 狂戦士化による減算(-15)
 * * 変異(MUT3_XTRA_EYES)による加算(+15)
 */
static ACTION_SKILL_POWER calc_search_freq(PlayerType *player_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_rp_ptr;

    if (player_ptr->mimic_form != MimicKindType::NONE) {
        tmp_rp_ptr = &mimic_info.at(player_ptr->mimic_form);
    } else {
        tmp_rp_ptr = &race_info[enum2i(player_ptr->prace)];
    }

    const player_class_info *c_ptr = &class_info[enum2i(player_ptr->pclass)];
    const player_personality *a_ptr = &personality_info[player_ptr->ppersonality];

    pow = tmp_rp_ptr->r_fos + c_ptr->c_fos + a_ptr->a_fos;
    pow += (c_ptr->x_fos * player_ptr->lev / 10);

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->get_flags().has(TR_SEARCH)) {
            pow += (o_ptr->pval * 5);
        }
    }

    if (is_shero(player_ptr)) {
        pow -= 15;
    }

    if (player_ptr->muta.has(PlayerMutationType::XTRA_EYES)) {
        pow += 15;
    }

    return pow;
}

/*!
 * @brief 打撃命中能力計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 打撃命中能力
 * @details
 * * 種族/職業/性格による加算とレベルによる追加加算
 */
static ACTION_SKILL_POWER calc_to_hit_melee(PlayerType *player_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_rp_ptr;
    const player_class_info *c_ptr = &class_info[enum2i(player_ptr->pclass)];
    const player_personality *a_ptr = &personality_info[player_ptr->ppersonality];

    if (player_ptr->mimic_form != MimicKindType::NONE) {
        tmp_rp_ptr = &mimic_info.at(player_ptr->mimic_form);
    } else {
        tmp_rp_ptr = &race_info[enum2i(player_ptr->prace)];
    }

    pow = tmp_rp_ptr->r_thn + c_ptr->c_thn + a_ptr->a_thn;
    pow += ((c_ptr->x_thn * player_ptr->lev / 10) + (a_ptr->a_thn * player_ptr->lev / 50));
    return pow;
}

/*!
 * @brief 射撃命中能力計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 射撃命中能力
 * @details
 * * 種族/職業/性格による加算とレベルによる追加加算
 */
static ACTION_SKILL_POWER calc_to_hit_shoot(PlayerType *player_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_rp_ptr;
    const player_class_info *c_ptr = &class_info[enum2i(player_ptr->pclass)];
    const player_personality *a_ptr = &personality_info[player_ptr->ppersonality];

    if (player_ptr->mimic_form != MimicKindType::NONE) {
        tmp_rp_ptr = &mimic_info.at(player_ptr->mimic_form);
    } else {
        tmp_rp_ptr = &race_info[enum2i(player_ptr->prace)];
    }

    pow = tmp_rp_ptr->r_thb + c_ptr->c_thb + a_ptr->a_thb;
    pow += ((c_ptr->x_thb * player_ptr->lev / 10) + (a_ptr->a_thb * player_ptr->lev / 50));
    return pow;
}

/*!
 * @brief 投擲命中能力計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 投擲命中能力
 * @details
 * * 種族/職業/性格による加算とレベルによる追加加算
 * * 狂戦士による減算(-20)
 */
static ACTION_SKILL_POWER calc_to_hit_throw(PlayerType *player_ptr)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_rp_ptr;
    const player_class_info *c_ptr = &class_info[enum2i(player_ptr->pclass)];
    const player_personality *a_ptr = &personality_info[player_ptr->ppersonality];

    if (player_ptr->mimic_form != MimicKindType::NONE) {
        tmp_rp_ptr = &mimic_info.at(player_ptr->mimic_form);
    } else {
        tmp_rp_ptr = &race_info[enum2i(player_ptr->prace)];
    }

    pow = tmp_rp_ptr->r_thb + c_ptr->c_thb + a_ptr->a_thb;
    pow += ((c_ptr->x_thb * player_ptr->lev / 10) + (a_ptr->a_thb * player_ptr->lev / 50));

    if (is_shero(player_ptr)) {
        pow -= 20;
    }

    return pow;
}

/*!
 * @brief 掘削能力計算
 * @param player_ptr プレイヤーへの参照ポインタ
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
static ACTION_SKILL_POWER calc_skill_dig(PlayerType *player_ptr)
{
    ItemEntity *o_ptr;

    ACTION_SKILL_POWER pow;

    pow = 0;

    if (PlayerRace(player_ptr).equals(PlayerRaceType::ENT) && !player_ptr->inventory_list[INVEN_MAIN_HAND].is_valid()) {
        pow += player_ptr->lev * 10;
    }

    if (is_shero(player_ptr)) {
        pow += 30;
    }

    pow += adj_str_dig[player_ptr->stat_index[A_STR]];

    if (PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER)) {
        pow += (100 + player_ptr->lev * 8);
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->get_flags().has(TR_TUNNEL)) {
            pow += (o_ptr->pval * 20);
        }
    }

    for (int i = 0; i < 2; i++) {
        o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + i];
        if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i) && !player_ptr->heavy_wield[i]) {
            pow += (o_ptr->weight / 10);
        }
    }

    if (is_shero(player_ptr)) {
        pow += 30;
    }

    if (pow < 1) {
        pow = 1;
    }

    return pow;
}

static bool is_martial_arts_mode(PlayerType *player_ptr)
{
    PlayerClass pc(player_ptr);
    auto has_martial_arts = pc.equals(PlayerClassType::MONK);
    has_martial_arts |= pc.equals(PlayerClassType::FORCETRAINER);
    has_martial_arts |= pc.equals(PlayerClassType::BERSERKER);
    return has_martial_arts && any_bits(empty_hands(player_ptr, true), EMPTY_HAND_MAIN) && !can_attack_with_sub_hand(player_ptr);
}

static bool is_heavy_wield(PlayerType *player_ptr, int i)
{
    const auto *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + i];

    return has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i) && (calc_weapon_weight_limit(player_ptr) < o_ptr->weight / 10);
}

static int16_t calc_num_blow(PlayerType *player_ptr, int i)
{
    int16_t num_blow = 1;

    const auto *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + i];
    PlayerClass pc(player_ptr);
    if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
        if (o_ptr->is_valid() && !player_ptr->heavy_wield[i]) {
            int str_index, dex_index;
            int num = 0, wgt = 0, mul = 0, div = 0;

            auto &info = class_info[enum2i(player_ptr->pclass)];
            num = info.num;
            wgt = info.wgt;
            mul = info.mul;

            if (pc.equals(PlayerClassType::CAVALRY) && player_ptr->riding && o_ptr->get_flags().has(TR_RIDING)) {
                num = 5;
                wgt = 70;
                mul = 4;
            }

            if (SpellHex(player_ptr).is_spelling_specific(HEX_XTRA_MIGHT) || SpellHex(player_ptr).is_spelling_specific(HEX_BUILDING)) {
                num++;
                wgt /= 2;
                mul += 2;
            }

            div = ((o_ptr->weight < wgt) ? wgt : o_ptr->weight);
            str_index = (adj_str_blow[player_ptr->stat_index[A_STR]] * mul / div);

            if (has_two_handed_weapons(player_ptr) && !has_disable_two_handed_bonus(player_ptr, 0)) {
                str_index += pc.equals(PlayerClassType::WARRIOR) || pc.equals(PlayerClassType::BERSERKER) ? (player_ptr->lev / 23 + 1) : 1;
            }
            if (pc.equals(PlayerClassType::NINJA)) {
                str_index = std::max(0, str_index - 1);
            }
            if (str_index > 11) {
                str_index = 11;
            }

            dex_index = (adj_dex_blow[player_ptr->stat_index[A_DEX]]);
            if (dex_index > 11) {
                dex_index = 11;
            }

            num_blow = blows_table[str_index][dex_index];
            if (num_blow > num) {
                num_blow = (int16_t)num;
            }

            num_blow += (int16_t)player_ptr->extra_blows[i];
            if (pc.equals(PlayerClassType::WARRIOR)) {
                num_blow += (player_ptr->lev / 40);
            } else if (pc.equals(PlayerClassType::BERSERKER)) {
                num_blow += (player_ptr->lev / 23);
            } else if (pc.equals(PlayerClassType::ROGUE) && (o_ptr->weight < 50) && (player_ptr->stat_index[A_DEX] >= 30)) {
                num_blow++;
            }

            if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::FUUJIN)) {
                num_blow -= 1;
            }

            if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
                num_blow = 1;
            }

            if (has_not_ninja_weapon(player_ptr, i)) {
                num_blow /= 2;
            }

            if (num_blow < 1) {
                num_blow = 1;
            }
        }
    }

    if (i != 0) {
        return num_blow;
    }
    /* Different calculation for monks with empty hands */
    if (is_martial_arts_mode(player_ptr)) {
        int blow_base = player_ptr->lev + adj_dex_blow[player_ptr->stat_index[A_DEX]];
        num_blow = 0;

        if (pc.equals(PlayerClassType::FORCETRAINER)) {
            if (blow_base > 18) {
                num_blow++;
            }
            if (blow_base > 31) {
                num_blow++;
            }
            if (blow_base > 44) {
                num_blow++;
            }
            if (blow_base > 58) {
                num_blow++;
            }
        } else {
            if (blow_base > 12) {
                num_blow++;
            }
            if (blow_base > 22) {
                num_blow++;
            }
            if (blow_base > 31) {
                num_blow++;
            }
            if (blow_base > 39) {
                num_blow++;
            }
            if (blow_base > 46) {
                num_blow++;
            }
            if (blow_base > 53) {
                num_blow++;
            }
            if (blow_base > 59) {
                num_blow++;
            }
        }

        if (heavy_armor(player_ptr) && !pc.equals(PlayerClassType::BERSERKER)) {
            num_blow /= 2;
        }

        if (pc.monk_stance_is(MonkStanceType::GENBU)) {
            num_blow -= 2;
            if (pc.equals(PlayerClassType::MONK) && (player_ptr->lev > 42)) {
                num_blow--;
            }
            if (num_blow < 0) {
                num_blow = 0;
            }
        } else if (pc.monk_stance_is(MonkStanceType::SUZAKU)) {
            num_blow /= 2;
        }

        num_blow += 1 + player_ptr->extra_blows[0];
    }

    return num_blow;
}

/*!
 * @brief 魔法失敗値計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 魔法失敗値
 * @details
 * * 性格なまけものなら加算(+10)
 * * 性格きれものなら減算(-3)
 * * 性格ちからじまんとがまんづよいなら加算(+1)
 * * 性格チャージマンなら加算(+5)
 * * 装備品にTRC::HARD_SPELLがあるなら加算(軽い呪いなら+3/重い呪いなら+10)
 */
static int16_t calc_to_magic_chance(PlayerType *player_ptr)
{
    int16_t chance = 0;

    if (player_ptr->ppersonality == PERSONALITY_LAZY) {
        chance += 10;
    }
    if (player_ptr->ppersonality == PERSONALITY_SHREWD) {
        chance -= 3;
    }
    if ((player_ptr->ppersonality == PERSONALITY_PATIENT) || (player_ptr->ppersonality == PERSONALITY_MIGHTY)) {
        chance++;
    }
    if (player_ptr->ppersonality == PERSONALITY_CHARGEMAN) {
        chance += 5;
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->curse_flags.has(CurseTraitType::HARD_SPELL)) {
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                chance += 10;
            } else {
                chance += 3;
            }
        }
    }
    return chance;
}

static ARMOUR_CLASS calc_base_ac(PlayerType *player_ptr)
{
    ARMOUR_CLASS ac = 0;
    if (player_ptr->yoiyami) {
        return 0;
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }
        ac += o_ptr->ac;
    }

    const auto o_ptr_mh = &player_ptr->inventory_list[INVEN_MAIN_HAND];
    const auto o_ptr_sh = &player_ptr->inventory_list[INVEN_SUB_HAND];
    if (o_ptr_mh->is_protector() || o_ptr_sh->is_protector()) {
        ac += player_ptr->skill_exp[PlayerSkillKindType::SHIELD] * (1 + player_ptr->lev / 22) / 2000;
    }

    return ac;
}

static ARMOUR_CLASS calc_to_ac(PlayerType *player_ptr, bool is_real_value)
{
    ARMOUR_CLASS ac = 0;
    if (player_ptr->yoiyami) {
        return 0;
    }

    ac += ((int)(adj_dex_ta[player_ptr->stat_index[A_DEX]]) - 128);

    switch (player_ptr->mimic_form) {
    case MimicKindType::NONE:
        break;
    case MimicKindType::DEMON:
        ac += 10;
        break;
    case MimicKindType::DEMON_LORD:
        ac += 20;
        break;
    case MimicKindType::VAMPIRE:
        ac += 10;
        break;
    }

    PlayerClass pc(player_ptr);
    if (pc.equals(PlayerClassType::BERSERKER)) {
        ac += 10 + player_ptr->lev / 2;
    }

    if (pc.equals(PlayerClassType::SORCERER)) {
        ac -= 50;
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        const auto *o_ptr = &player_ptr->inventory_list[i];
        const auto flags = o_ptr->get_flags();
        if (!o_ptr->is_valid()) {
            continue;
        }
        if (is_real_value || o_ptr->is_known()) {
            ac += o_ptr->to_a;
        }

        if (o_ptr->curse_flags.has(CurseTraitType::LOW_AC) || flags.has(TR_LOW_AC)) {
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                if (is_real_value || o_ptr->is_fully_known()) {
                    ac -= 30;
                }
            } else {
                if (is_real_value || o_ptr->is_fully_known()) {
                    ac -= 10;
                }
            }
        }

        if ((i == INVEN_SUB_HAND) && flags.has(TR_SUPPORTIVE)) {
            ac += 5;
        }
    }

    PlayerRace pr(player_ptr);
    if (pr.equals(PlayerRaceType::GOLEM) || pr.equals(PlayerRaceType::ANDROID)) {
        ac += 10 + (player_ptr->lev * 2 / 5);
    }

    if (set_quick_and_tiny(player_ptr)) {
        ac += 10;
    }

    if (set_musasi(player_ptr)) {
        ac += 10;
    }

    if (set_icing_and_twinkle(player_ptr)) {
        ac += 5;
    }

    if (player_ptr->muta.has(PlayerMutationType::WART_SKIN)) {
        ac += 5;
    }

    if (player_ptr->muta.has(PlayerMutationType::SCALES)) {
        ac += 10;
    }

    if (player_ptr->muta.has(PlayerMutationType::IRON_SKIN)) {
        ac += 25;
    }

    if (pc.is_martial_arts_pro() && !heavy_armor(player_ptr)) {
        if (!player_ptr->inventory_list[INVEN_BODY].is_valid()) {
            ac += (player_ptr->lev * 3) / 2;
        }
        if (!player_ptr->inventory_list[INVEN_OUTER].is_valid() && (player_ptr->lev > 15)) {
            ac += ((player_ptr->lev - 13) / 3);
        }
        if (!player_ptr->inventory_list[INVEN_SUB_HAND].is_valid() && (player_ptr->lev > 10)) {
            ac += ((player_ptr->lev - 8) / 3);
        }
        if (!player_ptr->inventory_list[INVEN_HEAD].is_valid() && (player_ptr->lev > 4)) {
            ac += (player_ptr->lev - 2) / 3;
        }
        if (!player_ptr->inventory_list[INVEN_ARMS].is_valid()) {
            ac += (player_ptr->lev / 2);
        }
        if (!player_ptr->inventory_list[INVEN_FEET].is_valid()) {
            ac += (player_ptr->lev / 3);
        }
    }

    if (player_ptr->realm1 == REALM_HEX) {
        if (SpellHex(player_ptr).is_spelling_specific(HEX_ICE_ARMOR)) {
            ac += 30;
        }

        for (int i = INVEN_MAIN_HAND; i <= INVEN_FEET; i++) {
            auto *o_ptr = &player_ptr->inventory_list[i];
            if (!o_ptr->is_valid()) {
                continue;
            }
            if (!o_ptr->is_protector()) {
                continue;
            }
            if (!o_ptr->is_cursed()) {
                continue;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::CURSED)) {
                ac += 5;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                ac += 7;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
                ac += 13;
            }
        }
    }

    if (pc.monk_stance_is(MonkStanceType::GENBU)) {
        ac += (player_ptr->lev * player_ptr->lev) / 50;
    } else if (pc.monk_stance_is(MonkStanceType::BYAKKO)) {
        ac -= 40;
    } else if (pc.monk_stance_is(MonkStanceType::SEIRYU)) {
        ac -= 50;
    } else if (pc.samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        ac -= 50;
    }

    if (player_ptr->ult_res || (pc.samurai_stance_is(SamuraiStanceType::MUSOU))) {
        ac += 100;
    } else if (player_ptr->tsubureru || player_ptr->shield || player_ptr->magicdef) {
        ac += 50;
    }

    if (is_blessed(player_ptr)) {
        ac += 5;
    }

    if (is_shero(player_ptr)) {
        ac -= 10;
    }

    if (pc.equals(PlayerClassType::NINJA)) {
        const auto bi_id_main = player_ptr->inventory_list[INVEN_MAIN_HAND].bi_id;
        const auto bi_id_sub = player_ptr->inventory_list[INVEN_SUB_HAND].bi_id;
        if (((bi_id_main == 0) || can_attack_with_main_hand(player_ptr)) && ((bi_id_sub == 0) || can_attack_with_sub_hand(player_ptr))) {
            ac += player_ptr->lev / 2 + 5;
        }
    }

    return ac;
}

/*!
 * @brief 二刀流ペナルティ量計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param slot ペナルティ量を計算する武器スロット
 * @return 二刀流ペナルティ量
 * @details
 * * 二刀流にしていなければ0
 * * 特別セットによる軽減
 * * EASY2_WEAPONによる軽減
 * * SUPPORTIVEを左に装備した場合の軽減
 * * 武蔵セットによる免除
 * * 竿状武器による増加
 */
int16_t calc_double_weapon_penalty(PlayerType *player_ptr, INVENTORY_IDX slot)
{
    int penalty = 0;

    if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
        const auto flags = player_ptr->inventory_list[INVEN_SUB_HAND].get_flags();

        penalty = ((100 - player_ptr->skill_exp[PlayerSkillKindType::TWO_WEAPON] / 160) - (130 - player_ptr->inventory_list[slot].weight) / 8);
        if (set_quick_and_tiny(player_ptr) || set_icing_and_twinkle(player_ptr) || set_anubis_and_chariot(player_ptr)) {
            penalty = penalty / 2 - 5;
        }

        for (uint i = FLAG_CAUSE_INVEN_MAIN_HAND; i < FLAG_CAUSE_MAX; i <<= 1) {
            if (penalty > 0 && any_bits(player_ptr->easy_2weapon, i)) {
                penalty /= 2;
            }
        }

        if (flags.has(TR_SUPPORTIVE)) {
            penalty = std::max(0, penalty - 10);
        }

        if (set_musasi(player_ptr)) {
            penalty = std::min(0, penalty);
        }

        if (player_ptr->inventory_list[slot].bi_key.tval() == ItemKindType::POLEARM) {
            penalty += 10;
        }
    }
    return (int16_t)penalty;
}

static bool is_riding_two_hands(PlayerType *player_ptr)
{
    if (!player_ptr->riding || none_bits(player_ptr->pet_extra_flags, PF_TWO_HANDS)) {
        return false;
    }

    if (has_two_handed_weapons(player_ptr) || (empty_hands(player_ptr, false) == EMPTY_HAND_NONE)) {
        return true;
    }

    switch (player_ptr->pclass) {
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
    case PlayerClassType::BERSERKER:
        return (empty_hands(player_ptr, false) != EMPTY_HAND_NONE) && !has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(player_ptr, INVEN_SUB_HAND);
    default:
        return false;
    }
}

static int16_t calc_riding_bow_penalty(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!player_ptr->riding) {
        return 0;
    }

    int16_t penalty = 0;

    if (PlayerClass(player_ptr).is_tamer()) {
        if (player_ptr->tval_ammo != ItemKindType::ARROW) {
            penalty = 5;
        }
    } else {
        penalty = monraces_info[floor_ptr->m_list[player_ptr->riding].r_idx].level - player_ptr->skill_exp[PlayerSkillKindType::RIDING] / 80;
        penalty += 30;
        if (penalty < 30) {
            penalty = 30;
        }
    }

    if (player_ptr->tval_ammo == ItemKindType::BOLT) {
        penalty *= 2;
    }

    return penalty;
}

void put_equipment_warning(PlayerType *player_ptr)
{
    bool heavy_shoot = is_heavy_shoot(player_ptr, &player_ptr->inventory_list[INVEN_BOW]);
    if (player_ptr->old_heavy_shoot != heavy_shoot) {
        if (heavy_shoot) {
            msg_print(_("こんな重い弓を装備しているのは大変だ。", "You have trouble wielding such a heavy bow."));
        } else if (player_ptr->inventory_list[INVEN_BOW].is_valid()) {
            msg_print(_("この弓なら装備していても辛くない。", "You have no trouble wielding your bow."));
        } else {
            msg_print(_("重い弓を装備からはずして体が楽になった。", "You feel relieved to put down your heavy bow."));
        }
        player_ptr->old_heavy_shoot = heavy_shoot;
    }

    for (int i = 0; i < 2; i++) {
        if (player_ptr->old_heavy_wield[i] != player_ptr->heavy_wield[i]) {
            if (player_ptr->heavy_wield[i]) {
                msg_print(_("こんな重い武器を装備しているのは大変だ。", "You have trouble wielding such a heavy weapon."));
            } else if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
                msg_print(_("これなら装備していても辛くない。", "You have no trouble wielding your weapon."));
            } else if (player_ptr->heavy_wield[1 - i]) {
                msg_print(_("まだ武器が重い。", "You still have trouble wielding a heavy weapon."));
            } else {
                msg_print(_("重い武器を装備からはずして体が楽になった。", "You feel relieved to put down your heavy weapon."));
            }

            player_ptr->old_heavy_wield[i] = player_ptr->heavy_wield[i];
        }

        if (player_ptr->old_riding_wield[i] != player_ptr->is_icky_riding_wield[i]) {
            if (player_ptr->is_icky_riding_wield[i]) {
                msg_print(_("この武器は乗馬中に使うにはむかないようだ。", "This weapon is not suitable for use while riding."));
            } else if (!player_ptr->riding) {
                msg_print(_("この武器は徒歩で使いやすい。", "This weapon is suitable for use on foot."));
            } else if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
                msg_print(_("これなら乗馬中にぴったりだ。", "This weapon is suitable for use while riding."));
            }

            player_ptr->old_riding_wield[i] = player_ptr->is_icky_riding_wield[i];
        }

        if (player_ptr->old_icky_wield[i] == player_ptr->is_icky_wield[i]) {
            continue;
        }

        if (player_ptr->is_icky_wield[i]) {
            msg_print(_("今の装備はどうも自分にふさわしくない気がする。", "You do not feel comfortable with your weapon."));
            if (w_ptr->is_loading_now) {
                chg_virtue(player_ptr, Virtue::FAITH, -1);
            }
        } else if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
            msg_print(_("今の装備は自分にふさわしい気がする。", "You feel comfortable with your weapon."));
        } else {
            msg_print(_("装備をはずしたら随分と気が楽になった。", "You feel more comfortable after removing your weapon."));
        }

        player_ptr->old_icky_wield[i] = player_ptr->is_icky_wield[i];
    }

    if (player_ptr->riding && (player_ptr->old_riding_ryoute != player_ptr->riding_ryoute)) {
        if (player_ptr->riding_ryoute) {
#ifdef JP
            msg_format("%s馬を操れない。", (empty_hands(player_ptr, false) == EMPTY_HAND_NONE) ? "両手がふさがっていて" : "");
#else
            msg_print("You are using both hand for fighting, and you can't control the pet you're riding.");
#endif
        } else {
#ifdef JP
            msg_format("%s馬を操れるようになった。", (empty_hands(player_ptr, false) == EMPTY_HAND_NONE) ? "手が空いて" : "");
#else
            msg_print("You began to control the pet you're riding with one hand.");
#endif
        }

        player_ptr->old_riding_ryoute = player_ptr->riding_ryoute;
    }

    PlayerClass pc(player_ptr);
    if ((pc.is_martial_arts_pro() || pc.equals(PlayerClassType::NINJA)) && (heavy_armor(player_ptr) != player_ptr->monk_notify_aux)) {
        if (heavy_armor(player_ptr)) {
            msg_print(_("装備が重くてバランスを取れない。", "The weight of your armor disrupts your balance."));
            if (w_ptr->is_loading_now) {
                chg_virtue(player_ptr, Virtue::HARMONY, -1);
            }
        } else {
            msg_print(_("バランスがとれるようになった。", "You regain your balance."));
        }

        player_ptr->monk_notify_aux = heavy_armor(player_ptr);
    }
}

static bool is_bare_knuckle(PlayerType *player_ptr)
{
    auto bare_knuckle = is_martial_arts_mode(player_ptr);
    bare_knuckle &= empty_hands(player_ptr, false) == (EMPTY_HAND_MAIN | EMPTY_HAND_SUB);
    return bare_knuckle;
}

static short calc_to_damage(PlayerType *player_ptr, INVENTORY_IDX slot, bool is_real_value)
{
    const auto *o_ptr = &player_ptr->inventory_list[slot];
    player_hand calc_hand = PLAYER_HAND_OTHER;
    if (slot == INVEN_MAIN_HAND) {
        calc_hand = PLAYER_HAND_MAIN;
    }
    if (slot == INVEN_SUB_HAND) {
        calc_hand = PLAYER_HAND_SUB;
    }

    auto damage = 0;
    damage += ((int)(adj_str_td[player_ptr->stat_index[A_STR]]) - 128);

    if (is_shero(player_ptr)) {
        damage += 3 + (player_ptr->lev / 5);
    }

    auto player_stun = player_ptr->effects()->stun();
    damage -= player_stun->get_damage_penalty();
    PlayerClass pc(player_ptr);
    const auto tval = o_ptr->bi_key.tval();
    if (pc.equals(PlayerClassType::PRIEST) && (o_ptr->get_flags().has_not(TR_BLESSED)) && ((tval == ItemKindType::SWORD) || (tval == ItemKindType::POLEARM))) {
        damage -= 2;
    } else if (pc.equals(PlayerClassType::BERSERKER)) {
        damage += player_ptr->lev / 6;
        if (((calc_hand == PLAYER_HAND_MAIN) && !can_attack_with_sub_hand(player_ptr)) || has_two_handed_weapons(player_ptr)) {
            damage += player_ptr->lev / 6;
        }
    } else if (pc.equals(PlayerClassType::SORCERER)) {
        auto is_suitable = o_ptr->bi_key == BaseitemKey(ItemKindType::HAFTED, SV_WIZSTAFF);
        is_suitable |= o_ptr->bi_key == BaseitemKey(ItemKindType::HAFTED, SV_NAMAKE_HAMMER);
        if (!is_suitable) {
            damage -= 200;
        } else {
            damage -= 10;
        }
    } else if (pc.equals(PlayerClassType::FORCETRAINER)) {
        // 練気術師は格闘ダメージに (気)/5 の修正を得る。
        if (is_martial_arts_mode(player_ptr) && calc_hand == PLAYER_HAND_MAIN) {
            damage += get_current_ki(player_ptr) / 5;
        }
    }

    if ((player_ptr->realm1 == REALM_HEX) && o_ptr->is_cursed()) {
        if (SpellHex(player_ptr).is_spelling_specific(HEX_RUNESWORD)) {
            if (o_ptr->curse_flags.has(CurseTraitType::CURSED)) {
                damage += 5;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                damage += 7;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
                damage += 13;
            }
        }
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        int bonus_to_d = 0;
        o_ptr = &player_ptr->inventory_list[i];
        const auto has_melee = has_melee_weapon(player_ptr, i);
        if (!o_ptr->is_valid() || (o_ptr->bi_key.tval() == ItemKindType::CAPTURE)) {
            continue;
        }

        if (((i == INVEN_MAIN_HAND) && has_melee) || ((i == INVEN_SUB_HAND) && has_melee) || (i == INVEN_BOW)) {
            continue;
        }

        if (!o_ptr->is_known() && !is_real_value) {
            continue;
        }
        bonus_to_d = o_ptr->to_d;

        if (pc.equals(PlayerClassType::NINJA)) {
            if (o_ptr->to_d > 0) {
                bonus_to_d = (o_ptr->to_d + 1) / 2;
            }
        }

        switch (player_melee_type(player_ptr)) {
        case MELEE_TYPE_BAREHAND_TWO:
        case MELEE_TYPE_WEAPON_TWOHAND:
            if (calc_hand == main_attack_hand(player_ptr)) {
                damage += (int16_t)bonus_to_d;
            }
            break;

        case MELEE_TYPE_BAREHAND_MAIN:
        case MELEE_TYPE_WEAPON_MAIN:
            if ((calc_hand == PLAYER_HAND_MAIN) && (i != INVEN_SUB_RING)) {
                damage += (int16_t)bonus_to_d;
            }
            break;

        case MELEE_TYPE_BAREHAND_SUB:
        case MELEE_TYPE_WEAPON_SUB:
            if ((calc_hand == PLAYER_HAND_SUB) && (i != INVEN_MAIN_RING)) {
                damage += (int16_t)bonus_to_d;
            }
            break;

        case MELEE_TYPE_WEAPON_DOUBLE:
            if (calc_hand == PLAYER_HAND_MAIN) {
                if (i == INVEN_MAIN_RING) {
                    damage += (int16_t)bonus_to_d;
                } else if (i != INVEN_SUB_RING) {
                    damage += (bonus_to_d > 0) ? (bonus_to_d + 1) / 2 : bonus_to_d;
                }
            }
            if (calc_hand == PLAYER_HAND_SUB) {
                if (i == INVEN_SUB_RING) {
                    damage += (int16_t)bonus_to_d;
                } else if (i != INVEN_MAIN_RING) {
                    damage += (bonus_to_d > 0) ? bonus_to_d / 2 : bonus_to_d;
                }
            }
            break;

        case MELEE_TYPE_SHIELD_DOUBLE:
            break;

        default:
            break;
        }
    }

    if (main_attack_hand(player_ptr) == calc_hand) {
        if (is_bare_knuckle(player_ptr) || !has_disable_two_handed_bonus(player_ptr, calc_hand)) {
            int bonus_to_d = 0;
            bonus_to_d = ((int)(adj_str_td[player_ptr->stat_index[A_STR]]) - 128) / 2;
            damage += std::max<int>(bonus_to_d, 1);
        }
    }

    if (is_martial_arts_mode(player_ptr) && (!heavy_armor(player_ptr) || !pc.equals(PlayerClassType::BERSERKER))) {
        damage += (player_ptr->lev / 6);
    }

    // 朱雀の構えをとっているとき、格闘ダメージに -(レベル)/6 の修正を得る。
    if (PlayerClass(player_ptr).monk_stance_is(MonkStanceType::SUZAKU)) {
        if (is_martial_arts_mode(player_ptr) && calc_hand == PLAYER_HAND_MAIN) {
            damage -= (player_ptr->lev / 6);
        }
    }

    return static_cast<short>(damage);
}

/*!
 * @brief 武器の命中修正を計算する。 / Calculate hit bonus from a wielded weapon.
 * @details
 * 'slot' MUST be INVEN_MAIN_HAND or INVEM_SUB_HAND.
 */
static short calc_to_hit(PlayerType *player_ptr, INVENTORY_IDX slot, bool is_real_value)
{
    auto hit = 0;

    /* Base bonuses */
    hit += ((int)(adj_dex_th[player_ptr->stat_index[A_DEX]]) - 128);
    hit += ((int)(adj_str_th[player_ptr->stat_index[A_STR]]) - 128);

    /* Temporary bonuses */
    if (is_blessed(player_ptr)) {
        hit += 10;
    }

    if (is_hero(player_ptr)) {
        hit += 12;
    }

    if (is_shero(player_ptr)) {
        hit += 12;
    }

    auto player_stun = player_ptr->effects()->stun();
    hit -= player_stun->get_damage_penalty();
    player_hand calc_hand = PLAYER_HAND_OTHER;
    if (slot == INVEN_MAIN_HAND) {
        calc_hand = PLAYER_HAND_MAIN;
    }
    if (slot == INVEN_SUB_HAND) {
        calc_hand = PLAYER_HAND_SUB;
    }

    /* Default hand bonuses */
    if (main_attack_hand(player_ptr) == calc_hand) {
        switch (player_melee_type(player_ptr)) {
        case MELEE_TYPE_BAREHAND_MAIN:
            if (player_ptr->riding) {
                break;
            }
            [[fallthrough]];
        case MELEE_TYPE_BAREHAND_SUB:
            if (player_ptr->riding) {
                break;
            }
            [[fallthrough]];
        case MELEE_TYPE_BAREHAND_TWO:
            hit += (player_ptr->skill_exp[PlayerSkillKindType::MARTIAL_ARTS] - PlayerSkill::weapon_exp_at(PlayerSkillRank::BEGINNER)) / 200;
            break;

        default:
            break;
        }

        if (is_bare_knuckle(player_ptr) || !has_disable_two_handed_bonus(player_ptr, calc_hand)) {
            int bonus_to_h = 0;
            bonus_to_h = ((int)(adj_str_th[player_ptr->stat_index[A_STR]]) - 128) + ((int)(adj_dex_th[player_ptr->stat_index[A_DEX]]) - 128);
            hit += std::max<int>(bonus_to_h, 1);
        }
    }

    /* Bonuses and penalties by weapon */
    PlayerClass pc(player_ptr);
    if (has_melee_weapon(player_ptr, slot)) {
        const auto *o_ptr = &player_ptr->inventory_list[slot];

        /* Traind bonuses */
        const auto tval = o_ptr->bi_key.tval();
        const auto sval = o_ptr->bi_key.sval().value();
        hit += (player_ptr->weapon_exp[tval][sval] - PlayerSkill::weapon_exp_at(PlayerSkillRank::BEGINNER)) / 200;

        /* Weight penalty */
        if (calc_weapon_weight_limit(player_ptr) < o_ptr->weight / 10) {
            hit += 2 * (calc_weapon_weight_limit(player_ptr) - o_ptr->weight / 10);
        }

        /* Low melee penalty */
        if ((o_ptr->is_fully_known() || is_real_value) && o_ptr->curse_flags.has(CurseTraitType::LOW_MELEE)) {
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                hit -= 15;
            } else {
                hit -= 5;
            }
        }

        /* Riding bonus and penalty */
        const auto flags = o_ptr->get_flags();
        if (player_ptr->riding > 0) {
            if (o_ptr->is_lance()) {
                hit += 15;
            } else if (flags.has_not(TR_RIDING)) {
                short penalty;
                if (PlayerClass(player_ptr).is_tamer()) {
                    penalty = 5;
                } else {
                    penalty = monraces_info[player_ptr->current_floor_ptr->m_list[player_ptr->riding].r_idx].level - player_ptr->skill_exp[PlayerSkillKindType::RIDING] / 80;
                    penalty += 30;
                    if (penalty < 30) {
                        penalty = 30;
                    }
                }

                hit -= penalty;
            }
        }

        /* Class penalties */
        if (pc.equals(PlayerClassType::PRIEST) && (flags.has_not(TR_BLESSED)) && ((tval == ItemKindType::SWORD) || (tval == ItemKindType::POLEARM))) {
            hit -= 2;
        } else if (pc.equals(PlayerClassType::BERSERKER)) {
            hit += player_ptr->lev / 5;
            if (((calc_hand == PLAYER_HAND_MAIN) && !can_attack_with_sub_hand(player_ptr)) || has_two_handed_weapons(player_ptr)) {
                hit += player_ptr->lev / 5;
            }
        } else if (pc.equals(PlayerClassType::SORCERER)) {
            auto is_suitable = o_ptr->bi_key == BaseitemKey(ItemKindType::HAFTED, SV_WIZSTAFF);
            is_suitable |= o_ptr->bi_key == BaseitemKey(ItemKindType::HAFTED, SV_NAMAKE_HAMMER);
            if (!is_suitable) {
                hit -= 200;
            } else {
                hit -= 30;
            }
        }

        if (has_not_ninja_weapon(player_ptr, (int)calc_hand) || has_not_monk_weapon(player_ptr, (int)calc_hand)) {
            hit -= 40;
        }

        /* Hex realm bonuses */
        if ((player_ptr->realm1 == REALM_HEX) && o_ptr->is_cursed()) {
            if (o_ptr->curse_flags.has(CurseTraitType::CURSED)) {
                hit += 5;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                hit += 7;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
                hit += 13;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::TY_CURSE)) {
                hit += 5;
            }
        }
    }

    /* Bonuses from inventory */
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];

        /* Ignore empty hands, handed weapons, bows and capture balls */
        const auto has_melee = has_melee_weapon(player_ptr, i);
        if (!o_ptr->is_valid() || o_ptr->bi_key.tval() == ItemKindType::CAPTURE) {
            continue;
        }

        if (((i == INVEN_MAIN_HAND) && has_melee) || ((i == INVEN_SUB_HAND) && has_melee) || (i == INVEN_BOW)) {
            continue;
        }

        /* Fake value does not include unknown objects' value */
        if (!o_ptr->is_known() && !is_real_value) {
            continue;
        }

        int bonus_to_h = o_ptr->to_h;

        /* When wields only a weapon */
        if (pc.equals(PlayerClassType::NINJA)) {
            if (o_ptr->to_h > 0) {
                bonus_to_h = (o_ptr->to_h + 1) / 2;
            }
        }

        switch (player_melee_type(player_ptr)) {
        case MELEE_TYPE_BAREHAND_TWO:
        case MELEE_TYPE_WEAPON_TWOHAND:
            if (calc_hand == main_attack_hand(player_ptr)) {
                hit += (int16_t)bonus_to_h;
            }
            break;

        case MELEE_TYPE_BAREHAND_MAIN:
        case MELEE_TYPE_WEAPON_MAIN:
            if ((calc_hand == PLAYER_HAND_MAIN) && (i != INVEN_SUB_RING)) {
                hit += (int16_t)bonus_to_h;
            }
            break;

        case MELEE_TYPE_BAREHAND_SUB:
        case MELEE_TYPE_WEAPON_SUB:
            if ((calc_hand == PLAYER_HAND_SUB) && (i != INVEN_MAIN_RING)) {
                hit += (int16_t)bonus_to_h;
            }
            break;

        case MELEE_TYPE_WEAPON_DOUBLE:
            if (calc_hand == PLAYER_HAND_MAIN) {
                if (i == INVEN_MAIN_RING) {
                    hit += (int16_t)bonus_to_h;
                } else if (i != INVEN_SUB_RING) {
                    hit += (bonus_to_h > 0) ? (bonus_to_h + 1) / 2 : bonus_to_h;
                }
            }
            if (calc_hand == PLAYER_HAND_SUB) {
                if (i == INVEN_SUB_RING) {
                    hit += (int16_t)bonus_to_h;
                } else if (i != INVEN_MAIN_RING) {
                    hit += (bonus_to_h > 0) ? bonus_to_h / 2 : bonus_to_h;
                }
            }
            break;

        case MELEE_TYPE_SHIELD_DOUBLE:
            break;

        default:
            break;
        }
    }

    /* Martial arts bonus */
    if (is_martial_arts_mode(player_ptr) && (!heavy_armor(player_ptr) || !pc.equals(PlayerClassType::BERSERKER))) {
        hit += (player_ptr->lev / 3);
    }

    /* Two handed combat penalty */
    hit -= calc_double_weapon_penalty(player_ptr, slot);

    // 朱雀の構えをとっているとき、格闘命中に -(レベル)/3 の修正を得る。
    if (PlayerClass(player_ptr).monk_stance_is(MonkStanceType::SUZAKU)) {
        if (is_martial_arts_mode(player_ptr) && calc_hand == PLAYER_HAND_MAIN) {
            hit -= (player_ptr->lev / 3);
        }
    }

    return static_cast<short>(hit);
}

static int16_t calc_to_hit_bow(PlayerType *player_ptr, bool is_real_value)
{
    int16_t pow = 0;

    pow += ((int)(adj_dex_th[player_ptr->stat_index[A_DEX]]) - 128);
    pow += ((int)(adj_str_th[player_ptr->stat_index[A_STR]]) - 128);

    {
        ItemEntity *o_ptr;
        o_ptr = &player_ptr->inventory_list[INVEN_BOW];
        if (o_ptr->is_valid()) {
            if (o_ptr->curse_flags.has(CurseTraitType::LOW_MELEE)) {
                if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                    pow -= 15;
                } else {
                    pow -= 5;
                }
            }
        }
    }

    auto player_stun = player_ptr->effects()->stun();
    pow -= player_stun->get_damage_penalty();
    if (is_blessed(player_ptr)) {
        pow += 10;
    }

    if (is_hero(player_ptr)) {
        pow += 12;
    }

    if (is_shero(player_ptr)) {
        pow -= 12;
    }

    auto *o_ptr = &player_ptr->inventory_list[INVEN_BOW];

    if (is_heavy_shoot(player_ptr, o_ptr)) {
        pow += 2 * (calc_bow_weight_limit(player_ptr) - o_ptr->weight / 10);
    }

    if (o_ptr->is_valid()) {
        if (!is_heavy_shoot(player_ptr, &player_ptr->inventory_list[INVEN_BOW])) {
            if (PlayerClass(player_ptr).equals(PlayerClassType::SNIPER) && (player_ptr->tval_ammo == ItemKindType::BOLT)) {
                pow += (10 + (player_ptr->lev / 5));
            }
        }
    }

    // 武器以外の装備による修正
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        int bonus_to_h;
        o_ptr = &player_ptr->inventory_list[i];
        const auto has_melee = has_melee_weapon(player_ptr, i);
        if (!o_ptr->is_valid() || (o_ptr->bi_key.tval() == ItemKindType::CAPTURE)) {
            continue;
        }

        if (((i == INVEN_MAIN_HAND) && has_melee) || ((i == INVEN_SUB_HAND) && has_melee) || (i == INVEN_BOW)) {
            continue;
        }

        bonus_to_h = o_ptr->to_h;

        if (PlayerClass(player_ptr).equals(PlayerClassType::NINJA)) {
            if (o_ptr->to_h > 0) {
                bonus_to_h = (o_ptr->to_h + 1) / 2;
            }
        }

        if (is_real_value || o_ptr->is_known()) {
            pow += (int16_t)bonus_to_h;
        }
    }

    pow -= calc_riding_bow_penalty(player_ptr);

    return pow;
}

static int16_t calc_to_damage_misc(PlayerType *player_ptr)
{
    ItemEntity *o_ptr;

    int16_t to_dam = 0;

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        int bonus_to_d = o_ptr->to_d;
        if (PlayerClass(player_ptr).equals(PlayerClassType::NINJA)) {
            if (o_ptr->to_d > 0) {
                bonus_to_d = (o_ptr->to_d + 1) / 2;
            }
        }
        to_dam += (int16_t)bonus_to_d;
    }

    if (is_shero(player_ptr)) {
        to_dam += 3 + (player_ptr->lev / 5);
    }

    auto player_stun = player_ptr->effects()->stun();
    to_dam -= player_stun->get_damage_penalty();
    to_dam += ((int)(adj_str_td[player_ptr->stat_index[A_STR]]) - 128);
    return to_dam;
}

static int16_t calc_to_hit_misc(PlayerType *player_ptr)
{
    ItemEntity *o_ptr;

    int16_t to_hit = 0;

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        int bonus_to_h = o_ptr->to_h;
        if (PlayerClass(player_ptr).equals(PlayerClassType::NINJA)) {
            if (o_ptr->to_h > 0) {
                bonus_to_h = (o_ptr->to_h + 1) / 2;
            }
        }
        to_hit += (int16_t)bonus_to_h;
    }

    if (is_blessed(player_ptr)) {
        to_hit += 10;
    }

    if (is_hero(player_ptr)) {
        to_hit += 12;
    }

    if (is_shero(player_ptr)) {
        to_hit += 12;
    }

    auto player_stun = player_ptr->effects()->stun();
    to_hit -= player_stun->get_damage_penalty();
    to_hit += ((int)(adj_dex_th[player_ptr->stat_index[A_DEX]]) - 128);
    to_hit += ((int)(adj_str_th[player_ptr->stat_index[A_STR]]) - 128);

    return to_hit;
}

static DICE_NUMBER calc_to_weapon_dice_num(PlayerType *player_ptr, INVENTORY_IDX slot)
{
    auto *o_ptr = &player_ptr->inventory_list[slot];
    return (player_ptr->riding > 0) && o_ptr->is_lance() ? 2 : 0;
}

/*!
 * @brief プレイヤーの所持重量制限を計算する /
 * Computes current weight limit.
 * @return 制限重量(ポンド)
 */
WEIGHT calc_weight_limit(PlayerType *player_ptr)
{
    WEIGHT i = (WEIGHT)adj_str_wgt[player_ptr->stat_index[A_STR]] * 50;
    if (PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER)) {
        i = i * 3 / 2;
    }
    return i;
}

/*!
 * @brief update のフラグに応じた更新をまとめて行う / Handle "update"
 * @details 更新処理の対象はプレイヤーの能力修正/光源寿命/HP/MP/魔法の学習状態、他多数の外界の状態判定。
 */
void update_creature(PlayerType *player_ptr)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (!rfu.any_stats()) {
        return;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (rfu.has(StatusRecalculatingFlag::AUTO_DESTRUCTION)) {
        rfu.reset_flag(StatusRecalculatingFlag::AUTO_DESTRUCTION);
        autopick_delayed_alter(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::COMBINATION)) {
        rfu.reset_flag(StatusRecalculatingFlag::COMBINATION);
        combine_pack(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::REORDER)) {
        rfu.reset_flag(StatusRecalculatingFlag::REORDER);
        reorder_pack(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::BONUS)) {
        rfu.reset_flag(StatusRecalculatingFlag::BONUS);
        PlayerAlignment(player_ptr).update_alignment();
        PlayerSkill ps(player_ptr);
        ps.apply_special_weapon_skill_max_values();
        ps.limit_weapon_skills_by_max_value();
        update_bonuses(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::TORCH)) {
        rfu.reset_flag(StatusRecalculatingFlag::TORCH);
        update_lite_radius(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::HP)) {
        rfu.reset_flag(StatusRecalculatingFlag::HP);
        update_max_hitpoints(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::MP)) {
        rfu.reset_flag(StatusRecalculatingFlag::MP);
        update_max_mana(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::SPELLS)) {
        rfu.reset_flag(StatusRecalculatingFlag::SPELLS);
        update_num_of_spells(player_ptr);
    }

    if (!w_ptr->character_generated || (w_ptr->character_icky_depth > 0)) {
        return;
    }

    if (rfu.has(StatusRecalculatingFlag::UN_LITE)) {
        rfu.reset_flag(StatusRecalculatingFlag::UN_LITE);
        forget_lite(floor_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::UN_VIEW)) {
        rfu.reset_flag(StatusRecalculatingFlag::UN_VIEW);
        forget_view(floor_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::VIEW)) {
        rfu.reset_flag(StatusRecalculatingFlag::VIEW);
        update_view(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::LITE)) {
        rfu.reset_flag(StatusRecalculatingFlag::LITE);
        update_lite(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::FLOW)) {
        rfu.reset_flag(StatusRecalculatingFlag::FLOW);
        update_flow(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::DISTANCE)) {
        rfu.reset_flag(StatusRecalculatingFlag::DISTANCE);
        update_monsters(player_ptr, true);
    }

    if (rfu.has(StatusRecalculatingFlag::MONSTER_LITE)) {
        rfu.reset_flag(StatusRecalculatingFlag::MONSTER_LITE);
        update_mon_lite(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::DELAY_VISIBILITY)) {
        rfu.reset_flag(StatusRecalculatingFlag::DELAY_VISIBILITY);
        delayed_visual_update(player_ptr);
    }

    if (rfu.has(StatusRecalculatingFlag::MONSTER_STATUSES)) {
        rfu.reset_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        update_monsters(player_ptr, false);
    }
}

/*!
 * @brief プレイヤーが魔道書を一冊も持っていないかを判定する
 * @return 魔道書を一冊も持っていないならTRUEを返す
 */
bool player_has_no_spellbooks(PlayerType *player_ptr)
{
    ItemEntity *o_ptr;
    for (int i = 0; i < INVEN_PACK; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (o_ptr->is_valid() && check_book_realm(player_ptr, o_ptr->bi_key)) {
            return false;
        }
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (const auto this_o_idx : floor_ptr->grid_array[player_ptr->y][player_ptr->x].o_idx_list) {
        o_ptr = &floor_ptr->o_list[this_o_idx];
        if (o_ptr->is_valid() && o_ptr->marked.has(OmType::FOUND) && check_book_realm(player_ptr, o_ptr->bi_key)) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief プレイヤーを指定座標に配置する / Place the player in the dungeon XXX XXX
 * @param x 配置先X座標
 * @param y 配置先Y座標
 * @return 配置に成功したらTRUE
 */
bool player_place(PlayerType *player_ptr, POSITION y, POSITION x)
{
    if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx != 0) {
        return false;
    }

    /* Save player location */
    player_ptr->y = y;
    player_ptr->x = x;
    return true;
}

/*!
 * @brief 種族アンバライトが出血時パターンの上に乗った際のペナルティ処理
 */
void wreck_the_pattern(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    int pattern_type = terrains_info[floor_ptr->grid_array[player_ptr->y][player_ptr->x].feat].subtype;
    if (pattern_type == PATTERN_TILE_WRECKED) {
        return;
    }

    msg_print(_("パターンを血で汚してしまった！", "You bleed on the Pattern!"));
    msg_print(_("何か恐ろしい事が起こった！", "Something terrible happens!"));

    if (!is_invuln(player_ptr)) {
        take_hit(player_ptr, DAMAGE_NOESCAPE, damroll(10, 8), _("パターン損壊", "corrupting the Pattern"));
    }

    int to_ruin = randint1(45) + 35;
    while (to_ruin--) {
        POSITION r_y, r_x;
        scatter(player_ptr, &r_y, &r_x, player_ptr->y, player_ptr->x, 4, PROJECT_NONE);

        if (pattern_tile(floor_ptr, r_y, r_x) && (terrains_info[floor_ptr->grid_array[r_y][r_x].feat].subtype != PATTERN_TILE_WRECKED)) {
            cave_set_feat(player_ptr, r_y, r_x, feat_pattern_corrupted);
        }
    }

    cave_set_feat(player_ptr, player_ptr->y, player_ptr->x, feat_pattern_corrupted);
}

/*!
 * @brief プレイヤーの経験値について整合性のためのチェックと調整を行う /
 * Advance experience levels and print experience
 */
void check_experience(PlayerType *player_ptr)
{
    if (player_ptr->exp < 0) {
        player_ptr->exp = 0;
    }
    if (player_ptr->max_exp < 0) {
        player_ptr->max_exp = 0;
    }
    if (player_ptr->max_max_exp < 0) {
        player_ptr->max_max_exp = 0;
    }

    if (player_ptr->exp > PY_MAX_EXP) {
        player_ptr->exp = PY_MAX_EXP;
    }
    if (player_ptr->max_exp > PY_MAX_EXP) {
        player_ptr->max_exp = PY_MAX_EXP;
    }
    if (player_ptr->max_max_exp > PY_MAX_EXP) {
        player_ptr->max_max_exp = PY_MAX_EXP;
    }

    if (player_ptr->exp > player_ptr->max_exp) {
        player_ptr->max_exp = player_ptr->exp;
    }
    if (player_ptr->max_exp > player_ptr->max_max_exp) {
        player_ptr->max_max_exp = player_ptr->max_exp;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::EXP);
    handle_stuff(player_ptr);

    PlayerRace pr(player_ptr);
    bool android = pr.equals(PlayerRaceType::ANDROID);
    PLAYER_LEVEL old_lev = player_ptr->lev;
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    };
    while ((player_ptr->lev > 1) && (player_ptr->exp < ((android ? player_exp_a : player_exp)[player_ptr->lev - 2] * player_ptr->expfact / 100L))) {
        player_ptr->lev--;
        rfu.set_flags(flags_srf);
        static constexpr auto flags_mwrf = {
            MainWindowRedrawingFlag::LEVEL,
            MainWindowRedrawingFlag::TITLE,
        };
        rfu.set_flags(flags_mwrf);
        rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
        handle_stuff(player_ptr);
    }

    bool level_reward = false;
    bool level_mutation = false;
    bool level_inc_stat = false;
    while ((player_ptr->lev < PY_MAX_LEVEL) && (player_ptr->exp >= ((android ? player_exp_a : player_exp)[player_ptr->lev - 1] * player_ptr->expfact / 100L))) {
        player_ptr->lev++;
        if (player_ptr->lev > player_ptr->max_plv) {
            player_ptr->max_plv = player_ptr->lev;

            if (PlayerClass(player_ptr).equals(PlayerClassType::CHAOS_WARRIOR) || player_ptr->muta.has(PlayerMutationType::CHAOS_GIFT)) {
                level_reward = true;
            }
            if (pr.equals(PlayerRaceType::BEASTMAN)) {
                if (one_in_(5)) {
                    level_mutation = true;
                }
            }
            level_inc_stat = true;

            exe_write_diary(player_ptr, DiaryKind::LEVELUP, player_ptr->lev);
        }

        sound(SOUND_LEVEL);
        msg_format(_("レベル %d にようこそ。", "Welcome to level %d."), player_ptr->lev);
        rfu.set_flags(flags_srf);
        const auto flags_mwrf_levelup = {
            MainWindowRedrawingFlag::LEVEL,
            MainWindowRedrawingFlag::TITLE,
            MainWindowRedrawingFlag::EXP,
        };
        rfu.set_flags(flags_mwrf_levelup);
        const auto &flags_swrf_levelup = {
            SubWindowRedrawingFlag::PLAYER,
            SubWindowRedrawingFlag::SPELL,
            SubWindowRedrawingFlag::INVENTORY,
        };
        rfu.set_flags(flags_swrf_levelup);
        player_ptr->level_up_message = true;
        handle_stuff(player_ptr);

        player_ptr->level_up_message = false;
        if (level_inc_stat) {
            if (!(player_ptr->max_plv % 10)) {
                int choice;
                screen_save();
                while (true) {
                    int n;

                    prt(format(_("        a) 腕力 (現在値 %s)", "        a) Str (cur %s)"), cnv_stat(player_ptr->stat_max[0]).data()), 2, 14);
                    prt(format(_("        b) 知能 (現在値 %s)", "        b) Int (cur %s)"), cnv_stat(player_ptr->stat_max[1]).data()), 3, 14);
                    prt(format(_("        c) 賢さ (現在値 %s)", "        c) Wis (cur %s)"), cnv_stat(player_ptr->stat_max[2]).data()), 4, 14);
                    prt(format(_("        d) 器用 (現在値 %s)", "        d) Dex (cur %s)"), cnv_stat(player_ptr->stat_max[3]).data()), 5, 14);
                    prt(format(_("        e) 耐久 (現在値 %s)", "        e) Con (cur %s)"), cnv_stat(player_ptr->stat_max[4]).data()), 6, 14);
                    prt(format(_("        f) 魅力 (現在値 %s)", "        f) Chr (cur %s)"), cnv_stat(player_ptr->stat_max[5]).data()), 7, 14);

                    prt("", 8, 14);
                    prt(_("        どの能力値を上げますか？", "        Which stat do you want to raise?"), 1, 14);

                    while (true) {
                        choice = inkey();
                        if ((choice >= 'a') && (choice <= 'f')) {
                            break;
                        }
                    }
                    for (n = 0; n < A_MAX; n++) {
                        if (n != choice - 'a') {
                            prt("", n + 2, 14);
                        }
                    }
                    if (input_check(_("よろしいですか？", "Are you sure? "))) {
                        break;
                    }
                }
                do_inc_stat(player_ptr, choice - 'a');
                screen_load();
            } else if (!(player_ptr->max_plv % 2)) {
                do_inc_stat(player_ptr, randint0(6));
            }
        }

        if (level_mutation) {
            msg_print(_("あなたは変わった気がする...", "You feel different..."));
            (void)gain_mutation(player_ptr, 0);
            level_mutation = false;
        }

        /*
         * 報酬でレベルが上ると再帰的に check_experience(player_ptr) が
         * 呼ばれるので順番を最後にする。
         */
        if (level_reward) {
            patron_list[player_ptr->chaos_patron].gain_level_reward(player_ptr, 0);
            level_reward = false;
        }

        rfu.set_flags(flags_srf);
        static constexpr auto flags_mwrf = {
            MainWindowRedrawingFlag::LEVEL,
            MainWindowRedrawingFlag::TITLE,
        };
        rfu.set_flags(flags_mwrf);
        static constexpr auto flags_swrf = {
            SubWindowRedrawingFlag::PLAYER,
            SubWindowRedrawingFlag::SPELL,
        };
        rfu.set_flags(flags_swrf);
        handle_stuff(player_ptr);
    }

    if (old_lev != player_ptr->lev) {
        autopick_load_pref(player_ptr, false);
    }
}

/*!
 * @brief 現在の修正後能力値を3～17及び18/xxx形式に変換する / Converts stat num into a six-char (right justified) string
 * @param val 能力値
 * @return std::string 右詰め6文字で記述した能力値
 */
std::string cnv_stat(int val)
{
    if (val <= 18) {
        return format("    %2d", val);
    }

    int bonus = (val - 18);
    if (bonus >= 220) {
        return "18/***";
    } else if (bonus >= 100) {
        return format("18/%03d", bonus);
    } else {
        return format(" 18/%02d", bonus);
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
int16_t modify_stat_value(int value, int amount)
{
    if (amount > 0) {
        for (int i = 0; i < amount; i++) {
            if (value < 18) {
                value++;
            } else {
                value += 10;
            }
        }
    } else if (amount < 0) {
        for (int i = 0; i < (0 - amount); i++) {
            if (value >= 18 + 10) {
                value -= 10;
            } else if (value > 18) {
                value = 18;
            } else if (value > 3) {
                value--;
            }
        }
    }

    return (int16_t)value;
}

/*!
 * @brief スコアを計算する /
 * Hack -- Calculates the total number of points earned		-JWT-
 * @details
 */
long calc_score(PlayerType *player_ptr)
{
    int arena_win = std::min<int>(player_ptr->arena_number, MAX_ARENA_MONS);

    int mult = 100;
    if (!preserve_mode) {
        mult += 10;
    }
    if (!autoroller) {
        mult += 10;
    }
    if (!smart_learn) {
        mult -= 20;
    }
    if (smart_cheat) {
        mult += 30;
    }
    if (ironman_shops) {
        mult += 50;
    }
    if (ironman_small_levels) {
        mult += 10;
    }
    if (ironman_empty_levels) {
        mult += 20;
    }
    if (!powerup_home) {
        mult += 50;
    }
    if (ironman_rooms) {
        mult += 100;
    }
    if (ironman_nightmare) {
        mult += 100;
    }

    if (mult < 5) {
        mult = 5;
    }

    DEPTH max_dl = 0;
    for (const auto &d_ref : dungeons_info) {
        if (max_dl < max_dlv[d_ref.idx]) {
            max_dl = max_dlv[d_ref.idx];
        }
    }

    uint32_t point_l = (player_ptr->max_max_exp + (100 * max_dl));
    uint32_t point_h = point_l / 0x10000L;
    point_l = point_l % 0x10000L;
    point_h *= mult;
    point_l *= mult;
    point_h += point_l / 0x10000L;
    point_l %= 0x10000L;

    point_l += ((point_h % 100) << 16);
    point_h /= 100;
    point_l /= 100;

    uint32_t point = (point_h << 16) + (point_l);
    if (player_ptr->arena_number >= 0) {
        point += (arena_win * arena_win * (arena_win > 29 ? 1000 : 100));
    }

    if (ironman_downward) {
        point *= 2;
    }
    if (PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER)) {
        if (PlayerRace(player_ptr).equals(PlayerRaceType::SPECTRE)) {
            point = point / 5;
        }
    }

    if ((player_ptr->ppersonality == PERSONALITY_MUNCHKIN) && point) {
        point = 1;
        if (w_ptr->total_winner) {
            point = 2;
        }
    }

    return point;
}

/*!
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 祝福状態ならばTRUE
 */
bool is_blessed(PlayerType *player_ptr)
{
    return player_ptr->blessed || music_singing(player_ptr, MUSIC_BLESS) || SpellHex(player_ptr).is_spelling_specific(HEX_BLESS);
}

bool is_tim_esp(PlayerType *player_ptr)
{
    auto sniper_data = PlayerClass(player_ptr).get_specific_data<SniperData>();
    auto sniper_concent = sniper_data ? sniper_data->concent : 0;
    return player_ptr->tim_esp || music_singing(player_ptr, MUSIC_MIND) || (sniper_concent >= CONCENT_TELE_THRESHOLD);
}

bool is_tim_stealth(PlayerType *player_ptr)
{
    return player_ptr->tim_stealth || music_singing(player_ptr, MUSIC_STEALTH);
}

bool is_time_limit_esp(PlayerType *player_ptr)
{
    auto sniper_data = PlayerClass(player_ptr).get_specific_data<SniperData>();
    auto sniper_concent = sniper_data ? sniper_data->concent : 0;
    return player_ptr->tim_esp || music_singing(player_ptr, MUSIC_MIND) || (sniper_concent >= CONCENT_TELE_THRESHOLD);
}

bool is_time_limit_stealth(PlayerType *player_ptr)
{
    return player_ptr->tim_stealth || music_singing(player_ptr, MUSIC_STEALTH);
}

/*!
 * @brief 口を使う継続的な処理を中断する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void stop_mouth(PlayerType *player_ptr)
{
    if (music_singing_any(player_ptr)) {
        stop_singing(player_ptr);
    }

    if (SpellHex(player_ptr).is_spelling_any()) {
        (void)SpellHex(player_ptr).stop_all_spells();
    }
}

bool is_fast(PlayerType *player_ptr)
{
    return player_ptr->effects()->acceleration()->is_fast() || music_singing(player_ptr, MUSIC_SPEED) || music_singing(player_ptr, MUSIC_SHERO);
}

bool is_invuln(PlayerType *player_ptr)
{
    return player_ptr->invuln || music_singing(player_ptr, MUSIC_INVULN);
}

bool is_hero(PlayerType *player_ptr)
{
    return player_ptr->hero || music_singing(player_ptr, MUSIC_HERO) || music_singing(player_ptr, MUSIC_SHERO);
}

bool is_shero(PlayerType *player_ptr)
{
    return player_ptr->shero || PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER);
}

bool is_echizen(PlayerType *player_ptr)
{
    return (player_ptr->ppersonality == PERSONALITY_COMBAT) || (player_ptr->inventory_list[INVEN_BOW].is_specific_artifact(FixedArtifactId::CRIMSON));
}

bool is_chargeman(PlayerType *player_ptr)
{
    return player_ptr->ppersonality == PERSONALITY_CHARGEMAN;
}

WEIGHT calc_weapon_weight_limit(PlayerType *player_ptr)
{
    WEIGHT weight = adj_str_hold[player_ptr->stat_index[A_STR]];

    if (has_two_handed_weapons(player_ptr)) {
        weight *= 2;
    }

    return weight;
}

WEIGHT calc_bow_weight_limit(PlayerType *player_ptr)
{
    WEIGHT weight = adj_str_hold[player_ptr->stat_index[A_STR]];

    return weight;
}

static player_hand main_attack_hand(PlayerType *player_ptr)
{
    switch (player_melee_type(player_ptr)) {
    case MELEE_TYPE_BAREHAND_TWO:
        return PLAYER_HAND_MAIN;
    case MELEE_TYPE_BAREHAND_MAIN:
        return PLAYER_HAND_MAIN;
    case MELEE_TYPE_BAREHAND_SUB:
        return PLAYER_HAND_SUB;
    case MELEE_TYPE_WEAPON_MAIN:
        return PLAYER_HAND_MAIN;
    case MELEE_TYPE_WEAPON_SUB:
        return PLAYER_HAND_SUB;
    case MELEE_TYPE_WEAPON_TWOHAND:
        return has_melee_weapon(player_ptr, INVEN_MAIN_HAND) ? PLAYER_HAND_MAIN : PLAYER_HAND_SUB;
    case MELEE_TYPE_WEAPON_DOUBLE:
        return PLAYER_HAND_MAIN;
    case MELEE_TYPE_SHIELD_DOUBLE:
        return PLAYER_HAND_MAIN;
    }
    return PLAYER_HAND_MAIN;
}

bool set_quick_and_tiny(PlayerType *player_ptr)
{
    auto is_quickly_tiny = player_ptr->inventory_list[INVEN_MAIN_HAND].is_specific_artifact(FixedArtifactId::QUICKTHORN);
    is_quickly_tiny &= player_ptr->inventory_list[INVEN_SUB_HAND].is_specific_artifact(FixedArtifactId::TINYTHORN);
    return is_quickly_tiny;
}

bool set_musasi(PlayerType *player_ptr)
{
    auto is_musasi = player_ptr->inventory_list[INVEN_MAIN_HAND].is_specific_artifact(FixedArtifactId::MUSASI_KATANA);
    is_musasi &= player_ptr->inventory_list[INVEN_SUB_HAND].is_specific_artifact(FixedArtifactId::MUSASI_WAKIZASI);
    return is_musasi;
}

bool set_icing_and_twinkle(PlayerType *player_ptr)
{
    auto can_call_ice_wind_saga = player_ptr->inventory_list[INVEN_MAIN_HAND].is_specific_artifact(FixedArtifactId::ICINGDEATH);
    can_call_ice_wind_saga &= player_ptr->inventory_list[INVEN_SUB_HAND].is_specific_artifact(FixedArtifactId::TWINKLE);
    return can_call_ice_wind_saga;
}

bool set_anubis_and_chariot(PlayerType *player_ptr)
{
    auto is_anubis_chariot = player_ptr->inventory_list[INVEN_MAIN_HAND].is_specific_artifact(FixedArtifactId::ANUBIS);
    is_anubis_chariot &= player_ptr->inventory_list[INVEN_SUB_HAND].is_specific_artifact(FixedArtifactId::SILVER_CHARIOT);
    return is_anubis_chariot;
}
