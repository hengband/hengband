#include "system/redrawing-flags-updater.h"
#include "util/enum-range.h"

RedrawingFlagsUpdater RedrawingFlagsUpdater::instance{};

RedrawingFlagsUpdater &RedrawingFlagsUpdater::get_instance()
{
    return instance;
}

bool RedrawingFlagsUpdater::any_main() const
{
    return this->main_window_flags.any();
}

bool RedrawingFlagsUpdater::any_sub() const
{
    return this->sub_window_flags.any();
}

bool RedrawingFlagsUpdater::any_stats() const
{
    return this->status_flags.any();
}

bool RedrawingFlagsUpdater::has(MainWindowRedrawingFlag flag) const
{
    return this->main_window_flags.has(flag);
}

bool RedrawingFlagsUpdater::has(SubWindowRedrawingFlag flag) const
{
    return this->sub_window_flags.has(flag);
}

bool RedrawingFlagsUpdater::has(StatusRecalculatingFlag flag) const
{
    return this->status_flags.has(flag);
}

bool RedrawingFlagsUpdater::has(InventoryArrangementFlag flag) const
{
    return this->inventory_arrangement_flags.has(flag);
}

bool RedrawingFlagsUpdater::has_any_of(const EnumClassFlagGroup<MainWindowRedrawingFlag> &flags) const
{
    return this->main_window_flags.has_any_of(flags);
}

bool RedrawingFlagsUpdater::has_any_of(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags) const
{
    return this->sub_window_flags.has_any_of(flags);
}

bool RedrawingFlagsUpdater::has_any_of(const EnumClassFlagGroup<StatusRecalculatingFlag> &flags) const
{
    return this->status_flags.has_any_of(flags);
}

bool RedrawingFlagsUpdater::has_any_of(const EnumClassFlagGroup<InventoryArrangementFlag> &flags) const
{
    return this->inventory_arrangement_flags.has_any_of(flags);
}

void RedrawingFlagsUpdater::set_flag(MainWindowRedrawingFlag flag)
{
    this->main_window_flags.set(flag);
}

void RedrawingFlagsUpdater::set_flag(SubWindowRedrawingFlag flag)
{
    this->sub_window_flags.set(flag);
}

void RedrawingFlagsUpdater::set_flag(StatusRecalculatingFlag flag)
{
    this->status_flags.set(flag);
}

void RedrawingFlagsUpdater::set_flag(InventoryArrangementFlag flag)
{
    this->inventory_arrangement_flags.set(flag);
}

void RedrawingFlagsUpdater::set_flags(const EnumClassFlagGroup<MainWindowRedrawingFlag> &flags)
{
    this->main_window_flags.set(flags);
}

void RedrawingFlagsUpdater::set_flags(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags)
{
    this->sub_window_flags.set(flags);
}

void RedrawingFlagsUpdater::set_flags(const EnumClassFlagGroup<StatusRecalculatingFlag> &flags)
{
    this->status_flags.set(flags);
}

void RedrawingFlagsUpdater::set_flags(const EnumClassFlagGroup<InventoryArrangementFlag> &flags)
{
    this->inventory_arrangement_flags.set(flags);
}

void RedrawingFlagsUpdater::reset_flag(MainWindowRedrawingFlag flag)
{
    this->main_window_flags.reset(flag);
}

void RedrawingFlagsUpdater::reset_flag(SubWindowRedrawingFlag flag)
{
    this->sub_window_flags.reset(flag);
}

void RedrawingFlagsUpdater::reset_flag(StatusRecalculatingFlag flag)
{
    this->status_flags.reset(flag);
}

void RedrawingFlagsUpdater::reset_flag(InventoryArrangementFlag flag)
{
    this->inventory_arrangement_flags.reset(flag);
}

void RedrawingFlagsUpdater::reset_flags(const EnumClassFlagGroup<MainWindowRedrawingFlag> &flags)
{
    this->main_window_flags.reset(flags);
}

void RedrawingFlagsUpdater::reset_flags(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags)
{
    this->sub_window_flags.reset(flags);
}

void RedrawingFlagsUpdater::reset_flags(const EnumClassFlagGroup<StatusRecalculatingFlag> &flags)
{
    this->status_flags.reset(flags);
}

void RedrawingFlagsUpdater::reset_flags(const EnumClassFlagGroup<InventoryArrangementFlag> &flags)
{
    this->inventory_arrangement_flags.reset(flags);
}

