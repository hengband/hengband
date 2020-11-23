#pragma once

#include "system/angband.h"

typedef struct flavor_type flavor_type;
void display_short_flavors(player_type *player_ptr, flavor_type *flavor_ptr);
void display_item_discount(flavor_type *flavor_ptr);
void display_item_fake_inscription(flavor_type *flavor_ptr);
