#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "util/int-char-converter.h"

#define REPEAT_MAX 20

/* Number of chars saved */
static int repeat__cnt = 0;

/* Current index */
static int repeat__idx = 0;

/* Saved "stuff" */
static COMMAND_CODE repeat__key[REPEAT_MAX];

void repeat_push(COMMAND_CODE what)
{
    if (repeat__cnt == REPEAT_MAX)
        return;

    repeat__key[repeat__cnt++] = what;
    ++repeat__idx;
}

bool repeat_pull(COMMAND_CODE *what)
{
    if (repeat__idx == repeat__cnt)
        return false;

    *what = repeat__key[repeat__idx++];
    return true;
}

void repeat_check(void)
{
    if (command_cmd == ESCAPE)
        return;
    if (command_cmd == ' ')
        return;
    if (command_cmd == '\r')
        return;
    if (command_cmd == '\n')
        return;

    COMMAND_CODE what;
    if (command_cmd == 'n') {
        repeat__idx = 0;
        if (repeat_pull(&what)) {
            command_cmd = what;
        }
    } else {
        repeat__cnt = 0;
        repeat__idx = 0;
        what = command_cmd;
        repeat_push(what);
    }
}