/*!
 * @brief 所持品が変わったときに、所持品の結合・並べ替えと一覧の描き直しを予約する
 * @details 結合・並べ替えはコマンドの区切りで update_inventory_arrangement() が行う。
 */
void RedrawingFlagsUpdater::request_inventory_arrangement()
{
    static constexpr auto flags_iaf = {
        InventoryArrangementFlag::COMBINATION,
        InventoryArrangementFlag::REORDER,
    };
    this->inventory_arrangement_flags.set(flags_iaf);
    this->sub_window_flags.set(SubWindowRedrawingFlag::INVENTORY);
}

/*!
 * @brief メインウィンドウの消去と全体の描き直しを予約する
 * @details 画面を消去してから、マップ・基本ステータス・拡張ステータス・装備のシンボルをすべて描き直す。
 */
void RedrawingFlagsUpdater::request_whole_main_window_redrawing()
{
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::EQUIPPY,
        MainWindowRedrawingFlag::MAP,
    };
    this->main_window_flags.set(flags_mwrf);
}

/*!
 * @brief マップの描き直しを予約する
 * @details メインウィンドウのマップに加え、マップを表示するサブウィンドウ (周辺の光景・ダンジョンの地形) も描き直す。
 */
void RedrawingFlagsUpdater::request_map_redrawing()
{
    this->main_window_flags.set(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    this->sub_window_flags.set(flags_swrf);
}

/*!
 * @brief モンスターの表示状態の更新と、マップの描き直しを予約する
 * @details モンスターの見え方が変わり得るとき (時間停止・無敵・画面のスクロールなど) に使う。
 * 描き直す範囲は request_map_redrawing() と同じ。
 */
void RedrawingFlagsUpdater::request_monster_update_and_map_redrawing()
{
    this->status_flags.set(StatusRecalculatingFlag::MONSTER_STATUSES);
    this->request_map_redrawing();
}

/*!
 * @brief アイテムの情報 (鑑定やアイテムの使用など) が変わったときの、アイテムを表示するサブウィンドウの描き直しを予約する
 * @details 所持品・装備品・足元のアイテム・発見済みのアイテムの一覧と、プレイヤーのステータスを描き直す。
 */
void RedrawingFlagsUpdater::request_item_lists_redrawing()
{
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
    };
    this->sub_window_flags.set(flags_swrf);
    this->request_floor_item_lists_redrawing();
}

/*!
 * @brief 床のアイテム (置いた・拾った・数が変わったなど) が変わったときの、床のアイテムを表示するサブウィンドウの描き直しを予約する
 * @details 足元のアイテムと発見済みのアイテムの一覧を描き直す。
 */
void RedrawingFlagsUpdater::request_floor_item_lists_redrawing()
{
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    this->sub_window_flags.set(flags_swrf);
}

/*!
 * @brief 呪文の情報を表示するサブウィンドウの描き直しを予約する
 * @details 呪文の一覧と、アイテムの知識 (魔法書を見たときの呪文の消費 MP・失敗率など) を描き直す。
 * 失敗率や消費 MP は、MP・朦朧・能力値・装備・レベル・熟練度など多くの値で変わる。
 */
void RedrawingFlagsUpdater::request_spell_info_redrawing()
{
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::SPELL,
        SubWindowRedrawingFlag::ITEM_KNOWLEDGE,
    };
    this->sub_window_flags.set(flags_swrf);
}

/*!
 * @brief アイテムの銘を変えたときの更新を予約する
 * @details 銘によって能力が変わる ({$} で警告を止める、{.} でランダムテレポートを止める) ので能力を計算し直す
 * (持ち物・装備の一覧の描き直しも、能力の再計算が要求する)。銘が揃うと束にまとめられるので、持ち物をまとめ直す。
 */
void RedrawingFlagsUpdater::request_inscription_change_updates()
{
    this->set_flag(StatusRecalculatingFlag::BONUS);
    this->set_flag(InventoryArrangementFlag::COMBINATION);
}

void RedrawingFlagsUpdater::fill_up_sub_flags()
{
    constexpr auto all_sub_window_flags = EnumRange(SubWindowRedrawingFlag::INVENTORY, SubWindowRedrawingFlag::MAX);
    this->sub_window_flags.set(all_sub_window_flags);
}

EnumClassFlagGroup<SubWindowRedrawingFlag> RedrawingFlagsUpdater::get_sub_intersection(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags) const
{
    return this->sub_window_flags & flags;
}
