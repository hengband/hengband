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
