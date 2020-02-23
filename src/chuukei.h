#pragma once

#ifdef CHUUKEI
extern bool chuukei_server;
extern bool chuukei_client;

extern int connect_chuukei_server(char *server_string);
extern void browse_chuukei(void);
extern void flush_ringbuf(void);
extern void prepare_chuukei_hooks(void);
#endif

extern void prepare_movie_hooks(void);
extern void prepare_browse_movie_aux(concptr filename);
extern void prepare_browse_movie(concptr filename);
extern void browse_movie(void);
