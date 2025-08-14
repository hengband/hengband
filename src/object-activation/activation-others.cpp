/*!
 * @brief 発動処理その他 (肥大化しがちなので適宜まとまりを別ファイルへ分割すること)
 * @date 2020/08/19
 * @author Hourier
 */

#include "object-activation/activation-others.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/special-options.h"
#include "hpmp/hp-mp-processor.h"
#include "mind/mind-archer.h"
#include "monster/monster-status.h"
#include "player-attack/player-attack.h"
#include "player/player-damage.h"
#include "player/player-status.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

bool activate_sunlight(PlayerType *player_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    msg_print(_("太陽光線が放たれた。", "A line of sunlight appears."));
    (void)lite_line(player_ptr, dir, Dice::roll(6, 8));
    return true;
}

bool activate_confusion(PlayerType *player_ptr)
{
    msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    confuse_monster(player_ptr, dir, 20);
    return true;
}

bool activate_banish_evil(PlayerType *player_ptr)
{
    if (banish_evil(player_ptr, 100)) {
        msg_print(_("アーティファクトの力が邪悪を打ち払った！", "The power of the artifact banishes evil!"));
    }

    return true;
}

bool activate_scare(PlayerType *player_ptr)
{
    if (music_singing_any(player_ptr)) {
        stop_singing(player_ptr);
    }

    if (SpellHex(player_ptr).is_spelling_any()) {
        (void)SpellHex(player_ptr).stop_all_spells();
    }

    msg_print(_("あなたは力強い突風を吹き鳴らした。周囲の敵が震え上っている!", "You wind a mighty blast; your enemies tremble!"));
    (void)turn_monsters(player_ptr, (3 * player_ptr->lev / 2) + 10);
    return true;
}

bool activate_aggravation(PlayerType *player_ptr, ItemEntity *o_ptr, std::string_view name)
{
    if (o_ptr->is_specific_artifact(FixedArtifactId::HYOUSIGI)) {
        msg_print(_("拍子木を打った。", "You beat your wooden clappers."));
    } else {
        msg_format(_("%sは不快な物音を立てた。", "The %s sounds an unpleasant noise."), name.data());
    }

    aggravate_monsters(player_ptr, 0);
    return true;
}

bool activate_stone_mud(PlayerType *player_ptr)
{
    msg_print(_("鼓動している...", "It pulsates..."));
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    wall_to_mud(player_ptr, dir, 20 + randint1(30));
    return true;
}

bool activate_judgement(PlayerType *player_ptr, std::string_view name)
{
    msg_format(_("%sは赤く明るく光った！", "The %s flashes bright red!"), name.data());
    chg_virtue(player_ptr, Virtue::KNOWLEDGE, 1);
    chg_virtue(player_ptr, Virtue::ENLIGHTEN, 1);
    wiz_lite(player_ptr, false);

    msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name.data());
    take_hit(player_ptr, DAMAGE_LOSELIFE, Dice::roll(3, 8), _("審判の宝石", "the Jewel of Judgement"));

    (void)detect_traps(player_ptr, DETECT_RAD_DEFAULT, true);
    (void)detect_doors(player_ptr, DETECT_RAD_DEFAULT);
    (void)detect_stairs(player_ptr, DETECT_RAD_DEFAULT);

    if (input_check(_("帰還の力を使いますか？", "Activate recall? "))) {
        (void)recall_player(player_ptr, randint0(21) + 15);
    }

    return true;
}

bool activate_telekinesis(PlayerType *player_ptr, std::string_view name)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    msg_format(_("%sを伸ばした。", "You stretched your %s."), name.data());
    fetch_item(player_ptr, dir, 500, true);
    return true;
}

bool activate_unique_detection(PlayerType *player_ptr)
{
    msg_print(_("奇妙な場所が頭の中に浮かんだ．．．", "Some strange places show up in your mind. And you see ..."));
    for (int i = player_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
        const auto &monster = player_ptr->current_floor_ptr->m_list[i];
        if (!monster.is_valid()) {
            continue;
        }

        const auto &monrace = monster.get_monrace();
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            msg_format(_("%s． ", "%s. "), monrace.name.data());
        }

        const auto message = monrace.get_message(monrace.name, MonsterMessageType::MESSAGE_DETECT_UNIQUE);
        if (message) {
            msg_print(*message);
        }
    }

    return true;
}

bool activate_dispel_curse(PlayerType *player_ptr, std::string_view name)
{
    msg_format(_("%sが真実を照らし出す...", "The %s exhibits the truth..."), name.data());
    (void)remove_all_curse(player_ptr);
    (void)probing(player_ptr);
    return true;
}

