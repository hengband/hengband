#include "core/visuals-reseter.h"
#include "core/window-redrawer.h"
#include "game-option/special-options.h"
#include "io/read-pref-file.h"
#include "system/baseitem/baseitem-service.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include <exception>

/*!
 * @brief オブジェクト、地形の表示シンボルなど初期化する / Reset the "visual" lists
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void reset_visuals(PlayerType *player_ptr)
{
    for (auto &terrain : TerrainList::get_instance()) {
        for (int j = 0; j < F_LIT_MAX; j++) {
            terrain.symbol_configs[j] = terrain.symbol_definitions[j];
        }
    }

    BaseitemService::reset_all_visuals();
    MonraceList::get_instance().reset_all_visuals();
    const auto pref_file = use_graphics ? "graf.prf" : "font.prf";
    process_pref_file(player_ptr, pref_file);
    std::stringstream ss;
    ss << (use_graphics ? "graf-" : "font-") << player_ptr->base_name << ".prf";
    process_pref_file(player_ptr, ss.str());
}

/*!
 * @brief グラフィックを使うかを切り替え、表示のシンボルを読み直して描き直す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param uses_graphics グラフィックを使うか
 */
static void switch_graphics(PlayerType *player_ptr, bool uses_graphics)
{
    use_graphics = uses_graphics;
    reset_visuals(player_ptr);
    RedrawingFlagsUpdater::get_instance().request_whole_main_window_redrawing();
    redraw_stuff(player_ptr);
}

/*!
 * @brief グラフィックを使っていれば外して描き直す
 * @param player_ptr プレイヤーへの参照ポインタ
 */
GraphicsSuspender::GraphicsSuspender(PlayerType *player_ptr)
    : player_ptr(player_ptr)
    , is_suspended(use_graphics)
    , uncaught_exceptions_on_suspend(std::uncaught_exceptions())
{
    if (this->is_suspended) {
        switch_graphics(this->player_ptr, false);
    }
}

/*!
 * @brief 外したグラフィックを元に戻して描き直す
 * @details 戻す処理 (表示のシンボルの読み直しなど) の例外は、変更前と同じく呼び出し元に伝える (noexcept(false))。
 * 例外で抜けるときは、変更前と同じく戻さない。巻き戻しの途中で戻す処理が例外を出すと、ゲームが終了してしまうため。
 */
GraphicsSuspender::~GraphicsSuspender() noexcept(false)
{
    if (std::uncaught_exceptions() > this->uncaught_exceptions_on_suspend) {
        return;
    }

    if (this->is_suspended) {
        switch_graphics(this->player_ptr, true);
    }
}
