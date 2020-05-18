#include "system/angband.h"
#include "birth/history-generator.h"
#include "birth/history.h"

/*!
 * @brief プレイヤーの生い立ちの自動生成を行う。 / Get the racial history, and social class, using the "history charts".
 * @return なし
 */
void get_history(player_type* creature_ptr)
{
    int i, n, chart, roll, social_class;

    char *s, *t;

    char buf[240];

    /* Clear the previous history strings */
    for (i = 0; i < 4; i++)
        creature_ptr->history[i][0] = '\0';

    /* Clear the history text */
    buf[0] = '\0';

    /* Initial social class */
    social_class = randint1(4);

    /* Starting place */
    switch (creature_ptr->prace) {
    case RACE_AMBERITE: {
        chart = 67;
        break;
    }
    case RACE_HUMAN:
    case RACE_BARBARIAN:
    case RACE_DUNADAN: {
        chart = 1;
        break;
    }
    case RACE_HALF_ELF: {
        chart = 4;
        break;
    }
    case RACE_ELF:
    case RACE_HIGH_ELF: {
        chart = 7;
        break;
    }
    case RACE_HOBBIT: {
        chart = 10;
        break;
    }
    case RACE_GNOME: {
        chart = 13;
        break;
    }
    case RACE_DWARF: {
        chart = 16;
        break;
    }
    case RACE_HALF_ORC: {
        chart = 19;
        break;
    }
    case RACE_HALF_TROLL: {
        chart = 22;
        break;
    }
    case RACE_DARK_ELF: {
        chart = 69;
        break;
    }
    case RACE_HALF_OGRE: {
        chart = 74;
        break;
    }
    case RACE_HALF_GIANT: {
        chart = 75;
        break;
    }
    case RACE_HALF_TITAN: {
        chart = 76;
        break;
    }
    case RACE_CYCLOPS: {
        chart = 77;
        break;
    }
    case RACE_YEEK: {
        chart = 78;
        break;
    }
    case RACE_KOBOLD: {
        chart = 82;
        break;
    }
    case RACE_KLACKON: {
        chart = 84;
        break;
    }
    case RACE_NIBELUNG: {
        chart = 87;
        break;
    }
    case RACE_DRACONIAN: {
        chart = 89;
        break;
    }
    case RACE_MIND_FLAYER: {
        chart = 92;
        break;
    }
    case RACE_IMP: {
        chart = 94;
        break;
    }
    case RACE_GOLEM: {
        chart = 98;
        break;
    }
    case RACE_SKELETON: {
        chart = 102;
        break;
    }
    case RACE_ZOMBIE: {
        chart = 107;
        break;
    }
    case RACE_VAMPIRE: {
        chart = 113;
        break;
    }
    case RACE_SPECTRE: {
        chart = 118;
        break;
    }
    case RACE_SPRITE: {
        chart = 124;
        break;
    }
    case RACE_BEASTMAN: {
        chart = 129;
        break;
    }
    case RACE_ENT: {
        chart = 137;
        break;
    }
    case RACE_ANGEL: {
        chart = 142;
        break;
    }
    case RACE_DEMON: {
        chart = 145;
        break;
    }
    case RACE_S_FAIRY: {
        chart = 148;
        break;
    }
    case RACE_KUTAR: {
        chart = 154;
        break;
    }
    case RACE_ANDROID: {
        chart = 155;
        break;
    }
    case RACE_MERFOLK: {
        chart = 170;
        break;
    }
    default: {
        chart = 0;
        break;
    }
    }

    /* Process the history */
    while (chart) {
        /* Start over */
        i = 0;

        /* Roll for nobility */
        roll = randint1(100);

        /* Access the proper entry in the table */
        while ((chart != bg[i].chart) || (roll > bg[i].roll)) {
            i++;
        }

        /* Acquire the textual history */
        (void)strcat(buf, bg[i].info);

        /* Add in the social class */
        social_class += (int)(bg[i].bonus) - 50;

        /* Enter the next chart */
        chart = bg[i].next;
    }

    /* Verify social class */
    if (social_class > 100)
        social_class = 100;
    else if (social_class < 1)
        social_class = 1;

    /* Save the social class */
    creature_ptr->sc = (s16b)social_class;

    /* Skip leading spaces */
    for (s = buf; *s == ' '; s++) /* loop */
        ;

    /* Get apparent length */
    n = strlen(s);

    /* Kill trailing spaces */

    while ((n > 0) && (s[n - 1] == ' '))
        s[--n] = '\0';

    {
        char temp[64 * 4];
        roff_to_buf(s, 60, temp, sizeof(temp));
        t = temp;
        for (i = 0; i < 4; i++) {
            if (t[0] == 0)
                break;
            else {
                strcpy(creature_ptr->history[i], t);
                t += strlen(t) + 1;
            }
        }
    }
}
