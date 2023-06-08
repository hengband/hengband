#include "wizard/wizard-item-modifier.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "artifact/random-art-generator.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/cheat-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-kind-hook.h"
#include "object/object-mark-types.h"
#include "object/object-value.h"
#include "spell-kind/spells-perception.h"
#include "spell/spells-object.h"
#include "system/alloc-entries.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/system-variables.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "wizard/wizard-special-process.h"
#include "world/world.h"
#include <algorithm>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

constexpr auto BASEITEM_MAX_DEPTH = 110; /*!< アイテムの階層毎生成率を表示する最大階 */

namespace {
/*!
 * @brief アイテム設定コマンド一覧表
 */
constexpr std::array wizard_sub_menu_table = {
    std::make_tuple('a', _("アーティファクト出現フラグリセット", "Restore aware flag of fixed artifact")),
    std::make_tuple('A', _("アーティファクトを出現済みにする", "Make a fixed artifact awared")),
    std::make_tuple('B', _("フロア相当の呪物ドロップ", "Drop cursed item")),
    std::make_tuple('c', _("フロア相当の一般品獲得ドロップ (★不許可)", "Drop normal item (excluding fixed artifacts)")),
    std::make_tuple('C', _("フロア相当の一般品獲得ドロップ (★許可)", "Drop normal item (including fixed artifacts)")),
    std::make_tuple('d', _("フロア相当の上質品獲得ドロップ", "Drop good item")),
    std::make_tuple('D', _("フロア相当の高級品獲得ドロップ", "Drop excellent item")),
    std::make_tuple('e', _("フロア相当のエゴ品獲得ドロップ", "Drop excellent item")),
    std::make_tuple('E', _("フロア相当の特別品獲得ドロップ", "Drop special item")),
    std::make_tuple('f', _("*鑑定*", "*Idenfity*")),
    std::make_tuple('i', _("鑑定", "Idenfity")),
    std::make_tuple('I', _("インベントリ全*鑑定*", "Idenfity all items fully in inventory")),
    std::make_tuple('l', _("指定アイテム番号まで一括鑑定", "Make items awared to target item id")),
    std::make_tuple('U', _("発動を変更する", "Modify item activation")),
    std::make_tuple('w', _("願い", "Wishing")),
};

/*!
 * @brief ゲーム設定コマンドの一覧を表示する
 */
void display_wizard_sub_menu()
{
    for (auto y = 1U; y <= wizard_sub_menu_table.size(); y++) {
        term_erase(14, y, 64);
    }

    int r = 1;
    int c = 15;
    for (const auto &[symbol, desc] : wizard_sub_menu_table) {
        std::stringstream ss;
        ss << symbol << ") " << desc;
        put_str(ss.str(), r++, c);
    }
}
}

/*!
 * @brief キャスト先の型の最小値、最大値でclampする。
 */
template <typename T>
T clamp_cast(int val)
{
    return static_cast<T>(std::clamp(val,
        static_cast<int>(std::numeric_limits<T>::min()),
        static_cast<int>(std::numeric_limits<T>::max())));
}

void wiz_restore_aware_flag_of_fixed_arfifact(FixedArtifactId reset_artifact_idx, bool aware = false);
void wiz_modify_item_activation(PlayerType *player_ptr);
void wiz_identify_full_inventory(PlayerType *player_ptr);

static void wiz_item_drop(PlayerType *player_ptr, const int num_items, const EnumClassFlagGroup<ItemMagicAppliance> &appliance)
{
    uint mode = AM_NONE;
    const auto is_cursed = appliance.has(ItemMagicAppliance::CURSED);
    if (is_cursed) {
        mode |= AM_CURSED;
    }

    if (appliance.has(ItemMagicAppliance::GOOD)) {
        mode |= AM_GOOD;
    }

    if (appliance.has(ItemMagicAppliance::GREAT)) {
        mode |= AM_GREAT;
    }

    if (appliance.has(ItemMagicAppliance::SPECIAL)) {
        mode |= AM_SPECIAL;
    }

    for (auto i = 0; i < num_items; i++) {
        ItemEntity item;
        if (!make_object(player_ptr, &item, mode)) {
            continue;
        }

        if (is_cursed && !item.is_cursed()) {
            i--;
            continue;
        }

        if (appliance.has(ItemMagicAppliance::EGO) && !item.is_ego()) {
            i--;
            continue;
        }

        if (!drop_near(player_ptr, &item, -1, player_ptr->y, player_ptr->x)) {
            msg_print_wizard(player_ptr, 0, "No item dropping space!");
            return;
        }
    }
}

/*!
 * @brief ゲーム設定コマンドの入力を受け付ける
 * @param player_ptr プレイヤーの情報へのポインタ
 */
