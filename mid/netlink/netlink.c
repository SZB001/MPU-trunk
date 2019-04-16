#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "netlink.h"
#include "log.h"


#define NETLINK_TEST    (25) // Protocol type number,need to be the same as kernel.
#define MAX_PAYLOAD     (20) // receive data


#if 0
#define NLMSG_ALIGNTO 4

#define NLMSG_ALIGN(len)  (((len)+NLMSG_ALIGNTO-1) & ~(NLMSG_ALIGNTO-1))
//宏NLMSG_ALIGN(len)用于得到不小于len且字节对齐的最小数值。

#define NLMSG_HDRLEN      ((int) NLMSG_ALIGN(sizeof(struct nlmsghdr)))
//头部长度

#define NLMSG_LENGTH(len) ((len)+NLMSG_ALIGN(NLMSG_HDRLEN))
//宏NLMSG_LENGTH(len)用于计算数据部分长度为len时实际的消息长度。它一般用于分配消息缓存。

#define NLMSG_SPACE(len)  NLMSG_ALIGN(NLMSG_LENGTH(len))
//宏NLMSG_SPACE(len)返回不小于NLMSG_LENGTH(len)且字节对齐的最小数值，它也用于分配消息缓存。

#define NLMSG_DATA(nlh)  ((void*)(((char*)nlh) + NLMSG_LENGTH(0)))
//宏NLMSG_DATA(nlh)用于取得消息的数据部分的首地址，设置和读取消息数据部分时需要使用该宏。
#endif


/****************************************************************
function:     netlink_sock
description:  greate a netlink socket fd
input:        none
output:       none
return:       socket fd
*****************************************************************/
int netlink_sock(void)
{
    return socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
}

/****************************************************************
function:     netlink_bind
description:  bind information about this application
input:        int sock_fd,
              BLOCK_TYPE type
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int netlink_bind(int sock_fd,BLOCK_TYPE type)
{
    int ret;
    struct sockaddr_nl addr;
    int netlink_mode= type; //1:non-blocking, 0:blocking
    
    memset(&addr, 0, sizeof(struct sockaddr_nl));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0;

    ret = bind(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_nl));
    if(ret < 0)
    {
    	log_e(LOG_MID, "netlink bind failed ,ret:%d", ret);
        return ret;
    }

    ioctl(sock_fd,FIONBIO,(u_long *)&netlink_mode);

    return 0;
}

/****************************************************************
function:     netlink_send
description:  send message to kernel
input:        int sock_fd,
              const char *message,
              int len,
              unsigned int pid,
              unsigned int group
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int netlink_send(int sock_fd, const char *message, int len, 
                      unsigned int pid, unsigned int group)
{
	struct nlmsghdr    *nlh;
    struct sockaddr_nl dest_addr;
    unsigned char temp[NLMSG_SPACE(MAX_PAYLOAD)];

    if (!message || (len > MAX_PAYLOAD) )
    {
    	log_e(LOG_MID, "para error!\n");
        return -1;
    }
	
	nlh = (struct nlmsghdr *)temp;

    nlh->nlmsg_len = NLMSG_SPACE(len);
    nlh->nlmsg_pid = getpid();  //Get pid,transfer to the kernel.
    nlh->nlmsg_flags = 0;
    memcpy(NLMSG_DATA(nlh), message, len);

    memset(&dest_addr, 0, sizeof(struct sockaddr_nl));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = pid;  //Set to 0 for from user to kernel
    dest_addr.nl_groups = group;
    
    //send message
    if (sendto(sock_fd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&dest_addr
               , sizeof(struct sockaddr_nl)) != nlh->nlmsg_len)
    {
        log_e(LOG_MID, "send error!\n");
        return -3;
    }

    return 0;
}

/****************************************************************
function:     netlink_recv
description:  recv message from kernel
input:        int sock_fd,
              char *message
output:       int *len
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int netlink_recv(int sock_fd, char *message, int *len)
{
	struct nlmsghdr *nlh = NULL;
	struct sockaddr_nl src_addr;
	socklen_t addrlen = sizeof(struct sockaddr_nl);
	unsigned char temp[NLMSG_SPACE(MAX_PAYLOAD)];

	if (!message || !len || (*len > MAX_PAYLOAD))
	{
		log_e(LOG_MID, "para error!\n");
		return -1;
	}
	
	nlh = (struct nlmsghdr *)temp;

	memset(&src_addr, 0, sizeof(struct sockaddr_nl));

	if (recvfrom(sock_fd, nlh, NLMSG_SPACE(MAX_PAYLOAD), 0, (struct sockaddr *)&src_addr, (socklen_t *)&addrlen) < 0)
	{
		log_e(LOG_MID, "recvmsg error!\n");
		return -3;
	}

	*len = nlh->nlmsg_len - NLMSG_SPACE(0);
	memcpy(message, (unsigned char *)NLMSG_DATA(nlh), *len);

	return 0;

}

