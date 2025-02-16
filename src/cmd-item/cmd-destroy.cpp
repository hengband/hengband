#include "cmd-item/cmd-destroy.h"
#include "autopick/autopick-registry.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/input-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-hook/hook-expendable.h"
#include "object-hook/hook-magic.h"
#include "object/item-use-flags.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-realm.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "status/action-setter.h"
#include "status/experience.h"
#include "system/baseitem/baseitem-key.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include <tuple>

/// 破壊するアイテムの選択結果: アイテム、インベントリもしくは床上アイテムのインデックス、個数
using SelectionResult = std::tuple<ItemEntity *, short, int>;

static bool check_destory_item(PlayerType *player_ptr, const ItemEntity &destroying_item, short i_idx)
{
    if (!confirm_destroy && (destroying_item.calc_price() <= 0)) {
        return true;
    }

    const auto item_name = describe_flavor(player_ptr, destroying_item, OD_OMIT_PREFIX);
    constexpr auto fmt = _("本当に%sを壊しますか? [y/n/Auto]", "Really destroy %s? [y/n/Auto]");
    const auto msg = format(fmt, item_name.data());
    msg_print(nullptr);
    message_add(msg);
    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MESSAGE);
    window_stuff(player_ptr);
    while (true) {
        prt(msg, 0, 0);
        char i = inkey();
        prt("", 0, 0);
        if (i == 'y' || i == 'Y') {
            return true;
        }

        if (i == ESCAPE || i == 'n' || i == 'N') {
            return false;
        }

        if (i != 'A') {
            continue;
        }

        if (autopick_autoregister(player_ptr, &destroying_item)) {
            autopick_alter_item(player_ptr, i_idx, true);
        }

        return false;
    }
}

static std::optional<SelectionResult> select_destroying_item(PlayerType *player_ptr, bool force_destroy)
{
    short i_idx;
    constexpr auto q = _("どのアイテムを壊しますか? ", "Destroy which item? ");
    constexpr auto s = _("壊せるアイテムを持っていない。", "You have nothing to destroy.");
    auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, USE_INVEN | USE_FLOOR);
    if (o_ptr == nullptr) {
        return std::nullopt;
    }

    if (!force_destroy && !check_destory_item(player_ptr, *o_ptr, i_idx)) {
        return std::nullopt;
    }

    if (o_ptr->number <= 1) {
        return std::make_optional<SelectionResult>(o_ptr, i_idx, 1);
    }

    const auto amt = input_quantity(o_ptr->number);
    if (amt <= 0) {
        return std::nullopt;
    }

    return std::make_optional<SelectionResult>(o_ptr, i_idx, amt);
}

/*!
 * @brief 一部職業で高位魔法書の破壊による経験値上昇の判定
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param destoryed_item 破壊したアイテム
 * return 魔法書の破壊によって経験値が入るならばTRUE
 */
static bool decide_magic_book_exp(PlayerType *player_ptr, const ItemEntity &destroyed_item)
{
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return false;
    }

    PlayerClass pc(player_ptr);
    const auto tval = destroyed_item.bi_key.tval();
    if (pc.equals(PlayerClassType::WARRIOR) || pc.equals(PlayerClassType::BERSERKER)) {
        return tval != ItemKindType::HISSATSU_BOOK;
    }

    if (!pc.equals(PlayerClassType::PALADIN)) {
        return false;
    }

    auto is_good_magic_realm = (tval == ItemKindType::LIFE_BOOK) || (tval == ItemKindType::CRUSADE_BOOK);
    if (PlayerRealm(player_ptr).realm1().is_good_attribute()) {
        return !is_good_magic_realm;
    } else {
        return is_good_magic_realm;
    }
}

static void gain_exp_by_destroying_magic_book(PlayerType *player_ptr, const ItemEntity &destroyed_item)
{
    const auto gain_expr = decide_magic_book_exp(player_ptr, destroyed_item);
    if (!gain_expr || (player_ptr->exp >= PY_MAX_EXP)) {
        return;
    }

    auto tester_exp = player_ptr->max_exp / 20;
    if (tester_exp > 10000) {
        tester_exp = 10000;
    }

    if (destroyed_item.bi_key.sval() < 3) {
        tester_exp /= 4;
    }

    if (tester_exp < 1) {
        tester_exp = 1;
    }

    msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
    gain_exp(player_ptr, tester_exp * destroyed_item.number);
}

static void process_destroy_magic_book(PlayerType *player_ptr, const ItemEntity &destroyed_item)
{
    const BaseitemKey &bi_key = destroyed_item.bi_key;
    if (!bi_key.is_high_level_book()) {
        return;
    }

    const auto tval = bi_key.tval();
    gain_exp_by_destroying_magic_book(player_ptr, destroyed_item);
    if (tval == ItemKindType::LIFE_BOOK) {
        chg_virtue(player_ptr, Virtue::UNLIFE, 1);
        chg_virtue(player_ptr, Virtue::VITALITY, -1);
    } else if (tval == ItemKindType::DEATH_BOOK) {
        chg_virtue(player_ptr, Virtue::UNLIFE, -1);
        chg_virtue(player_ptr, Virtue::VITALITY, 1);
    }

    if ((destroyed_item.to_a != 0) || (destroyed_item.to_h != 0) || (destroyed_item.to_d != 0)) {
        chg_virtue(player_ptr, Virtue::ENCHANT, -1);
    }

    if (object_value_real(&destroyed_item) > 30000) {
        chg_virtue(player_ptr, Virtue::SACRIFICE, 2);
    } else if (object_value_real(&destroyed_item) > 10000) {
        chg_virtue(player_ptr, Virtue::SACRIFICE, 1);
    }
}

static void exe_destroy_item(PlayerType *player_ptr, ItemEntity &destroying_item, short i_idx, int amount)
{
    auto destroyed_item = destroying_item.clone();
    destroyed_item.number = amount;
    const auto item_name = describe_flavor(player_ptr, destroyed_item, 0);
    msg_format(_("%sを壊した。", "You destroy %s."), item_name.data());
    sound(SoundKind::DESTITEM);
    reduce_charges(&destroying_item, amount);
    vary_item(player_ptr, i_idx, -amount);
    process_destroy_magic_book(player_ptr, destroyed_item);
    if ((destroyed_item.to_a != 0) || (destroyed_item.to_d != 0) || (destroyed_item.to_h != 0)) {
        chg_virtue(player_ptr, Virtue::HARMONY, 1);
    }

    if (i_idx >= INVEN_MAIN_HAND) {
        calc_android_exp(player_ptr);
    }
}

/*!
 * @brief アイテムを破壊するコマンドのメインルーチン / Destroy an item
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_destroy(PlayerType *player_ptr)
{
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    const auto selection_result = select_destroying_item(player_ptr, (command_arg > 0));
    if (!selection_result) {
        return;
    }

    const auto &[o_ptr, i_idx, amt] = *selection_result;
    PlayerEnergy energy(player_ptr);
    energy.set_player_turn_energy(100);
    if (!can_player_destroy_object(o_ptr)) {
        energy.reset_player_turn();
        const auto item_name = describe_flavor(player_ptr, *o_ptr, 0);
        msg_format(_("%sは破壊不可能だ。", "You cannot destroy %s."), item_name.data());
        return;
    }

    exe_destroy_item(player_ptr, *o_ptr, i_idx, amt);
}