void wizard_item_modifier(PlayerType *player_ptr)
{
    screen_save();
    display_wizard_sub_menu();

    char cmd;
    get_com("Player Command: ", &cmd, false);
    screen_load();

    switch (cmd) {
    case ESCAPE:
    case ' ':
    case '\n':
    case '\r':
        break;
    case 'a':
        wiz_restore_aware_flag_of_fixed_arfifact(i2enum<FixedArtifactId>(command_arg));
        break;
    case 'A':
        wiz_restore_aware_flag_of_fixed_arfifact(i2enum<FixedArtifactId>(command_arg), true);
        break;
    case 'B':
        command_arg = std::clamp<short>(command_arg, 1, 999);
        wiz_item_drop(player_ptr, command_arg, { ItemMagicAppliance::CURSED });
        break;
    case 'c':
        command_arg = std::clamp<short>(command_arg, 1, 999);
        wiz_item_drop(player_ptr, command_arg, { ItemMagicAppliance::NO_FIXED_ART });
        break;
    case 'C':
        command_arg = std::clamp<short>(command_arg, 1, 999);
        wiz_item_drop(player_ptr, command_arg, {});
        break;
    case 'd':
        command_arg = std::clamp<short>(command_arg, 1, 999);
        wiz_item_drop(player_ptr, command_arg, { ItemMagicAppliance::GOOD });
        break;
    case 'D':
        command_arg = std::clamp<short>(command_arg, 1, 999);
        wiz_item_drop(player_ptr, command_arg, { ItemMagicAppliance::GOOD, ItemMagicAppliance::GREAT });
        break;
    case 'e':
        command_arg = std::clamp<short>(command_arg, 1, 999);
        wiz_item_drop(player_ptr, command_arg, { ItemMagicAppliance::GOOD, ItemMagicAppliance::GREAT, ItemMagicAppliance::EGO });
        break;
    case 'E':
        command_arg = std::clamp<short>(command_arg, 1, 999);
        wiz_item_drop(player_ptr, command_arg, { ItemMagicAppliance::GOOD, ItemMagicAppliance::GREAT, ItemMagicAppliance::SPECIAL });
        break;
    case 'f':
        identify_fully(player_ptr, false);
        break;
    case 'i':
        (void)ident_spell(player_ptr, false);
        break;
    case 'I':
        wiz_identify_full_inventory(player_ptr);
        break;
    case 'l':
        wiz_learn_items_all(player_ptr);
        break;
    case 'U':
        wiz_modify_item_activation(player_ptr);
        break;
    case 'w':
        do_cmd_wishing(player_ptr, -1, true, true, true);
        break;
    }
}

/*!
 * @brief 固定アーティファクトの出現フラグをリセットする
 * @param a_idx 指定したアーティファクトID
 * @details 外からはenum class を受け取るが、この関数内では数値の直指定処理なので数値型にキャストする.
 */
void wiz_restore_aware_flag_of_fixed_arfifact(FixedArtifactId reset_artifact_idx, bool aware)
{
    auto max_a_idx = enum2i(artifacts_info.rbegin()->first);
    int int_a_idx = enum2i(reset_artifact_idx);
    if (int_a_idx <= 0) {
        if (!get_value("Artifact ID", 1, max_a_idx, &int_a_idx)) {
            return;
        }
    }

    ArtifactsInfo::get_instance().get_artifact(i2enum<FixedArtifactId>(int_a_idx)).is_generated = aware;
    msg_print(aware ? "Modified." : "Restored.");
}

/*!
 * @brief オブジェクトに発動を追加する/変更する
 * @param catser_ptr プレイヤー情報への参照ポインタ
 */
void wiz_modify_item_activation(PlayerType *player_ptr)
{
    constexpr auto q = _("どのアイテムの発動を変更しますか？ ", "Which item? ");
    constexpr auto s = _("発動を変更するアイテムがない。", "Nothing to do with.");
    short item;
    auto *o_ptr = choose_object(player_ptr, &item, q, s, USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT);
    if (!o_ptr) {
        return;
    }

    int val;
    if (!get_value("Activation ID", enum2i(RandomArtActType::NONE), enum2i(RandomArtActType::MAX) - 1, &val)) {
        return;
    }

    auto act_idx = i2enum<RandomArtActType>(val);
    o_ptr->art_flags.set(TR_ACTIVATE);
    o_ptr->activation_id = act_idx;
}

/*!
 * @brief インベントリ内のアイテムを全て*鑑定*済みにする
 * @param catser_ptr プレイヤー情報への参照ポインタ
 */
void wiz_identify_full_inventory(PlayerType *player_ptr)
{
    for (int i = 0; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        auto &baseitem = o_ptr->get_baseitem();
        baseitem.aware = true; //!< @note 記録には残さないためTRUEを立てるのみ
        set_bits(o_ptr->ident, IDENT_KNOWN | IDENT_FULL_KNOWN);
        o_ptr->marked.set(OmType::TOUCHED);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
    };
    rfu.set_flags(flags_swrf);
}

/*!
 * @brief アイテムの階層毎生成率を表示する / Output a rarity graph for a type of object.
 * @param tval ベースアイテムの大項目ID
 * @param sval ベースアイテムの小項目ID
 * @param row 表示列
 * @param col 表示行
 */
