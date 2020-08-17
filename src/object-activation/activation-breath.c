#include "object-activation/activation-breath.h"
#include "object-enchant/dragon-breaths-table.h"
#include "object/object-flags.h"
#include "spell-kind/spells-launcher.h"
#include "spell-realm/spells-hex.h"
#include "system/object-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 発動によるブレスの属性をアイテムの耐性から選択し、実行を処理する。/ Dragon breath activation
 * @details 対象となる耐性は dragonbreath_info テーブルを参照のこと。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動実行の是非を返す。
 */
bool activate_dragon_breath(player_type *user_ptr, object_type *o_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    BIT_FLAGS resistance_flags[TR_FLAG_SIZE];
    object_flags(user_ptr, o_ptr, resistance_flags);

    int type[20];
    int n = 0;
    concptr name[20];
    for (int i = 0; dragonbreath_info[i].flag != 0; i++) {
        if (have_flag(resistance_flags, dragonbreath_info[i].flag)) {
            type[n] = dragonbreath_info[i].type;
            name[n] = dragonbreath_info[i].name;
            n++;
        }
    }

    if (n == 0)
        return FALSE;

    if (music_singing_any(user_ptr))
        stop_singing(user_ptr);

    if (hex_spelling_any(user_ptr))
        stop_hex_spell_all(user_ptr);

    int t = randint0(n);
    msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), name[t]);
    fire_breath(user_ptr, type[t], dir, 250, 4);
    return TRUE;
}
