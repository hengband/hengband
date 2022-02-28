#include "player/player-personality.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/trc-types.h"

/*!
 * @brief 性格情報 /
 * Player Character
 */
const player_personality personality_info[MAX_PERSONALITIES] = {
    {
#ifdef JP
        "ふつう",
#endif
        "Ordinary",
        { 0, 0, 0, 0, 0, 0 },
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 0 },

    {
#ifdef JP
        "ちからじまん",
#endif
        "Mighty",
        { 2, -2, -1, 0, 1, 0 },
        -5, -5, -3, -1, -2, -2, 10, 0,
        1, 1, 0 },

    {
#ifdef JP
        "きれもの",
#endif
        "Shrewd",
        { -2, 2, 0, 1, -1, -1 },
        3, 8, 2, 0, -2, 5, -8, -5,
        -1, 1, 0 },

    {
#ifdef JP
        "しあわせもの",
#endif
        "Pious",
        { 0, -1, 2, -1, 0, 1 },
        -5, 2, 4, -1, 3, -2, -3, -6,
        0, 1, 0 },

    {
#ifdef JP
        "すばしっこい",
#endif
        "Nimble",
        { -1, 1, -1, 2, -1, -1 },
        7, 2, -1, 1, 5, 5, 0, 10,
        0, 0, 0 },

    {
#ifdef JP
        "いのちしらず",
#endif
        "Fearless",
        { 2, 1, 1, -1, -1, 0 },
        -5, 5, -2, 0, 2, -2, 10, 10,
        -1, 1, 0 },

    {
#ifdef JP
        "コンバット",
#endif
        "Combat",
        { 1, -1, -2, 2, 0, 1 },
        -2, -3, -3, 0, -1, 2, 5, 5,
        0, 0, 0 },

    {
#ifdef JP
        "なまけもの",
#endif
        "Lazy",
        { -2, -2, -2, -2, -2, -2 },
        -5, -5, -3, -1, -4, -2, -8, -8,
        -1, 1, 0 },

    {
#ifdef JP
        "セクシーギャル",
#endif
        "Sexy",
        { 1, 1, 1, 1, 1, 3 },
        10, 5, 3, 0, 4, 2, 10, 10,
        0, 1, 1 },

    {
#ifdef JP
        "ラッキーマン",
#endif
        "Lucky",
        { -2, -2, -2, -2, -2, 2 },
        10, 7, 3, 2, 10, 8, 15, 15,
        0, 1, 2 },

    {
#ifdef JP
        "がまんづよい",
#endif
        "Patient",
        { -1, -1, 1, -2, 2, 0 },
        -5, -3, 3, 1, 0, -3, -6, -6,
        1, 0, 0 },

    {
#ifdef JP
        "いかさま",
#endif
        "Munchkin",
        { 10, 10, 10, 10, 10, 10 },
        20, 40, 30, 10, 40, 40, 80, 80,
        15, 1, 0 },

    {
#ifdef JP
        "チャージマン",
#endif
        "Chargeman",
        { 2, -2, -2, 0, 1, -2 },
        -7, 7, -5, -1, -2, -4, 15, 20,
        -1, 0, 0 },

};

const player_personality *ap_ptr;