static void prt_alloc(const BaseitemKey &bi_key, TERM_LEN row, TERM_LEN col)
{
    uint32_t rarity[BASEITEM_MAX_DEPTH]{};
    uint32_t total[BASEITEM_MAX_DEPTH]{};
    int32_t display[22]{};

    auto home = 0;
    for (auto i = 0; i < BASEITEM_MAX_DEPTH; i++) {
        auto total_frac = 0;
        constexpr auto magnificant = CHANCE_BASEITEM_LEVEL_BOOST * BASEITEM_MAX_DEPTH;
        for (const auto &entry : alloc_kind_table) {
            auto prob = 0;
            if (entry.level <= i) {
                prob = entry.prob1 * magnificant;
            } else if (entry.level - 1 > 0) {
                prob = entry.prob1 * i * BASEITEM_MAX_DEPTH / (entry.level - 1);
            }

            const auto &baseitem = entry.get_baseitem();
            total[i] += prob / magnificant;
            total_frac += prob % magnificant;

            if (baseitem.bi_key == bi_key) {
                home = baseitem.level;
                rarity[i] += prob / magnificant;
            }
        }

        total[i] += total_frac / magnificant;
    }

    for (auto i = 0; i < 22; i++) {
        auto possibility = 0;
        for (int j = i * BASEITEM_MAX_DEPTH / 22; j < (i + 1) * BASEITEM_MAX_DEPTH / 22; j++) {
            possibility += rarity[j] * 100000 / total[j];
        }

        display[i] = possibility / 5;
    }

    for (auto i = 0; i < 22; i++) {
        term_putch(col, row + i + 1, TERM_WHITE, '|');
        prt(format("%2dF", (i * 5)), row + i + 1, col);
        if ((i * BASEITEM_MAX_DEPTH / 22 <= home) && (home < (i + 1) * BASEITEM_MAX_DEPTH / 22)) {
            c_prt(TERM_RED, format("%3d.%04d%%", display[i] / 1000, display[i] % 1000), row + i + 1, col + 3);
        } else {
            c_prt(TERM_WHITE, format("%3d.%04d%%", display[i] / 1000, display[i] % 1000), row + i + 1, col + 3);
        }
    }

    concptr r = "+---Rate---+";
    prt(r, row, col);
}

/*!
 * @brief 32ビット変数のビット配列を並べて描画する / Output a long int in binary format.
 */
static void prt_binary(BIT_FLAGS flags, const int row, int col)
{
    uint32_t bitmask;
    for (int i = bitmask = 1; i <= 32; i++, bitmask *= 2) {
        if (flags & bitmask) {
            term_putch(col++, row, TERM_BLUE, '*');
        } else {
            term_putch(col++, row, TERM_WHITE, '-');
        }
    }
}

/*!
 * @brief アイテムの詳細ステータスを表示する /
 * Change various "permanent" player variables.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 詳細を表示するアイテム情報の参照ポインタ
 */
static void wiz_display_item(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    auto flags = object_flags(o_ptr);
    auto get_seq_32bits = [](const TrFlags &flags, uint start) {
        BIT_FLAGS result = 0U;
        for (auto i = 0U; i < 32 && start + i < flags.size(); i++) {
            if (flags.has(i2enum<tr_type>(start + i))) {
                result |= 1U << i;
            }
        }
        return result;
    };
    int j = 13;
    for (int i = 1; i <= 23; i++) {
        prt("", i, j - 2);
    }

    prt_alloc(o_ptr->bi_key, 1, 0);
    const auto item_name = describe_flavor(player_ptr, o_ptr, OD_STORE);
    prt(item_name, 2, j);

    auto line = 4;
    const auto &bi_key = o_ptr->bi_key;
    const auto item_level = o_ptr->get_baseitem().level;
    prt(format("kind = %-5d  level = %-4d  tval = %-5d  sval = %-5d", o_ptr->bi_id, item_level, enum2i(bi_key.tval()), bi_key.sval().value()), line, j);
    prt(format("number = %-3d  wgt = %-6d  ac = %-5d    damage = %dd%d", o_ptr->number, o_ptr->weight, o_ptr->ac, o_ptr->dd, o_ptr->ds), ++line, j);
    prt(format("pval = %-5d  toac = %-5d  tohit = %-4d  todam = %-4d", o_ptr->pval, o_ptr->to_a, o_ptr->to_h, o_ptr->to_d), ++line, j);
    prt(format("fixed_artifact_idx = %-4d  ego_idx = %-4d  cost = %d", enum2i(o_ptr->fixed_artifact_idx), enum2i(o_ptr->ego_idx), object_value_real(o_ptr)), ++line, j);
    prt(format("ident = %04x  activation_id = %-4d  timeout = %-d", o_ptr->ident, enum2i(o_ptr->activation_id), o_ptr->timeout), ++line, j);
    prt(format("chest_level = %-4d  fuel = %-d", o_ptr->chest_level, o_ptr->fuel), ++line, j);
    prt(format("smith_hit = %-4d  smith_damage = %-4d", o_ptr->smith_hit, o_ptr->smith_damage), ++line, j);
    prt(format("cursed  = %-4lX  captured_monster_speed = %-4d", o_ptr->curse_flags.to_ulong(), o_ptr->captured_monster_speed), ++line, j);
    prt(format("captured_monster_max_hp = %-4d  captured_monster_max_hp = %-4d", o_ptr->captured_monster_current_hp, o_ptr->captured_monster_max_hp), ++line, j);

    prt("+------------FLAGS1------------+", ++line, j);
    prt("AFFECT........SLAY........BRAND.", ++line, j);
    prt("      mf      cvae      xsqpaefc", ++line, j);
    prt("siwdccsossidsahanvudotgddhuoclio", ++line, j);
    prt("tnieohtctrnipttmiinmrrnrrraiierl", ++line, j);
    prt("rtsxnarelcfgdkcpmldncltggpksdced", ++line, j);
    prt_binary(get_seq_32bits(flags, 32 * 0), ++line, j);

    prt("+------------FLAGS2------------+", ++line, j);
    prt("SUST....IMMUN.RESIST............", ++line, j);
    prt("      reaefctrpsaefcpfldbc sn   ", ++line, j);
    prt("siwdcciaclioheatcliooeialoshtncd", ++line, j);
    prt("tnieohdsierlrfraierliatrnnnrhehi", ++line, j);
    prt("rtsxnaeydcedwlatdcedsrekdfddrxss", ++line, j);
    prt_binary(get_seq_32bits(flags, 32 * 1), ++line, j);

    line = 13;
    prt("+------------FLAGS3------------+", line, j + 32);
    prt("fe cnn t      stdrmsiiii d ab   ", ++line, j + 32);
    prt("aa aoomywhs lleeieihgggg rtgl   ", ++line, j + 32);
    prt("uu utmacaih eielgggonnnnaaere   ", ++line, j + 32);
    prt("rr reanurdo vtieeehtrrrrcilas   ", ++line, j + 32);
    prt("aa algarnew ienpsntsaefctnevs   ", ++line, j + 32);
    prt_binary(get_seq_32bits(flags, 32 * 2), ++line, j + 32);

    prt("+------------FLAGS4------------+", ++line, j + 32);
    prt("KILL....ESP.........            ", ++line, j + 32);
    prt("aeud tghaud tgdhegnu            ", ++line, j + 32);
    prt("nvneoriunneoriruvoon            ", ++line, j + 32);
    prt("iidmroamidmroagmionq            ", ++line, j + 32);
    prt("mlenclnmmenclnnnldlu            ", ++line, j + 32);
    prt_binary(get_seq_32bits(flags, 32 * 3), ++line, j + 32);
}

