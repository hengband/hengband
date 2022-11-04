#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

bool kind_is_cloak(KIND_OBJECT_IDX k_idx);
bool kind_is_polearm(KIND_OBJECT_IDX k_idx);
bool kind_is_sword(KIND_OBJECT_IDX k_idx);
bool kind_is_book(KIND_OBJECT_IDX k_idx);
bool kind_is_good_book(KIND_OBJECT_IDX k_idx);
bool kind_is_armor(KIND_OBJECT_IDX k_idx);
bool kind_is_hafted(KIND_OBJECT_IDX k_idx);
bool kind_is_potion(KIND_OBJECT_IDX k_idx);
bool kind_is_boots(KIND_OBJECT_IDX k_idx);
bool kind_is_amulet(KIND_OBJECT_IDX k_idx);
bool kind_is_good(KIND_OBJECT_IDX k_idx);

class BaseitemKey;
short lookup_kind(const BaseitemKey &key); // @details 作業用ラッパーメソッド、今後は下のメソッドではなくこちらに寄せる.
KIND_OBJECT_IDX lookup_kind(ItemKindType tval, OBJECT_SUBTYPE_VALUE sval);
