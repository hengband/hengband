#include "cmd-action/cmd-spell.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "hpmp/hp-mp-processor.h"
#include "player-info/class-info.h"
#include "player/attack-defense-types.h"
#include "player/player-status.h"
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
#include "spell-realm/spells-song.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/experience.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 歌の開始を処理する / Start singing if the player is a Bard
 * @param spell 領域魔法としてのID
 * @param song 魔法効果のID
 */
static void start_singing(PlayerType *player_ptr, SPELL_IDX spell, int32_t song)
{
    /* Remember the song index */
    set_singing_song_effect(player_ptr, song);

    /* Remember the index of the spell which activated the song */
    set_singing_song_id(player_ptr, (byte)spell);

    /* Now the player is singing */
    set_action(player_ptr, ACTION_SING);

    player_ptr->update |= (PU_BONUS);
    player_ptr->redraw |= (PR_STATUS);
}

/*!
 * @brief 歌の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 歌ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST / SpellProcessType::FAIL / SPELL_CONT / SpellProcessType::STOP)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列ポインタを返す。SpellProcessType::CAST / SpellProcessType::FAIL / SPELL_CONT / SpellProcessType::STOP 時はnullptr文字列を返す。
 */
concptr do_music_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool name = mode == SpellProcessType::NAME;
    bool desc = mode == SpellProcessType::DESCRIPTION;
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;
    bool fail = mode == SpellProcessType::FAIL;
    bool cont = mode == SpellProcessType::CONTNUATION;
    bool stop = mode == SpellProcessType::STOP;

    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0:
        if (name) {
            return _("遅鈍の歌", "Song of Holding");
        }
        if (desc) {
            return _("視界内の全てのモンスターを減速させる。抵抗されると無効。", "Attempts to slow all monsters in sight.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("ゆっくりとしたメロディを口ずさみ始めた．．．", "You start humming a slow, steady melody..."));
            start_singing(player_ptr, spell, MUSIC_SLOW);
        }

        {
            POWER power = plev;

            if (info) {
                return info_power(power);
            }

            if (cont) {
                slow_monsters(player_ptr, plev);
            }
        }
        break;

    case 1:
        if (name) {
            return _("祝福の歌", "Song of Blessing");
        }
        if (desc) {
            return _("命中率とACのボーナスを得る。", "Gives a bonus to hit and AC for a few turns.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("厳かなメロディを奏で始めた．．．", "The holy power of the Music of the Ainur enters you..."));
            start_singing(player_ptr, spell, MUSIC_BLESS);
        }

        if (stop) {
            if (!player_ptr->blessed) {
                msg_print(_("高潔な気分が消え失せた。", "The prayer has expired."));
            }
        }

        break;

    case 2:
        if (name) {
            return _("崩壊の音色", "Wrecking Note");
        }
        if (desc) {
            return _("轟音のボルトを放つ。", "Fires a bolt of sound.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        {
            DICE_NUMBER dice = 4 + (plev - 1) / 5;
            DICE_SID sides = 4;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_bolt(player_ptr, AttributeType::SOUND, dir, damroll(dice, sides));
            }
        }
        break;

    case 3:
        if (name) {
            return _("朦朧の旋律", "Stun Pattern");
        }
        if (desc) {
            return _("視界内の全てのモンスターを朦朧させる。抵抗されると無効。", "Attempts to stun all monsters in sight.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("眩惑させるメロディを奏で始めた．．．", "You weave a pattern of sounds to bewilder and daze..."));
            start_singing(player_ptr, spell, MUSIC_STUN);
        }

        {
            DICE_NUMBER dice = plev / 10;
            DICE_SID sides = 2;

            if (info) {
                return info_power_dice(dice, sides);
            }

            if (cont) {
                stun_monsters(player_ptr, damroll(dice, sides));
            }
        }

        break;

    case 4:
        if (name) {
            return _("生命の流れ", "Flow of Life");
        }
        if (desc) {
            return _("体力を少し回復させる。", "Heals HP a little.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("歌を通して体に活気が戻ってきた．．．", "Life flows through you as you sing a song of healing..."));
            start_singing(player_ptr, spell, MUSIC_L_LIFE);
        }

        {
            DICE_NUMBER dice = 2;
            DICE_SID sides = 6;

            if (info) {
                return info_heal(dice, sides, 0);
            }

            if (cont) {
                hp_player(player_ptr, damroll(dice, sides));
            }
        }

        break;

    case 5:
        if (name) {
            return _("太陽の歌", "Song of the Sun");
        }
        if (desc) {
            return _("光源が照らしている範囲か部屋全体を永久に明るくする。", "Lights up nearby area and the inside of a room permanently.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        {
            DICE_NUMBER dice = 2;
            DICE_SID sides = plev / 2;
            POSITION rad = plev / 10 + 1;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                msg_print(_("光り輝く歌が辺りを照らした。", "Your uplifting song brings brightness to dark places..."));
                lite_area(player_ptr, damroll(dice, sides), rad);
            }
        }
        break;

    case 6:
        if (name) {
            return _("恐怖の歌", "Song of Fear");
        }
        if (desc) {
            return _("視界内の全てのモンスターを恐怖させる。抵抗されると無効。", "Attempts to scare all monsters in sight.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("おどろおどろしいメロディを奏で始めた．．．", "You start weaving a fearful pattern..."));
            start_singing(player_ptr, spell, MUSIC_FEAR);
        }

        {
            POWER power = plev;

            if (info) {
                return info_power(power);
            }

            if (cont) {
                project_all_los(player_ptr, AttributeType::TURN_ALL, power);
            }
        }

        break;

    case 7:
        if (name) {
            return _("戦いの歌", "Heroic Ballad");
        }

        if (desc) {
            return _("ヒーロー気分になる。", "Removes fear. Gives a bonus to hit for a while. Heals you for 10 HP.");
        }

        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("激しい戦いの歌を歌った．．．", "You start singing a song of intense fighting..."));

            (void)hp_player(player_ptr, 10);
            (void)BadStatusSetter(player_ptr).afraidness(0);
            player_ptr->update |= PU_HP;
            start_singing(player_ptr, spell, MUSIC_HERO);
        }

        if (stop) {
            if (!player_ptr->hero) {
                msg_print(_("ヒーローの気分が消え失せた。", "The heroism wears off."));
                /* Recalculate hitpoints */
                player_ptr->update |= (PU_HP);
            }
        }

        break;
    case 8:
        if (name) {
            return _("霊的知覚", "Clairaudience");
        }
        if (desc) {
            return _("近くの罠/扉/"
                     "階段を感知する。レベル15で全てのモンスター、20で財宝とアイテムを感知できるようになる。レベル25で周辺の地形を感知し、40でその階全体を永久"
                     "に照らし、ダンジョン内のすべてのアイテムを感知する。この効果は歌い続けることで順に起こる。",
                "Detects traps, doors and stairs in your vicinity. And detects all monsters at level 15, treasures and items at level 20. Maps nearby area at "
                "level 25. Lights and know the whole level at level 40. These effects accumulate as the song continues.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("静かな音楽が感覚を研ぎ澄まさせた．．．", "Your quiet music sharpens your sense of hearing..."));
            /* Hack -- Initialize the turn count */
            set_singing_count(player_ptr, 0);
            start_singing(player_ptr, spell, MUSIC_DETECT);
        }

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info) {
                return info_radius(rad);
            }

            if (cont) {
                int count = get_singing_count(player_ptr);

                if (count >= 19) {
                    wiz_lite(player_ptr, false);
                }
                if (count >= 11) {
                    map_area(player_ptr, rad);
                    if (plev > 39 && count < 19) {
                        set_singing_count(player_ptr, count + 1);
                    }
                }
                if (count >= 6) {
                    /* There are too many hidden treasure.  So... */
                    /* detect_treasure(rad); */
                    detect_objects_gold(player_ptr, rad);
                    detect_objects_normal(player_ptr, rad);

                    if (plev > 24 && count < 11) {
                        set_singing_count(player_ptr, count + 1);
                    }
                }
                if (count >= 3) {
                    detect_monsters_invis(player_ptr, rad);
                    detect_monsters_normal(player_ptr, rad);

                    if (plev > 19 && count < A_MAX) {
                        set_singing_count(player_ptr, count + 1);
                    }
                }
                detect_traps(player_ptr, rad, true);
                detect_doors(player_ptr, rad);
                detect_stairs(player_ptr, rad);

                if (plev > 14 && count < 3) {
                    set_singing_count(player_ptr, count + 1);
                }
            }
        }

        break;

    case 9:
        if (name) {
            return _("魂の歌", "Soul Shriek");
        }
        if (desc) {
            return _("視界内の全てのモンスターに対して精神攻撃を行う。", "Damages all monsters in sight with PSI damages.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("精神を捻じ曲げる歌を歌った．．．", "You start singing a song of soul in pain..."));
            start_singing(player_ptr, spell, MUSIC_PSI);
        }

        {
            DICE_NUMBER dice = 1;
            DICE_SID sides = plev * 3 / 2;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cont) {
                project_all_los(player_ptr, AttributeType::PSI, damroll(dice, sides));
            }
        }

        break;

    case 10:
        if (name) {
            return _("知識の歌", "Song of Lore");
        }
        if (desc) {
            return _("自分のいるマスと隣りのマスに落ちているアイテムを鑑定する。", "Identifies all items which are in the adjacent squares.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("この世界の知識が流れ込んできた．．．", "You recall the rich lore of the world..."));
            start_singing(player_ptr, spell, MUSIC_ID);
        }

        {
            POSITION rad = 1;

            if (info) {
                return info_radius(rad);
            }

            /*
             * 歌の開始時にも効果発動：
             * MP不足で鑑定が発動される前に歌が中断してしまうのを防止。
             */
            if (cont || cast) {
                project(player_ptr, 0, rad, player_ptr->y, player_ptr->x, 0, AttributeType::IDENTIFY, PROJECT_ITEM);
            }
        }

        break;

    case 11:
        if (name) {
            return _("隠遁の歌", "Hiding Tune");
        }
        if (desc) {
            return _("隠密行動能力を上昇させる。", "Gives improved stealth.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("あなたの姿が景色にとけこんでいった．．．", "Your song carries you beyond the sight of mortal eyes..."));
            start_singing(player_ptr, spell, MUSIC_STEALTH);
        }

        if (stop) {
            if (!player_ptr->tim_stealth) {
                msg_print(_("姿がはっきりと見えるようになった。", "You are no longer hidden."));
            }
        }

        break;

    case 12:
        if (name) {
            return _("幻影の旋律", "Illusion Pattern");
        }
        if (desc) {
            return _("視界内の全てのモンスターを混乱させる。抵抗されると無効。", "Attempts to confuse all monsters in sight.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("辺り一面に幻影が現れた．．．", "You weave a pattern of sounds to beguile and confuse..."));
            start_singing(player_ptr, spell, MUSIC_CONF);
        }

        {
            POWER power = plev * 2;

            if (info) {
                return info_power(power);
            }

            if (cont) {
                confuse_monsters(player_ptr, power);
            }
        }

        break;

    case 13:
        if (name) {
            return _("破滅の叫び", "Doomcall");
        }
        if (desc) {
            return _("視界内の全てのモンスターに対して轟音攻撃を行う。", "Damages all monsters in sight with booming sound.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("轟音が響いた．．．", "The fury of the Downfall of Numenor lashes out..."));
            start_singing(player_ptr, spell, MUSIC_SOUND);
        }

        {
            DICE_NUMBER dice = 10 + plev / 5;
            DICE_SID sides = 7;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cont) {
                project_all_los(player_ptr, AttributeType::SOUND, damroll(dice, sides));
            }
        }

        break;

    case 14:
        if (name) {
            return _("フィリエルの歌", "Firiel's Song");
        }
        if (desc) {
            return _("周囲の死体や骨を生き返す。", "Resurrects nearby corpses and skeletons. And makes them your pets.");
        }

        {
            /* Stop singing before start another */
            if (cast || fail) {
                stop_singing(player_ptr);
            }

            if (cast) {
                msg_print(_("生命と復活のテーマを奏で始めた．．．", "The themes of life and revival are woven into your song..."));
                animate_dead(player_ptr, 0, player_ptr->y, player_ptr->x);
            }
        }
        break;

    case 15:
        if (name) {
            return _("旅の仲間", "Fellowship Chant");
        }
        if (desc) {
            return _("視界内の全てのモンスターを魅了する。抵抗されると無効。", "Attempts to charm all monsters in sight.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("安らかなメロディを奏で始めた．．．", "You weave a slow, soothing melody of imploration..."));
            start_singing(player_ptr, spell, MUSIC_CHARM);
        }

        {
            DICE_NUMBER dice = 10 + plev / 15;
            DICE_SID sides = 6;

            if (info) {
                return info_power_dice(dice, sides);
            }

            if (cont) {
                charm_monsters(player_ptr, damroll(dice, sides));
            }
        }

        break;

    case 16:
        if (name) {
            return _("フルゥの行進曲", "Hru's March");
        }
        if (desc) {
            return _("壁を掘り進む。自分の足元のアイテムは蒸発する。", "Makes you be able to burrow into walls. Objects under your feet evaporate.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("粉砕するメロディを奏で始めた．．．", "You weave a violent pattern of sounds to break walls."));
            start_singing(player_ptr, spell, MUSIC_WALL);
        }

        {
            /*
             * 歌の開始時にも効果発動：
             * MP不足で効果が発動される前に歌が中断してしまうのを防止。
             */
            if (cont || cast) {
                project(player_ptr, 0, 0, player_ptr->y, player_ptr->x, 0, AttributeType::DISINTEGRATE, PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE);
            }
        }
        break;

    case 17:
        if (name) {
            return _("フィンロドの護り", "Finrod's Resistance");
        }
        if (desc) {
            return _("酸、電撃、炎、冷気、毒に対する耐性を得る。装備による耐性に累積する。",
                "Gives resistance to fire, cold, electricity, acid and poison. These resistances can be added to those from equipment for more powerful "
                "resistances.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("元素の力に対する忍耐の歌を歌った。", "You sing a song of perseverance against powers..."));
            start_singing(player_ptr, spell, MUSIC_RESIST);
        }

        if (stop) {
            if (!player_ptr->oppose_acid) {
                msg_print(_("酸への耐性が薄れた気がする。", "You feel less resistant to acid."));
            }

            if (!player_ptr->oppose_elec) {
                msg_print(_("電撃への耐性が薄れた気がする。", "You feel less resistant to elec."));
            }

            if (!player_ptr->oppose_fire) {
                msg_print(_("火への耐性が薄れた気がする。", "You feel less resistant to fire."));
            }

            if (!player_ptr->oppose_cold) {
                msg_print(_("冷気への耐性が薄れた気がする。", "You feel less resistant to cold."));
            }

            if (!player_ptr->oppose_pois) {
                msg_print(_("毒への耐性が薄れた気がする。", "You feel less resistant to pois."));
            }
        }

        break;

    case 18:
        if (name) {
            return _("ホビットのメロディ", "Hobbit Melodies");
        }
        if (desc) {
            return _("加速する。", "Hastes you.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("軽快な歌を口ずさみ始めた．．．", "You start singing a joyful pop song..."));
            start_singing(player_ptr, spell, MUSIC_SPEED);
        }

        if (stop) {
            if (!player_ptr->fast) {
                msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            }
        }

        break;

    case 19:
        if (name) {
            return _("歪んだ世界", "World Contortion");
        }
        if (desc) {
            return _("近くのモンスターをテレポートさせる。抵抗されると無効。", "Teleports all nearby monsters away unless resisted.");
        }

        {
            POSITION rad = plev / 15 + 1;
            POWER power = plev * 3 + 1;

            if (info) {
                return info_radius(rad);
            }

            /* Stop singing before start another */
            if (cast || fail) {
                stop_singing(player_ptr);
            }

            if (cast) {
                msg_print(_("歌が空間を歪めた．．．", "Reality whirls wildly as you sing a dizzying melody..."));
                project(player_ptr, 0, rad, player_ptr->y, player_ptr->x, power, AttributeType::AWAY_ALL, PROJECT_KILL);
            }
        }
        break;

    case 20:
        if (name) {
            return _("退散の歌", "Dispelling Chant");
        }
        if (desc) {
            return _("視界内の全てのモンスターにダメージを与える。邪悪なモンスターに特に大きなダメージを与える。",
                "Damages all monsters in sight. Hurts evil monsters greatly.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("耐えられない不協和音が敵を責め立てた．．．", "You cry out in an ear-wracking voice..."));
            start_singing(player_ptr, spell, MUSIC_DISPEL);
        }

        {
            DICE_SID m_sides = plev * 3;
            DICE_SID e_sides = plev * 3;

            if (info) {
                return format("%s1d%d+1d%d", KWD_DAM, m_sides, e_sides);
            }

            if (cont) {
                dispel_monsters(player_ptr, randint1(m_sides));
                dispel_evil(player_ptr, randint1(e_sides));
            }
        }
        break;

    case 21:
        if (name) {
            return _("サルマンの甘言", "The Voice of Saruman");
        }
        if (desc) {
            return _("視界内の全てのモンスターを減速させ、眠らせようとする。抵抗されると無効。", "Attempts to slow and put to sleep all monsters in sight.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("優しく、魅力的な歌を口ずさみ始めた．．．", "You start humming a gentle and attractive song..."));
            start_singing(player_ptr, spell, MUSIC_SARUMAN);
        }

        {
            POWER power = plev;

            if (info) {
                return info_power(power);
            }

            if (cont) {
                slow_monsters(player_ptr, plev);
                sleep_monsters(player_ptr, plev);
            }
        }

        break;

    case 22:
        if (name) {
            return _("嵐の音色", "Song of the Tempest");
        }
        if (desc) {
            return _("轟音のビームを放つ。", "Fires a beam of sound.");
        }

        {
            DICE_NUMBER dice = 15 + (plev - 1) / 2;
            DICE_SID sides = 10;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            /* Stop singing before start another */
            if (cast || fail) {
                stop_singing(player_ptr);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_beam(player_ptr, AttributeType::SOUND, dir, damroll(dice, sides));
            }
        }
        break;

    case 23:
        if (name) {
            return _("もう一つの世界", "Ambarkanta");
        }
        if (desc) {
            return _("現在の階を再構成する。", "Recreates current dungeon level.");
        }

        {
            int base = 15;
            DICE_SID sides = 20;

            if (info) {
                return info_delay(base, sides);
            }

            /* Stop singing before start another */
            if (cast || fail) {
                stop_singing(player_ptr);
            }

            if (cast) {
                msg_print(_("周囲が変化し始めた．．．", "You sing of the primeval shaping of Middle-earth..."));
                reserve_alter_reality(player_ptr, randint0(sides) + base);
            }
        }
        break;

    case 24:
        if (name) {
            return _("破壊の旋律", "Wrecking Pattern");
        }
        if (desc) {
            return _(
                "周囲のダンジョンを揺らし、壁と床をランダムに入れ変える。", "Shakes dungeon structure, and results in random swapping of floors and walls.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("破壊的な歌が響きわたった．．．", "You weave a pattern of sounds to contort and shatter..."));
            start_singing(player_ptr, spell, MUSIC_QUAKE);
        }

        {
            POSITION rad = 10;

            if (info) {
                return info_radius(rad);
            }

            if (cont) {
                earthquake(player_ptr, player_ptr->y, player_ptr->x, 10, 0);
            }
        }

        break;

    case 25:
        if (name) {
            return _("停滞の歌", "Stationary Shriek");
        }
        if (desc) {
            return _("視界内の全てのモンスターを麻痺させようとする。抵抗されると無効。", "Attempts to freeze all monsters in sight.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("ゆっくりとしたメロディを奏で始めた．．．", "You weave a very slow pattern which is almost likely to stop..."));
            start_singing(player_ptr, spell, MUSIC_STASIS);
        }

        {
            POWER power = plev * 4;

            if (info) {
                return info_power(power);
            }

            if (cont) {
                stasis_monsters(player_ptr, power);
            }
        }

        break;

    case 26:
        if (name) {
            return _("エルベレスの聖歌", "Elbereth's Chant");
        }
        if (desc) {
            return _("自分のいる床の上に、モンスターが通り抜けたり召喚されたりすることができなくなるルーンを描く。",
                "Sets a rune on the floor beneath you. If you are on a rune, monsters cannot attack you but can try to break the rune.");
        }

        {
            /* Stop singing before start another */
            if (cast || fail) {
                stop_singing(player_ptr);
            }

            if (cast) {
                msg_print(_("歌が神聖な場を作り出した．．．", "The holy power of the Music is creating sacred field..."));
                create_rune_protection_one(player_ptr);
            }
        }
        break;

    case 27: {
        if (name) {
            return _("英雄の詩", "The Hero's Poem");
        }

        if (desc) {
            return _("加速し、ヒーロー気分になり、視界内の全てのモンスターにダメージを与える。", "Hastes you. Gives heroism. Damages all monsters in sight.");
        }

        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("英雄の歌を口ずさんだ．．．", "You chant a powerful, heroic call to arms..."));
            (void)hp_player(player_ptr, 10);
            (void)BadStatusSetter(player_ptr).afraidness(0);
            player_ptr->update |= PU_HP;
            start_singing(player_ptr, spell, MUSIC_SHERO);
        }

        if (stop) {
            if (!player_ptr->hero) {
                msg_print(_("ヒーローの気分が消え失せた。", "The heroism wears off."));
                player_ptr->update |= PU_HP;
            }

            if (!player_ptr->fast) {
                msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            }
        }

        DICE_NUMBER dice = 1;
        DICE_SID sides = plev * 3;
        if (info) {
            return info_damage(dice, sides, 0);
        }

        if (cont) {
            dispel_monsters(player_ptr, damroll(dice, sides));
        }

        break;
    }
    case 28: {
        if (name) {
            return _("ヤヴァンナの助け", "Relief of Yavanna");
        }

        if (desc) {
            return _("強力な回復の歌で、負傷と朦朧状態も全快する。", "Powerful healing song. Also completely heals cuts and being stunned.");
        }

        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("歌を通して体に活気が戻ってきた．．．", "Life flows through you as you sing the song..."));
            start_singing(player_ptr, spell, MUSIC_H_LIFE);
        }

        auto dice = 15;
        auto sides = 10;
        if (info) {
            return info_heal(dice, sides, 0);
        }

        if (cont) {
            hp_player(player_ptr, damroll(dice, sides));
            BadStatusSetter bss(player_ptr);
            (void)bss.stun(0);
            (void)bss.cut(0);
        }

        break;
    }
    case 29:
        if (name) {
            return _("再生の歌", "Goddess's rebirth");
        }
        if (desc) {
            return _("すべてのステータスと経験値を回復する。", "Restores all stats and experience.");
        }

        {
            /* Stop singing before start another */
            if (cast || fail) {
                stop_singing(player_ptr);
            }

            if (cast) {
                msg_print(
                    _("暗黒の中に光と美をふりまいた。体が元の活力を取り戻した。", "You strew light and beauty in the dark as you sing. You feel refreshed."));
                (void)restore_all_status(player_ptr);
                (void)restore_level(player_ptr);
            }
        }
        break;

    case 30:
        if (name) {
            return _("サウロンの魔術", "Wizardry of Sauron");
        }
        if (desc) {
            return _("非常に強力でごく小さい轟音の球を放つ。", "Fires an extremely powerful tiny ball of sound.");
        }

        {
            DICE_NUMBER dice = 50 + plev;
            DICE_SID sides = 10;
            POSITION rad = 0;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            /* Stop singing before start another */
            if (cast || fail) {
                stop_singing(player_ptr);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_ball(player_ptr, AttributeType::SOUND, dir, damroll(dice, sides), rad);
            }
        }
        break;

    case 31:
        if (name) {
            return _("フィンゴルフィンの挑戦", "Fingolfin's Challenge");
        }
        if (desc) {
            return _("ダメージを受けなくなるバリアを張る。", "Generates a barrier which completely protects you from almost all damage.");
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("フィンゴルフィンの冥王への挑戦を歌った．．．", "You recall the valor of Fingolfin's challenge to the Dark Lord..."));

            player_ptr->redraw |= (PR_MAP);
            player_ptr->update |= (PU_MONSTERS);
            player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);

            start_singing(player_ptr, spell, MUSIC_INVULN);
        }

        if (stop) {
            if (!player_ptr->invuln) {
                msg_print(_("無敵ではなくなった。", "The invulnerability wears off."));

                player_ptr->redraw |= (PR_MAP);
                player_ptr->update |= (PU_MONSTERS);
                player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
            }
        }

        break;
    }

    return "";
}
