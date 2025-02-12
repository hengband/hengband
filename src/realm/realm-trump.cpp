#include "realm/realm-trump.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "effect/attribute-types.h"
#include "effect/spells-effect-util.h"
#include "game-option/input-options.h"
#include "monster-floor/place-monster-types.h"
#include "mutation/mutation-calculator.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-chaos.h"
#include "spell-realm/spells-trump.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/sight-setter.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "view/display-messages.h"

/*!
 * @brief トランプ領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は std::nullopt を返す。
 */
std::optional<std::string> do_trump_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;
    bool fail = mode == SpellProcessType::FAIL;

    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0: {
        POSITION range = 10;

        if (info) {
            return info_range(range);
        }

        if (cast) {
            teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
        }
    } break;

    case 1: {
        if (cast || fail) {
            msg_print(_("あなたは蜘蛛のカードに集中する...", "You concentrate on the trump of a spider..."));
            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, 0, SUMMON_SPIDER, PM_ALLOW_GROUP)) {
                if (fail) {
                    msg_print(_("召喚された蜘蛛は怒っている！", "The summoned spiders get angry!"));
                }
            }
        }
    } break;

    case 2: {
        if (info) {
            return KWD_RANDOM;
        }

        if (cast) {
            cast_shuffle(player_ptr);
        }
    } break;

    case 3: {
        if (cast) {
            if (!reset_recall(player_ptr)) {
                return std::nullopt;
            }
        }
    } break;

    case 4: {
        POSITION range = plev * 4;

        if (info) {
            return info_range(range);
        }

        if (cast) {
            teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
        }
    } break;

    case 5: {
        int base = 25;
        const Dice dice(1, 30);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_esp(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 6: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return std::nullopt;
            }

            fire_beam(player_ptr, AttributeType::AWAY_ALL, dir, power);
        }
    } break;

    case 7: {
        if (cast || fail) {
            summon_type type = (!fail ? SUMMON_ANIMAL_RANGER : SUMMON_ANIMAL);
            msg_print(_("あなたは動物のカードに集中する...", "You concentrate on the trump of an animal..."));
            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, 0, type, 0L)) {
                if (fail) {
                    msg_print(_("召喚された動物は怒っている！", "The summoned animal gets angry!"));
                }
            }
        }
    } break;

    case 8: {
        WEIGHT weight = plev * 15;

        if (info) {
            return info_weight(weight);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return std::nullopt;
            }

            fetch_item(player_ptr, dir, weight, false);
        }
    } break;

    case 9: {
        if (cast || fail) {
            POSITION x, y;
            summon_type type;

            if (cast) {
                if (!target_set(player_ptr, TARGET_KILL)) {
                    return std::nullopt;
                }
                x = target_col;
                y = target_row;
            } else {
                /* Summons near player when failed */
                x = player_ptr->x;
                y = player_ptr->y;
            }

            if (PlayerClass(player_ptr).equals(PlayerClassType::BEASTMASTER)) {
                type = SUMMON_KAMIKAZE_LIVING;
            } else {
                type = SUMMON_KAMIKAZE;
            }

            msg_print(_("あなたはカミカゼのカードに集中する...", "You concentrate on several trumps at once..."));
            if (trump_summoning(player_ptr, 2 + randint0(plev / 7), !fail, y, x, 0, type, 0L)) {
                if (fail) {
                    msg_print(_("召喚されたモンスターは怒っている！", "The summoned creatures get angry!"));
                }
            }
        }
    } break;

    case 10: {
        /* Phantasmal Servant is not summoned as enemy when failed */
        if (cast) {
            int summon_lev = plev * 2 / 3 + randint1(plev / 2);

            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, (summon_lev * 3 / 2), SUMMON_PHANTOM, 0L)) {
                msg_print(_("御用でございますか、御主人様？", "'Your wish, master?'"));
            }
        }
    } break;

    case 11: {
        if (cast) {
            /* Temporary enable target_pet option */
            bool old_target_pet = target_pet;
            target_pet = true;

            const auto dir = get_aim_dir(player_ptr);

            /* Restore target_pet option */
            target_pet = old_target_pet;

            if (!dir) {
                return std::nullopt;
            }

            speed_monster(player_ptr, dir, plev);
        }
    } break;

    case 12: {
        if (cast) {
            if (!input_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)"))) {
                return std::nullopt;
            }
            teleport_level(player_ptr, 0);
        }
    } break;

    case 13: {
        POSITION range = plev / 2 + 10;

        if (info) {
            return info_range(range);
        }

        if (cast) {
            msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
            if (!dimension_door(player_ptr)) {
                return std::nullopt;
            }
        }
    } break;

    case 14: {
        int base = 15;
        const Dice dice(1, 20);

        if (info) {
            return info_delay(base, dice);
        }

        if (cast) {
            if (!recall_player(player_ptr, dice.roll() + base)) {
                return std::nullopt;
            }
        }
    } break;

    case 15: {
        int power = plev * 4;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            banish_monsters(player_ptr, power);
        }
    } break;

    case 16: {
        if (cast) {
            /* HACK -- No range limit */
            project_length = -1;

            const auto dir = get_aim_dir(player_ptr);

            /* Restore range to default */
            project_length = 0;

            if (!dir) {
                return std::nullopt;
            }

            teleport_swap(player_ptr, dir);
        }
    } break;

    case 17: {
        if (cast || fail) {
            msg_print(_("あなたはアンデッドのカードに集中する...", "You concentrate on the trump of an undead creature..."));
            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, 0, SUMMON_UNDEAD, 0L)) {
                if (fail) {
                    msg_print(_("召喚されたアンデッドは怒っている！", "The summoned undead creature gets angry!"));
                }
            }
        }
    } break;

    case 18: {
        if (cast || fail) {
            msg_print(_("あなたは爬虫類のカードに集中する...", "You concentrate on the trump of a reptile..."));
            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, 0, SUMMON_HYDRA, 0L)) {
                if (fail) {
                    msg_print(_("召喚された爬虫類は怒っている！", "The summoned reptile gets angry!"));
                }
            }
        }
    } break;

    case 19: {
        if (cast || fail) {
            summon_type type;
            msg_print(_("あなたはモンスターのカードに集中する...", "You concentrate on several trumps at once..."));
            if (PlayerClass(player_ptr).equals(PlayerClassType::BEASTMASTER)) {
                type = SUMMON_LIVING;
            } else {
                type = SUMMON_NONE;
            }

            if (trump_summoning(player_ptr, (1 + (plev - 15) / 10), !fail, player_ptr->y, player_ptr->x, 0, type, 0L)) {
                if (fail) {
                    msg_print(_("召喚されたモンスターは怒っている！", "The summoned creatures get angry!"));
                }
            }
        }
    } break;

    case 20: {
        if (cast || fail) {
            msg_print(_("あなたはハウンドのカードに集中する...", "You concentrate on the trump of a hound..."));
            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, 0, SUMMON_HOUND, PM_ALLOW_GROUP)) {
                if (fail) {
                    msg_print(_("召喚されたハウンドは怒っている！", "The summoned hounds get angry!"));
                }
            }
        }
    } break;

    case 21: {
        if (cast) {
            brand_weapon(player_ptr, 5);
        }
    } break;

    case 22:
        if (cast) {
            become_living_trump(player_ptr);
        }
        break;

    case 23: {
        if (cast || fail) {
            msg_print(_("あなたはサイバーデーモンのカードに集中する...", "You concentrate on the trump of a Cyberdemon..."));
            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, 0, SUMMON_CYBER, 0L)) {
                if (fail) {
                    msg_print(_("召喚されたサイバーデーモンは怒っている！", "The summoned Cyberdemon gets angry!"));
                }
            }
        }
    } break;

    case 24: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_all(player_ptr, rad);
        }
    } break;

    case 25: {
        if (cast) {
            if (!identify_fully(player_ptr, false)) {
                return std::nullopt;
            }
        }
    } break;

    case 26: {
        int heal = plev * 10 + 200;

        if (info) {
            return info_heal(heal);
        }

        if (cast) {
            /* Temporary enable target_pet option */
            bool old_target_pet = target_pet;
            target_pet = true;

            const auto dir = get_aim_dir(player_ptr);

            /* Restore target_pet option */
            target_pet = old_target_pet;

            if (!dir) {
                return std::nullopt;
            }

            heal_monster(player_ptr, dir, heal);
        }
    } break;

    case 27: {
        if (cast || fail) {
            msg_print(_("あなたはドラゴンのカードに集中する...", "You concentrate on the trump of a dragon..."));
            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, 0, SUMMON_DRAGON, 0L)) {
                if (fail) {
                    msg_print(_("召喚されたドラゴンは怒っている！", "The summoned dragon gets angry!"));
                }
            }
        }
    } break;

    case 28: {
        int dam = plev * 2;
        POSITION rad = 2;

        if (info) {
            return info_multi_damage(dam);
        }

        if (cast) {
            cast_meteor(player_ptr, dam, rad);
        }
    } break;

    case 29: {
        if (cast || fail) {
            msg_print(_("あなたはデーモンのカードに集中する...", "You concentrate on the trump of a demon..."));
            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, 0, SUMMON_DEMON, 0L)) {
                if (fail) {
                    msg_print(_("召喚されたデーモンは怒っている！", "The summoned demon gets angry!"));
                }
            }
        }
    } break;

    case 30: {
        if (cast || fail) {
            msg_print(_("あなたは強力なアンデッドのカードに集中する...", "You concentrate on the trump of a greater undead being..."));
            /* May allow unique depend on level and dice roll */
            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, 0, SUMMON_HI_UNDEAD, PM_ALLOW_UNIQUE)) {
                if (fail) {
                    msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead creature gets angry!"));
                }
            }
        }
    } break;

    case 31: {
        if (cast) {
            summon_type type;

            if (PlayerClass(player_ptr).equals(PlayerClassType::BEASTMASTER)) {
                type = SUMMON_HI_DRAGON_LIVING;
            } else {
                type = SUMMON_HI_DRAGON;
            }

            msg_print(_("あなたは古代ドラゴンのカードに集中する...", "You concentrate on the trump of an ancient dragon..."));
            /* May allow unique depend on level and dice roll */
            if (trump_summoning(player_ptr, 1, !fail, player_ptr->y, player_ptr->x, 0, type, PM_ALLOW_UNIQUE)) {
                if (fail) {
                    msg_print(_("召喚された古代ドラゴンは怒っている！", "The summoned ancient dragon gets angry!"));
                }
            }
        }
    } break;
    }

    return "";
}
