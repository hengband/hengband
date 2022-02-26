#include "object-activation/activation-breath.h"
#include "effect/attribute-types.h"
#include "object-enchant/dragon-breaths-table.h"
#include "object/object-flags.h"
#include "object/tval-types.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/element-resistance.h"
#include "sv-definition/sv-ring-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 発動によるブレスの属性をアイテムの耐性から選択し、実行を処理する。/ Dragon breath activation
 * @details 対象となる耐性は dragonbreath_info テーブルを参照のこと。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動実行の是非を返す。
 */
bool activate_dragon_breath(PlayerType *player_ptr, ObjectType *o_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    auto resistance_flags = object_flags(o_ptr);

    AttributeType type[20];
    int n = 0;
    concptr name[20];
    for (int i = 0; dragonbreath_info[i].flag != 0; i++) {
        if (resistance_flags.has(dragonbreath_info[i].flag)) {
            type[n] = dragonbreath_info[i].type;
            name[n] = dragonbreath_info[i].name;
            n++;
        }
    }

    if (n == 0) {
        return false;
    }

    if (music_singing_any(player_ptr)) {
        stop_singing(player_ptr);
    }

    if (SpellHex(player_ptr).is_spelling_any()) {
        (void)SpellHex(player_ptr).stop_all_spells();
    }

    int t = randint0(n);
    msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), name[t]);
    fire_breath(player_ptr, type[t], dir, 250, 4);
    return true;
}

bool activate_breath_fire(PlayerType *player_ptr, ObjectType *o_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    fire_breath(player_ptr, AttributeType::FIRE, dir, 200, 2);
    if ((o_ptr->tval == ItemKindType::RING) && (o_ptr->sval == SV_RING_FLAMES)) {
        (void)set_oppose_fire(player_ptr, randint1(20) + 20, false);
    }

    return true;
}

bool activate_breath_cold(PlayerType *player_ptr, ObjectType *o_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    fire_breath(player_ptr, AttributeType::COLD, dir, 200, 2);
    if ((o_ptr->tval == ItemKindType::RING) && (o_ptr->sval == SV_RING_ICE)) {
        (void)set_oppose_cold(player_ptr, randint1(20) + 20, false);
    }

    return true;
}
