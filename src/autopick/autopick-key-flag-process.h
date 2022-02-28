#pragma once

#define MATCH_KEY(KEY) (!strncmp(ptr, KEY, strlen(KEY))                             \
                            ? (ptr += strlen(KEY), (' ' == *ptr) ? ptr++ : 0, true) \
                            : false)
#define MATCH_KEY2(KEY) (!strncmp(ptr, KEY, strlen(KEY))                                             \
                             ? (prev_ptr = ptr, ptr += strlen(KEY), (' ' == *ptr) ? ptr++ : 0, true) \
                             : false)

#ifdef JP
#define ADD_KEY(KEY) strcat(ptr, KEY)
#else
#define ADD_KEY(KEY) (strcat(ptr, KEY), strcat(ptr, " "))
#endif
#define ADD_KEY2(KEY) strcat(ptr, KEY)

#define ADD_FLG(FLG) (entry->flag[FLG / 32] |= (1UL << (FLG % 32)))
#define REM_FLG(FLG) (entry->flag[FLG / 32] &= ~(1UL << (FLG % 32)))
#define ADD_FLG_NOUN(FLG) (ADD_FLG(FLG), prev_flg = FLG)
#define IS_FLG(FLG) (entry->flag[FLG / 32] & (1UL << (FLG % 32)))
