#include "specific-object/bloody-moon.h"
#include "artifact/fixed-art-types.h"
#include "core/player-update-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/tr-types.h"
#include "player-base/player-race.h"
#include "racial/racial-android.h"
#include "system/artifact-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 固定アーティファクト『ブラッディムーン』の特性を変更する。
 * @details スレイ2d2種、及びone_resistance()による耐性1d2種、pval2種を得る。
 * @param o_ptr 対象のオブジェクト構造体 (ブラッディムーン)のポインタ
 */
void get_bloody_moon_flags(ObjectType *o_ptr)
{
    o_ptr->art_flags = a_info[ART_BLOOD].flags;

    int dummy = randint1(2) + randint1(2);
    for (int i = 0; i < dummy; i++) {
        int flag = randint0(26);
        if (flag >= 20) {
            o_ptr->art_flags.set(TR_KILL_UNDEAD + flag - 20);
        } else if (flag == 19) {
            o_ptr->art_flags.set(TR_KILL_ANIMAL);
        } else if (flag == 18) {
            o_ptr->art_flags.set(TR_SLAY_HUMAN);
        } else {
            o_ptr->art_flags.set(TR_CHAOTIC + flag);
        }
    }

    dummy = randint1(2);
    for (int i = 0; i < dummy; i++) {
        one_resistance(o_ptr);
    }

    for (int i = 0; i < 2; i++) {
        int tmp = randint0(11);
        if (tmp < A_MAX) {
            o_ptr->art_flags.set(TR_STR + tmp);
        } else {
            o_ptr->art_flags.set(TR_STEALTH + tmp - 6);
        }
    }
}

/*!
 * @brief Let's dance a RONDO!!
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr ブラッディ・ムーンへの参照ポインタ
 * @return オブジェクト情報に異常がない限りTRUE
 */
bool activate_bloody_moon(PlayerType *player_ptr, ObjectType *o_ptr)
{
    if (o_ptr->fixed_artifact_idx != ART_BLOOD) {
        return false;
    }

    msg_print(_("鎌が明るく輝いた...", "Your scythe glows brightly!"));
    get_bloody_moon_flags(o_ptr);
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        calc_android_exp(player_ptr);
    }

    player_ptr->update |= PU_BONUS | PU_HP;
    return true;
}
