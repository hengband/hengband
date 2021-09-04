#pragma once

#include "system/angband.h"

#define MAX_IMAGE_OBJECT_HACK 19
#define MAX_IMAGE_MONSTER_HACK 53

extern byte display_autopick;
extern char image_object_hack[MAX_IMAGE_OBJECT_HACK];
extern char image_monster_hack[MAX_IMAGE_MONSTER_HACK];

struct player_type;
void map_info(player_type *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp, TERM_COLOR *tap, SYMBOL_CODE *tcp);
