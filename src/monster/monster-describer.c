#include "monster/monster-describer.h"
#include "floor/floor.h"
#include "io/files-util.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster2.h" // todo 相互参照している.

/*!
 * @brief モンスターの呼称を作成する / Build a string describing a monster in some way.
 * @param desc 記述出力先の文字列参照ポインタ
 * @param m_ptr モンスターの参照ポインタ
 * @param mode 呼称オプション
 * @return なし
 */
void monster_desc(player_type *player_ptr, char *desc, monster_type *m_ptr, BIT_FLAGS mode)
{
    monster_race *r_ptr;
    r_ptr = &r_info[m_ptr->ap_r_idx];
    concptr name = (mode & MD_TRUE_NAME) ? (r_name + real_r_ptr(m_ptr)->name) : (r_name + r_ptr->name);
    GAME_TEXT silly_name[1024];
    bool named = FALSE;
    if (player_ptr->image && !(mode & MD_IGNORE_HALLU)) {
        if (one_in_(2)) {
            if (!get_rnd_line(_("silly_j.txt", "silly.txt"), m_ptr->r_idx, silly_name))
                named = TRUE;
        }

        if (!named) {
            monster_race *hallu_race;

            do {
                hallu_race = &r_info[randint1(max_r_idx - 1)];
            } while (!hallu_race->name || (hallu_race->flags1 & RF1_UNIQUE));

            strcpy(silly_name, (r_name + hallu_race->name));
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
#ifdef JP
        case 0x00:
            res = "何か";
            break;
        case 0x00 + (MD_OBJECTIVE):
            res = "何か";
            break;
        case 0x00 + (MD_POSSESSIVE):
            res = "何かの";
            break;
        case 0x00 + (MD_POSSESSIVE | MD_OBJECTIVE):
            res = "何か自身";
            break;
        case 0x00 + (MD_INDEF_HIDDEN):
            res = "何か";
            break;
        case 0x00 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
            res = "何か";
            break;
        case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
            res = "何か";
            break;
        case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
            res = "それ自身";
            break;
#else
        case 0x00:
            res = "it";
            break;
        case 0x00 + (MD_OBJECTIVE):
            res = "it";
            break;
        case 0x00 + (MD_POSSESSIVE):
            res = "its";
            break;
        case 0x00 + (MD_POSSESSIVE | MD_OBJECTIVE):
            res = "itself";
            break;
        case 0x00 + (MD_INDEF_HIDDEN):
            res = "something";
            break;
        case 0x00 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
            res = "something";
            break;
        case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
            res = "something's";
            break;
        case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
            res = "itself";
            break;
#endif

#ifdef JP
        case 0x10:
            res = "彼";
            break;
        case 0x10 + (MD_OBJECTIVE):
            res = "彼";
            break;
        case 0x10 + (MD_POSSESSIVE):
            res = "彼の";
            break;
        case 0x10 + (MD_POSSESSIVE | MD_OBJECTIVE):
            res = "彼自身";
            break;
        case 0x10 + (MD_INDEF_HIDDEN):
            res = "誰か";
            break;
        case 0x10 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
            res = "誰か";
            break;
        case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
            res = "誰かの";
            break;
        case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
            res = "彼自身";
            break;
#else
        case 0x10:
            res = "he";
            break;
        case 0x10 + (MD_OBJECTIVE):
            res = "him";
            break;
        case 0x10 + (MD_POSSESSIVE):
            res = "his";
            break;
        case 0x10 + (MD_POSSESSIVE | MD_OBJECTIVE):
            res = "himself";
            break;
        case 0x10 + (MD_INDEF_HIDDEN):
            res = "someone";
            break;
        case 0x10 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
            res = "someone";
            break;
        case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
            res = "someone's";
            break;
        case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
            res = "himself";
            break;
#endif

#ifdef JP
        case 0x20:
            res = "彼女";
            break;
        case 0x20 + (MD_OBJECTIVE):
            res = "彼女";
            break;
        case 0x20 + (MD_POSSESSIVE):
            res = "彼女の";
            break;
        case 0x20 + (MD_POSSESSIVE | MD_OBJECTIVE):
            res = "彼女自身";
            break;
        case 0x20 + (MD_INDEF_HIDDEN):
            res = "誰か";
            break;
        case 0x20 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
            res = "誰か";
            break;
        case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
            res = "誰かの";
            break;
        case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
            res = "彼女自身";
            break;
#else
        case 0x20:
            res = "she";
            break;
        case 0x20 + (MD_OBJECTIVE):
            res = "her";
            break;
        case 0x20 + (MD_POSSESSIVE):
            res = "her";
            break;
        case 0x20 + (MD_POSSESSIVE | MD_OBJECTIVE):
            res = "herself";
            break;
        case 0x20 + (MD_INDEF_HIDDEN):
            res = "someone";
            break;
        case 0x20 + (MD_INDEF_HIDDEN | MD_OBJECTIVE):
            res = "someone";
            break;
        case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE):
            res = "someone's";
            break;
        case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE):
            res = "herself";
            break;
#endif
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
            if ((m_ptr->mflag2 & MFLAG2_CHAMELEON) && !(mode & MD_TRUE_NAME)) {
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

    if ((mode & MD_IGNORE_HALLU) && (m_ptr->mflag2 & MFLAG2_CHAMELEON)) {
        if (r_ptr->flags1 & RF1_UNIQUE) {
            strcat(desc, _("(カメレオンの王)", "(Chameleon Lord)"));
        } else {
            strcat(desc, _("(カメレオン)", "(Chameleon)"));
        }
    }

    if ((mode & MD_IGNORE_HALLU) && !is_original_ap(m_ptr)) {
        strcat(desc, format("(%s)", r_name + r_info[m_ptr->r_idx].name));
    }

    /* Handle the Possessive as a special afterthought */
    if (mode & MD_POSSESSIVE) {
        (void)strcat(desc, _("の", "'s"));
    }
}
