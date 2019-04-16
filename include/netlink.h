#ifndef __NETLINK_H__
#define __NETLINK_H__


typedef enum  BLOCK_TYPE
{
    BLOCK     = 0,
    NONBLOCK  = 1,
} BLOCK_TYPE;


int netlink_sock(void);
int netlink_bind(int sock_fd, BLOCK_TYPE type);
int netlink_send(int sock_fd, const char *message, int len,unsigned int pid, unsigned int group);
int netlink_recv(int sock_fd, char *message, int *len);


#endif
