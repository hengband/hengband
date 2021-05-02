#pragma once

typedef struct flavor_type flavor_type;
typedef struct player_type player_type;
void display_short_flavors(player_type *player_ptr, flavor_type *flavor_ptr);
void display_item_discount(flavor_type *flavor_ptr);
void display_item_fake_inscription(flavor_type *flavor_ptr);
