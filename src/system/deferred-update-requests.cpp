#include "system/deferred-update-requests.h"
#include "util/enum-range.h"

DeferredUpdateRequests DeferredUpdateRequests::instance{};

DeferredUpdateRequests &DeferredUpdateRequests::get_instance()
{
    return instance;
}

bool DeferredUpdateRequests::any_main() const
{
    return this->main_window_flags.any();
}

bool DeferredUpdateRequests::any_sub() const
{
    return this->sub_window_flags.any();
}

bool DeferredUpdateRequests::any_stats() const
{
    return this->status_flags.any();
}

/*!
 * @brief 所持品が変わったときに、所持品の結合・並べ替えと一覧の描き直しを予約する
 * @details 結合・並べ替えはコマンドの区切りで update_inventory_arrangement() が行う。
 */
void DeferredUpdateRequests::request_inventory_arrangement()
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
void DeferredUpdateRequests::request_whole_main_window_redrawing()
{
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::EQUIPPY,
    };
    this->main_window_flags.set(flags_mwrf);
    this->request_map_redrawing();
}

/*!
 * @brief 画面全体を描き直すときの、表示の元になる値の再計算を予約する
 * @details 能力・光源の明るさ・HP・MP・呪文、モンスターの見え方と、視界と明るさ (request_sight_recalculation()) を計算し直す。
 * 画面の描き直しのコマンドや、画面の大きさを変えたときに、メインウィンドウの全体の描き直し (request_whole_main_window_redrawing()) と合わせて呼ぶ。
 */
void DeferredUpdateRequests::request_whole_status_recalculation()
{
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    this->status_flags.set(flags_srf);
    this->request_player_status_recalculation();
    this->request_sight_recalculation();
}

/*!
 * @brief マップの描き直しを予約する
 * @details 地図のサブウィンドウ (ダンジョン全体図 OVERHEAD・自分の周囲 DUNGEON) は、メインウィンドウのマップを描き直すとき (print_map()) に
 * 描き直しが要求されるので、ここでは要求しない。画面の保存中などでマップを描き直すまでの間は、
 * 全体図だけが表示の変化を調べて描き直され (request_overhead_redrawing_if_changed())、自分の周囲はマップを描き直すときまで待つ。
 */
void DeferredUpdateRequests::request_map_redrawing()
{
    this->main_window_flags.set(MainWindowRedrawingFlag::MAP);
}

/*!
 * @brief モンスターの表示状態の更新と、マップの描き直しを予約する
 * @details モンスターの見え方が変わり得るとき (時間停止・無敵・画面のスクロールなど) に使う。
 * 描き直す範囲は request_map_redrawing() と同じ。
 */
void DeferredUpdateRequests::request_monster_update_and_map_redrawing()
{
    this->status_flags.set(StatusRecalculatingFlag::MONSTER_STATUSES);
    this->request_map_redrawing();
}

/*!
 * @brief 視界と明るさの計算し直しを予約する
 * @details 視界と明るさの印をいったん全て消してから計算し直し (前の視界や明るさの印が残らないように)、モンスターの光も計算し直す。
 * 目が見えなくなる・見えるようになる、地図を忘れる、地形が大きく変わる (地震・破壊・壁の消滅) ときに呼ぶ。
 * 地形が変わるときは、モンスターの経路 (FLOW) の計算し直しを別に要求する。
 */
void DeferredUpdateRequests::request_sight_recalculation()
{
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
    };
    this->status_flags.set(flags_srf);
}

/*!
 * @brief アイテムの情報 (鑑定やアイテムの使用など) が変わったときの、アイテムを表示するサブウィンドウの描き直しを予約する
 * @details 所持品・装備品・足元のアイテム・発見済みのアイテムの一覧と、プレイヤーのステータスを描き直す。
 */
void DeferredUpdateRequests::request_item_lists_redrawing()
{
    this->request_carried_item_lists_redrawing();
    this->request_floor_item_lists_redrawing();
    this->sub_window_flags.set(SubWindowRedrawingFlag::PLAYER);
}

/*!
 * @brief 所持品・装備品の一覧の描き直しを予約する
 * @details 持ち物だけの表示が変わったとき (能力の再計算・装備品の発動・選べるアイテムの印など) に呼ぶ。
 * 足元の品の表示も変わりうるとき (充填・使用回数・鑑定など) は request_item_lists_redrawing() を使う。
 */
void DeferredUpdateRequests::request_carried_item_lists_redrawing()
{
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
    };
    this->sub_window_flags.set(flags_swrf);
}

/*!
 * @brief 床のアイテム (置いた・拾った・数が変わったなど) が変わったときの、床のアイテムを表示するサブウィンドウの描き直しを予約する
 * @details 足元のアイテムと発見済みのアイテムの一覧を描き直す。
 */
void DeferredUpdateRequests::request_floor_item_lists_redrawing()
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
void DeferredUpdateRequests::request_spell_info_redrawing()
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
void DeferredUpdateRequests::request_inscription_change_updates()
{
    this->set_flag(StatusRecalculatingFlag::BONUS);
    this->set_flag(InventoryArrangementFlag::COMBINATION);
}

/*!
 * @brief 装備 (または装備の能力) が変わったときの更新を予約する
 * @details 装備の修正値や能力でプレイヤーの能力を、光源の能力で光源の明るさを、装備の重さや籠手 (能力や修正値によっては最大 MP が減らない) で最大 MP を計算し直す。
 * 装備の品が入れ替わったとき (装備する・外す・無くなるなど) と、装備品の能力が変わったとき (アーティファクト生成・凡庸化・呪縛など) に呼ぶ。
 */
