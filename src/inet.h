#pragma once

/* inet.c */
extern int soc_write(int sd, char *buf, size_t sz);
extern int soc_read(int sd, char *buf, size_t sz);
extern void set_proxy(char *default_url, int default_port);
extern int connect_server(int timeout, concptr host, int port);
extern int disconnect_server(int sd);
extern concptr soc_err(void);

extern void prepare_movie_hooks(void);
extern void prepare_browse_movie_aux(concptr filename);
extern void prepare_browse_movie(concptr filename);
extern void browse_movie(void);
extern bool browsing_movie;

