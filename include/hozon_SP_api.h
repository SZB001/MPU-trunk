#ifndef __HOZON_SP_API_H__
#define __HOZON_SP_API_H__


extern int sockproxy_init(INIT_PHASE phase);
extern int sockproxy_run(void);
extern int sockproxy_socketState(void);
extern int sockproxy_MsgSend(uint8_t* msg,int len,void (*sync)(void));
extern void sockproxy_socketclose(void);


#endif