/*!
 * @brief 検査対象のアイテムを基準とした生成テストを行う /
 * Try to create an item again. Output some statistics.    -Bernd-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 生成テストの基準となるアイテム情報の参照ポインタ
 * The statistics are correct now.  We acquire a clean grid, and then
 * repeatedly place an object in this grid, copying it into an item
 * holder, and then deleting the object.  We fiddle with the artifact
 * counter flags to prevent weirdness.  We use the items to collect
 * statistics on item creation relative to the initial item.
 */
static void wiz_statistics(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    constexpr auto q = "Rolls: %d  Correct: %d  Matches: %d  Better: %d  Worse: %d  Other: %d";
    constexpr auto p = "Enter number of items to roll: ";
    constexpr auto pmt = "Roll for [n]ormal, [g]ood, or [e]xcellent treasure? ";
    if (o_ptr->is_fixed_artifact()) {
        o_ptr->get_fixed_artifact().is_generated = false;
    }

    uint32_t i, matches, better, worse, other, correct;
    uint32_t test_rolls = 1000000;
    char ch;
    BIT_FLAGS mode;
    std::optional<std::string> input_rolls;
    while (true) {
        wiz_display_item(player_ptr, o_ptr);
        if (!get_com(pmt, &ch, false)) {
            break;
        }

        std::string quality = "";
        if (ch == 'n' || ch == 'N') {
            mode = 0L;
            quality = "normal";
        } else if (ch == 'g' || ch == 'G') {
            mode = AM_GOOD;
            quality = "good";
        } else if (ch == 'e' || ch == 'E') {
            mode = AM_GOOD | AM_GREAT;
            quality = "excellent";
        } else {
            break;
        }

        input_rolls = get_string(p, 10, std::to_string(test_rolls));
        if (input_rolls.has_value()) {
            test_rolls = std::stoi(input_rolls.value());
        }

        test_rolls = std::max<uint>(1, test_rolls);
        msg_format("Creating a lot of %s items. Base level = %d.", quality.data(), player_ptr->current_floor_ptr->dun_level);
        msg_print(nullptr);

        correct = matches = better = worse = other = 0;
        for (i = 0; i <= test_rolls; i++) {
            if ((i < 100) || (i % 100 == 0)) {
                inkey_scan = true;
                if (inkey()) {
                    flush();
                    break; // stop rolling
                }

                prt(format(q, i, correct, matches, better, worse, other), 0, 0);
                term_fresh();
            }

            ItemEntity forge;
            auto *q_ptr = &forge;
            q_ptr->wipe();
            make_object(player_ptr, q_ptr, mode);
            if (q_ptr->is_fixed_artifact()) {
                q_ptr->get_fixed_artifact().is_generated = false;
            }

            if (o_ptr->bi_key != q_ptr->bi_key) {
                continue;
            }

            correct++;
            const auto is_same_fixed_artifact_idx = o_ptr->is_specific_artifact(q_ptr->fixed_artifact_idx);
            if ((q_ptr->pval == o_ptr->pval) && (q_ptr->to_a == o_ptr->to_a) && (q_ptr->to_h == o_ptr->to_h) && (q_ptr->to_d == o_ptr->to_d) && is_same_fixed_artifact_idx) {
                matches++;
            } else if ((q_ptr->pval >= o_ptr->pval) && (q_ptr->to_a >= o_ptr->to_a) && (q_ptr->to_h >= o_ptr->to_h) && (q_ptr->to_d >= o_ptr->to_d)) {
                better++;
            } else if ((q_ptr->pval <= o_ptr->pval) && (q_ptr->to_a <= o_ptr->to_a) && (q_ptr->to_h <= o_ptr->to_h) && (q_ptr->to_d <= o_ptr->to_d)) {
                worse++;
            } else {
                other++;
            }
        }

        msg_format(q, i, correct, matches, better, worse, other);
        msg_print(nullptr);
    }

    if (o_ptr->is_fixed_artifact()) {
        o_ptr->get_fixed_artifact().is_generated = true;
    }
}

