#include "player/player-sex.h"

/*
 * Pointer to the player tables
 * (sex, race, class, magic)
 */
const player_sex_type *sp_ptr;

/*!
 * @brief 性別表記 /
 * Player Sexes
 * @details
 * <pre>
 *      Title,
 *      Winner
 * </pre>
 */
const player_sex_type sex_info[MAX_SEXES] = {
    {
        { "女性", "Female" },
        { "クイーン", "Queen" },
    },
    {
        { "男性", "Male" },
        { "キング", "King" },
    },
};
