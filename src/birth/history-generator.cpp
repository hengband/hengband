#include "birth/history-generator.h"
#include "birth/history.h"
#include "player-info/race-types.h"
#include "system/player-type-definition.h"
#include "util/buffer-shaper.h"

static int get_history_chart(PlayerType *player_ptr)
{
    switch (player_ptr->prace) {
    case PlayerRaceType::AMBERITE:
        return 67;
    case PlayerRaceType::HUMAN:
    case PlayerRaceType::BARBARIAN:
    case PlayerRaceType::DUNADAN:
        return 1;
    case PlayerRaceType::HALF_ELF:
        return 4;
    case PlayerRaceType::ELF:
    case PlayerRaceType::HIGH_ELF:
        return 7;
    case PlayerRaceType::HOBBIT:
        return 10;
    case PlayerRaceType::GNOME:
        return 13;
    case PlayerRaceType::DWARF:
        return 16;
    case PlayerRaceType::HALF_ORC:
        return 19;
    case PlayerRaceType::HALF_TROLL:
        return 22;
    case PlayerRaceType::DARK_ELF:
        return 69;
    case PlayerRaceType::HALF_OGRE:
        return 74;
    case PlayerRaceType::HALF_GIANT:
        return 75;
    case PlayerRaceType::HALF_TITAN:
        return 76;
    case PlayerRaceType::CYCLOPS:
        return 77;
    case PlayerRaceType::YEEK:
        return 78;
    case PlayerRaceType::KOBOLD:
        return 82;
    case PlayerRaceType::KLACKON:
        return 84;
    case PlayerRaceType::NIBELUNG:
        return 87;
    case PlayerRaceType::DRACONIAN:
        return 89;
    case PlayerRaceType::MIND_FLAYER:
        return 92;
    case PlayerRaceType::IMP:
        return 94;
    case PlayerRaceType::GOLEM:
        return 98;
    case PlayerRaceType::SKELETON:
        return 102;
    case PlayerRaceType::ZOMBIE:
        return 107;
    case PlayerRaceType::VAMPIRE:
        return 113;
    case PlayerRaceType::SPECTRE:
        return 118;
    case PlayerRaceType::SPRITE:
        return 124;
    case PlayerRaceType::BEASTMAN:
        return 129;
    case PlayerRaceType::ENT:
        return 137;
    case PlayerRaceType::ARCHON:
        return 142;
    case PlayerRaceType::BALROG:
        return 145;
    case PlayerRaceType::S_FAIRY:
        return 148;
    case PlayerRaceType::KUTAR:
        return 154;
    case PlayerRaceType::ANDROID:
        return 155;
    case PlayerRaceType::MERFOLK:
        return 170;
    default:
        return 0;
    }
}

/*!
 * @brief 生い立ちを画面に表示しつつ、種族から社会的地位を決定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param buf 生い立ち情報のバッファ
 * @details 画面表示と社会的地位の決定が密結合していて分離できない
 */
static void decide_social_class(PlayerType *player_ptr, char *buf)
{
    int social_class = randint1(4);
    int chart = get_history_chart(player_ptr);
    while (chart != 0) {
        int i = 0;
        int roll = randint1(100);
        while ((chart != backgrounds[i].chart) || (roll > backgrounds[i].roll)) {
            i++;
        }

        (void)strcat(buf, backgrounds[i].info);
        social_class += (int)(backgrounds[i].bonus) - 50;
        chart = backgrounds[i].next;
    }

    if (social_class > 100) {
        social_class = 100;
    } else if (social_class < 1) {
        social_class = 1;
    }

    player_ptr->sc = (int16_t)social_class;
}

/*!
 * @brief プレイヤーの生い立ちの自動生成を行う。 / Get the racial history, and social class, using the "history charts".
 */
void get_history(PlayerType *player_ptr)
{
    for (int i = 0; i < 4; i++) {
        player_ptr->history[i][0] = '\0';
    }

    char buf[240];
    buf[0] = '\0';
    decide_social_class(player_ptr, buf);

    /* loop */
    char *s;
    for (s = buf; *s == ' '; s++) {
        ;
    }

    int n = strlen(s);
    while ((n > 0) && (s[n - 1] == ' ')) {
        s[--n] = '\0';
    }

    {
        char temp[64 * 4];
        shape_buffer(s, 60, temp, sizeof(temp));
        char *t;
        t = temp;
        for (int i = 0; i < 4; i++) {
            if (t[0] == 0) {
                break;
            } else {
                strcpy(player_ptr->history[i], t);
                t += strlen(t) + 1;
            }
        }
    }
}
