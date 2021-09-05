#include "realm/realm-trump.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "effect/spells-effect-util.h"
#include "game-option/input-options.h"
#include "monster-floor/place-monster-types.h"
#include "mutation/mutation-calculator.h"
#include "player/player-class.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-chaos.h"
#include "spell-realm/spells-trump.h"
#include "spell/spell-types.h"
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
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST)
 * @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST時はnullptr文字列を返す。
 */
concptr do_trump_spell(player_type *caster_ptr, SPELL_IDX spell, spell_type mode)
{
    bool name = (mode == SPELL_NAME) ? true : false;
    bool desc = (mode == SPELL_DESC) ? true : false;
    bool info = (mode == SPELL_INFO) ? true : false;
    bool cast = (mode == SPELL_CAST) ? true : false;
    bool fail = (mode == SPELL_FAIL) ? true : false;

    DIRECTION dir;
    PLAYER_LEVEL plev = caster_ptr->lev;

    switch (spell) {
    case 0:
        if (name)
            return _("ショート・テレポート", "Phase Door");
        if (desc)
            return _("近距離のテレポートをする。", "Teleports you a short distance.");

        {
            POSITION range = 10;

            if (info)
                return info_range(range);

            if (cast) {
                teleport_player(caster_ptr, range, TELEPORT_SPONTANEOUS);
            }
        }
        break;

    case 1:
        if (name)
            return _("蜘蛛のカード", "Trump Spiders");
        if (desc)
            return _("蜘蛛を召喚する。", "Summons spiders.");

        {
            if (cast || fail) {
                msg_print(_("あなたは蜘蛛のカードに集中する...", "You concentrate on the trump of an spider..."));
                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, 0, SUMMON_SPIDER, PM_ALLOW_GROUP)) {
                    if (fail) {
                        msg_print(_("召喚された蜘蛛は怒っている！", "The summoned spiders get angry!"));
                    }
                }
            }
        }
        break;

    case 2:
        if (name)
            return _("シャッフル", "Shuffle");
        if (desc)
            return _("カードの占いをする。", "Causes random effects.");

        {
            if (info)
                return KWD_RANDOM;

            if (cast) {
                cast_shuffle(caster_ptr);
            }
        }
        break;

    case 3:
        if (name)
            return _("フロア・リセット", "Reset Recall");
        if (desc)
            return _("最深階を変更する。", "Resets the 'deepest' level for recall spell.");

        {
            if (cast) {
                if (!reset_recall(caster_ptr))
                    return nullptr;
            }
        }
        break;

    case 4:
        if (name)
            return _("テレポート", "Teleport");
        if (desc)
            return _("遠距離のテレポートをする。", "Teleports you a long distance.");

        {
            POSITION range = plev * 4;

            if (info)
                return info_range(range);

            if (cast) {
                teleport_player(caster_ptr, range, TELEPORT_SPONTANEOUS);
            }
        }
        break;

    case 5:
        if (name)
            return _("感知のカード", "Trump Spying");
        if (desc)
            return _("一定時間、テレパシー能力を得る。", "Gives telepathy for a while.");

        {
            int base = 25;
            DICE_SID sides = 30;

            if (info)
                return info_duration(base, sides);

            if (cast) {
                set_tim_esp(caster_ptr, randint1(sides) + base, false);
            }
        }
        break;

    case 6:
        if (name)
            return _("テレポート・モンスター", "Teleport Away");
        if (desc)
            return _("モンスターをテレポートさせるビームを放つ。抵抗されると無効。", "Teleports all monsters on the line away unless resisted.");

        {
            int power = plev;

            if (info)
                return info_power(power);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                fire_beam(caster_ptr, GF_AWAY_ALL, dir, power);
            }
        }
        break;

    case 7:
        if (name)
            return _("動物のカード", "Trump Animals");
        if (desc)
            return _("1体の動物を召喚する。", "Summons an animal.");

        {
            if (cast || fail) {
                summon_type type = (!fail ? SUMMON_ANIMAL_RANGER : SUMMON_ANIMAL);
                msg_print(_("あなたは動物のカードに集中する...", "You concentrate on the trump of an animal..."));
                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, 0, type, 0L)) {
                    if (fail) {
                        msg_print(_("召喚された動物は怒っている！", "The summoned animal gets angry!"));
                    }
                }
            }
        }
        break;

    case 8:
        if (name)
            return _("移動のカード", "Trump Reach");
        if (desc)
            return _("アイテムを自分の足元へ移動させる。", "Pulls a distant item close to you.");

        {
            WEIGHT weight = plev * 15;

            if (info)
                return info_weight(weight);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                fetch_item(caster_ptr, dir, weight, false);
            }
        }
        break;

    case 9:
        if (name)
            return _("カミカゼのカード", "Trump Kamikaze");
        if (desc)
            return _("複数の爆発するモンスターを召喚する。", "Summons multiple exploding monsters.");

        {
            if (cast || fail) {
                POSITION x, y;
                summon_type type;

                if (cast) {
                    if (!target_set(caster_ptr, TARGET_KILL))
                        return nullptr;
                    x = target_col;
                    y = target_row;
                } else {
                    /* Summons near player when failed */
                    x = caster_ptr->x;
                    y = caster_ptr->y;
                }

                if (caster_ptr->pclass == CLASS_BEASTMASTER)
                    type = SUMMON_KAMIKAZE_LIVING;
                else
                    type = SUMMON_KAMIKAZE;

                msg_print(_("あなたはカミカゼのカードに集中する...", "You concentrate on several trumps at once..."));
                if (trump_summoning(caster_ptr, 2 + randint0(plev / 7), !fail, y, x, 0, type, 0L)) {
                    if (fail) {
                        msg_print(_("召喚されたモンスターは怒っている！", "The summoned creatures get angry!"));
                    }
                }
            }
        }
        break;

    case 10:
        if (name)
            return _("幻霊召喚", "Phantasmal Servant");
        if (desc)
            return _("1体の幽霊を召喚する。", "Summons a ghost.");

        {
            /* Phantasmal Servant is not summoned as enemy when failed */
            if (cast) {
                int summon_lev = plev * 2 / 3 + randint1(plev / 2);

                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, (summon_lev * 3 / 2), SUMMON_PHANTOM, 0L)) {
                    msg_print(_("御用でございますか、御主人様？", "'Your wish, master?'"));
                }
            }
        }
        break;

    case 11:
        if (name)
            return _("スピード・モンスター", "Haste Monster");
        if (desc)
            return _("モンスター1体を加速させる。", "Hastes a monster.");

        {
            if (cast) {
                bool result;

                /* Temporary enable target_pet option */
                bool old_target_pet = target_pet;
                target_pet = true;

                result = get_aim_dir(caster_ptr, &dir);

                /* Restore target_pet option */
                target_pet = old_target_pet;

                if (!result)
                    return nullptr;

                speed_monster(caster_ptr, dir, plev);
            }
        }
        break;

    case 12:
        if (name)
            return _("テレポート・レベル", "Teleport Level");
        if (desc)
            return _("瞬時に上か下の階にテレポートする。", "Instantly teleports you up or down a level.");

        {
            if (cast) {
                if (!get_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)")))
                    return nullptr;
                teleport_level(caster_ptr, 0);
            }
        }
        break;

    case 13:
        if (name)
            return _("次元の扉", "Dimension Door");
        if (desc)
            return _("短距離内の指定した場所にテレポートする。", "Teleports you to a given location.");

        {
            POSITION range = plev / 2 + 10;

            if (info)
                return info_range(range);

            if (cast) {
                msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
                if (!dimension_door(caster_ptr))
                    return nullptr;
            }
        }
        break;

    case 14:
        if (name)
            return _("帰還の呪文", "Word of Recall");
        if (desc)
            return _("地上にいるときはダンジョンの最深階へ、ダンジョンにいるときは地上へと移動する。",
                "Recalls player from dungeon to town or from town to the deepest level of dungeon.");

        {
            int base = 15;
            DICE_SID sides = 20;

            if (info)
                return info_delay(base, sides);

            if (cast) {
                if (!recall_player(caster_ptr, randint0(21) + 15))
                    return nullptr;
            }
        }
        break;

    case 15:
        if (name)
            return _("怪物追放", "Banish");
        if (desc)
            return _("視界内の全てのモンスターをテレポートさせる。抵抗されると無効。", "Teleports all monsters in sight away unless resisted.");

        {
            int power = plev * 4;

            if (info)
                return info_power(power);

            if (cast) {
                banish_monsters(caster_ptr, power);
            }
        }
        break;

    case 16:
        if (name)
            return _("位置交換のカード", "Swap Position");
        if (desc)
            return _("1体のモンスターと位置を交換する。", "Swap positions of you and a monster.");

        {
            if (cast) {
                bool result;

                /* HACK -- No range limit */
                project_length = -1;

                result = get_aim_dir(caster_ptr, &dir);

                /* Restore range to default */
                project_length = 0;

                if (!result)
                    return nullptr;

                teleport_swap(caster_ptr, dir);
            }
        }
        break;

    case 17:
        if (name)
            return _("アンデッドのカード", "Trump Undead");
        if (desc)
            return _("1体のアンデッドを召喚する。", "Summons an undead monster.");

        {
            if (cast || fail) {
                msg_print(_("あなたはアンデッドのカードに集中する...", "You concentrate on the trump of an undead creature..."));
                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, 0, SUMMON_UNDEAD, 0L)) {
                    if (fail) {
                        msg_print(_("召喚されたアンデッドは怒っている！", "The summoned undead creature gets angry!"));
                    }
                }
            }
        }
        break;

    case 18:
        if (name)
            return _("爬虫類のカード", "Trump Reptile");
        if (desc)
            return _("1体のヒドラを召喚する。", "Summons a hydra.");

        {
            if (cast || fail) {
                msg_print(_("あなたは爬虫類のカードに集中する...", "You concentrate on the trump of a reptile..."));
                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, 0, SUMMON_HYDRA, 0L)) {
                    if (fail) {
                        msg_print(_("召喚された爬虫類は怒っている！", "The summoned reptile gets angry!"));
                    }
                }
            }
        }
        break;

    case 19:
        if (name)
            return _("モンスターのカード", "Trump Monsters");
        if (desc)
            return _("複数のモンスターを召喚する。", "Summons some monsters.");

        {
            if (cast || fail) {
                summon_type type;
                msg_print(_("あなたはモンスターのカードに集中する...", "You concentrate on several trumps at once..."));
                if (caster_ptr->pclass == CLASS_BEASTMASTER)
                    type = SUMMON_LIVING;
                else
                    type = SUMMON_NONE;

                if (trump_summoning(caster_ptr, (1 + (plev - 15) / 10), !fail, caster_ptr->y, caster_ptr->x, 0, type, 0L)) {
                    if (fail) {
                        msg_print(_("召喚されたモンスターは怒っている！", "The summoned creatures get angry!"));
                    }
                }
            }
        }
        break;

    case 20:
        if (name)
            return _("ハウンドのカード", "Trump Hounds");
        if (desc)
            return _("1グループのハウンドを召喚する。", "Summons a group of hounds.");

        {
            if (cast || fail) {
                msg_print(_("あなたはハウンドのカードに集中する...", "You concentrate on the trump of a hound..."));
                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, 0, SUMMON_HOUND, PM_ALLOW_GROUP)) {
                    if (fail) {
                        msg_print(_("召喚されたハウンドは怒っている！", "The summoned hounds get angry!"));
                    }
                }
            }
        }
        break;

    case 21:
        if (name)
            return _("トランプの刃", "Trump Branding");
        if (desc)
            return _("武器にトランプの属性をつける。", "Makes current weapon a Trump weapon.");

        {
            if (cast) {
                brand_weapon(caster_ptr, 5);
            }
        }
        break;

    case 22:
        if (name)
            return _("人間トランプ", "Living Trump");
        if (desc)
            return _("ランダムにテレポートする突然変異か、自分の意思でテレポートする突然変異が身につく。",
                "Gives mutation which makes you teleport randomly or makes you able to teleport at will.");
        if (cast)
            become_living_trump(caster_ptr);
        break;

    case 23:
        if (name)
            return _("サイバーデーモンのカード", "Trump Cyberdemon");
        if (desc)
            return _("1体のサイバーデーモンを召喚する。", "Summons a cyber demon.");

        {
            if (cast || fail) {
                msg_print(_("あなたはサイバーデーモンのカードに集中する...", "You concentrate on the trump of a Cyberdemon..."));
                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, 0, SUMMON_CYBER, 0L)) {
                    if (fail) {
                        msg_print(_("召喚されたサイバーデーモンは怒っている！", "The summoned Cyberdemon gets angry!"));
                    }
                }
            }
        }
        break;

    case 24:
        if (name)
            return _("予見のカード", "Trump Divination");
        if (desc)
            return _("近くの全てのモンスター、罠、扉、階段、財宝、そしてアイテムを感知する。",
                "Detects all monsters, traps, doors, stairs, treasures and items in your vicinity.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_all(caster_ptr, rad);
            }
        }
        break;

    case 25:
        if (name)
            return _("知識のカード", "Trump Lore");
        if (desc)
            return _("アイテムの持つ能力を完全に知る。", "*Identifies* an item.");

        {
            if (cast) {
                if (!identify_fully(caster_ptr, false))
                    return nullptr;
            }
        }
        break;

    case 26:
        if (name)
            return _("回復モンスター", "Heal Monster");
        if (desc)
            return _("モンスター1体の体力を回復させる。", "Heals a monster.");

        {
            int heal = plev * 10 + 200;

            if (info)
                return info_heal(0, 0, heal);

            if (cast) {
                bool result;

                /* Temporary enable target_pet option */
                bool old_target_pet = target_pet;
                target_pet = true;

                result = get_aim_dir(caster_ptr, &dir);

                /* Restore target_pet option */
                target_pet = old_target_pet;

                if (!result)
                    return nullptr;

                heal_monster(caster_ptr, dir, heal);
            }
        }
        break;

    case 27:
        if (name)
            return _("ドラゴンのカード", "Trump Dragon");
        if (desc)
            return _("1体のドラゴンを召喚する。", "Summons a dragon.");

        {
            if (cast || fail) {
                msg_print(_("あなたはドラゴンのカードに集中する...", "You concentrate on the trump of a dragon..."));
                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, 0, SUMMON_DRAGON, 0L)) {
                    if (fail) {
                        msg_print(_("召喚されたドラゴンは怒っている！", "The summoned dragon gets angry!"));
                    }
                }
            }
        }
        break;

    case 28:
        if (name)
            return _("隕石のカード", "Trump Meteor");
        if (desc)
            return _("自分の周辺に隕石を落とす。", "Causes meteorites to fall down on nearby random locations.");

        {
            HIT_POINT dam = plev * 2;
            POSITION rad = 2;

            if (info)
                return info_multi_damage(dam);

            if (cast) {
                cast_meteor(caster_ptr, dam, rad);
            }
        }
        break;

    case 29:
        if (name)
            return _("デーモンのカード", "Trump Demon");
        if (desc)
            return _("1体の悪魔を召喚する。", "Summons a demon.");

        {
            if (cast || fail) {
                msg_print(_("あなたはデーモンのカードに集中する...", "You concentrate on the trump of a demon..."));
                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, 0, SUMMON_DEMON, 0L)) {
                    if (fail) {
                        msg_print(_("召喚されたデーモンは怒っている！", "The summoned demon gets angry!"));
                    }
                }
            }
        }
        break;

    case 30:
        if (name)
            return _("地獄のカード", "Trump Greater Undead");
        if (desc)
            return _("1体の上級アンデッドを召喚する。", "Summons a greater undead.");

        {
            if (cast || fail) {
                msg_print(_("あなたは強力なアンデッドのカードに集中する...", "You concentrate on the trump of a greater undead being..."));
                /* May allow unique depend on level and dice roll */
                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, 0, SUMMON_HI_UNDEAD, PM_ALLOW_UNIQUE)) {
                    if (fail) {
                        msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead creature gets angry!"));
                    }
                }
            }
        }
        break;

    case 31:
        if (name)
            return _("古代ドラゴンのカード", "Trump Ancient Dragon");
        if (desc)
            return _("1体の古代ドラゴンを召喚する。", "Summons an ancient dragon.");

        {
            if (cast) {
                summon_type type;

                if (caster_ptr->pclass == CLASS_BEASTMASTER)
                    type = SUMMON_HI_DRAGON_LIVING;
                else
                    type = SUMMON_HI_DRAGON;

                msg_print(_("あなたは古代ドラゴンのカードに集中する...", "You concentrate on the trump of an ancient dragon..."));
                /* May allow unique depend on level and dice roll */
                if (trump_summoning(caster_ptr, 1, !fail, caster_ptr->y, caster_ptr->x, 0, type, PM_ALLOW_UNIQUE)) {
                    if (fail) {
                        msg_print(_("召喚された古代ドラゴンは怒っている！", "The summoned ancient dragon gets angry!"));
                    }
                }
            }
        }
        break;
    }

    return "";
}