/*!
 * @brief アイテムの質を選択して再生成する /
 * Apply magic to an item or turn it into an artifact. -Bernd-
 * @param o_ptr 再生成の対象となるアイテム情報の参照ポインタ
 */
static void wiz_reroll_item(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    if (o_ptr->is_fixed_or_random_artifact()) {
        return;
    }

    ItemEntity forge;
    ItemEntity *q_ptr;
    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);

    char ch;
    bool changed = false;
    while (true) {
        wiz_display_item(player_ptr, q_ptr);
        if (!get_com("[a]ccept, [w]orthless, [c]ursed, [n]ormal, [g]ood, [e]xcellent, [s]pecial? ", &ch, false)) {
            if (q_ptr->is_fixed_artifact()) {
                q_ptr->get_fixed_artifact().is_generated = false;
                q_ptr->fixed_artifact_idx = FixedArtifactId::NONE;
            }

            changed = false;
            break;
        }

        if (ch == 'A' || ch == 'a') {
            changed = true;
            break;
        }

        if (q_ptr->is_fixed_artifact()) {
            q_ptr->get_fixed_artifact().is_generated = false;
            q_ptr->fixed_artifact_idx = FixedArtifactId::NONE;
        }

        switch (tolower(ch)) {
        /* Apply bad magic, but first clear object */
        case 'w':
            q_ptr->prep(o_ptr->bi_id);
            ItemMagicApplier(player_ptr, q_ptr, player_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT | AM_CURSED).execute();
            break;
        /* Apply bad magic, but first clear object */
        case 'c':
            q_ptr->prep(o_ptr->bi_id);
            ItemMagicApplier(player_ptr, q_ptr, player_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_CURSED).execute();
            break;
        /* Apply normal magic, but first clear object */
        case 'n':
            q_ptr->prep(o_ptr->bi_id);
            ItemMagicApplier(player_ptr, q_ptr, player_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART).execute();
            break;
        /* Apply good magic, but first clear object */
        case 'g':
            q_ptr->prep(o_ptr->bi_id);
            ItemMagicApplier(player_ptr, q_ptr, player_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD).execute();
            break;
        /* Apply great magic, but first clear object */
        case 'e':
            q_ptr->prep(o_ptr->bi_id);
            ItemMagicApplier(player_ptr, q_ptr, player_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT).execute();
            break;
        /* Apply special magic, but first clear object */
        case 's':
            q_ptr->prep(o_ptr->bi_id);
            ItemMagicApplier(player_ptr, q_ptr, player_ptr->current_floor_ptr->dun_level, AM_GOOD | AM_GREAT | AM_SPECIAL).execute();
            if (!q_ptr->is_fixed_or_random_artifact()) {
                become_random_artifact(player_ptr, q_ptr, false);
            }

            break;
        default:
            break;
        }

        q_ptr->iy = o_ptr->iy;
        q_ptr->ix = o_ptr->ix;
        q_ptr->marked = o_ptr->marked;
    }

    if (!changed) {
        return;
    }

    o_ptr->copy_from(q_ptr);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::SPELL,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);
}

/*!
 * @briefアイテムの基礎能力値を調整する / Tweak an item
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 調整するアイテムの参照ポインタ
 */
static void wiz_tweak_item(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    if (o_ptr->is_fixed_or_random_artifact()) {
        return;
    }

    constexpr auto p = "Enter new 'pval' setting: ";
    const auto initial_pval = format("%d", o_ptr->pval);
    const auto pval_opt = get_string(p, 5, initial_pval.data());
    if (!pval_opt.has_value()) {
        return;
    }

    const auto &pval = pval_opt.value();
    o_ptr->pval = clamp_cast<int16_t>(std::stoi(pval));
    wiz_display_item(player_ptr, o_ptr);

    constexpr auto a = "Enter new 'to_a' setting: ";
    const auto initial_ac = format("%d", o_ptr->to_a);
    const auto ac_opt = get_string(a, 5, initial_ac);
    if (!ac_opt.has_value()) {
        return;
    }

    const auto &ac = ac_opt.value();
    o_ptr->to_a = clamp_cast<int16_t>(std::stoi(ac));
    wiz_display_item(player_ptr, o_ptr);

    constexpr auto h = "Enter new 'to_h' setting: ";
    const auto initial_hit = format("%d", o_ptr->to_h);
    const auto hit_opt = get_string(h, 5, initial_hit);
    if (!hit_opt.has_value()) {
        return;
    }

    const auto &hit = hit_opt.value();
    o_ptr->to_h = clamp_cast<int16_t>(std::stoi(hit));
    wiz_display_item(player_ptr, o_ptr);

    constexpr auto d = "Enter new 'to_d' setting: ";
    const auto initial_damage = format("%d", (int)o_ptr->to_d);
    const auto damage_opt = get_string(d, 5, initial_damage);
    if (!damage_opt.has_value()) {
        return;
    }

    const auto &damage = damage_opt.value();
    o_ptr->to_d = clamp_cast<int16_t>(std::stoi(damage));
    wiz_display_item(player_ptr, o_ptr);
}

