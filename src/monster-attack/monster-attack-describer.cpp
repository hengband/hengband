/*!
 * @brief モンスターの打撃種別を記述すると共に、切り傷/朦朧値を追加する
 * @date 2020/05/31
 * @author Hourier
 */

#include "monster-attack/monster-attack-describer.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-attack/insults-moans.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/race-indice-types.h"
#include "system/angband.h"
#include "system/monster-entity.h"

static void show_jaian_song(MonsterAttackPlayer *monap_ptr)
{
#ifdef JP
    constexpr static auto songs = {
        "「♪お～れはジャイアン～～ガ～キだいしょう～」",
        "「♪て～んかむ～てきのお～とこだぜ～～」",
        "「♪の～び太スネ夫はメじゃないよ～～」",
        "「♪け～んかスポ～ツ～どんとこい～」",
        "「♪うた～も～～う～まいぜ～まかしとけ～」",
        "「♪お～れはジャイアン～～ガ～キだいしょう～」",
        "「♪ま～ちいちば～んのに～んきもの～～」",
        "「♪べんきょうしゅくだいメじゃないよ～～」",
        "「♪きはやさし～くて～ち～からもち～」",
        "「♪かお～も～～スタイルも～バツグンさ～」",
        "「♪お～れはジャイアン～～ガ～キだいしょう～」",
        "「♪がっこうい～ちの～あ～ばれんぼう～～」",
        "「♪ド～ラもドラミもメじゃないよ～～」",
        "「♪よじげんぽけっと～な～くたって～」",
        "「♪あし～の～～ながさ～は～まけないぜ～」",
    };

    monap_ptr->act = rand_choice(songs);
#else
    monap_ptr->act = "horribly sings 'I AM GIAAAAAN. THE BOOOSS OF THE KIIIIDS.'";
#endif
}

static void monster_attack_show(MonsterAttackPlayer *monap_ptr)
{
#ifdef JP
    monap_ptr->abbreviate = -1;
#endif
    if (monap_ptr->m_ptr->r_idx == MonsterRaceId::JAIAN) {
        show_jaian_song(monap_ptr);
    } else {
        if (one_in_(3)) {
            monap_ptr->act = _("は♪僕らは楽しい家族♪と歌っている。", "sings 'We are a happy family.'");
        } else {
            monap_ptr->act = _("は♪アイ ラブ ユー、ユー ラブ ミー♪と歌っている。", "sings 'I love you, you love me.'");
        }
    }

    sound(SOUND_SHOW);
}

