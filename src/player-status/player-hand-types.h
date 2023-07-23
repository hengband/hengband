#pragma once

/*!< Empty hand status */
enum empty_hand_status {
    EMPTY_HAND_NONE = 0x0000, /*!<Both hands are used */
    EMPTY_HAND_SUB = 0x0001, /*!<Sub hand is empty */
    EMPTY_HAND_MAIN = 0x0002, /*!<Main hand is empty */
};

/*!< Weapon hand status */
enum player_hand {
    PLAYER_HAND_MAIN = 0x0000,
    PLAYER_HAND_SUB = 0x0001,
    PLAYER_HAND_OTHER = 0x0002,
};
