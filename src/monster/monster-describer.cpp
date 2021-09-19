#include "monster/monster-describer.h"
#include "io/files-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#ifdef JP
#else
#include "locale/english.h"
#endif

/*!
 * @brief モンスターの呼称を作成する / Build a string describing a monster in some way.
 * @param desc 記述出力先の文字列参照ポインタ
 * @param m_ptr モンスターの参照ポインタ
 * @param mode 呼称オプション
 */
void monster_desc(player_type *player_ptr, char *desc, monster_type *m_ptr, BIT_FLAGS mode)
{
    monster_race *r_ptr;
    r_ptr = &r_info[m_ptr->ap_r_idx];
    concptr name = (mode & MD_TRUE_NAME) ? real_r_ptr(m_ptr)->name.c_str() : r_ptr->name.c_str();
    GAME_TEXT silly_name[1024];
    bool named = false;
    if (player_ptr->image && !(mode & MD_IGNORE_HALLU)) {
        if (one_in_(2)) {
            if (!get_rnd_line(_("silly_j.txt", "silly.txt"), m_ptr->r_idx, silly_name))
                named = true;
        }

        if (!named) {
            monster_race *hallu_race;

            do {
                hallu_race = &r_info[randint1(max_r_idx - 1)];
            } while (hallu_race->name.empty() || (hallu_race->flags1 & RF1_UNIQUE));

            strcpy(silly_name, (hallu_race->name.c_str()));
        }

        name = silly_name;
    }

    bool seen = (m_ptr && ((mode & MD_ASSUME_VISIBLE) || (!(mode & MD_ASSUME_HIDDEN) && m_ptr->ml)));
    bool pron = (m_ptr && ((seen && (mode & MD_PRON_VISIBLE)) || (!seen && (mode & MD_PRON_HIDDEN))));

    /* First, try using pronouns, or describing hidden monsters */
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!seen || pron) {
        int kind = 0x00;
        if (r_ptr->flags1 & (RF1_FEMALE))
            kind = 0x20;
        else if (r_ptr->flags1 & (RF1_MALE))
            kind = 0x10;

        if (!m_ptr || !pron)
            kind = 0x00;

        concptr res = _("何か", "it");
        switch (kind + (mode & (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE))) {
        case 0x00:
            res = _("何か", "it");
            break;
        case 0x00 + (MD_OBJECTIVE):
            res = _("何か", "it");
            break;
        case 0x00 + (MD_POSSESSIVE):
            res = _("何かの", "its");
            break;
        case 0x00 + (MD_POSSESSIVE | MD_OBJECTIVE):
            res = _("何か自身", "itself");
            break;
        case 0x00 + (MD_INDEF_HIDDEN):
            res = _("何か", "something");
            break;
        case 0x00 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
            res = _("何か", "something");
            break;
        case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
            res = _("何か", "something's");
            break;
        case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
            res = _("それ自身", "itself");
            break;
        case 0x10:
            res = _("彼", "he");
            break;
        case 0x10 + (MD_OBJECTIVE):
            res = _("彼", "him");
            break;
        case 0x10 + (MD_POSSESSIVE):
            res = _("彼の", "his");
            break;
        case 0x10 + (MD_POSSESSIVE | MD_OBJECTIVE):
            res = _("彼自身", "himself");
            break;
        case 0x10 + (MD_INDEF_HIDDEN):
            res = _("誰か", "someone");
            break;
        case 0x10 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
            res = _("誰か", "someone");
            break;
        case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
            res = _("誰かの", "someone's");
            break;
        case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
            res = _("彼自身", "himself");
            break;
        case 0x20:
            res = _("彼女", "she");
            break;
        case 0x20 + (MD_OBJECTIVE):
            res = _("彼女", "her");
            break;
        case 0x20 + (MD_POSSESSIVE):
            res = _("彼女の", "her");
            break;
        case 0x20 + (MD_POSSESSIVE | MD_OBJECTIVE):
            res = _("彼女自身", "herself");
            break;
        case 0x20 + (MD_INDEF_HIDDEN):
            res = _("誰か", "someone");
            break;
        case 0x20 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
            res = _("誰か", "someone");
            break;
        case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
            res = _("誰かの", "someone's");
            break;
        case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
            res = _("彼女自身", "herself");
            break;
        }

        (void)strcpy(desc, res);
        return;
    }

    /* Handle visible monsters, "reflexive" request */
    if ((mode & (MD_POSSESSIVE | MD_OBJECTIVE)) == (MD_POSSESSIVE | MD_OBJECTIVE)) {
        /* The monster is visible, so use its gender */
        if (r_ptr->flags1 & (RF1_FEMALE))
            strcpy(desc, _("彼女自身", "herself"));
        else if (r_ptr->flags1 & (RF1_MALE))
            strcpy(desc, _("彼自身", "himself"));
        else
            strcpy(desc, _("それ自身", "itself"));
        return;
    }

    /* Handle all other visible monster requests */
    /* Tanuki? */
    if (is_pet(m_ptr) && !is_original_ap(m_ptr)) {
#ifdef JP
        char *t;
        char buf[128];
        strcpy(buf, name);
        t = buf;
        while (strncmp(t, "』", 2) && *t)
            t++;
        if (*t) {
            *t = '\0';
            (void)sprintf(desc, "%s？』", buf);
        } else
            (void)sprintf(desc, "%s？", name);
#else
        (void)sprintf(desc, "%s?", name);
#endif
    } else {
        if ((r_ptr->flags1 & RF1_UNIQUE) && !(player_ptr->image && !(mode & MD_IGNORE_HALLU))) {
            if (m_ptr->mflag2.has(MFLAG2::CHAMELEON) && !(mode & MD_TRUE_NAME)) {
#ifdef JP
                char *t;
                char buf[128];
                strcpy(buf, name);
                t = buf;
                while (strncmp(t, "』", 2) && *t)
                    t++;
                if (*t) {
                    *t = '\0';
                    (void)sprintf(desc, "%s？』", buf);
                } else
                    (void)sprintf(desc, "%s？", name);
#else
                (void)sprintf(desc, "%s?", name);
#endif
            } else if (player_ptr->phase_out && !(player_ptr->riding && (&floor_ptr->m_list[player_ptr->riding] == m_ptr))) {
                (void)sprintf(desc, _("%sもどき", "fake %s"), name);
            } else {
                (void)strcpy(desc, name);
            }
        } else if (mode & MD_INDEF_VISIBLE) {
#ifdef JP
            (void)strcpy(desc, "");
#else
            (void)strcpy(desc, is_a_vowel(name[0]) ? "an " : "a ");
#endif
            (void)strcat(desc, name);
        } else {
            if (is_pet(m_ptr))
                (void)strcpy(desc, _("あなたの", "your "));
            else
                (void)strcpy(desc, _("", "the "));

            (void)strcat(desc, name);
        }
    }

    if (m_ptr->nickname) {
        char buf[128];
        sprintf(buf, _("「%s」", " called %s"), quark_str(m_ptr->nickname));
        strcat(desc, buf);
    }

    if (player_ptr->riding && (&floor_ptr->m_list[player_ptr->riding] == m_ptr)) {
        strcat(desc, _("(乗馬中)", "(riding)"));
    }

    if ((mode & MD_IGNORE_HALLU) && m_ptr->mflag2.has(MFLAG2::CHAMELEON)) {
        if (r_ptr->flags1 & RF1_UNIQUE) {
            strcat(desc, _("(カメレオンの王)", "(Chameleon Lord)"));
        } else {
            strcat(desc, _("(カメレオン)", "(Chameleon)"));
        }
    }

    if ((mode & MD_IGNORE_HALLU) && !is_original_ap(m_ptr)) {
        strcat(desc, format("(%s)", r_info[m_ptr->r_idx].name.c_str()));
    }

    /* Handle the Possessive as a special afterthought */
    if (mode & MD_POSSESSIVE) {
        (void)strcat(desc, _("の", "'s"));
    }
}

