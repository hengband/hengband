#include "cmd-action/cmd-spell.h"
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
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "timed-effect/timed-effects.h"
#include "util/dice.h"
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

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
}

/*!
 * @brief 歌の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 歌ID
 * @param mode 処理内容 (NAME / SPELL_DESC / INFO / CAST / FAIL / SPELL_CONT / STOP)
 * @return
 * NAME / SPELL_DESC / INFO 時には文字列を返す.
 * CAST / FAIL / SPELL_CONT / STOP 時は tl::nullopt を返す.
 */
tl::optional<std::string> do_music_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;
    bool fail = mode == SpellProcessType::FAIL;
    bool cont = mode == SpellProcessType::CONTNUATION;
    bool stop = mode == SpellProcessType::STOP;

    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0:
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
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        {
            const Dice dice(4 + (plev - 1) / 5, 4);

            if (info) {
                return info_damage(dice);
            }

            if (cast) {
                const auto dir = get_aim_dir(player_ptr);
                if (!dir) {
                    return tl::nullopt;
                }

                fire_bolt(player_ptr, AttributeType::SOUND, dir, dice.roll());
            }
        }
        break;

    case 3:
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("眩惑させるメロディを奏で始めた．．．", "You weave a pattern of sounds to bewilder and daze..."));
            start_singing(player_ptr, spell, MUSIC_STUN);
        }

        {
            const Dice dice(plev / 10, 2);

            if (info) {
                return info_power_dice(dice);
            }

            if (cont) {
                stun_monsters(player_ptr, dice.roll());
            }
        }

        break;

    case 4:
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("歌を通して体に活気が戻ってきた．．．", "Life flows through you as you sing a song of healing..."));
            start_singing(player_ptr, spell, MUSIC_L_LIFE);
        }

        {
            const Dice dice(2, 6);

            if (info) {
                return info_heal(dice);
            }

            if (cont) {
                hp_player(player_ptr, dice.roll());
            }
        }

        break;

    case 5:
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        {
            const Dice dice(2, plev / 2);
            POSITION rad = plev / 10 + 1;

            if (info) {
                return info_damage(dice);
            }

            if (cast) {
                msg_print(_("光り輝く歌が辺りを照らした。", "Your uplifting song brings brightness to dark places..."));
                lite_area(player_ptr, dice.roll(), rad);
            }
        }
        break;

    case 6:
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
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("激しい戦いの歌を歌った．．．", "You start singing a song of intense fighting..."));

            (void)hp_player(player_ptr, 10);
            (void)BadStatusSetter(player_ptr).set_fear(0);
            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::HP);
            start_singing(player_ptr, spell, MUSIC_HERO);
        }

        if (stop) {
            if (!player_ptr->hero) {
                msg_print(_("ヒーローの気分が消え失せた。", "The heroism wears off."));
                RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::HP);
            }
        }

        break;
    case 8:
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
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("精神を捻じ曲げる歌を歌った．．．", "You start singing a song of soul in pain..."));
            start_singing(player_ptr, spell, MUSIC_PSI);
        }

        {
            const Dice dice(1, plev * 3 / 2);

            if (info) {
                return info_damage(dice);
            }

            if (cont) {
                project_all_los(player_ptr, AttributeType::PSI, dice.roll());
            }
        }

        break;

    case 10:
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
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("轟音が響いた．．．", "The fury of the Downfall of Numenor lashes out..."));
            start_singing(player_ptr, spell, MUSIC_SOUND);
        }

        {
            const Dice dice(10 + plev / 5, 7);

            if (info) {
                return info_damage(dice);
            }

            if (cont) {
                project_all_los(player_ptr, AttributeType::SOUND, dice.roll());
            }
        }

        break;

    case 14: {
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("生命と復活のテーマを奏で始めた．．．", "The themes of life and revival are woven into your song..."));
            animate_dead(player_ptr, 0, player_ptr->y, player_ptr->x);
        }
    } break;

    case 15:
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("安らかなメロディを奏で始めた．．．", "You weave a slow, soothing melody of imploration..."));
            start_singing(player_ptr, spell, MUSIC_CHARM);
        }

        {
            const Dice dice(10 + plev / 15, 6);

            if (info) {
                return info_power_dice(dice);
            }

            if (cont) {
                charm_monsters(player_ptr, dice.roll());
            }
        }

        break;

    case 16:
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
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("軽快な歌を口ずさみ始めた．．．", "You start singing a joyful pop song..."));
            start_singing(player_ptr, spell, MUSIC_SPEED);
        }

        if (stop) {
            if (!player_ptr->effects()->acceleration().is_fast()) {
                msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            }
        }

        break;

    case 19: {
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
    } break;

    case 20:
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("耐えられない不協和音が敵を責め立てた．．．", "You cry out in an ear-wracking voice..."));
            start_singing(player_ptr, spell, MUSIC_DISPEL);
        }

        {
            const Dice m_dice(1, plev * 3);
            const Dice e_dice(1, plev * 3);

            if (info) {
                return format("%s%s+%s", KWD_DAM, m_dice.to_string().data(), e_dice.to_string().data());
            }

            if (cont) {
                dispel_monsters(player_ptr, m_dice.roll());
                dispel_evil(player_ptr, e_dice.roll());
            }
        }
        break;

    case 21:
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

    case 22: {
        const Dice dice(15 + (plev - 1) / 2, 10);

        if (info) {
            return info_damage(dice);
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_beam(player_ptr, AttributeType::SOUND, dir, dice.roll());
        }
    } break;

    case 23: {
        int base = 15;
        const Dice dice(1, 20);

        if (info) {
            return info_delay(base, dice);
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("周囲が変化し始めた．．．", "You sing of the primeval shaping of Middle-earth..."));
            reserve_alter_reality(player_ptr, dice.roll() + base);
        }
    } break;

    case 24:
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
                earthquake(player_ptr, player_ptr->get_position(), 10);
            }
        }

        break;

    case 25:
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

    case 26: {
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("歌が神聖な場を作り出した．．．", "The holy power of the Music is creating sacred field..."));
            create_rune_protection_one(player_ptr);
        }
    } break;

    case 27: {

        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("英雄の歌を口ずさんだ．．．", "You chant a powerful, heroic call to arms..."));
            (void)hp_player(player_ptr, 10);
            (void)BadStatusSetter(player_ptr).set_fear(0);
            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::HP);
            start_singing(player_ptr, spell, MUSIC_SHERO);
        }

        if (stop) {
            if (!player_ptr->hero) {
                msg_print(_("ヒーローの気分が消え失せた。", "The heroism wears off."));
                RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::HP);
            }

            if (!player_ptr->effects()->acceleration().is_fast()) {
                msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            }
        }

        const Dice dice(1, plev * 3);
        if (info) {
            return info_damage(dice);
        }

        if (cont) {
            dispel_monsters(player_ptr, dice.roll());
        }

        break;
    }
    case 28: {

        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("歌を通して体に活気が戻ってきた．．．", "Life flows through you as you sing the song..."));
            start_singing(player_ptr, spell, MUSIC_H_LIFE);
        }

        const Dice dice(15, 10);
        if (info) {
            return info_heal(dice);
        }

        if (cont) {
            hp_player(player_ptr, dice.roll());
            BadStatusSetter bss(player_ptr);
            (void)bss.set_stun(0);
            (void)bss.set_cut(0);
        }

        break;
    }
    case 29: {
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
    } break;

    case 30: {
        const Dice dice(50 + plev, 10);
        POSITION rad = 0;

        if (info) {
            return info_damage(dice);
        }

        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::SOUND, dir, dice.roll(), rad);
        }
    } break;

    case 31:
        /* Stop singing before start another */
        if (cast || fail) {
            stop_singing(player_ptr);
        }

        if (cast) {
            msg_print(_("フィンゴルフィンの冥王への挑戦を歌った．．．", "You recall the valor of Fingolfin's challenge to the Dark Lord..."));
            auto &rfu = RedrawingFlagsUpdater::get_instance();
            rfu.set_flag(MainWindowRedrawingFlag::MAP);
            rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
            static constexpr auto flags = {
                SubWindowRedrawingFlag::OVERHEAD,
                SubWindowRedrawingFlag::DUNGEON,
            };
            rfu.set_flags(flags);
            start_singing(player_ptr, spell, MUSIC_INVULN);
        }

        if (stop) {
            if (!player_ptr->invuln) {
                msg_print(_("無敵ではなくなった。", "The invulnerability wears off."));
                auto &rfu = RedrawingFlagsUpdater::get_instance();
                rfu.set_flag(MainWindowRedrawingFlag::MAP);
                rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
                static constexpr auto flags = {
                    SubWindowRedrawingFlag::OVERHEAD,
                    SubWindowRedrawingFlag::DUNGEON,
                };
                rfu.set_flags(flags);
            }
        }

        break;
    }

    return "";
}
