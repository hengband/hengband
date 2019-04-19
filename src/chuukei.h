#pragma once

#ifdef CHUUKEI
/* chuukei.c */
extern bool chuukei_server;
extern bool chuukei_client;

extern int connect_chuukei_server(char *server_string);
extern void browse_chuukei(void);
extern void flush_ringbuf(void);
extern void prepare_chuukei_hooks(void);
#endif