/*!
 * @brief ダメージを受けたモンスターの様子を記述する / Dump a message describing a monster's reaction to damage
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター情報ID
 * @param dam 与えたダメージ
 * @details
 * Technically should attempt to treat "Beholder"'s as jelly's
 */
void message_pain(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    GAME_TEXT m_name[MAX_NLEN];


    monster_desc(player_ptr, m_name, m_ptr, 0);

    if (dam == 0) {
        if (m_ptr->ml) {
            msg_format(_("%^sはダメージを受けていない。", "%^s is unharmed."), m_name);
        }
        return;
    }

    HIT_POINT newhp = m_ptr->hp;
    HIT_POINT oldhp = newhp + dam;
    HIT_POINT tmp = (newhp * 100L) / oldhp;
    PERCENTAGE percentage = tmp;

    if (angband_strchr(",ejmvwQ", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%^sはほとんど気にとめていない。", "%^s barely notices."), m_name);
        else if (percentage > 75)
            msg_format(_("%^sはしり込みした。", "%^s flinches."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sは縮こまった。", "%^s squelches."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sは痛みに震えた。", "%^s quivers in pain."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは身もだえした。", "%^s writhes about."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sは苦痛で身もだえした。", "%^s writhes in agony."), m_name);
        else
            msg_format(_("%^sはぐにゃぐにゃと痙攣した。", "%^s jerks limply."), m_name);
        return;
    }

    if (angband_strchr("l", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%^sはほとんど気にとめていない。", "%^s barely notices."), m_name);
        else if (percentage > 75)
            msg_format(_("%^sはしり込みした。", "%^s flinches."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sは躊躇した。", "%^s hesitates."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sは痛みに震えた。", "%^s quivers in pain."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは身もだえした。", "%^s writhes about."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sは苦痛で身もだえした。", "%^s writhes in agony."), m_name);
        else
            msg_format(_("%^sはぐにゃぐにゃと痙攣した。", "%^s jerks limply."), m_name);
        return;
    }

    if (angband_strchr("g#+<>", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%sは攻撃を気にとめていない。", "%^s ignores the attack."), m_name);
        else if (percentage > 75)
            msg_format(_("%sは攻撃に肩をすくめた。", "%^s shrugs off the attack."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sは雷鳴のように吠えた。", "%^s roars thunderously."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sは苦しげに吠えた。", "%^s rumbles."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sはうめいた。", "%^s grunts."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sは躊躇した。", "%^s hesitates."), m_name);
        else
            msg_format(_("%^sはくしゃくしゃになった。", "%^s crumples."), m_name);
        return;
    }

    if (angband_strchr("JMR", r_ptr->d_char) || !isalpha(r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%^sはほとんど気にとめていない。", "%^s barely notices."), m_name);
        else if (percentage > 75)
            msg_format(_("%^sはシーッと鳴いた。", "%^s hisses."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sは怒って頭を上げた。", "%^s rears up in anger."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sは猛然と威嚇した。", "%^s hisses furiously."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは身もだえした。", "%^s writhes about."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sは苦痛で身もだえした。", "%^s writhes in agony."), m_name);
        else
            msg_format(_("%^sはぐにゃぐにゃと痙攣した。", "%^s jerks limply."), m_name);
        return;
    }

    if (angband_strchr("f", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%sは攻撃に肩をすくめた。", "%^s shrugs off the attack."), m_name);
        else if (percentage > 75)
            msg_format(_("%^sは吠えた。", "%^s roars."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sは怒って吠えた。", "%^s growls angrily."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sは痛みでシーッと鳴いた。", "%^s hisses with pain."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは痛みで弱々しく鳴いた。", "%^s mewls in pain."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sは苦痛にうめいた。", "%^s hisses in agony."), m_name);
        else
            msg_format(_("%sは哀れな鳴き声を出した。", "%^s mewls pitifully."), m_name);
        return;
    }

    if (angband_strchr("acFIKS", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%sは攻撃を気にとめていない。", "%^s ignores the attack."), m_name);
        else if (percentage > 75)
            msg_format(_("%^sはキーキー鳴いた。", "%^s chitters."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sはヨロヨロ逃げ回った。", "%^s scuttles about."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sはうるさく鳴いた。", "%^s twitters."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは痛みに痙攣した。", "%^s jerks in pain."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sは苦痛で痙攣した。", "%^s jerks in agony."), m_name);
        else
            msg_format(_("%^sはピクピクひきつった。", "%^s twitches."), m_name);
        return;
    }

    if (angband_strchr("B", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%^sはさえずった。", "%^s chirps."), m_name);
        else if (percentage > 75)
            msg_format(_("%^sはピーピー鳴いた。", "%^s twitters."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sはギャーギャー鳴いた。", "%^s squawks."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sはギャーギャー鳴きわめいた。", "%^s chatters."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは苦しんだ。", "%^s jeers."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sはのたうち回った。", "%^s flutters about."), m_name);
        else
            msg_format(_("%^sはキーキーと鳴き叫んだ。", "%^s squeaks."), m_name);
        return;
    }

    if (angband_strchr("duDLUW", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%sは攻撃を気にとめていない。", "%^s ignores the attack."), m_name);
        else if (percentage > 75)
            msg_format(_("%^sはしり込みした。", "%^s flinches."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sは痛みでシーッと鳴いた。", "%^s hisses in pain."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sは痛みでうなった。", "%^s snarls with pain."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは痛みに吠えた。", "%^s roars with pain."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sは苦しげに叫んだ。", "%^s gasps."), m_name);
        else
            msg_format(_("%^sは弱々しくうなった。", "%^s snarls feebly."), m_name);
        return;
    }

    if (angband_strchr("s", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%sは攻撃を気にとめていない。", "%^s ignores the attack."), m_name);
        else if (percentage > 75)
            msg_format(_("%sは攻撃に肩をすくめた。", "%^s shrugs off the attack."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sはカタカタと笑った。", "%^s rattles."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sはよろめいた。", "%^s stumbles."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sはカタカタ言った。", "%^s rattles."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sはよろめいた。", "%^s staggers."), m_name);
        else
            msg_format(_("%^sはガタガタ言った。", "%^s clatters."), m_name);
        return;
    }

    if (angband_strchr("z", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%sは攻撃を気にとめていない。", "%^s ignores the attack."), m_name);
        else if (percentage > 75)
            msg_format(_("%sは攻撃に肩をすくめた。", "%^s shrugs off the attack."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sはうめいた。", "%^s groans."), m_name);
        else if (percentage > 35)
            msg_format(_("%sは苦しげにうめいた。", "%^s moans."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは躊躇した。", "%^s hesitates."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sはうなった。", "%^s grunts."), m_name);
        else
            msg_format(_("%^sはよろめいた。", "%^s staggers."), m_name);
        return;
    }

    if (angband_strchr("G", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%sは攻撃を気にとめていない。", "%^s ignores the attack."), m_name);
        else if (percentage > 75)
            msg_format(_("%sは攻撃に肩をすくめた。", "%^s shrugs off the attack."), m_name);
        else if (percentage > 50)
            msg_format(_("%sはうめいた。", "%^s moans."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sは泣きわめいた。", "%^s wails."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは吠えた。", "%^s howls."), m_name);
        else if (percentage > 10)
            msg_format(_("%sは弱々しくうめいた。", "%^s moans softly."), m_name);
        else
            msg_format(_("%^sはかすかにうめいた。", "%^s sighs."), m_name);
        return;
    }

    if (angband_strchr("CZ", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%^sは攻撃に肩をすくめた。", "%^s shrugs off the attack."), m_name);
        else if (percentage > 75)
            msg_format(_("%^sは痛みでうなった。", "%^s snarls with pain."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sは痛みでキャンキャン吠えた。", "%^s yelps in pain."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sは痛みで鳴きわめいた。", "%^s howls in pain."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは苦痛のあまり鳴きわめいた。", "%^s howls in agony."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sは苦痛でもだえ苦しんだ。", "%^s writhes in agony."), m_name);
        else
            msg_format(_("%^sは弱々しく吠えた。", "%^s yelps feebly."), m_name);
        return;
    }

    if (angband_strchr("Xbilqrt", r_ptr->d_char)) {
        if (percentage > 95)
            msg_format(_("%^sは攻撃を気にとめていない。", "%^s ignores the attack."), m_name);
        else if (percentage > 75)
            msg_format(_("%^sは痛みでうなった。", "%^s grunts with pain."), m_name);
        else if (percentage > 50)
            msg_format(_("%^sは痛みで叫んだ。", "%^s squeals in pain."), m_name);
        else if (percentage > 35)
            msg_format(_("%^sは痛みで絶叫した。", "%^s shrieks in pain."), m_name);
        else if (percentage > 20)
            msg_format(_("%^sは苦痛のあまり絶叫した。", "%^s shrieks in agony."), m_name);
        else if (percentage > 10)
            msg_format(_("%^sは苦痛でもだえ苦しんだ。", "%^s writhes in agony."), m_name);
        else
            msg_format(_("%^sは弱々しく叫んだ。", "%^s cries out feebly."), m_name);
        return;
    }

    if (percentage > 95)
        msg_format(_("%^sは攻撃に肩をすくめた。", "%^s shrugs off the attack."), m_name);
    else if (percentage > 75)
        msg_format(_("%^sは痛みでうなった。", "%^s grunts with pain."), m_name);
    else if (percentage > 50)
        msg_format(_("%^sは痛みで叫んだ。", "%^s cries out in pain."), m_name);
    else if (percentage > 35)
        msg_format(_("%^sは痛みで絶叫した。", "%^s screams in pain."), m_name);
    else if (percentage > 20)
        msg_format(_("%^sは苦痛のあまり絶叫した。", "%^s screams in agony."), m_name);
    else if (percentage > 10)
        msg_format(_("%^sは苦痛でもだえ苦しんだ。", "%^s writhes in agony."), m_name);
    else
        msg_format(_("%^sは弱々しく叫んだ。", "%^s cries out feebly."), m_name);
}
