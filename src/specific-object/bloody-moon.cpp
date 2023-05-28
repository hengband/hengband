#include "specific-object/bloody-moon.h"
#include "artifact/fixed-art-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/tr-types.h"
#include "player-base/player-race.h"
#include "racial/racial-android.h"
#include "system/artifact-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

constexpr auto BLOODY_MOON_SLAYING_FLAG_CANDIDATES = {
    TR_SLAY_ANIMAL,
    TR_SLAY_EVIL,
    TR_SLAY_UNDEAD,
    TR_SLAY_DEMON,
    TR_SLAY_ORC,
    TR_SLAY_TROLL,
    TR_SLAY_GIANT,
    TR_SLAY_DRAGON,
    TR_SLAY_HUMAN,
    TR_KILL_ANIMAL,
    TR_KILL_UNDEAD,
    TR_KILL_DEMON,
    TR_KILL_ORC,
    TR_KILL_TROLL,
    TR_KILL_GIANT,
    TR_KILL_DRAGON,
    TR_KILL_HUMAN,
    TR_BRAND_POIS,
    TR_BRAND_ACID,
    TR_BRAND_ELEC,
    TR_BRAND_FIRE,
    TR_BRAND_COLD,
    TR_CHAOTIC,
    TR_VAMPIRIC,
    TR_VORPAL,
    TR_EARTHQUAKE,
};

constexpr auto BLOODY_MOON_PVAL_FLAG_CANDIDATES = {
    TR_STR,
    TR_INT,
    TR_WIS,
    TR_DEX,
    TR_CON,
    TR_CHR,
    TR_STEALTH,
    TR_SEARCH,
    TR_INFRA,
    TR_TUNNEL,
    TR_SPEED,
};

/*!
 * @brief 固定アーティファクト『ブラッディムーン』の特性を変更する。
 * @details スレイ2d2種、及びone_resistance()による耐性1d2種、pval2種を得る。
 * @param o_ptr 対象のオブジェクト構造体 (ブラッディムーン)のポインタ
 */
void get_bloody_moon_flags(ItemEntity *o_ptr)
{
    o_ptr->art_flags = ArtifactsInfo::get_instance().get_artifact(FixedArtifactId::BLOOD).flags;

    for (int i = 0, count = damroll(2, 2); i < count; i++) {
        const auto flag = rand_choice(BLOODY_MOON_SLAYING_FLAG_CANDIDATES);
        o_ptr->art_flags.set(flag);
    }

    for (int i = 0, count = randint1(2); i < count; i++) {
        one_resistance(o_ptr);
    }

    for (int i = 0; i < 2; i++) {
        const auto flag = rand_choice(BLOODY_MOON_PVAL_FLAG_CANDIDATES);
        o_ptr->art_flags.set(flag);
    }
}

/*!
 * @brief Let's dance a RONDO!!
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr ブラッディ・ムーンへの参照ポインタ
 * @return オブジェクト情報に異常がない限りTRUE
 */
bool activate_bloody_moon(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    if (!o_ptr->is_specific_artifact(FixedArtifactId::BLOOD)) {
        return false;
    }

    msg_print(_("鎌が明るく輝いた...", "Your scythe glows brightly!"));
    get_bloody_moon_flags(o_ptr);
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        calc_android_exp(player_ptr);
    }

    const auto flags = {
        StatusRedrawingFlag::BONUS,
        StatusRedrawingFlag::HP,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    return true;
}
