#include "birth/history-generator.h"
#include "birth/history.h"
#include "player/player-race-types.h"
#include "system/player-type-definition.h"
#include "util/buffer-shaper.h"

static int get_history_chart(player_type *creature_ptr)
{
    switch (creature_ptr->prace) {
    case player_race_type::RACE_AMBERITE:
        return 67;
    case player_race_type::RACE_HUMAN:
    case player_race_type::RACE_BARBARIAN:
    case player_race_type::RACE_DUNADAN:
        return 1;
    case player_race_type::RACE_HALF_ELF:
        return 4;
    case player_race_type::RACE_ELF:
    case player_race_type::RACE_HIGH_ELF:
        return 7;
    case player_race_type::RACE_HOBBIT:
        return 10;
    case player_race_type::RACE_GNOME:
        return 13;
    case player_race_type::RACE_DWARF:
        return 16;
    case player_race_type::RACE_HALF_ORC:
        return 19;
    case player_race_type::RACE_HALF_TROLL:
        return 22;
    case player_race_type::RACE_DARK_ELF:
        return 69;
    case player_race_type::RACE_HALF_OGRE:
        return 74;
    case player_race_type::RACE_HALF_GIANT:
        return 75;
    case player_race_type::RACE_HALF_TITAN:
        return 76;
    case player_race_type::RACE_CYCLOPS:
        return 77;
    case player_race_type::RACE_YEEK:
        return 78;
    case player_race_type::RACE_KOBOLD:
        return 82;
    case player_race_type::RACE_KLACKON:
        return 84;
    case player_race_type::RACE_NIBELUNG:
        return 87;
    case player_race_type::RACE_DRACONIAN:
        return 89;
    case player_race_type::RACE_MIND_FLAYER:
        return 92;
    case player_race_type::RACE_IMP:
        return 94;
    case player_race_type::RACE_GOLEM:
        return 98;
    case player_race_type::RACE_SKELETON:
        return 102;
    case player_race_type::RACE_ZOMBIE:
        return 107;
    case player_race_type::RACE_VAMPIRE:
        return 113;
    case player_race_type::RACE_SPECTRE:
        return 118;
    case player_race_type::RACE_SPRITE:
        return 124;
    case player_race_type::RACE_BEASTMAN:
        return 129;
    case player_race_type::RACE_ENT:
        return 137;
    case player_race_type::RACE_ARCHON:
        return 142;
    case player_race_type::RACE_BALROG:
        return 145;
    case player_race_type::RACE_S_FAIRY:
        return 148;
    case player_race_type::RACE_KUTAR:
        return 154;
    case player_race_type::RACE_ANDROID:
        return 155;
    case player_race_type::RACE_MERFOLK:
        return 170;
    default:
        return 0;
    }
}

/*!
 * @brief 生い立ちを画面に表示しつつ、種族から社会的地位を決定する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param buf 生い立ち情報のバッファ
 * @details 画面表示と社会的地位の決定が密結合していて分離できない
 */
static void decide_social_class(player_type *creature_ptr, char *buf)
{
    int social_class = randint1(4);
    int chart = get_history_chart(creature_ptr);
    while (chart != 0) {
        int i = 0;
        int roll = randint1(100);
        while ((chart != bg[i].chart) || (roll > bg[i].roll)) {
            i++;
        }

        (void)strcat(buf, bg[i].info);
        social_class += (int)(bg[i].bonus) - 50;
        chart = bg[i].next;
    }

    if (social_class > 100)
        social_class = 100;
    else if (social_class < 1)
        social_class = 1;

    creature_ptr->sc = (s16b)social_class;
}

/*!
 * @brief プレイヤーの生い立ちの自動生成を行う。 / Get the racial history, and social class, using the "history charts".
 */
void get_history(player_type *creature_ptr)
{
    for (int i = 0; i < 4; i++)
        creature_ptr->history[i][0] = '\0';

    char buf[240];
    buf[0] = '\0';
    decide_social_class(creature_ptr, buf);

    /* loop */
    char *s;
    for (s = buf; *s == ' '; s++)
        ;

    int n = strlen(s);
    while ((n > 0) && (s[n - 1] == ' '))
        s[--n] = '\0';

    {
        char temp[64 * 4];
        shape_buffer(s, 60, temp, sizeof(temp));
        char *t;
        t = temp;
        for (int i = 0; i < 4; i++) {
            if (t[0] == 0)
                break;
            else {
                strcpy(creature_ptr->history[i], t);
                t += strlen(t) + 1;
            }
        }
    }
}