void DeferredUpdateRequests::request_equipment_change_updates()
{
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::MP,
    };
    this->status_flags.set(flags_srf);
}

/*!
 * @brief 乗馬の状態が変わったときの描き直しを予約する
 * @details 乗馬中の表示 (EXTRA の行の「乗馬」) と、乗っているモンスターの体力の表示を描き直す。
 * 乗り降りでは能力も変わるので、能力の再計算と合わせて要求する request_riding_change_updates() から呼ぶ。
 * 階を移るときは画面全体を描き直すので、呼んでも呼ばなくてもよい。
 */
void DeferredUpdateRequests::request_riding_status_redrawing()
{
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::UHEALTH,
    };
    this->main_window_flags.set(flags_mwrf);
}

/*!
 * @brief 乗馬の状態が変わったときの更新を予約する
 * @details 乗っているモンスターの速度などで決まる能力を計算し直し、乗馬中の表示と乗っているモンスターの体力の表示を描き直す。
 * モンスターに乗ったとき・降りたとき (落馬・乗っているモンスターが消えたとき・乗っているペットを放したときを含む) に呼ぶ。
 * 視界や地図など、乗り降りで位置が変わることによる更新は、それぞれの箇所で要求する。
 */
void DeferredUpdateRequests::request_riding_change_updates()
{
    this->status_flags.set(StatusRecalculatingFlag::BONUS);
    this->request_riding_status_redrawing();
}

/*!
 * @brief HP と MP の表示の描き直しを予約する
 * @details メインウィンドウの HP と MP の両方を描き直す (HP と MP の変換や、呪術の表示を直すときなど)。
 * 呪文の情報 (呪文の一覧・アイテムの知識) のサブウィンドウは HP・MP を描き直すときに要求され、プレイヤーの情報のサブウィンドウは
 * 前に描いた HP・MP と比べて変わっていれば描き直される。
 */
void DeferredUpdateRequests::request_hp_mp_redrawing()
{
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    this->main_window_flags.set(flags_mwrf);
}

/*!
 * @brief 所持金の表示の描き直しを予約する
 * @details メインウィンドウの所持金と、プレイヤーの情報のサブウィンドウを描き直す。
 */
void DeferredUpdateRequests::request_gold_redrawing()
{
    this->main_window_flags.set(MainWindowRedrawingFlag::GOLD);
    this->sub_window_flags.set(SubWindowRedrawingFlag::PLAYER);
}

/*!
 * @brief 光源の燃料を補充したときの更新を予約する
 * @details 光源の明るさを計算し直し (燃料が切れていた光源が再び灯ることも、暗黒の光源の燃料で消えることもある)、
 * 残りのターンを表示する装備品の一覧を描き直す。
 * 燃料が燃えて減るとき・燃え尽きたときは、notice_lite_change() で別に要求する。
 */
void DeferredUpdateRequests::request_light_fuel_change_updates()
{
    this->status_flags.set(StatusRecalculatingFlag::TORCH);
    this->sub_window_flags.set(SubWindowRedrawingFlag::EQUIPMENT);
}

/*!
 * @brief 能力と最大 HP の再計算を予約する
 * @details 最大 HP が変わりうるとき (士気高揚・狂戦士化・つよし・変身などの一時的な効果の始まりと終わり、
 * モンスターの魔力消去で歌・呪術が途切れたときと途切れていた歌・呪術を再開したとき、
 * ブラッディ・ムーンの能力の付け直し、キャラクターの作成) に呼ぶ。
 * 最大 MP と覚えられる呪文の数も計算し直すなら request_player_status_recalculation() を使う。
 */
void DeferredUpdateRequests::request_bonus_and_max_hp_recalculation()
{
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
    };
    this->status_flags.set(flags_srf);
}

/*!
 * @brief 最大 MP と覚えられる呪文の数の再計算を予約する
 * @details 呪文に使う能力値 (知能・賢さ・魅力) の段階が変わったときに呼ぶ。
 */
void DeferredUpdateRequests::request_max_mp_and_spells_recalculation()
{
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    };
    this->status_flags.set(flags_srf);
}

/*!
 * @brief プレイヤーの能力・最大 HP・最大 MP・覚えられる呪文の数の再計算を予約する
 * @details レベルや職業・種族が変わったとき、フロアを移ったとき、呪術の維持が変わったとき、待機していたスコアを登録するときなど、
 * プレイヤーの能力をひととおり計算し直すときに呼ぶ (request_bonus_and_max_hp_recalculation() と
 * request_max_mp_and_spells_recalculation() を合わせたもの)。
 */
void DeferredUpdateRequests::request_player_status_recalculation()
{
    this->request_bonus_and_max_hp_recalculation();
    this->request_max_mp_and_spells_recalculation();
}

void DeferredUpdateRequests::fill_up_sub_flags()
{
    constexpr auto all_sub_window_flags = EnumRange(SubWindowRedrawingFlag::INVENTORY, SubWindowRedrawingFlag::MAX);
    this->sub_window_flags.set(all_sub_window_flags);
}

EnumClassFlagGroup<SubWindowRedrawingFlag> DeferredUpdateRequests::get_sub_intersection(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags) const
{
    return this->sub_window_flags & flags;
}
