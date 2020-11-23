#pragma once

#include "system/angband.h"

#ifdef CHUUKEI
extern bool chuukei_server;
extern bool chuukei_client;

int connect_chuukei_server(char *server_string);
void browse_chuukei(void);
void flush_ringbuf(void);
void prepare_chuukei_hooks(void);
#endif

void prepare_movie_hooks(player_type *player_ptr);
void prepare_browse_movie_aux(concptr filename);
void prepare_browse_movie(concptr filename);
void browse_movie(void);
