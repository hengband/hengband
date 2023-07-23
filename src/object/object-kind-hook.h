#pragma once

bool kind_is_cloak(short bi_id);
bool kind_is_polearm(short bi_id);
bool kind_is_sword(short bi_id);
bool kind_is_book(short bi_id);
bool kind_is_good_book(short bi_id);
bool kind_is_armor(short bi_id);
bool kind_is_hafted(short bi_id);
bool kind_is_potion(short bi_id);
bool kind_is_boots(short bi_id);
bool kind_is_amulet(short bi_id);
bool kind_is_good(short bi_id);

class BaseitemKey;
short lookup_baseitem_id(const BaseitemKey &key);
