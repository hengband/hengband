#include "birth/history-generator.h"
#include "birth/history.h"
#include "player-info/race-types.h"
#include "system/player-type-definition.h"
#include "util/buffer-shaper.h"

static int get_history_chart(player_type *creature_ptr)
{
    switch (creature_ptr->prace) {
    case player_race_type::AMBERITE:
        return 67;
    case player_race_type::HUMAN:
    case player_race_type::BARBARIAN:
    case player_race_type::DUNADAN:
        return 1;
    case player_race_type::HALF_ELF:
        return 4;
    case player_race_type::ELF:
    case player_race_type::HIGH_ELF:
        return 7;
    case player_race_type::HOBBIT:
        return 10;
    case player_race_type::GNOME:
        return 13;
    case player_race_type::DWARF:
        return 16;
    case player_race_type::HALF_ORC:
        return 19;
    case player_race_type::HALF_TROLL:
        return 22;
    case player_race_type::DARK_ELF:
        return 69;
    case player_race_type::HALF_OGRE:
        return 74;
    case player_race_type::HALF_GIANT:
        return 75;
    case player_race_type::HALF_TITAN:
        return 76;
    case player_race_type::CYCLOPS:
        return 77;
    case player_race_type::YEEK:
        return 78;
    case player_race_type::KOBOLD:
        return 82;
    case player_race_type::KLACKON:
        return 84;
    case player_race_type::NIBELUNG:
        return 87;
    case player_race_type::DRACONIAN:
        return 89;
    case player_race_type::MIND_FLAYER:
        return 92;
    case player_race_type::IMP:
        return 94;
    case player_race_type::GOLEM:
        return 98;
    case player_race_type::SKELETON:
        return 102;
    case player_race_type::ZOMBIE:
        return 107;
    case player_race_type::VAMPIRE:
        return 113;
    case player_race_type::SPECTRE:
        return 118;
    case player_race_type::SPRITE:
        return 124;
    case player_race_type::BEASTMAN:
        return 129;
    case player_race_type::ENT:
        return 137;
    case player_race_type::ARCHON:
        return 142;
    case player_race_type::BALROG:
        return 145;
    case player_race_type::S_FAIRY:
        return 148;
    case player_race_type::KUTAR:
        return 154;
    case player_race_type::ANDROID:
        return 155;
    case player_race_type::MERFOLK:
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

    creature_ptr->sc = (int16_t)social_class;
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
