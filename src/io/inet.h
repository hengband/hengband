#pragma once

extern int soc_write(int sd, char *buf, size_t sz);
extern int soc_read(int sd, char *buf, size_t sz);
extern void set_proxy(char *default_url, int default_port);
extern int connect_server(int timeout, concptr host, int port);
extern int disconnect_server(int sd);
extern concptr soc_err(void);

extern bool browsing_movie;

