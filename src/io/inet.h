#pragma once

#include "system/angband.h"

int soc_write(int sd, char *buf, size_t sz);
int soc_read(int sd, char *buf, size_t sz);
void set_proxy(char *default_url, int default_port);
int connect_server(int timeout, concptr host, int port);
int disconnect_server(int sd);
concptr soc_err(void);
