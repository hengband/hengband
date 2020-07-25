#include "core/disturbance.h"
#include "action/travel-execution.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "io/input-key-requester.h"
#include "player/attack-defense-types.h"
#include "status/action-setter.h"
#include "target/target-checker.h"
#include "term/screen-processor.h"

/*
 * Something has happened to disturb the player.
 * The first arg indicates a major disturbance, which affects search.
 * The second arg is currently unused, but could induce output flush.
 * All disturbance cancels repeated commands, resting, and running.
 */
void disturb(player_type *creature_ptr, bool stop_search, bool stop_travel)
{
    if (command_rep) {
        command_rep = 0;
        creature_ptr->redraw |= PR_STATE;
    }

    if ((creature_ptr->action == ACTION_REST) || (creature_ptr->action == ACTION_FISH) || (stop_search && (creature_ptr->action == ACTION_SEARCH)))
        set_action(creature_ptr, ACTION_NONE);

    if (creature_ptr->running) {
        creature_ptr->running = 0;
        if (center_player && !center_running)
            verify_panel(creature_ptr);

        creature_ptr->update |= PU_TORCH;
        creature_ptr->update |= PU_FLOW;
    }

    if (stop_travel) {
        travel.run = 0;
        if (center_player && !center_running)
            verify_panel(creature_ptr);

        creature_ptr->update |= PU_TORCH;
    }

    if (flush_disturb)
        flush();
}
