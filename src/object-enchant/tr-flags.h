#pragma once

#include "object-enchant/tr-types.h"
#include "util/flag-group.h"

/*! オブジェクトの特性フラグtr_typeの集合を表すクラス */
using TrFlags = FlagGroup<tr_type, TR_FLAG_MAX>;

/*! pvalを増減させる特性フラグがONになっているFlagGroup定数オブジェクト */
inline const TrFlags TR_PVAL_FLAG_MASK{
    TR_STR,
    TR_INT,
    TR_WIS,
    TR_DEX,
    TR_CON,
    TR_CHR,
    TR_MAGIC_MASTERY,
    TR_STEALTH,
    TR_SEARCH,
    TR_INFRA,
    TR_TUNNEL,
    TR_SPEED,
    TR_BLOWS,
};