/*!
 * @brief 検査対象のアイテムの数を変更する /
 * Change the quantity of a the item
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 変更するアイテム情報構造体の参照ポインタ
 */
static void wiz_quantity_item(ItemEntity *o_ptr)
{
    if (o_ptr->is_fixed_or_random_artifact()) {
        return;
    }

    int tmp_qnt = o_ptr->number;
    const auto initial_num = format("%d", (int)o_ptr->number);
    const auto num_opt = get_string("Quantity: ", 2, initial_num);
    if (num_opt.has_value()) {
        auto tmp_int = std::clamp(std::stoi(num_opt.value()), 1, 99);
        o_ptr->number = static_cast<uint8_t>(tmp_int);
    }

    if (o_ptr->bi_key.tval() == ItemKindType::ROD) {
        o_ptr->pval = o_ptr->pval * o_ptr->number / tmp_qnt;
    }
}

/*!
 * @brief アイテムを弄るデバッグコマンド
 * Play with an item. Options include:
 * @details
 *   - Output statistics (via wiz_roll_item)<br>
 *   - Reroll item (via wiz_reroll_item)<br>
 *   - Change properties (via wiz_tweak_item)<br>
 *   - Change the number of items (via wiz_quantity_item)<br>
 */
void wiz_modify_item(PlayerType *player_ptr)
{
    constexpr auto q = "Play with which object? ";
    constexpr auto s = "You have nothing to play with.";
    short item;
    auto *o_ptr = choose_object(player_ptr, &item, q, s, USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT);
    if (!o_ptr) {
        return;
    }

    screen_save();

    ItemEntity forge;
    ItemEntity *q_ptr;
    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    char ch;
    bool changed = false;
    while (true) {
        wiz_display_item(player_ptr, q_ptr);
        if (!get_com("[a]ccept [s]tatistics [r]eroll [t]weak [q]uantity? ", &ch, false)) {
            changed = false;
            break;
        }

        if (ch == 'A' || ch == 'a') {
            changed = true;
            break;
        }

        if (ch == 's' || ch == 'S') {
            wiz_statistics(player_ptr, q_ptr);
        }

        if (ch == 'r' || ch == 'R') {
            wiz_reroll_item(player_ptr, q_ptr);
        }

        if (ch == 't' || ch == 'T') {
            wiz_tweak_item(player_ptr, q_ptr);
        }

        if (ch == 'q' || ch == 'Q') {
            wiz_quantity_item(q_ptr);
        }
    }

    screen_load();
    if (changed) {
        msg_print("Changes accepted.");

        o_ptr->copy_from(q_ptr);
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        static constexpr auto flags_srf = {
            StatusRecalculatingFlag::BONUS,
            StatusRecalculatingFlag::COMBINATION,
            StatusRecalculatingFlag::REORDER,
        };
        rfu.set_flags(flags_srf);
        static constexpr auto flags_swrf = {
            SubWindowRedrawingFlag::INVENTORY,
            SubWindowRedrawingFlag::EQUIPMENT,
            SubWindowRedrawingFlag::SPELL,
            SubWindowRedrawingFlag::PLAYER,
            SubWindowRedrawingFlag::FLOOR_ITEMS,
            SubWindowRedrawingFlag::FOUND_ITEMS,
        };
        rfu.set_flags(flags_swrf);
    } else {
        msg_print("Changes ignored.");
    }
}

/*!
 * @brief オブジェクトの装備スロットがエゴが有効なスロットかどうか判定
 */
static int is_slot_able_to_be_ego(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    int slot = wield_slot(player_ptr, o_ptr);

    if (slot > -1) {
        return slot;
    }

    if (o_ptr->is_ammo()) {
        return INVEN_AMMO;
    }

    return -1;
}

/*!
 * @brief 願ったが消えてしまった場合のメッセージ
 */
static void wishing_puff_of_smoke(void)
{
    msg_print(_("何かが足下に転がってきたが、煙のように消えてしまった。",
        "You feel something roll beneath your feet, but it disappears in a puff of smoke!"));
}

/*!
 * @brief 願ったが消えてしまった場合のメッセージ
 * @param player_ptr 願ったプレイヤー情報への参照ポインタ
 * @param prob ★などを願った場合の生成確率
 * @param art_ok アーティファクトの生成を許すならTRUE
 * @param ego_ok エゴの生成を許すならTRUE
 * @param confirm 願わない場合に確認するかどうか
 * @return 願った結果
 */
