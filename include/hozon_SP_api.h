#ifndef __HOZON_SP_API_H__
#define __HOZON_SP_API_H__

typedef enum
{
	PP_SP_COLSE_GB = 100,//
	PP_SP_COLSE_CDL = 200,//
	PP_SP_COLSE_PP = 300,//
    PP_SP_COLSE_SP = 400,//
} PP_SOCK_CLOSE_TYPE;

extern int sockproxy_init(INIT_PHASE phase);
extern int sockproxy_run(void);
extern int sockproxy_socketState(void);
extern int sockproxy_MsgSend(uint8_t* msg,int len,void (*sync)(void));
extern void sockproxy_socketclose(int type);
extern int sockproxy_sgsocketState(void);
extern void sockproxy_showParameters(void);
#endif
