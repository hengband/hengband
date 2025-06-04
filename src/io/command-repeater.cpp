#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "util/int-char-converter.h"

#define REPEAT_MAX 20

/* Number of chars saved */
static int repeat_counter = 0;

/* Current index */
static int repeat_index = 0;

/* Saved "stuff" */
static short repeat_keys[REPEAT_MAX];

void repeat_push(short command)
{
    if (repeat_counter == REPEAT_MAX) {
        return;
    }

    repeat_keys[repeat_counter++] = command;
    ++repeat_index;
}

std::optional<short> repeat_pull()
{
    return repeat_index == repeat_counter ? std::nullopt : std::make_optional(repeat_keys[repeat_index++]);
}

void repeat_check()
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

    if (command_cmd == 'n') {
        repeat_index = 0;
        if (const auto code = repeat_pull(); code) {
            command_cmd = *code;
        }
    } else {
        repeat_counter = 0;
        repeat_index = 0;
        repeat_push(command_cmd);
    }
}