bool activate_cure_lw(PlayerType *player_ptr)
{
    (void)BadStatusSetter(player_ptr).set_fear(0);
    (void)hp_player(player_ptr, 30);
    return true;
}

bool activate_grand_cross(PlayerType *player_ptr)
{
    msg_print(_("「闇に還れ！」", "You say, 'Return to darkness!'"));
    constexpr auto flags = PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID;
    (void)project(player_ptr, 0, 8, player_ptr->y, player_ptr->x, (randint1(100) + 200) * 2, AttributeType::HOLY_FIRE, flags);
    return true;
}

bool activate_call_chaos(PlayerType *player_ptr)
{
    msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
    call_chaos(player_ptr);
    return true;
}

bool activate_dispel_evil(PlayerType *player_ptr)
{
    msg_print(_("神聖な雰囲気が充満した...", "It floods the area with goodness..."));
    dispel_evil(player_ptr, player_ptr->lev * 5);
    return true;
}

bool activate_dispel_good(PlayerType *player_ptr)
{
    msg_print(_("邪悪な雰囲気が充満した...", "It floods the area with evil..."));
    dispel_good(player_ptr, player_ptr->lev * 5);
    return true;
}

bool activate_all_monsters_detection(PlayerType *player_ptr)
{
    (void)detect_monsters_invis(player_ptr, 255);
    (void)detect_monsters_normal(player_ptr, 255);
    return true;
}

bool activate_all_detection(PlayerType *player_ptr)
{
    msg_print(_("白く明るく輝いている...", "It glows bright white..."));
    msg_print(_("心にイメージが浮かんできた...", "An image forms in your mind..."));
    detect_all(player_ptr, DETECT_RAD_DEFAULT);
    return true;
}

bool activate_extra_detection(PlayerType *player_ptr)
{
    msg_print(_("明るく輝いている...", "It glows brightly..."));
    detect_all(player_ptr, DETECT_RAD_DEFAULT);
    probing(player_ptr);
    identify_fully(player_ptr, false);
    return true;
}

bool activate_fully_identification(PlayerType *player_ptr)
{
    msg_print(_("黄色く輝いている...", "It glows yellow..."));
    identify_fully(player_ptr, false);
    return true;
}

/*!
 * @brief switch_activation() から個々のスペルへの依存性をなくすためのシンタックスシュガー
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 発動に成功したらTRUE
 */
bool activate_identification(PlayerType *player_ptr)
{
    return ident_spell(player_ptr, false);
}

bool activate_pesticide(PlayerType *player_ptr)
{
    msg_print(_("あなたは害虫を一掃した。", "You exterminate some pests."));
    (void)dispel_monsters(player_ptr, 4);
    return true;
}

/*!
 * @brief switch_activation() から個々のスペルへの依存性をなくすためのシンタックスシュガー
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 発動に成功したらTRUE
 */
bool activate_whirlwind(PlayerType *player_ptr)
{
    massacre(player_ptr);
    return true;
}

bool activate_blinding_light(PlayerType *player_ptr, std::string_view name)
{
    msg_format(_("%sが眩しい光で輝いた...", "The %s gleams with blinding light..."), name.data());
    (void)fire_ball(player_ptr, AttributeType::LITE, Direction::self(), 300, 6);
    confuse_monsters(player_ptr, 3 * player_ptr->lev / 2);
    return true;
}

bool activate_sleep(PlayerType *player_ptr)
{
    msg_print(_("深青色に輝いている...", "It glows deep blue..."));
    sleep_monsters_touch(player_ptr);
    return true;
}

bool activate_door_destroy(PlayerType *player_ptr)
{
    msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
    destroy_doors_touch(player_ptr);
    return true;
}

bool activate_earthquake(PlayerType *player_ptr)
{
    earthquake(player_ptr, player_ptr->get_position(), 5);
    return true;
}

bool activate_recharge(PlayerType *player_ptr)
{
    recharge(player_ptr, 130);
    return true;
}

bool activate_recharge_extra(PlayerType *player_ptr, std::string_view name)
{
    msg_format(_("%sが白く輝いた．．．", "The %s gleams with blinding light..."), name.data());
    return recharge(player_ptr, 1000);
}

bool activate_shikofumi(PlayerType *player_ptr)
{
    msg_print(_("力強く四股を踏んだ。", "You stamp. (as if you are in a ring.)"));
    (void)BadStatusSetter(player_ptr).set_fear(0);
    (void)set_hero(player_ptr, randint1(20) + 20, false);
    (void)dispel_evil(player_ptr, player_ptr->lev * 3);
    return true;
}

bool activate_terror(PlayerType *player_ptr)
{
    turn_monsters(player_ptr, 40 + player_ptr->lev);
    return true;
}

