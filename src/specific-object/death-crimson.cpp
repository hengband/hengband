#include "specific-object/death-crimson.h"
#include "artifact/fixed-art-types.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/geometry.h"
#include "player-base/player-class.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief クリムゾンを発射する / Fire Crimson, evoluting gun.
 @ @param player_ptr プレイヤーへの参照ポインタ
 * @return キャンセルした場合 false.
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
static bool fire_crimson(PlayerType *player_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    auto [ty, tx] = player_ptr->get_position() + dir.vec() * 99;
    if (dir.is_targetting() && target_okay(player_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    int num = 1;
    if (PlayerClass(player_ptr).equals(PlayerClassType::ARCHER)) {
        if (player_ptr->lev >= 10) {
            num++;
        }

        if (player_ptr->lev >= 30) {
            num++;
        }

        if (player_ptr->lev >= 45) {
            num++;
        }
    }

    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    for (int i = 0; i < num; i++) {
        (void)project(player_ptr, 0, player_ptr->lev / 20 + 1, ty, tx, player_ptr->lev * player_ptr->lev * 6 / 50, AttributeType::ROCKET, flg);
    }

    return true;
}

bool activate_crimson(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    if (!o_ptr->is_specific_artifact(FixedArtifactId::CRIMSON)) {
        return false;
    }

    msg_print(_("せっかくだから『クリムゾン』をぶっぱなすぜ！", "I'll fire CRIMSON! SEKKAKUDAKARA!"));
    return fire_crimson(player_ptr);
}