WishResultType do_cmd_wishing(PlayerType *player_ptr, int prob, bool allow_art, bool allow_ego, bool confirm)
{
    static const std::vector<std::string> fixed_strings = {
#ifdef JP
        "燃えない",
        "錆びない",
        "腐食しない",
        "安定した",
#else
        "rotproof",
        "fireproof",
        "rustproof",
        "erodeproof",
        "corrodeproof",
        "fixed",
#endif
    };

    ItemEntity forge;
    auto *o_ptr = &forge;
    bool wish_art = false;
    bool wish_randart = false;
    bool wish_ego = false;
    bool exam_base = true;
    bool ok_art = randint0(100) < prob;
    bool ok_ego = randint0(100) < 50 + prob;
    bool must = prob < 0;
    bool blessed = false;
    bool fixed = true;

    std::string wish;
    while (true) {
        const auto wish_opt = get_string(_("何をお望み？ ", "For what do you wish?"), MAX_NLEN - 1);
        if (wish_opt.has_value()) {
            wish = str_rtrim(wish_opt.value());
            break;
        }

        constexpr auto mes_no_wish = _("何も願いません。本当によろしいですか？", "Do you wish nothing, really? ");
        if (confirm && !get_check(mes_no_wish)) {
            continue;
        }

        return WishResultType::NOTHING;
    }

#ifndef JP
    str_tolower(wish.data());

    /* remove 'a' */
    if (wish.starts_with("a ")) {
        wish = str_ltrim(wish.substr(0, 2));
    } else if (wish.starts_with("an ")) {
        wish = str_ltrim(wish.substr(0, 2));
    }
#endif // !JP

    if (wish.starts_with(_("祝福された", "blessed"))) {
        wish = str_ltrim(wish.substr(0, _(10, 7)));
        blessed = true;
    }

    for (const auto &fixed_string : fixed_strings) {
        if (wish.starts_with(fixed_string)) {
            wish = str_ltrim(wish.substr(0, fixed_string.length()));
            fixed = true;
            break;
        }
    }

#ifdef JP
    if (wish.starts_with("★")) {
        wish = str_ltrim(wish.substr(0, 2));
        wish_art = true;
        exam_base = false;
    } else
#endif

        if (wish.starts_with(_("☆", "The "))) {
        wish = str_ltrim(wish.substr(0, _(2, 4)));
        wish_art = true;
        wish_randart = true;
    }

    /* wishing random ego ? */
    else if (wish.starts_with(_("高級な", "excellent "))) {
        wish = str_ltrim(wish.substr(0, _(6, 9)));
        wish_ego = true;
    }

    if (wish.empty()) {
        msg_print(_("名前がない！", "What?"));
        return WishResultType::NOTHING;
    }

    if (!allow_art && wish_art) {
        msg_print(_("アーティファクトは願えない!", "You can not wish artifacts!"));
        return WishResultType::NOTHING;
    }

    if (cheat_xtra) {
        msg_format("Wishing %s....", wish.data());
    }

    std::vector<short> k_ids;
    std::vector<EgoType> e_ids;
    if (exam_base) {
        int max_len = 0;
        for (const auto &baseitem : baseitems_info) {
            if (baseitem.idx == 0 || baseitem.name.empty()) {
                continue;
            }

            o_ptr->prep(baseitem.idx);
#ifdef JP
            const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY | OD_STORE));
#else
            auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY | OD_STORE));
            str_tolower(item_name.data());
#endif
            if (cheat_xtra) {
                msg_format("Matching object No.%d %s", baseitem.idx, item_name.data());
            }

            const int len = item_name.length();
            if (wish.find(item_name) != std::string::npos) {
                if (len > max_len) {
                    k_ids.push_back(baseitem.idx);
                    max_len = len;
                }
            }
        }

        if (allow_ego && k_ids.size() == 1) {
            short bi_id = k_ids.back();
            o_ptr->prep(bi_id);

            for (const auto &[e_idx, ego] : egos_info) {
                if (ego.idx == EgoType::NONE || ego.name.empty()) {
                    continue;
                }

                std::string item_name(ego.name);
#ifdef JP
#else
                str_tolower(item_name.data());
#endif
                if (cheat_xtra) {
                    msg_format("matching ego no.%d %s...", enum2i(ego.idx), item_name.data());
                }

                if (wish.find(item_name) != std::string::npos) {
                    if (is_slot_able_to_be_ego(player_ptr, o_ptr) != ego.slot) {
                        continue;
                    }

                    e_ids.push_back(ego.idx);
                }
            }
        }
    }

    std::vector<FixedArtifactId> a_ids;

    if (allow_art) {
        char a_desc[MAX_NLEN] = "\0";
        char *a_str = a_desc;

        int len;
        int mlen = 0;
        for (const auto &[a_idx, artifact] : artifacts_info) {
            if (a_idx == FixedArtifactId::NONE || artifact.name.empty()) {
                continue;
            }

            const auto bi_id = lookup_baseitem_id(artifact.bi_key);
            if (bi_id == 0) {
                continue;
            }

            o_ptr->prep(bi_id);
            o_ptr->fixed_artifact_idx = a_idx;

#ifdef JP
            const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY | OD_STORE));
#else
            auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY | OD_STORE));
            str_tolower(item_name.data());
#endif
            a_str = a_desc;
            strcpy(a_desc, artifact.name.data());

            if (*a_str == '$') {
                a_str++;
            }
#ifdef JP
            /* remove quotes */
            if (!strncmp(a_str, "『", 2)) {
                a_str += 2;
                char *s = strstr(a_str, "』");
                *s = '\0';
            }
            /* remove 'of' */
            else {
                int l = strlen(a_str);
                if (!strrncmp(a_str, "の", 2)) {
                    a_str[l - 2] = '\0';
                }
            }
#else
            /* remove quotes */
            if (a_str[0] == '\'') {
                a_str += 1;
                char *s = strchr(a_desc, '\'');
                *s = '\0';
            }
            /* remove 'of ' */
            else if (!strncmp(a_str, (const char *)"of ", 3)) {
                a_str += 3;
            }

            str_tolower(a_str);
