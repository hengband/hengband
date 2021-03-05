﻿#include "cmd-action/cmd-spell.h"
#include "core/hp-mp-processor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "player/attack-defense-types.h"
#include "player/player-class.h"
#include "realm/realm-song-numbers.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-world.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/experience.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 歌の開始を処理する / Start singing if the player is a Bard
 * @param spell 領域魔法としてのID
 * @param song 魔法効果のID
 * @return なし
 */
static void start_singing(player_type *caster_ptr, SPELL_IDX spell, MAGIC_NUM1 song)
{
    /* Remember the song index */
    SINGING_SONG_EFFECT(caster_ptr) = (MAGIC_NUM1)song;

    /* Remember the index of the spell which activated the song */
    SINGING_SONG_ID(caster_ptr) = (MAGIC_NUM2)spell;

    /* Now the player is singing */
    set_action(caster_ptr, ACTION_SING);

    caster_ptr->update |= (PU_BONUS);
    caster_ptr->redraw |= (PR_STATUS);
}

/*!
 * @brief 歌の各処理を行う
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param spell 歌ID
 * @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST / SPELL_FAIL / SPELL_CONT / SPELL_STOP)
 * @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST / SPELL_FAIL / SPELL_CONT / SPELL_STOP 時はNULL文字列を返す。
 */
