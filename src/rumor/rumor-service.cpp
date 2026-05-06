#include "rumor/rumor-service.h"
#include "io/files-util.h"
#include "rumor/rumor-list.h"
#include "system/monrace/monrace-list.h"
#include "util/angband-files.h"

/*!
 * @brief 噂に関する初期化のファサード
 */
void RumorService::initialize()
{
    auto &rumors = RumorList::get_instance();
    const auto path = path_build(ANGBAND_DIR_FILE, _("rumors_j.txt", "rumors.txt"));
    rumors.read_rumors(path);
}

/*!
 * @brief 噂に関する初期後処理のファサード
 */
void RumorService::retouch()
{
    auto &rumors = RumorList::get_instance();
    rumors.add_towns();
    rumors.add_shallow_dungeons();
    rumors.add_normal_monsters();
    rumors.add_shallow_artifacts();
    rumors.add_deep_dungeons();
    rumors.add_unique_monsters();
    rumors.add_deep_artifacts();
    rumors.validate();
    rumors.make_table();
}