void describe_monster_attack_method(MonsterAttackPlayer *monap_ptr)
{
    switch (monap_ptr->method) {
    case RaceBlowMethodType::HIT: {
        monap_ptr->act = _("殴られた。", "hits you.");
        monap_ptr->do_cut = monap_ptr->do_stun = 1;
        monap_ptr->touched = true;
        sound(SOUND_ENEMY_HIT);
        break;
    }
    case RaceBlowMethodType::TOUCH: {
        monap_ptr->act = _("触られた。", "touches you.");
        monap_ptr->touched = true;
        sound(SOUND_TOUCH);
        break;
    }
    case RaceBlowMethodType::PUNCH: {
        monap_ptr->act = _("パンチされた。", "punches you.");
        monap_ptr->touched = true;
        monap_ptr->do_stun = 1;
        sound(SOUND_ENEMY_HIT);
        break;
    }
    case RaceBlowMethodType::KICK: {
        monap_ptr->act = _("蹴られた。", "kicks you.");
        monap_ptr->touched = true;
        monap_ptr->do_stun = 1;
        sound(SOUND_ENEMY_HIT);
        break;
    }
    case RaceBlowMethodType::CLAW: {
        monap_ptr->act = _("ひっかかれた。", "claws you.");
        monap_ptr->touched = true;
        monap_ptr->do_cut = 1;
        sound(SOUND_CLAW);
        break;
    }
    case RaceBlowMethodType::BITE: {
        monap_ptr->act = _("噛まれた。", "bites you.");
        monap_ptr->do_cut = 1;
        monap_ptr->touched = true;
        sound(SOUND_BITE);
        break;
    }
    case RaceBlowMethodType::STING: {
        monap_ptr->act = _("刺された。", "stings you.");
        monap_ptr->touched = true;
        sound(SOUND_STING);
        break;
    }
    case RaceBlowMethodType::SLASH: {
        monap_ptr->act = _("斬られた。", "slashes you.");
        monap_ptr->touched = true;
        monap_ptr->do_cut = 1;
        sound(SOUND_CLAW);
        break;
    }
    case RaceBlowMethodType::BUTT: {
        monap_ptr->act = _("角で突かれた。", "butts you.");
        monap_ptr->do_stun = 1;
        monap_ptr->touched = true;
        sound(SOUND_ENEMY_HIT);
        break;
    }
    case RaceBlowMethodType::CRUSH: {
        monap_ptr->act = _("体当たりされた。", "crushes you.");
        monap_ptr->do_stun = 1;
        monap_ptr->touched = true;
        sound(SOUND_CRUSH);
        break;
    }
    case RaceBlowMethodType::ENGULF: {
        monap_ptr->act = _("飲み込まれた。", "engulfs you.");
        monap_ptr->touched = true;
        sound(SOUND_CRUSH);
        break;
    }
    case RaceBlowMethodType::CHARGE: {
#ifdef JP
        monap_ptr->abbreviate = -1;
#endif
        monap_ptr->act = _("は請求書をよこした。", "charges you.");
        monap_ptr->touched = true;

        /* このコメントはジョークが効いているので残しておく / Note! This is "charges", not "charges at". */
        sound(SOUND_BUY);
        break;
    }
    case RaceBlowMethodType::CRAWL: {
#ifdef JP
        monap_ptr->abbreviate = -1;
#endif
        monap_ptr->act = _("が体の上を這い回った。", "crawls on you.");
        monap_ptr->touched = true;
        sound(SOUND_SLIME);
        break;
    }
    case RaceBlowMethodType::DROOL: {
        monap_ptr->act = _("よだれをたらされた。", "drools on you.");
        sound(SOUND_SLIME);
        break;
    }
    case RaceBlowMethodType::SPIT: {
        monap_ptr->act = _("唾を吐かれた。", "spits on you.");
        sound(SOUND_SLIME);
        break;
    }
    case RaceBlowMethodType::EXPLODE: {
#ifdef JP
        monap_ptr->abbreviate = -1;
#endif
        monap_ptr->act = _("は爆発した。", "explodes.");
        monap_ptr->explode = true;
        break;
    }
    case RaceBlowMethodType::GAZE: {
        monap_ptr->act = _("にらまれた。", "gazes at you.");
        break;
    }
    case RaceBlowMethodType::WAIL: {
        monap_ptr->act = _("泣き叫ばれた。", "wails at you.");
        sound(SOUND_WAIL);
        break;
    }
    case RaceBlowMethodType::SPORE: {
        monap_ptr->act = _("胞子を飛ばされた。", "releases spores at you.");
        sound(SOUND_SLIME);
        break;
    }
    case RaceBlowMethodType::XXX4: {
#ifdef JP
        monap_ptr->abbreviate = -1;
#endif
        monap_ptr->act = _("が XXX4 を発射した。", "projects XXX4's at you.");
        break;
    }
    case RaceBlowMethodType::BEG: {
        monap_ptr->act = _("金をせがまれた。", "begs you for money.");
        sound(SOUND_MOAN);
        break;
    }
    case RaceBlowMethodType::INSULT: {
#ifdef JP
        monap_ptr->abbreviate = -1;
#endif
        monap_ptr->act = desc_insult[randint0(monap_ptr->m_ptr->r_idx == MonsterRaceId::DEBBY ? 10 : 8)];
        sound(SOUND_MOAN);
        break;
    }
    case RaceBlowMethodType::MOAN: {
#ifdef JP
        monap_ptr->abbreviate = -1;
#endif
        monap_ptr->act = rand_choice(desc_moan);
        sound(SOUND_MOAN);
        break;
    }
    case RaceBlowMethodType::SHOW: {
        monster_attack_show(monap_ptr);
        break;
    }

    case RaceBlowMethodType::NONE:
    case RaceBlowMethodType::SHOOT:
    case RaceBlowMethodType::MAX:
        break;
    }
}