concptr do_music_spell(player_type *caster_ptr, SPELL_IDX spell, spell_type mode)
{
    bool name = (mode == SPELL_NAME) ? TRUE : FALSE;
    bool desc = (mode == SPELL_DESC) ? TRUE : FALSE;
    bool info = (mode == SPELL_INFO) ? TRUE : FALSE;
    bool cast = (mode == SPELL_CAST) ? TRUE : FALSE;
    bool fail = (mode == SPELL_FAIL) ? TRUE : FALSE;
    bool cont = (mode == SPELL_CONT) ? TRUE : FALSE;
    bool stop = (mode == SPELL_STOP) ? TRUE : FALSE;

    DIRECTION dir;
    PLAYER_LEVEL plev = caster_ptr->lev;

    switch (spell) {
    case 0:
        if (name)
            return _("遅鈍の歌", "Song of Holding");
        if (desc)
            return _("視界内の全てのモンスターを減速させる。抵抗されると無効。", "Attempts to slow all monsters in sight.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("ゆっくりとしたメロディを口ずさみ始めた．．．", "You start humming a slow, steady melody..."));
            start_singing(caster_ptr, spell, MUSIC_SLOW);
        }

        {
            POWER power = plev;

            if (info)
                return info_power(power);

            if (cont) {
                slow_monsters(caster_ptr, plev);
            }
        }
        break;

    case 1:
        if (name)
            return _("祝福の歌", "Song of Blessing");
        if (desc)
            return _("命中率とACのボーナスを得る。", "Gives a bonus to hit and AC for a few turns.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("厳かなメロディを奏で始めた．．．", "The holy power of the Music of the Ainur enters you..."));
            start_singing(caster_ptr, spell, MUSIC_BLESS);
        }

        if (stop) {
            if (!caster_ptr->blessed) {
                msg_print(_("高潔な気分が消え失せた。", "The prayer has expired."));
            }
        }

        break;

    case 2:
        if (name)
            return _("崩壊の音色", "Wrecking Note");
        if (desc)
            return _("轟音のボルトを放つ。", "Fires a bolt of sound.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        {
            DICE_NUMBER dice = 4 + (plev - 1) / 5;
            DICE_SID sides = 4;

            if (info)
                return info_damage(dice, sides, 0);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return NULL;

                fire_bolt(caster_ptr, GF_SOUND, dir, damroll(dice, sides));
            }
        }
        break;

    case 3:
        if (name)
            return _("朦朧の旋律", "Stun Pattern");
        if (desc)
            return _("視界内の全てのモンスターを朦朧させる。抵抗されると無効。", "Attempts to stun all monsters in sight.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("眩惑させるメロディを奏で始めた．．．", "You weave a pattern of sounds to bewilder and daze..."));
            start_singing(caster_ptr, spell, MUSIC_STUN);
        }

        {
            DICE_NUMBER dice = plev / 10;
            DICE_SID sides = 2;

            if (info)
                return info_power_dice(dice, sides);

            if (cont) {
                stun_monsters(caster_ptr, damroll(dice, sides));
            }
        }

        break;

    case 4:
        if (name)
            return _("生命の流れ", "Flow of Life");
        if (desc)
            return _("体力を少し回復させる。", "Heals HP a little.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("歌を通して体に活気が戻ってきた．．．", "Life flows through you as you sing a song of healing..."));
            start_singing(caster_ptr, spell, MUSIC_L_LIFE);
        }

        {
            DICE_NUMBER dice = 2;
            DICE_SID sides = 6;

            if (info)
                return info_heal(dice, sides, 0);

            if (cont) {
                hp_player(caster_ptr, damroll(dice, sides));
            }
        }

        break;

    case 5:
        if (name)
            return _("太陽の歌", "Song of the Sun");
        if (desc)
            return _("光源が照らしている範囲か部屋全体を永久に明るくする。", "Lights up nearby area and the inside of a room permanently.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        {
            DICE_NUMBER dice = 2;
            DICE_SID sides = plev / 2;
            POSITION rad = plev / 10 + 1;

            if (info)
                return info_damage(dice, sides, 0);

            if (cast) {
                msg_print(_("光り輝く歌が辺りを照らした。", "Your uplifting song brings brightness to dark places..."));
                lite_area(caster_ptr, damroll(dice, sides), rad);
            }
        }
        break;

    case 6:
        if (name)
            return _("恐怖の歌", "Song of Fear");
        if (desc)
            return _("視界内の全てのモンスターを恐怖させる。抵抗されると無効。", "Attempts to scare all monsters in sight.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("おどろおどろしいメロディを奏で始めた．．．", "You start weaving a fearful pattern..."));
            start_singing(caster_ptr, spell, MUSIC_FEAR);
        }

        {
            POWER power = plev;

            if (info)
                return info_power(power);

            if (cont) {
                project_all_los(caster_ptr, GF_TURN_ALL, power);
            }
        }

        break;

    case 7:
        if (name)
            return _("戦いの歌", "Heroic Ballad");
        if (desc)
            return _("ヒーロー気分になる。", "Removes fear. Gives a bonus to hit for a while. Heals you for 10 HP.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("激しい戦いの歌を歌った．．．", "You start singing a song of intense fighting..."));

            (void)hp_player(caster_ptr, 10);
            (void)set_afraid(caster_ptr, 0);

            /* Recalculate hitpoints */
            caster_ptr->update |= (PU_HP);

            start_singing(caster_ptr, spell, MUSIC_HERO);
        }

        if (stop) {
            if (!caster_ptr->hero) {
                msg_print(_("ヒーローの気分が消え失せた。", "The heroism wears off."));
                /* Recalculate hitpoints */
                caster_ptr->update |= (PU_HP);
            }
        }

        break;

    case 8:
        if (name)
            return _("霊的知覚", "Clairaudience");
        if (desc)
            return _("近くの罠/扉/"
                     "階段を感知する。レベル15で全てのモンスター、20で財宝とアイテムを感知できるようになる。レベル25で周辺の地形を感知し、40でその階全体を永久"
                     "に照らし、ダンジョン内のすべてのアイテムを感知する。この効果は歌い続けることで順に起こる。",
                "Detects traps, doors and stairs in your vicinity. And detects all monsters at level 15, treasures and items at level 20. Maps nearby area at "
                "level 25. Lights and know the whole level at level 40. These effects accumulate as the song continues.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("静かな音楽が感覚を研ぎ澄まさせた．．．", "Your quiet music sharpens your sense of hearing..."));
            /* Hack -- Initialize the turn count */
            SINGING_COUNT(caster_ptr) = 0;
            start_singing(caster_ptr, spell, MUSIC_DETECT);
        }

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cont) {
                int count = SINGING_COUNT(caster_ptr);

                if (count >= 19)
                    wiz_lite(caster_ptr, FALSE);
                if (count >= 11) {
                    map_area(caster_ptr, rad);
                    if (plev > 39 && count < 19)
                        SINGING_COUNT(caster_ptr) = count + 1;
                }
                if (count >= 6) {
                    /* There are too many hidden treasure.  So... */
                    /* detect_treasure(rad); */
                    detect_objects_gold(caster_ptr, rad);
                    detect_objects_normal(caster_ptr, rad);

                    if (plev > 24 && count < 11)
                        SINGING_COUNT(caster_ptr) = count + 1;
                }
                if (count >= 3) {
                    detect_monsters_invis(caster_ptr, rad);
                    detect_monsters_normal(caster_ptr, rad);

                    if (plev > 19 && count < A_MAX)
                        SINGING_COUNT(caster_ptr) = count + 1;
                }
                detect_traps(caster_ptr, rad, TRUE);
                detect_doors(caster_ptr, rad);
                detect_stairs(caster_ptr, rad);

                if (plev > 14 && count < 3)
                    SINGING_COUNT(caster_ptr) = count + 1;
            }
        }

        break;

    case 9:
        if (name)
            return _("魂の歌", "Soul Shriek");
        if (desc)
            return _("視界内の全てのモンスターに対して精神攻撃を行う。", "Damages all monsters in sight with PSI damages.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("精神を捻じ曲げる歌を歌った．．．", "You start singing a song of soul in pain..."));
            start_singing(caster_ptr, spell, MUSIC_PSI);
        }

        {
            DICE_NUMBER dice = 1;
            DICE_SID sides = plev * 3 / 2;

            if (info)
                return info_damage(dice, sides, 0);

            if (cont) {
                project_all_los(caster_ptr, GF_PSI, damroll(dice, sides));
            }
        }

        break;

    case 10:
        if (name)
            return _("知識の歌", "Song of Lore");
        if (desc)
            return _("自分のいるマスと隣りのマスに落ちているアイテムを鑑定する。", "Identifies all items which are in the adjacent squares.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("この世界の知識が流れ込んできた．．．", "You recall the rich lore of the world..."));
            start_singing(caster_ptr, spell, MUSIC_ID);
        }

        {
            POSITION rad = 1;

            if (info)
                return info_radius(rad);

            /*
             * 歌の開始時にも効果発動：
             * MP不足で鑑定が発動される前に歌が中断してしまうのを防止。
             */
            if (cont || cast) {
                project(caster_ptr, 0, rad, caster_ptr->y, caster_ptr->x, 0, GF_IDENTIFY, PROJECT_ITEM, -1);
            }
        }

        break;

    case 11:
        if (name)
            return _("隠遁の歌", "Hiding Tune");
        if (desc)
            return _("隠密行動能力を上昇させる。", "Gives improved stealth.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("あなたの姿が景色にとけこんでいった．．．", "Your song carries you beyond the sight of mortal eyes..."));
            start_singing(caster_ptr, spell, MUSIC_STEALTH);
        }

        if (stop) {
            if (!caster_ptr->tim_stealth) {
                msg_print(_("姿がはっきりと見えるようになった。", "You are no longer hidden."));
            }
        }

        break;

    case 12:
        if (name)
            return _("幻影の旋律", "Illusion Pattern");
        if (desc)
            return _("視界内の全てのモンスターを混乱させる。抵抗されると無効。", "Attempts to confuse all monsters in sight.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("辺り一面に幻影が現れた．．．", "You weave a pattern of sounds to beguile and confuse..."));
            start_singing(caster_ptr, spell, MUSIC_CONF);
        }

        {
            POWER power = plev * 2;

            if (info)
                return info_power(power);

            if (cont) {
                confuse_monsters(caster_ptr, power);
            }
        }

        break;

    case 13:
        if (name)
            return _("破滅の叫び", "Doomcall");
        if (desc)
            return _("視界内の全てのモンスターに対して轟音攻撃を行う。", "Damages all monsters in sight with booming sound.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("轟音が響いた．．．", "The fury of the Downfall of Numenor lashes out..."));
            start_singing(caster_ptr, spell, MUSIC_SOUND);
        }

        {
            DICE_NUMBER dice = 10 + plev / 5;
            DICE_SID sides = 7;

            if (info)
                return info_damage(dice, sides, 0);

            if (cont) {
                project_all_los(caster_ptr, GF_SOUND, damroll(dice, sides));
            }
        }

        break;

    case 14:
        if (name)
            return _("フィリエルの歌", "Firiel's Song");
        if (desc)
            return _("周囲の死体や骨を生き返す。", "Resurrects nearby corpses and skeletons. And makes them your pets.");

        {
            /* Stop singing before start another */
            if (cast || fail)
                stop_singing(caster_ptr);

            if (cast) {
                msg_print(_("生命と復活のテーマを奏で始めた．．．", "The themes of life and revival are woven into your song..."));
                animate_dead(caster_ptr, 0, caster_ptr->y, caster_ptr->x);
            }
        }
        break;

    case 15:
        if (name)
            return _("旅の仲間", "Fellowship Chant");
        if (desc)
            return _("視界内の全てのモンスターを魅了する。抵抗されると無効。", "Attempts to charm all monsters in sight.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("安らかなメロディを奏で始めた．．．", "You weave a slow, soothing melody of imploration..."));
            start_singing(caster_ptr, spell, MUSIC_CHARM);
        }

        {
            DICE_NUMBER dice = 10 + plev / 15;
            DICE_SID sides = 6;

            if (info)
                return info_power_dice(dice, sides);

            if (cont) {
                charm_monsters(caster_ptr, damroll(dice, sides));
            }
        }

        break;

    case 16:
        if (name)
            return _("分解音波", "Sound of disintegration");
        if (desc)
            return _("壁を掘り進む。自分の足元のアイテムは蒸発する。", "Makes you be able to burrow into walls. Objects under your feet evaporate.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("粉砕するメロディを奏で始めた．．．", "You weave a violent pattern of sounds to break walls."));
            start_singing(caster_ptr, spell, MUSIC_WALL);
        }

        {
            /*
             * 歌の開始時にも効果発動：
             * MP不足で効果が発動される前に歌が中断してしまうのを防止。
             */
            if (cont || cast) {
                project(caster_ptr, 0, 0, caster_ptr->y, caster_ptr->x, 0, GF_DISINTEGRATE, PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE, -1);
            }
        }
        break;

    case 17:
        if (name)
            return _("元素耐性", "Finrod's Resistance");
        if (desc)
            return _("酸、電撃、炎、冷気、毒に対する耐性を得る。装備による耐性に累積する。",
                "Gives resistance to fire, cold, electricity, acid and poison. These resistances can be added to those from equipment for more powerful "
                "resistances.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("元素の力に対する忍耐の歌を歌った。", "You sing a song of perseverance against powers..."));
            start_singing(caster_ptr, spell, MUSIC_RESIST);
        }

        if (stop) {
            if (!caster_ptr->oppose_acid) {
                msg_print(_("酸への耐性が薄れた気がする。", "You feel less resistant to acid."));
            }

            if (!caster_ptr->oppose_elec) {
                msg_print(_("電撃への耐性が薄れた気がする。", "You feel less resistant to elec."));
            }

            if (!caster_ptr->oppose_fire) {
                msg_print(_("火への耐性が薄れた気がする。", "You feel less resistant to fire."));
            }

            if (!caster_ptr->oppose_cold) {
                msg_print(_("冷気への耐性が薄れた気がする。", "You feel less resistant to cold."));
            }

            if (!caster_ptr->oppose_pois) {
                msg_print(_("毒への耐性が薄れた気がする。", "You feel less resistant to pois."));
            }
        }

        break;

    case 18:
        if (name)
            return _("ホビットのメロディ", "Hobbit Melodies");
        if (desc)
            return _("加速する。", "Hastes you.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("軽快な歌を口ずさみ始めた．．．", "You start singing a joyful pop song..."));
            start_singing(caster_ptr, spell, MUSIC_SPEED);
        }

        if (stop) {
            if (!caster_ptr->fast) {
                msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            }
        }

        break;

    case 19:
        if (name)
            return _("歪んだ世界", "World Contortion");
        if (desc)
            return _("近くのモンスターをテレポートさせる。抵抗されると無効。", "Teleports all nearby monsters away unless resisted.");

        {
            POSITION rad = plev / 15 + 1;
            POWER power = plev * 3 + 1;

            if (info)
                return info_radius(rad);

            /* Stop singing before start another */
            if (cast || fail)
                stop_singing(caster_ptr);

            if (cast) {
                msg_print(_("歌が空間を歪めた．．．", "Reality whirls wildly as you sing a dizzying melody..."));
                project(caster_ptr, 0, rad, caster_ptr->y, caster_ptr->x, power, GF_AWAY_ALL, PROJECT_KILL, -1);
            }
        }
        break;

    case 20:
        if (name)
            return _("退散の歌", "Dispelling chant");
        if (desc)
            return _("視界内の全てのモンスターにダメージを与える。邪悪なモンスターに特に大きなダメージを与える。",
                "Damages all monsters in sight. Hurts evil monsters greatly.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("耐えられない不協和音が敵を責め立てた．．．", "You cry out in an ear-wracking voice..."));
            start_singing(caster_ptr, spell, MUSIC_DISPEL);
        }

        {
            DICE_SID m_sides = plev * 3;
            DICE_SID e_sides = plev * 3;

            if (info)
                return format("%s1d%d+1d%d", KWD_DAM, m_sides, e_sides);

            if (cont) {
                dispel_monsters(caster_ptr, randint1(m_sides));
                dispel_evil(caster_ptr, randint1(e_sides));
            }
        }
        break;

    case 21:
        if (name)
            return _("サルマンの甘言", "The Voice of Saruman");
        if (desc)
            return _("視界内の全てのモンスターを減速させ、眠らせようとする。抵抗されると無効。", "Attempts to slow and put to sleep all monsters in sight.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("優しく、魅力的な歌を口ずさみ始めた．．．", "You start humming a gentle and attractive song..."));
            start_singing(caster_ptr, spell, MUSIC_SARUMAN);
        }

        {
            POWER power = plev;

            if (info)
                return info_power(power);

            if (cont) {
                slow_monsters(caster_ptr, plev);
                sleep_monsters(caster_ptr, plev);
            }
        }

        break;

    case 22:
        if (name)
            return _("嵐の音色", "Song of the Tempest");
        if (desc)
            return _("轟音のビームを放つ。", "Fires a beam of sound.");

        {
            DICE_NUMBER dice = 15 + (plev - 1) / 2;
            DICE_SID sides = 10;

            if (info)
                return info_damage(dice, sides, 0);

            /* Stop singing before start another */
            if (cast || fail)
                stop_singing(caster_ptr);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return NULL;

                fire_beam(caster_ptr, GF_SOUND, dir, damroll(dice, sides));
            }
        }
        break;

    case 23:
        if (name)
            return _("もう一つの世界", "Ambarkanta");
        if (desc)
            return _("現在の階を再構成する。", "Recreates current dungeon level.");

        {
            int base = 15;
            DICE_SID sides = 20;

            if (info)
                return info_delay(base, sides);

            /* Stop singing before start another */
            if (cast || fail)
                stop_singing(caster_ptr);

            if (cast) {
                msg_print(_("周囲が変化し始めた．．．", "You sing of the primeval shaping of Middle-earth..."));
                reserve_alter_reality(caster_ptr, randint0(sides) + base);
            }
        }
        break;

    case 24:
        if (name)
            return _("破壊の旋律", "Wrecking Pattern");
        if (desc)
            return _(
                "周囲のダンジョンを揺らし、壁と床をランダムに入れ変える。", "Shakes dungeon structure, and results in random swapping of floors and walls.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("破壊的な歌が響きわたった．．．", "You weave a pattern of sounds to contort and shatter..."));
            start_singing(caster_ptr, spell, MUSIC_QUAKE);
        }

        {
            POSITION rad = 10;

            if (info)
                return info_radius(rad);

            if (cont) {
                earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 10, 0);
            }
        }

        break;

    case 25:
        if (name)
            return _("停滞の歌", "Stationary Shriek");
        if (desc)
            return _("視界内の全てのモンスターを麻痺させようとする。抵抗されると無効。", "Attempts to freeze all monsters in sight.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("ゆっくりとしたメロディを奏で始めた．．．", "You weave a very slow pattern which is almost likely to stop..."));
            start_singing(caster_ptr, spell, MUSIC_STASIS);
        }

        {
            POWER power = plev * 4;

            if (info)
                return info_power(power);

            if (cont) {
                stasis_monsters(caster_ptr, power);
            }
        }

        break;

    case 26:
        if (name)
            return _("守りの歌", "Endurance");
        if (desc)
            return _("自分のいる床の上に、モンスターが通り抜けたり召喚されたりすることができなくなるルーンを描く。",
                "Sets a rune on the floor beneath you. If you are on a rune, monsters cannot attack you but can try to break the rune.");

        {
            /* Stop singing before start another */
            if (cast || fail)
                stop_singing(caster_ptr);

            if (cast) {
                msg_print(_("歌が神聖な場を作り出した．．．", "The holy power of the Music is creating sacred field..."));
                create_rune_protection_one(caster_ptr);
            }
        }
        break;

    case 27:
        if (name)
            return _("英雄の詩", "The Hero's Poem");
        if (desc)
            return _("加速し、ヒーロー気分になり、視界内の全てのモンスターにダメージを与える。", "Hastes you. Gives heroism. Damages all monsters in sight.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("英雄の歌を口ずさんだ．．．", "You chant a powerful, heroic call to arms..."));
            (void)hp_player(caster_ptr, 10);
            (void)set_afraid(caster_ptr, 0);

            /* Recalculate hitpoints */
            caster_ptr->update |= (PU_HP);

            start_singing(caster_ptr, spell, MUSIC_SHERO);
        }

        if (stop) {
            if (!caster_ptr->hero) {
                msg_print(_("ヒーローの気分が消え失せた。", "The heroism wears off."));
                /* Recalculate hitpoints */
                caster_ptr->update |= (PU_HP);
            }

            if (!caster_ptr->fast) {
                msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            }
        }

        {
            DICE_NUMBER dice = 1;
            DICE_SID sides = plev * 3;

            if (info)
                return info_damage(dice, sides, 0);

            if (cont) {
                dispel_monsters(caster_ptr, damroll(dice, sides));
            }
        }
        break;

    case 28:
        if (name)
            return _("ヤヴァンナの助け", "Relief of Yavanna");
        if (desc)
            return _("強力な回復の歌で、負傷と朦朧状態も全快する。", "Powerful healing song. Also completely heals cuts and being stunned.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("歌を通して体に活気が戻ってきた．．．", "Life flows through you as you sing the song..."));
            start_singing(caster_ptr, spell, MUSIC_H_LIFE);
        }

        {
            DICE_NUMBER dice = 15;
            DICE_SID sides = 10;

            if (info)
                return info_heal(dice, sides, 0);

            if (cont) {
                hp_player(caster_ptr, damroll(dice, sides));
                set_stun(caster_ptr, 0);
                set_cut(caster_ptr, 0);
            }
        }

        break;

    case 29:
        if (name)
            return _("再生の歌", "Goddess's rebirth");
        if (desc)
            return _("すべてのステータスと経験値を回復する。", "Restores all stats and experience.");

        {
            /* Stop singing before start another */
            if (cast || fail)
                stop_singing(caster_ptr);

            if (cast) {
                msg_print(
                    _("暗黒の中に光と美をふりまいた。体が元の活力を取り戻した。", "You strew light and beauty in the dark as you sing. You feel refreshed."));
                (void)restore_all_status(caster_ptr);
                (void)restore_level(caster_ptr);
            }
        }
        break;

    case 30:
        if (name)
            return _("サウロンの魔術", "Wizardry of Sauron");
        if (desc)
            return _("非常に強力でごく小さい轟音の球を放つ。", "Fires an extremely powerful tiny ball of sound.");

        {
            DICE_NUMBER dice = 50 + plev;
            DICE_SID sides = 10;
            POSITION rad = 0;

            if (info)
                return info_damage(dice, sides, 0);

            /* Stop singing before start another */
            if (cast || fail)
                stop_singing(caster_ptr);

            if (cast) {
                if (!get_aim_dir(caster_ptr, &dir))
                    return NULL;

                fire_ball(caster_ptr, GF_SOUND, dir, damroll(dice, sides), rad);
            }
        }
        break;

    case 31:
        if (name)
            return _("フィンゴルフィンの挑戦", "Fingolfin's Challenge");
        if (desc)
            return _("ダメージを受けなくなるバリアを張る。", "Generates a barrier which completely protects you from almost all damage.");

        /* Stop singing before start another */
        if (cast || fail)
            stop_singing(caster_ptr);

        if (cast) {
            msg_print(_("フィンゴルフィンの冥王への挑戦を歌った．．．", "You recall the valor of Fingolfin's challenge to the Dark Lord..."));

            caster_ptr->redraw |= (PR_MAP);
            caster_ptr->update |= (PU_MONSTERS);
            caster_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);

            start_singing(caster_ptr, spell, MUSIC_INVULN);
        }

        if (stop) {
            if (!caster_ptr->invuln) {
                msg_print(_("無敵ではなくなった。", "The invulnerability wears off."));

                caster_ptr->redraw |= (PR_MAP);
                caster_ptr->update |= (PU_MONSTERS);
                caster_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
            }
        }

        break;
    }

    return "";
}
