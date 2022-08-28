#include "cmd-item/cmd-usestaff.h"
#include "action/action-limited.h"
#include "floor/floor-object.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object-use/use-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-staff-only.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "sv-definition/sv-staff-types.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 杖の効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sval オブジェクトのsval
 * @param use_charge 使用回数を消費したかどうかを返す参照ポインタ
 * @param powerful 強力発動上の処理ならばTRUE
 * @param magic 魔道具術上の処理ならばTRUE
 * @param known 判明済ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
int staff_effect(PlayerType *player_ptr, OBJECT_SUBTYPE_VALUE sval, bool *use_charge, bool powerful, bool magic, bool known)
{
    int k;
    bool ident = false;
    PLAYER_LEVEL lev = powerful ? player_ptr->lev * 2 : player_ptr->lev;
    POSITION detect_rad = powerful ? DETECT_RAD_DEFAULT * 3 / 2 : DETECT_RAD_DEFAULT;

    BadStatusSetter bss(player_ptr);
    switch (sval) {
    case SV_STAFF_DARKNESS:
        if (!has_resist_blind(player_ptr) && !has_resist_dark(player_ptr)) {
            if (bss.mod_blindness(3 + randint1(5))) {
                ident = true;
            }
        }

        if (unlite_area(player_ptr, 10, (powerful ? 6 : 3))) {
            ident = true;
        }

        break;
    case SV_STAFF_SLOWNESS: {
        if (bss.mod_deceleration(randint1(30) + 15, false)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_HASTE_MONSTERS: {
        if (speed_monsters(player_ptr)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_SUMMONING: {
        const int times = randint1(powerful ? 8 : 4);
        for (k = 0; k < times; k++) {
            if (summon_specific(player_ptr, 0, player_ptr->y, player_ptr->x, player_ptr->current_floor_ptr->dun_level, SUMMON_NONE,
                    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET))) {
                ident = true;
            }
        }
        break;
    }

    case SV_STAFF_TELEPORTATION: {
        teleport_player(player_ptr, (powerful ? 150 : 100), 0L);
        ident = true;
        break;
    }

    case SV_STAFF_IDENTIFY: {
        if (powerful) {
            if (!identify_fully(player_ptr, false)) {
                *use_charge = false;
            }
        } else {
            if (!ident_spell(player_ptr, false)) {
                *use_charge = false;
            }
        }
        ident = true;
        break;
    }

    case SV_STAFF_REMOVE_CURSE: {
        bool result = (powerful ? remove_all_curse(player_ptr) : remove_curse(player_ptr)) != 0;
        if (result) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_STARLITE:
        ident = starlight(player_ptr, magic);
        break;

    case SV_STAFF_LITE: {
        if (lite_area(player_ptr, damroll(2, 8), (powerful ? 4 : 2))) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_MAPPING: {
        map_area(player_ptr, powerful ? DETECT_RAD_MAP * 3 / 2 : DETECT_RAD_MAP);
        ident = true;
        break;
    }

    case SV_STAFF_DETECT_GOLD: {
        if (detect_treasure(player_ptr, detect_rad)) {
            ident = true;
        }
        if (detect_objects_gold(player_ptr, detect_rad)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_DETECT_ITEM: {
        if (detect_objects_normal(player_ptr, detect_rad)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_DETECT_TRAP: {
        if (detect_traps(player_ptr, detect_rad, known)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_DETECT_DOOR: {
        if (detect_doors(player_ptr, detect_rad)) {
            ident = true;
        }
        if (detect_stairs(player_ptr, detect_rad)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_DETECT_INVIS: {
        if (set_tim_invis(player_ptr, player_ptr->tim_invis + 12 + randint1(12), false)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_DETECT_EVIL: {
        if (detect_monsters_evil(player_ptr, detect_rad)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_CURE_LIGHT: {
        ident = cure_light_wounds(player_ptr, (powerful ? 4 : 2), 8);
        break;
    }

    case SV_STAFF_CURING: {
        ident = true_healing(player_ptr, 0);
        if (set_shero(player_ptr, 0, true)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_HEALING: {
        if (cure_critical_wounds(player_ptr, powerful ? 500 : 300)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_THE_MAGI: {
        if (do_res_stat(player_ptr, A_INT)) {
            ident = true;
        }
        ident |= restore_mana(player_ptr, false);
        if (set_shero(player_ptr, 0, true)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_SLEEP_MONSTERS: {
        if (sleep_monsters(player_ptr, lev)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_SLOW_MONSTERS: {
        if (slow_monsters(player_ptr, lev)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_SPEED: {
        if (set_acceleration(player_ptr, randint1(30) + (powerful ? 30 : 15), false)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_PROBING: {
        ident = probing(player_ptr);
        break;
    }

    case SV_STAFF_DISPEL_EVIL: {
        ident = dispel_evil(player_ptr, powerful ? 120 : 80);
        break;
    }

    case SV_STAFF_POWER: {
        ident = dispel_monsters(player_ptr, powerful ? 225 : 150);
        break;
    }

    case SV_STAFF_HOLINESS: {
        ident = cleansing_nova(player_ptr, magic, powerful);
        break;
    }

    case SV_STAFF_GENOCIDE: {
        ident = symbol_genocide(player_ptr, (magic ? lev + 50 : 200), true);
        break;
    }

    case SV_STAFF_EARTHQUAKES: {
        if (earthquake(player_ptr, player_ptr->y, player_ptr->x, (powerful ? 15 : 10), 0)) {
            ident = true;
        } else {
            msg_print(_("ダンジョンが揺れた。", "The dungeon trembles."));
        }

        break;
    }

    case SV_STAFF_DESTRUCTION: {
        ident = destroy_area(player_ptr, player_ptr->y, player_ptr->x, (powerful ? 18 : 13) + randint0(5), false);
        break;
    }

    case SV_STAFF_ANIMATE_DEAD: {
        ident = animate_dead(player_ptr, 0, player_ptr->y, player_ptr->x);
        break;
    }

    case SV_STAFF_MSTORM: {
        ident = unleash_mana_storm(player_ptr, powerful);
        break;
    }

    case SV_STAFF_NOTHING: {
        msg_print(_("何も起らなかった。", "Nothing happens."));
        if (PlayerRace(player_ptr).food() == PlayerRaceFoodType::MANA) {
            msg_print(_("もったいない事をしたような気がする。食べ物は大切にしなくては。", "What a waste.  It's your food!"));
        }
        break;
    }
    }
    return ident;
}

/*!
 * @brief 杖を使うコマンドのメインルーチン /
 */
void do_cmd_use_staff(PlayerType *player_ptr)
{
    OBJECT_IDX item;
    concptr q, s;

    if (player_ptr->wild_mode) {
        return;
    }

    if (cmd_limit_arena(player_ptr)) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    q = _("どの杖を使いますか? ", "Use which staff? ");
    s = _("使える杖がない。", "You have no staff to use.");
    if (!choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), TvalItemTester(ItemKindType::STAFF))) {
        return;
    }

    ObjectUseEntity(player_ptr, item).execute();
}