#endif
            const auto match_name = _(item_name.data() + 2, item_name.data());
            if (cheat_xtra) {
                msg_format("Matching artifact No.%d %s(%s)", enum2i(a_idx), a_desc, match_name);
            }

            std::vector<const char *> l = { a_str, artifact.name.data(), match_name };
            for (size_t c = 0; c < l.size(); c++) {
                if (wish == l.at(c)) {
                    len = strlen(l.at(c));
                    if (len > mlen) {
                        a_ids.push_back(a_idx);
                        mlen = len;
                    }
                }
            }
        }
    }

    if (w_ptr->wizard && (a_ids.size() > 1 || e_ids.size() > 1)) {
        msg_print(_("候補が多すぎる！", "Too many matches!"));
        return WishResultType::FAIL;
    }

    if (a_ids.size() == 1) {
        const auto a_idx = a_ids.back();
        const auto &artifact = ArtifactsInfo::get_instance().get_artifact(a_idx);
        if (must || (ok_art && !artifact.is_generated)) {
            (void)create_named_art(player_ptr, a_idx, player_ptr->y, player_ptr->x);
        } else {
            wishing_puff_of_smoke();
        }

        return WishResultType::ARTIFACT;
    }

    if (!allow_ego && (wish_ego || e_ids.size() > 0)) {
        msg_print(_("エゴアイテムは願えない！", "Can not wish ego item."));
        return WishResultType::NOTHING;
    }

    if (k_ids.size() == 1) {
        const auto bi_id = k_ids.back();
        const auto &baseitem = baseitems_info[bi_id];
        auto a_idx = FixedArtifactId::NONE;
        if (baseitem.gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
            for (const auto &[a_idx_loop, artifact_loop] : artifacts_info) {
                if (a_idx_loop == FixedArtifactId::NONE || artifact_loop.bi_key != baseitem.bi_key) {
                    continue;
                }

                a_idx = a_idx_loop;
                break;
            }
        }

        if (a_idx != FixedArtifactId::NONE) {
            const auto &artifact = ArtifactsInfo::get_instance().get_artifact(a_idx);
            if (must || (ok_art && !artifact.is_generated)) {
                (void)create_named_art(player_ptr, a_idx, player_ptr->y, player_ptr->x);
            } else {
                wishing_puff_of_smoke();
            }

            return WishResultType::ARTIFACT;
        }

        if (wish_randart) {
            if (must || ok_art) {
                do {
                    o_ptr->prep(bi_id);
                    ItemMagicApplier(player_ptr, o_ptr, baseitem.level, AM_SPECIAL | AM_NO_FIXED_ART).execute();
                } while (!o_ptr->is_random_artifact() || o_ptr->is_ego() || o_ptr->is_cursed());

                if (o_ptr->is_random_artifact()) {
                    drop_near(player_ptr, o_ptr, -1, player_ptr->y, player_ptr->x);
                }
            } else {
                wishing_puff_of_smoke();
            }
            return WishResultType::ARTIFACT;
        }

        WishResultType res = WishResultType::NOTHING;
        if (allow_ego && (wish_ego || e_ids.size() > 0)) {
            if (must || ok_ego) {
                if (e_ids.size() > 0) {
                    o_ptr->prep(bi_id);
                    o_ptr->ego_idx = e_ids[0];
                    apply_ego(o_ptr, player_ptr->current_floor_ptr->base_level);
                } else {
                    int max_roll = 1000;
                    int i = 0;
                    for (i = 0; i < max_roll; i++) {
                        o_ptr->prep(bi_id);
                        ItemMagicApplier(player_ptr, o_ptr, baseitem.level, AM_GREAT | AM_NO_FIXED_ART).execute();
                        if (o_ptr->is_random_artifact()) {
                            continue;
                        }

                        if (wish_ego) {
                            break;
                        }

                        EgoType e_idx = EgoType::NONE;
                        for (auto e : e_ids) {
                            if (o_ptr->ego_idx == e) {
                                e_idx = e;
                                break;
                            }
                        }

                        if (e_idx != EgoType::NONE) {
                            break;
                        }
                    }

                    if (i == max_roll) {
                        msg_print(_("失敗！もう一度願ってみてください。", "Failed! Try again."));
                        return WishResultType::FAIL;
                    }
                }
            } else {
                wishing_puff_of_smoke();
            }

            res = WishResultType::EGO;
        } else {
            for (int i = 0; i < 100; i++) {
                o_ptr->prep(bi_id);
                ItemMagicApplier(player_ptr, o_ptr, 0, AM_NO_FIXED_ART).execute();
                if (!o_ptr->is_cursed()) {
                    break;
                }
            }
            res = WishResultType::NORMAL;
        }

        if (blessed && wield_slot(player_ptr, o_ptr) != -1) {
            o_ptr->art_flags.set(TR_BLESSED);
        }

        if (fixed && wield_slot(player_ptr, o_ptr) != -1) {
            o_ptr->art_flags.set(TR_IGNORE_ACID);
            o_ptr->art_flags.set(TR_IGNORE_FIRE);
        }

        (void)drop_near(player_ptr, o_ptr, -1, player_ptr->y, player_ptr->x);

        return res;
    }

    msg_print(_("うーん、そんなものは存在しないようだ。", "Ummmm, that is not existing..."));
    return WishResultType::FAIL;
}
