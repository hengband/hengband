#pragma once

#include "system/angband.h"

extern bool find_ignore_stairs; /* Run past stairs */
extern bool find_ignore_doors; /* Run through open doors */
extern bool find_cut; /* Run past known corners */
extern bool check_abort; /* Check for user abort while continuous command */
extern bool flush_failure; /* Flush input on various failures */
extern bool flush_disturb; /* Flush input whenever disturbed */
extern bool disturb_move; /* Disturb whenever any monster moves */
extern bool disturb_high; /* Disturb whenever high-level monster moves */
extern bool disturb_near; /* Disturb whenever viewable monster moves */
extern bool disturb_pets; /* Disturb when visible pets move */
extern bool disturb_panel; /* Disturb whenever map panel changes */
extern bool disturb_state; /* Disturb whenever player state changes */
extern bool disturb_minor; /* Disturb whenever boring things happen */
extern bool ring_bell; /* Audible bell (on errors, etc) */
extern bool disturb_trap_detect; /* Disturb when leaving trap detected area */
extern bool alert_trap_detect; /* Alert when leaving trap detected area */
