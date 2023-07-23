#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "util/int-char-converter.h"

#define REPEAT_MAX 20

/* Number of chars saved */
static int repeat_counter = 0;

/* Current index */
static int repeat_index = 0;

/* Saved "stuff" */
static COMMAND_CODE repeat_keys[REPEAT_MAX];

void repeat_push(COMMAND_CODE what)
{
    if (repeat_counter == REPEAT_MAX) {
        return;
    }

    repeat_keys[repeat_counter++] = what;
    ++repeat_index;
}

bool repeat_pull(COMMAND_CODE *what)
{
    if (repeat_index == repeat_counter) {
        return false;
    }

    *what = repeat_keys[repeat_index++];
    return true;
}

void repeat_check(void)
{
    if (command_cmd == ESCAPE) {
        return;
    }
    if (command_cmd == ' ') {
        return;
    }
    if (command_cmd == '\r') {
        return;
    }
    if (command_cmd == '\n') {
        return;
    }

    COMMAND_CODE what;
    if (command_cmd == 'n') {
        repeat_index = 0;
        if (repeat_pull(&what)) {
            command_cmd = what;
        }
    } else {
        repeat_counter = 0;
        repeat_index = 0;
        what = command_cmd;
        repeat_push(what);
    }
}
