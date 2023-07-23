#include "game-option/disturbance-options.h"

bool find_ignore_stairs; /* Run past stairs */
bool find_ignore_doors; /* Run through open doors */
bool find_cut; /* Run past known corners */
bool check_abort; /* Check for user abort while in repeated command */
bool flush_failure; /* Flush input on various failures */
bool flush_disturb; /* Flush input whenever disturbed */
bool disturb_move; /* Disturb whenever any monster moves */
bool disturb_high; /* Disturb whenever high-level monster moves */
bool disturb_near; /* Disturb whenever viewable monster moves */
bool disturb_pets; /* Disturb when visible pets move */
bool disturb_panel; /* Disturb whenever map panel changes */
bool disturb_state; /* Disturb whenever player state changes */
bool disturb_minor; /* Disturb whenever boring things happen */
bool ring_bell; /* Audible bell (on errors, etc) */
bool disturb_trap_detect; /* Disturb when leaving trap detected area */
bool alert_trap_detect; /* Alert when leaving trap detected area */
