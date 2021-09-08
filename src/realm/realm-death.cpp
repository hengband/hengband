#include "realm/realm-death.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "hpmp/hp-mp-processor.h"
#include "player-base/player-class.h"
#include "player-info/race-info.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell/spell-types.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"

/*!
 * @brief 暗黒領域魔法の各処理を行う
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST)
 * @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST時はnullptr文字列を返す。
 */
concptr do_death_spell(player_type *caster_ptr, SPELL_IDX spell, spell_type mode)
{
    bool name = (mode == SPELL_NAME) ? true : false;
    bool desc = (mode == SPELL_DESC) ? true : false;
    bool info = (mode == SPELL_INFO) ? true : false;
    bool cast = (mode == SPELL_CAST) ? true : false;

    DIRECTION dir;
    PLAYER_LEVEL plev = caster_ptr->lev;

    switch (spell) {
    case 0:
        if (name)
            return _("無生命感知", "Detect Unlife");
        if (desc)
            return _("近くの生命のないモンスターを感知する。", "Detects all nonliving monsters in your vicinity.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_monsters_nonliving(caster_ptr, rad);
            }
        }
        break;

    case 1:
        if (name)
            return _("呪殺弾", "Malediction");
        if (desc)
            return _("ごく小さな邪悪な力を持つボールを放つ。善良なモンスターには大きなダメージを与える。",
                "Fires a tiny ball of evil power which hurts good monsters greatly.");

        {
            DICE_NUMBER dice = 3 + (plev - 1) / 5;
            DICE_SID sides = 4;
            POSITION rad = 0;

            if (info)
                return info_damage(dice, sides, 0);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                /*
                 * A radius-0 ball may (1) be aimed at
                 * objects etc., and will affect them;
                 * (2) may be aimed at ANY visible
                 * monster, unlike a 'bolt' which must
                 * travel to the monster.
                 */

                fire_ball(caster_ptr, GF_HELL_FIRE, dir, damroll(dice, sides), rad);

                if (one_in_(5)) {
                    /* Special effect first */
                    int effect = randint1(1000);

                    if (effect == 666)
                        fire_ball_hide(caster_ptr, GF_DEATH_RAY, dir, plev * 200, 0);
                    else if (effect < 500)
                        fire_ball_hide(caster_ptr, GF_TURN_ALL, dir, plev, 0);
                    else if (effect < 800)
                        fire_ball_hide(caster_ptr, GF_OLD_CONF, dir, plev, 0);
                    else
                        fire_ball_hide(caster_ptr, GF_STUN, dir, plev, 0);
                }
            }
        }
        break;

    case 2:
        if (name)
            return _("邪悪感知", "Detect Evil");
        if (desc)
            return _("近くの邪悪なモンスターを感知する。", "Detects all evil monsters in your vicinity.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_monsters_evil(caster_ptr, rad);
            }
        }
        break;

    case 3:
        if (name)
            return _("悪臭雲", "Stinking Cloud");
        if (desc)
            return _("毒の球を放つ。", "Fires a ball of poison.");

        {
            HIT_POINT dam = 10 + plev / 2;
            POSITION rad = 2;

            if (info)
                return info_damage(0, 0, dam);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                fire_ball(caster_ptr, GF_POIS, dir, dam, rad);
            }
        }
        break;

    case 4:
        if (name)
            return _("黒い眠り", "Black Sleep");
        if (desc)
            return _("1体のモンスターを眠らせる。抵抗されると無効。", "Attempts to put a monster to sleep.");

        {
            int power = plev;

            if (info)
                return info_power(power);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                sleep_monster(caster_ptr, dir, plev);
            }
        }
        break;

    case 5:
        if (name)
            return _("耐毒", "Resist Poison");
        if (desc)
            return _("一定時間、毒への耐性を得る。装備による耐性に累積する。",
                "Gives resistance to poison. This resistance can be added to that from equipment for more powerful resistance.");

        {
            int base = 20;

            if (info)
                return info_duration(base, base);

            if (cast) {
                set_oppose_pois(caster_ptr, randint1(base) + base, false);
            }
        }
        break;

    case 6:
        if (name)
            return _("恐慌", "Horrify");
        if (desc)
            return _("モンスター1体を恐怖させ、朦朧させる。抵抗されると無効。", "Attempts to scare and stun a monster.");

        {
            int power = plev;

            if (info)
                return info_power(power);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                fear_monster(caster_ptr, dir, plev);
                stun_monster(caster_ptr, dir, plev);
            }
        }
        break;

    case 7:
        if (name)
            return _("アンデッド従属", "Enslave Undead");
        if (desc)
            return _("アンデッド1体を魅了する。抵抗されると無効。", "Attempts to charm an undead monster.");

        {
            int power = plev;

            if (info)
                return info_power(power);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                control_one_undead(caster_ptr, dir, plev);
            }
        }
        break;

    case 8:
        if (name)
            return _("エントロピーの球", "Orb of Entropy");
        if (desc)
            return _("生命のある者のHPと最大HP双方にダメージを与える効果のある球を放つ。", "Fires a ball which reduces both HP and MaxHP of living monsters.");

        {
            DICE_NUMBER dice = 3;
            DICE_SID sides = 6;
            POSITION rad = (plev < 30) ? 2 : 3;
            int base;

            if (PlayerClass(caster_ptr).is_wizard())
                base = plev + plev / 2;
            else
                base = plev + plev / 4;

            if (info)
                return info_damage(dice, sides, base);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                fire_ball(caster_ptr, GF_HYPODYNAMIA, dir, damroll(dice, sides) + base, rad);
            }
        }
        break;

    case 9:
        if (name)
            return _("地獄の矢", "Nether Bolt");
        if (desc)
            return _("地獄のボルトもしくはビームを放つ。", "Fires a bolt or beam of nether.");

        {
            DICE_NUMBER dice = 8 + (plev - 5) / 4;
            DICE_SID sides = 8;

            if (info)
                return info_damage(dice, sides, 0);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                fire_bolt_or_beam(caster_ptr, beam_chance(caster_ptr), GF_NETHER, dir, damroll(dice, sides));
            }
        }
        break;

    case 10:
        if (name)
            return _("殺戮雲", "Cloud kill");
        if (desc)
            return _("自分を中心とした毒の球を発生させる。", "Generates a ball of poison centered on you.");

        {
            HIT_POINT dam = (30 + plev) * 2;
            POSITION rad = plev / 10 + 2;

            if (info)
                return info_damage(0, 0, dam / 2);

            if (cast) {
                project(caster_ptr, 0, rad, caster_ptr->y, caster_ptr->x, dam, GF_POIS, PROJECT_KILL | PROJECT_ITEM);
            }
        }
        break;

    case 11:
        if (name)
            return _("モンスター消滅", "Genocide One");
        if (desc)
            return _("モンスター1体を消し去る。経験値やアイテムは手に入らない。抵抗されると無効。", "Attempts to eradicate one monster.");

        {
            int power = plev + 50;

            if (info)
                return info_power(power);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                fire_ball_hide(caster_ptr, GF_GENOCIDE, dir, power, 0);
            }
        }
        break;

    case 12:
        if (name)
            return _("毒の刃", "Poison Branding");
        if (desc)
            return _("武器に毒の属性をつける。", "Makes current weapon poison branded.");

        {
            if (cast) {
                brand_weapon(caster_ptr, 3);
            }
        }
        break;

    case 13:
        if (name)
            return _("吸血の矢", "Vampiric Bolt");
        if (desc)
            return _("ボルトによりモンスター1体から生命力を吸いとる。吸いとった生命力によって満腹度が上がる。",
                "Fires a bolt which transfers HP from a monster to you. You will also gain nutritional sustenance from this.");

        {
            DICE_NUMBER dice = 1;
            DICE_SID sides = plev * 2;
            int base = plev * 2;

            if (info)
                return info_damage(dice, sides, base);

            if (cast) {
                HIT_POINT dam = base + damroll(dice, sides);

                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                if (hypodynamic_bolt(caster_ptr, dir, dam)) {
                    chg_virtue(caster_ptr, V_SACRIFICE, -1);
                    chg_virtue(caster_ptr, V_VITALITY, -1);

                    hp_player(caster_ptr, dam);

                    /*
                     * Gain nutritional sustenance:
                     * 150/hp drained
                     *
                     * A Food ration gives 5000
                     * food points (by contrast)
                     * Don't ever get more than
                     * "Full" this way But if we
                     * ARE Gorged, it won't cure
                     * us
                     */
                    dam = caster_ptr->food + MIN(5000, 100 * dam);

                    /* Not gorged already */
                    if (caster_ptr->food < PY_FOOD_MAX)
                        set_food(caster_ptr, dam >= PY_FOOD_MAX ? PY_FOOD_MAX - 1 : dam);
                }
            }
        }
        break;

    case 14:
        if (name)
            return _("反魂の術", "Animate dead");
        if (desc)
            return _("周囲の死体や骨を生き返す。", "Resurrects nearby corpses and skeletons. And makes them your pets.");

        {
            if (cast) {
                animate_dead(caster_ptr, 0, caster_ptr->y, caster_ptr->x);
            }
        }
        break;

    case 15:
        if (name)
            return _("抹殺", "Genocide");
        if (desc)
            return _("指定した文字のモンスターを現在の階から消し去る。抵抗されると無効。",
                "Eliminates an entire class of monster, exhausting you. Powerful or unique monsters may resist.");

        {
            int power = plev + 50;

            if (info)
                return info_power(power);

            if (cast) {
                symbol_genocide(caster_ptr, power, true);
            }
        }
        break;

    case 16:
        if (name)
            return _("狂戦士化", "Berserk");
        if (desc)
            return _("狂戦士化し、恐怖を除去する。", "Gives a bonus to hit and HP, immunity to fear for a while. But decreases AC.");

        {
            int base = 25;

            if (info)
                return info_duration(base, base);

            if (cast) {
                (void)berserk(caster_ptr, base + randint1(base));
            }
        }
        break;

    case 17:
        if (name)
            return _("悪霊召喚", "Invoke Spirits");
        if (desc)
            return _("ランダムで様々な効果が起こる。", "Causes random effects.");

        {
            if (info)
                return KWD_RANDOM;

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                cast_invoke_spirits(caster_ptr, dir);
            }
        }
        break;

    case 18:
        if (name)
            return _("暗黒の矢", "Dark Bolt");
        if (desc)
            return _("暗黒のボルトもしくはビームを放つ。", "Fires a bolt or beam of darkness.");

        {
            DICE_NUMBER dice = 4 + (plev - 5) / 4;
            DICE_SID sides = 8;

            if (info)
                return info_damage(dice, sides, 0);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                fire_bolt_or_beam(caster_ptr, beam_chance(caster_ptr), GF_DARK, dir, damroll(dice, sides));
            }
        }
        break;

    case 19:
        if (name)
            return _("狂乱戦士", "Battle Frenzy");
        if (desc)
            return _("狂戦士化し、恐怖を除去し、加速する。", "Gives another bonus to hit and HP, immunity to fear for a while. Hastes you. But decreases AC.");

        {
            int b_base = 25;
            int sp_base = plev / 2;
            int sp_sides = 20 + plev / 2;

            if (info)
                return info_duration(b_base, b_base);

            if (cast) {
                (void)berserk(caster_ptr, b_base + randint1(b_base));
                set_fast(caster_ptr, randint1(sp_sides) + sp_base, false);
            }
        }
        break;

    case 20:
        if (name)
            return _("吸血の刃", "Vampiric Branding");
        if (desc)
            return _("武器に吸血の属性をつける。", "Makes current weapon Vampiric.");

        {
            if (cast) {
                brand_weapon(caster_ptr, 4);
            }
        }
        break;

    case 21:
        if (name)
            return _("吸血の連矢", "Vampiric Bolts");
        if (desc)
            return _("3連射のボルトによりモンスター1体から生命力を吸いとる。吸いとった生命力によって体力が回復する。",
                "Fires 3 bolts. Each of the bolts absorbs some HP from a monster and gives them to you.");
        {
            HIT_POINT dam = 100;

            if (info)
                return format("%s3*%d", KWD_DAM, dam);

            if (cast) {
                int i;

                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                chg_virtue(caster_ptr, V_SACRIFICE, -1);
                chg_virtue(caster_ptr, V_VITALITY, -1);

                for (i = 0; i < 3; i++) {
                    if (hypodynamic_bolt(caster_ptr, dir, dam))
                        hp_player(caster_ptr, dam);
                }
            }
        }
        break;

    case 22:
        if (name)
            return _("死の言魂", "Nether Wave");
        if (desc)
            return _("視界内の生命のあるモンスターにダメージを与える。", "Damages all living monsters in sight.");

        {
            DICE_SID sides = plev * 3;

            if (info)
                return info_damage(1, sides, 0);

            if (cast) {
                dispel_living(caster_ptr, randint1(sides));
            }
        }
        break;

    case 23:
        if (name)
            return _("暗黒の嵐", "Darkness Storm");
        if (desc)
            return _("巨大な暗黒の球を放つ。", "Fires a huge ball of darkness.");

        {
            HIT_POINT dam = 100 + plev * 2;
            POSITION rad = 4;

            if (info)
                return info_damage(0, 0, dam);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                fire_ball(caster_ptr, GF_DARK, dir, dam, rad);
            }
        }
        break;

    case 24:
        if (name)
            return _("死の光線", "Death Ray");
        if (desc)
            return _("死の光線を放つ。", "Fires a beam of death.");

        {
            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                death_ray(caster_ptr, dir, plev);
            }
        }
        break;

    case 25:
        if (name)
            return _("死者召喚", "Raise the Dead");
        if (desc)
            return _("1体のアンデッドを召喚する。", "Summons an undead monster.");
        if (cast)
            cast_summon_undead(caster_ptr, (plev * 3) / 2);
        break;

    case 26:
        if (name)
            return _("死者の秘伝", "Secrets of the Dead");
        if (desc)
            return _("アイテムを1つ識別する。レベルが高いとアイテムの能力を完全に知ることができる。", "Identifies or, at higher levels, *identifies* an item.");

        {
            if (cast) {
                if (randint1(50) > plev) {
                    if (!ident_spell(caster_ptr, false))
                        return nullptr;
                } else {
                    if (!identify_fully(caster_ptr, false))
                        return nullptr;
                }
            }
        }
        break;

    case 27:
        if (name)
            return _("吸血鬼変化", "Polymorph Vampire");
        if (desc)
            return _("一定時間、吸血鬼に変化する。変化している間は本来の種族の能力を失い、代わりに吸血鬼としての能力を得る。",
                "Causes you to mimic a vampire for a while. You lose the abilities of your original race and get the abilities of a vampire for that time.");

        {
            int base = 10 + plev / 2;

            if (info)
                return info_duration(base, base);

            if (cast) {
                set_mimic(caster_ptr, base + randint1(base), MIMIC_VAMPIRE, false);
            }
        }
        break;

    case 28:
        if (name)
            return _("経験値復活", "Restore Life");
        if (desc)
            return _("失った経験値を回復する。", "Restores lost experience.");

        {
            if (cast) {
                restore_level(caster_ptr);
            }
        }
        break;

    case 29:
        if (name)
            return _("周辺抹殺", "Mass Genocide");
        if (desc)
            return _("自分の周囲にいるモンスターを現在の階から消し去る。抵抗されると無効。",
                "Eliminates all nearby monsters, exhausting you. Powerful or unique monsters may be able to resist.");

        {
            int power = plev + 50;

            if (info)
                return info_power(power);

            if (cast) {
                mass_genocide(caster_ptr, power, true);
            }
        }
        break;

    case 30:
        if (name)
            return _("地獄の劫火", "Hellfire");
        if (desc)
            return _(
                "邪悪な力を持つ宝珠を放つ。善良なモンスターには大きなダメージを与える。", "Fires a powerful ball of evil power. Hurts good monsters greatly.");

        {
            HIT_POINT dam = 666;
            POSITION rad = 3;

            if (info)
                return info_damage(0, 0, dam);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return nullptr;

                fire_ball(caster_ptr, GF_HELL_FIRE, dir, dam, rad);
                take_hit(caster_ptr, DAMAGE_USELIFE, 20 + randint1(30), _("地獄の劫火の呪文を唱えた疲労", "the strain of casting Hellfire"));
            }
        }
        break;

    case 31:
        if (name)
            return _("幽体化", "Wraithform");
        if (desc)
            return _("一定時間、壁を通り抜けることができ受けるダメージが軽減される幽体の状態に変身する。",
                "Causes you to be ghost-like for a while. That reduces the damage you take and allows you to pass through walls.");

        {
            int base = plev / 2;

            if (info)
                return info_duration(base, base);

            if (cast) {
                set_wraith_form(caster_ptr, randint1(base) + base, false);
            }
        }
        break;
    }

    return "";
}