bool activate_map_light(PlayerType *player_ptr)
{
    msg_print(_("眩しく輝いた...", "It shines brightly..."));
    map_area(player_ptr, DETECT_RAD_MAP);
    lite_area(player_ptr, Dice::roll(2, 15), 3);
    return true;
}

bool activate_exploding_rune(PlayerType *player_ptr)
{
    msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
    create_rune_explosion(player_ptr, player_ptr->y, player_ptr->x);
    return true;
}

bool activate_protection_rune(PlayerType *player_ptr)
{
    msg_print(_("ブルーに明るく輝いている...", "It glows light blue..."));
    create_rune_protection_one(player_ptr);
    return true;
}

bool activate_protection_elbereth(PlayerType *player_ptr)
{
    BadStatusSetter bss(player_ptr);
    msg_print(_("エルベレスよ、我を護り給え！", "A Elbereth gilthoniel!"));
    create_rune_protection_one(player_ptr);
    (void)bss.set_fear(0);
    (void)bss.set_blindness(0);
    (void)bss.hallucination(0);
    set_blessed(player_ptr, randint0(25) + 25, true);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ABILITY_SCORE);
    return true;
}

bool activate_light(PlayerType *player_ptr, std::string_view name)
{
    msg_format(_("%sから澄んだ光があふれ出た...", "The %s wells with clear light..."), name.data());
    (void)lite_area(player_ptr, Dice::roll(2, 15), 3);
    return true;
}

bool activate_recall(PlayerType *player_ptr)
{
    msg_print(_("やわらかな白色に輝いている...", "It glows soft white..."));
    return recall_player(player_ptr, randint0(21) + 15);
}

bool activate_tree_creation(PlayerType *player_ptr, ItemEntity *o_ptr, std::string_view name)
{
    const auto randart_name = o_ptr->is_random_artifact() ? o_ptr->randart_name->data() : "";
    msg_format(_("%s%sから明るい緑の光があふれ出た...", "The %s%s wells with clear light..."), name.data(), randart_name);
    return tree_creation(player_ptr, player_ptr->y, player_ptr->x);
}

bool activate_animate_dead(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    msg_print(_("黄金色の光が溢れ出た...", "It emitted a golden light..."));
    if (o_ptr->is_specific_artifact(FixedArtifactId::EXCALIBORG)) {
        msg_print(_("ぴぴるぴるぴるぴぴるぴ～♪", "Pipiru piru piru pipiru pii"));
    }

    return animate_dead(player_ptr, 0, player_ptr->y, player_ptr->x);
}

bool activate_detect_treasure(PlayerType *player_ptr)
{
    msg_print(_("金と銀に彩られている...", "It shines with gold and silver..."));
    return detect_treasure(player_ptr, DETECT_RAD_DEFAULT);
}

bool activate_create_ammo(PlayerType *player_ptr)
{
    msg_print(_("ダイアナの祝福を感じた...", "You feel Diana's breath..."));
    return create_ammo(player_ptr);
}

bool activate_dispel_magic(PlayerType *player_ptr)
{
    msg_print(_("鈍い色に光った...", "It glowed in a dull color..."));
    const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
    if (!pos) {
        return false;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto m_idx = floor.get_grid(*pos).m_idx;
    if (m_idx == 0) {
        return true;
    }

    const auto p_pos = player_ptr->get_position();
    if (!floor.has_los_at(*pos) || !projectable(floor, p_pos, *pos)) {
        return true;
    }

    dispel_monster_status(player_ptr, m_idx);
    return true;
}

bool activate_whistle(PlayerType *player_ptr, const ItemEntity &item)
{
    if (item.bi_key.tval() != ItemKindType::WHISTLE) {
        return false;
    }

    if (music_singing_any(player_ptr)) {
        stop_singing(player_ptr);
    }

    if (SpellHex(player_ptr).is_spelling_any()) {
        (void)SpellHex(player_ptr).stop_all_spells();
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    std::vector<short> pet_index;
    for (short pet_indice = floor.m_max - 1; pet_indice >= 1; pet_indice--) {
        const auto &monster = floor.m_list[pet_indice];
        if (monster.is_pet() && !monster.is_riding()) {
            pet_index.push_back(pet_indice);
        }
    }

    std::stable_sort(pet_index.begin(), pet_index.end(), [&floor](auto x, auto y) { return floor.order_pet_whistle(x, y); });
    for (auto pet_indice : pet_index) {
        teleport_monster_to(player_ptr, pet_indice, player_ptr->y, player_ptr->x, 100, TELEPORT_PASSIVE);
    }

    return true;
}
