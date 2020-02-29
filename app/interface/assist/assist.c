/****************************************************************
file:         assist.c
description:  the source file of tbox assist implementation
date:         2017/7/14
author        liuwei
****************************************************************/
#include "com_app_def.h"
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include "assist.h"
#include "tcom_api.h"
#include "cfg_api.h"
#include "at_api.h"
#include "timer.h"
#include "rds.h"
#include "shell_api.h"
#include "assist_shell.h"
#include "nm_api.h"
#include "msg_parse.h"

static pthread_t assist_tid;    /* thread id */
static unsigned char assist_msgbuf[1024];

unsigned char request_water[4];

unsigned char request_num = 0;

int response_data_len = 0;
unsigned char response_data[MAX_SEND_BUF_SIZE];

int response_data_send_len = 0;
unsigned char response_data_send[MAX_SEND_BUF_SIZE * 2];
int sock_tcp_fd = -1;
int sock_udp_fd = -1;
static timer_t recreate_timer;
static int client_fd[MAX_CLIENT_NUM];
static struct_client clients[MAX_CLIENT_NUM];

unsigned char tcp_recv_buf[MAX_CLIENT_NUM][MAX_RECV_BUF_SIZE];
unsigned char udp_recv_buf[MAX_RECV_BUF_SIZE * 4];
unsigned char tmpbuf[BUF_SIZE * 4];
unsigned char reqbuff[BUF_SIZE * 500];

unsigned char pathtree[BUF_SIZE * 256];
unsigned int pathtree_size;

static struct_file_info filebuff;

static struct sockaddr_in udp_addr; /* udp client addr */

static unsigned int udpfile_total = 0;
static unsigned int udpfile_start = 0;
static unsigned int udpfile_len = 0;

static struct_op_map op_map[] =
{
    {OPERATE_UNICOM, "CHN-UNICOM"},
    {OPERATE_CMCC,  "CMCC"},
    {OPERATE_CUCC,  "CHN-CT"},
};


//unsigned char testlist[]="/flash/SD/DBC/E00090264_C11CB_EP2_20170210_c.dbc    C   56382   2000-01-01 00:00:01\n/fctdata/11111111(2).txt   C   159391  2000-01-01 00:12:05\n/fctdata/11111111(3).txt   C   361119  2000-01-01 00:24:24\n/fctdata/11111111(4).txt   C   46239   2000-01-01 00:04:28";
//unsigned char testlist[]="test.dbc    C\n/media/sdcard/test.txt   C\n/media/sdcard/test.txtt\nfctdata/11111111(2).txt C   159391  2000-01-01 00:12:05\n/fctdata/11111111(3).txt\n/fctdata/11111111(4).txt";

void makeResponseTailAndSend();
void processClientRequest(unsigned char *data, int datalen);
void executeRequestCommand(short requestCode, unsigned char *commandData, int datalen);
void makeSendResponse(short responseCode, unsigned char *data, int datalen);
void makeNGResponse(PROTOCOLNG_TYPE type, unsigned short reqCode);

int startCreateServerSocket()
{
    int i = 0;
    struct sockaddr_in serv_addr;

    socklen_t serv_addr_len = 0;

    for (i = 0; i < MAX_CLIENT_NUM; i++)
    {
        client_fd[i] = -1;
        clients[i].fd = -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    sock_tcp_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_tcp_fd < 0)
    {
        log_e(LOG_ASSIST, "Fail to socket,error:%s", strerror(errno));
        return -1;
    }

    bzero(&serv_addr, sizeof(serv_addr)); //������serv_addr��0
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ASSIST_SERVER_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    unsigned int value = 1;

    if (setsockopt(sock_tcp_fd, SOL_SOCKET, SO_REUSEADDR,
                   (void *)&value, sizeof(value)) < 0)
    {
        log_e(LOG_ASSIST, "Fail to setsockop, terror:%s", strerror(errno));
        return -2;
    }


    serv_addr_len = sizeof(serv_addr);

    if (bind(sock_tcp_fd, (struct sockaddr *)&serv_addr, serv_addr_len) < 0)
    {
        log_e(LOG_ASSIST, "Fail to bind,error:%s", strerror(errno));
        return -3;
    }

    if (listen(sock_tcp_fd, BACK_LOG) < 0)
    {
        log_e(LOG_ASSIST, "Fail to listen,error:%s", strerror(errno));
        return -4;
    }

    log_o(LOG_ASSIST, "Assist module create server socket success");

    return 0;
}

int startCreateUDPServerSocket(void)
{
    int opt = 1;
    struct sockaddr_in server;

    sock_udp_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_udp_fd == -1)
    {
        log_e(LOG_ASSIST, "udp socket failed, ret:%s", strerror(errno));
        return -1;
    }

    if (setsockopt(sock_udp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        log_e(LOG_ASSIST, "udp setsockopt failed, ret:%s", strerror(errno));
        return -1;
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(ASSIST_SERVER_PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_udp_fd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        log_e(LOG_ASSIST, "udp bind failed, ret:%s", strerror(errno));
        return -1;
    }

    int flags = fcntl(sock_udp_fd, F_GETFL, 0);
    fcntl(sock_udp_fd, F_SETFL, flags | O_NONBLOCK);

    return 0;
}


unsigned char *mergeFilepathOnSdcard(unsigned char *nowpath)
{
    unsigned char oldpath[256];

    if (strstr((const char *)nowpath, COM_SDCARD_DIR) == (char *)nowpath)
    {
        return nowpath;
    }

    memset(oldpath, 0, sizeof(oldpath));
    strcpy((char *)oldpath, (char *)nowpath);
    strcpy((char *)nowpath, COM_SDCARD_DIR);
    strcat((char *)nowpath, (char *)oldpath);

    return nowpath;
}

unsigned char makeChecksum(unsigned char *data, int datalen)
{
    unsigned char ret;
    int i = 0;

    ret = data[i++];

    while (i < datalen)
    {
        ret = ret ^ data[i++];
    }

    log_buf_dump(LOG_ASSIST, "Checksum buff", data, datalen);
    log_i(LOG_ASSIST, "checksum result:0x%02x", ret);
    return ret;
}
void TcpProcessProtocol(unsigned char *data, unsigned int datalen, void *para)
{
    int clientfd = *(int *)para;

    //log_test_dump(LOG_ASSIST, "assist TCP recv buff:", data, datalen);

    processClientRequest(data, datalen);
    makeResponseTailAndSend(clientfd);
}

void UdpProcessProtocol(unsigned char *data, unsigned int datalen, int clientfd)
{
    int i = 0, j = 0;
    unsigned char *protocoldata = (unsigned char *)tmpbuf;
    int realdatalen = 0;
    unsigned char *dataptr;

    if (datalen < 3 || (data[0] != IDENTIFIER && data[datalen - 1] != IDENTIFIER))
    {
        log_e(LOG_ASSIST, "invalid data format");
        return;
    }

    memcpy(protocoldata, data + 1, datalen - 2);

    //ת�崦��
    dataptr = data + 1;
    j = 0;

    for (i = 0; i < datalen - 3; i++)
    {
        if (dataptr[i] == TRANSFER_CHAR)
        {
            if ((i + 1 < datalen - 3) && (dataptr[i + 1] == 0x01))
            {
                protocoldata[j++] = TRANSFER_CHAR;
                i++;
            }
            else if ((i + 1 < datalen - 3) && (dataptr[i + 1] == 0x02))
            {
                protocoldata[j++] = 0x7e;
                i++;
            }
            else
            {
                log_e(LOG_ASSIST, "Parse data failed!!");
            }
        }
        else
        {
            protocoldata[j++] = dataptr[i];
        }
    }

    realdatalen = j;

    log_buf_dump(LOG_ASSIST, "after transfer:", protocoldata, realdatalen);

    //У��
    unsigned char checkresult = makeChecksum(protocoldata, realdatalen);

    if (checkresult == data[datalen - 2])
    {
        log_i(LOG_ASSIST, "check success\n");

        //����
        processClientRequest(protocoldata, realdatalen);

        //����response
        makeResponseTailAndSend(clientfd);
    }
    else
    {
        log_e(LOG_ASSIST, "check failed\n");
    }

    memset(tmpbuf, 0, sizeof(tmpbuf));
    protocoldata = NULL;
}

void makeResponseTailAndSend(int clientfd)
{
    unsigned char checksum = 0;
    int i = 0;
    int sendlen;
    int addr_len = sizeof(struct sockaddr_in);


    if (response_data_len <= 5)
    {
        log_e(LOG_ASSIST, "error ,don't send response");
        return;
    }

    //init
    response_data_send_len = 0;
    memset(response_data_send, 0, sizeof(response_data_send));
    response_data_send[response_data_send_len ++] = IDENTIFIER;

    //У��
    checksum = makeChecksum(response_data, response_data_len);
    response_data[response_data_len ++] = checksum;
    //responsestr[response_data_len ++] = IDENTIFIER;

    //ת��

    for (i = 0; i < response_data_len; i++)
    {
        if (response_data[i] == 0x7d || response_data[i] == 0x7e)
        {
            response_data_send[response_data_send_len ++] = 0x7d;
            response_data_send[response_data_send_len ++] = (response_data[i] == 0x7d ? 0x01 : 0x02);
        }
        else
        {
            response_data_send[response_data_send_len ++] = response_data[i];
        }
    }

    response_data_send[response_data_send_len++] = IDENTIFIER;

    //log
    log_i(LOG_ASSIST, "Start send fd:%d", clientfd);
    log_buf_dump(LOG_ASSIST, "send response:", response_data_send, response_data_send_len);

    //����
    if (clientfd == sock_udp_fd)
    {
        sendlen = sendto(clientfd, response_data_send, response_data_send_len, 0,
                         (struct sockaddr *)(&udp_addr),
                         addr_len);

    }
    else
    {
        sendlen = send(clientfd, response_data_send, response_data_send_len, 0);
    }

    if (sendlen < response_data_send_len)
    {
        log_e(LOG_ASSIST, "send response failed!!!!!");
    }
    else
    {
        log_i(LOG_ASSIST, "send response success");
    }
}

void processClientRequest(unsigned char *data, int datalen)
{
    int pos = 0;
    int i;

    //��ˮ
    memcpy(request_water, data, 4);
    pos += 4;

    //���������
    request_num = data[pos++];
    log_i(LOG_ASSIST, "request number : %d\n", request_num);

    //��ʼ��response  buffer
    memset(response_data, 0, sizeof(response_data));

    //����response������Ϣ

    memcpy(&response_data[0], request_water, 4);
    response_data[4] = request_num;
    response_data_len = 5;


    for (i = 0; i < request_num; i++)
    {
        log_i(LOG_ASSIST, "request number index: %d\n", i);

        //��ȡ������
        short ch1 = data[pos ++];
        short ch2 = data[pos++] ;

        //log_i(LOG_ASSIST,"ch1 = %d, ch2 = %d",ch1,ch2);

        short requestCode = ch1  + (ch2 << 8);
        log_i(LOG_ASSIST, "requestcode : 0x%x\n", requestCode);

        //���������ݳ���
        ch1 = data[pos ++];
        ch2 = data[pos++] ;

        //log_i(LOG_ASSIST,"ch1 = %d, ch2 = %d",ch1,ch2);
        short request_datalen = ch1 + (ch2 << 8) ;
        log_i(LOG_ASSIST, "request_datalen : 0x%x\n", request_datalen);

        if (request_datalen == 0)
        {
            executeRequestCommand(requestCode, NULL, 0);
        }
        else
        {
            //��ȡ��������
            unsigned char *request_data = (unsigned char *)reqbuff;
            memset(reqbuff, 0, sizeof(reqbuff));
            memcpy(request_data, &data[pos], request_datalen);
            pos += request_datalen;
            log_buf_dump(LOG_ASSIST, "client request_data:", request_data, request_datalen);

            if (*(request_data) == DATA_TYPE_STRING)
            {
                log_i(LOG_ASSIST, "client request str:%s", request_data + 1);
            }

            executeRequestCommand(requestCode, request_data, request_datalen);
            request_data = NULL;
        }
    }

}

int readFileData(unsigned char *filename, unsigned int startpos, unsigned int readlen,
                 unsigned char *outbuff, unsigned int *readsize)
{
    struct stat file_state;
    int ret , fd, k;
    unsigned int downsize = readlen;

    log_i(LOG_ASSIST, "startpos:%d, readlen:%d", startpos, readlen);
    ret = stat((char *)filename, &file_state);

    fd = open((char *)filename, O_RDONLY, S_IRUSR | S_IWUSR);

    if (fd < 0)
    {
        log_e(LOG_ASSIST, "open failed, error:%s, file name: %s", strerror(errno), (char *)filename);
        return -3;
    }

    if (file_state.st_size - startpos < readlen)
    {
        downsize = file_state.st_size - startpos;
    }

    for (k = 0; k < 3; k++)
    {
        ret = pread(fd, outbuff, downsize, startpos);

        if (ret > 0)
        {
            break;
        }
        else
        {
            if (EINTR == errno)
            {
                continue;
            }
        }
    }

    if (ret <= 0)
    {
        log_e(LOG_ASSIST, "read file error");
        ret = -2;
    }
    else
    {
        ret = 0;
    }

    *readsize = downsize;

    close(fd);
    return ret;
}

int scanallfiles(unsigned char *basepath)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1000];
    unsigned char filepath[256];
    struct stat fstat;
    int ret;
    int childnum = 0;

    if ((dir = opendir((char *)basepath)) == NULL)
    {
        log_e(LOG_ASSIST, "Open dir error...");
        return -1;
    }

    log_i(LOG_ASSIST, "dir:%s\n", (char *)basepath);

    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) ///current dir OR parrent dir
        {
            continue;
        }
        else if (ptr->d_type == 8)   ///file
        {
            memset(filepath, 0, sizeof(filepath));

            sprintf((char *)filepath, "%s/%s", (char *)basepath, ptr->d_name);
            log_i(LOG_ASSIST, "file:%s\n", ptr->d_name);
            ret = stat((char *)filepath, &fstat);
            struct tm *local;

            if (ret != 0)
            {
                log_e(LOG_ASSIST, "stat file error:%d  errcode:%d", ret, errno);
            }

            local = localtime(&fstat.st_mtime);

            sprintf((char *)pathtree, "%s%s/%s\t%c\t%u\t%04u-%02u-%02u %02u:%02u:%02u\r\n", (char *)pathtree,
                    basepath,
                    ptr->d_name, 'C', (unsigned int)fstat.st_size, local->tm_year + 1900,
                    local->tm_mon + 1, local->tm_mday,
                    local->tm_hour, local->tm_min, local->tm_sec);
            pathtree_size = strlen((char *)pathtree);
            childnum ++;
        }
        else if (ptr->d_type == 10)   ///link file
        {
            log_i(LOG_ASSIST, "link_file:%s\n", ptr->d_name);
        }
        else if (ptr->d_type == 4)   ///dir
        {
            memset(base, '\0', sizeof(base));
            strcpy(base, (char *)basepath);
            strcat(base, "/");
            strcat(base, ptr->d_name);
            scanallfiles((unsigned char *)base);
            childnum ++;
        }
    }

    log_i(LOG_ASSIST, "Child num:%d", childnum);

    if (childnum == 0)
    {
        sprintf((char *)pathtree, "%s%s/\r\n", (char *)pathtree, basepath);
        pathtree_size = strlen((char *)pathtree);
    }

    closedir(dir);
    return 0;
}

int getSDFilename(unsigned int startpos, unsigned int len, unsigned char *outbuff,
                  unsigned int *outsize)
{
    if (startpos == 0)
    {
        memset(pathtree, 0, sizeof(pathtree));
        scanallfiles((unsigned char *)COM_SDCARD_DIR);

        if (dir_exists(COM_USRDATA_DIR))
        {
            scanallfiles((unsigned char *)COM_USRDATA_DIR);
        }

        log_i(LOG_ASSIST, "FILE LIST:%s", pathtree);
        *outsize = pathtree_size;
    }
    else
    {
        if (startpos + len < pathtree_size)
        {
            memcpy(outbuff, pathtree + startpos - 1, len);
            *outsize = len;
        }
        else
        {
            memcpy(outbuff, pathtree + startpos - 1, pathtree_size - startpos + 1);
            *outsize = pathtree_size - startpos + 1;
        }
    }

    return 0;
}

int processDataDownload(unsigned char *inbuff, unsigned char *outbuff, unsigned int *outsize)
{
    int namelen = *((int *)inbuff);
    unsigned char name[256];
    unsigned int startpos, reqlen, readsize;
    int pos = 0;

    memset(name, 0, sizeof(name));
    memcpy(name, inbuff + sizeof(int), namelen);

    startpos = *((int *)(inbuff + sizeof(namelen) + namelen));
    reqlen  = *((int *)(inbuff + sizeof(namelen) + namelen + sizeof(startpos)));

    outbuff[pos++] = DATA_TYPE_BIN;
    outbuff[pos++] = startpos;
    outbuff[pos++] = startpos >> 8;
    outbuff[pos++] = startpos >> 16;
    outbuff[pos++] = startpos >> 24;
    outbuff[pos++] = reqlen;
    outbuff[pos++] = reqlen >> 8;
    outbuff[pos++] = reqlen >> 16;
    outbuff[pos++] = reqlen >> 24;

    log_i(LOG_ASSIST, "name: %s,namelen: %d, startpos: %d, reqlen: %d", name, namelen, startpos,
          reqlen);

    if (0 == strcmp((char *)name, "/"))
    {
        getSDFilename(startpos, reqlen, outbuff + pos, &readsize);
    }
    else
    {
        readFileData(name, startpos - 1, reqlen, outbuff + pos, &readsize);
    }

    *outsize = pos + readsize;
    return 0;
}

int processDataUDPDownload(unsigned char *inbuff, unsigned char *outbuff, unsigned int *outsize)
{

    unsigned char *filename = inbuff;

    //int namelen = *((int *)inbuff);
    //unsigned char name[200];
    unsigned int startpos, reqlen, readsize;
    int pos = 0;
    unsigned int len;

    //memset(name, 0, sizeof(name));
    //memcpy(name, inbuff + sizeof(int), namelen);

    if (udpfile_start >= udpfile_total)
    {
        udpfile_start = 1;
    }

    if ((udpfile_start + udpfile_len) > udpfile_total)
    {
        len = udpfile_total - udpfile_start + 1;
    }
    else
    {
        len = udpfile_len;
    }

    //startpos = *((int *)(inbuff + sizeof(namelen) + namelen));
    //reqlen  = *((int *)(inbuff + sizeof(namelen) + namelen + sizeof(startpos)));
    startpos = udpfile_start;
    reqlen  = len;


    outbuff[pos++] = DATA_TYPE_BIN;
    outbuff[pos++] = startpos;
    outbuff[pos++] = startpos >> 8;
    outbuff[pos++] = startpos >> 16;
    outbuff[pos++] = startpos >> 24;
    outbuff[pos++] = reqlen;
    outbuff[pos++] = reqlen >> 8;
    outbuff[pos++] = reqlen >> 16;
    outbuff[pos++] = reqlen >> 24;

    if (0 == strcmp((char *)filename, "/"))
    {
        getSDFilename(startpos, reqlen, outbuff + pos, &readsize);
    }
    else
    {
        readFileData(filename, startpos - 1, reqlen, outbuff + pos, &readsize);
    }

    udpfile_start += len;

    *outsize = pos + readsize;

    return 0;
}


/****************************************************************
function:     assist_get_call_status
description:  get call status use for app assist
input:        none
output:       unsigned char *status
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int assist_get_call_status(void)
{
    int status;
    int temp = at_get_call_status();

    /* incoming call */
    if ((temp == 4) || (temp == 5))
    {
        status = 0x01;
    }
    /* outgoing call */
   // else if ((temp == 2) || (temp == 3))
    else if(temp == 2)
    {
        status = 0x03;
    }
	else if(temp == 3)
	{
        status = 0x06;
    }
    /* on line */
    else if ((temp == 0) || (temp == 1))
    {
        status = 0x04;
    }
    /* idle */
    else if (temp == 6 || temp == 7)
    {
        status = 0x05;
    }
    else
    {
        status = 0x05;
    }

    return status;
}

void executeRequestCommand(short requestCode, unsigned char *commandData, int datalen)
{
    unsigned char data[2048];
    int datacount = 0;

    memset(data, 0, sizeof(data));

    log_i(LOG_ASSIST, "request code: %04x", requestCode);

    switch (requestCode)
    {
        //I/B-CALL
        case REQUEST_BCALL:
            {
                //do bcall
                unsigned char bcall[32];
                int ret;
                unsigned int len;

                memset(bcall, 0, sizeof(bcall));
                len = sizeof(bcall);
                ret = cfg_get_para(CFG_ITEM_BCALL, bcall, &len);

                if (ret != 0)
                {
                    log_e(LOG_ASSIST, "bcall read failed");
                    break;
                }

                if (strlen((char *)bcall) > 0)
                {
                    makecall((char *)bcall);
                    makeSendResponse(RESPONSE_BCALL, NULL, 0);
                }

                break;
            }

        case REQUEST_ICALL:
            {
                //do icall
                unsigned char icall[32];
                int ret;
                unsigned int len;

                memset(icall, 0, sizeof(icall));
                len = sizeof(icall);
                ret = cfg_get_para(CFG_ITEM_ICALL, icall, &len);

                if (ret != 0)
                {
                    log_e(LOG_ASSIST, "icall read failed");
                    break;
                }

                if (strlen((char *)icall) > 0)
                {
                    makecall((char *)icall);
                    makeSendResponse(RESPONSE_BCALL, NULL, 0);
                }

                break;
            }

        case REQUEST_ACCEPT_CALL:
            {
                //do accetp call
                answercall();
                makeSendResponse(RESPONSE_ACCEPT_CALL, NULL, 0);
                break;
            }

        case REQUEST_HANGUP_CALL:
            {
                //do hangup call
                disconnectcall();
                makeSendResponse(RESPONSE_HANGUP_CALL, NULL, 0);
                break;
            }

        case REQUEST_CALL_STATE:
            {
                data[datacount++] = 0x01;
                data[datacount++] = assist_get_call_status();
                makeSendResponse(RESPONSE_CALL_STATE, data, datacount);
                break;
            }

        //DATA
        case REQUEST_OPERATOR:
            {
                unsigned char opname[20];
                unsigned char opindex = 0xff;
                int i;

                memset(opname, 0, sizeof(opname));
                nm_get_operator(opname);

                for (i = 0; i < sizeof(op_map) / sizeof(struct_op_map); i++)
                {
                    if (0 == strcmp((char *)op_map[i].op_name, (char *)opname))
                    {
                        opindex = op_map[i].op_index;
                        break;
                    }
                }

                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_U8;
                data[1] = opindex;

                if (opindex != 0xff)
                {
                    makeSendResponse(RESPONSE_OPERATOR, data, 2);
                }

                break;
            }

        case REQUEST_STANDARD:
            {
                unsigned char nettype;

                nettype = nm_get_net_type();
                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_U8;
                data[1] = nettype;
                makeSendResponse(RESPONSE_STANDARD, data, 2);
                break;
            }

        case REQUEST_STANDARD_CFG:
            {
                int ret;
                unsigned char nettype;
                unsigned int len;

                len = sizeof(nettype);
                ret = cfg_get_para(CFG_ITEM_NET_TYPE, &nettype, &len);

                if (ret != 0)
                {
                    log_e(LOG_ASSIST, "net type read failed");
                    break;
                }

                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_U8;
                data[1] = nettype;
                makeSendResponse(RESPONSE_STANDARD, data, 2);
                break;
            }

        case REQUEST_SIGNAL:
            {
                unsigned char sigpower;

                sigpower = nm_get_signal();

                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_U8;
                data[1] = sigpower;
                makeSendResponse(RESPONSE_SIGNAL, data, 2);
                break;
            }

        case REQUEST_DATA_OPERAT:
            {
                unsigned char operate = commandData[1];

                cfg_set_para(CFG_ITEM_DCOM_SET, &operate, sizeof(operate));
                makeSendResponse(RESPONSE_DATA_OPERAT, NULL, 0);
                break;
            }

        case REQUEST_CFG_STANDARD:
            {
                unsigned char standard = commandData[1];
                unsigned int len;

                //����������ʽ
                len = sizeof(standard);
                cfg_set_para(CFG_ITEM_NET_TYPE, &standard, len);
                makeSendResponse(RESPONSE_CFG_STANDARD, NULL, 0);
                break;
            }

        case REQUEST_DATA_STATE:
            {
                unsigned char state;

                //��ȡ����ͨ�ſ���״̬
                state = nm_get_dcom();
                data[0] = DATA_TYPE_U8;
                data[1] = state;
                makeSendResponse(RESPONSE_DATA_STATE, data, 2);
                break;
            }

        //WLAN APP
        case REQUEST_TBOX_MAC:
            {
                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_STRING;
                wifi_get_ap_mac((char *)(data + 1));
                makeSendResponse(RESPONSE_TBOX_MAC, data, strlen((char *)data));
                break;
            }

        case REQUEST_CLIENT_MAC:
            {
                unsigned int len = 0;

                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_STRING;
                wifi_get_sta((char *)(data + 1), &len);
                makeSendResponse(RESPONSE_CLIENT_MAC, data, len);
                break;
            }

        case REQUEST_OPERATE_WLAN:
            {
                unsigned char operate = commandData[1];

                if (operate == 0)
                {
                    //close wlan
                    wifi_disable();
                }
                else if (operate == 1)
                {
                    //open wlan
                    wifi_enable();
                }

                makeSendResponse(RESPONSE_OPERATE_WLAN, NULL, 0);
                break;
            }

        case REQUEST_SET_SSID:
            {
                //set ssid
                if (wifi_set_ssid((char *)&commandData[1]) == 0)
                {
                    makeSendResponse(RESPONSE_SET_SSID, NULL, 0);
                }
                else
                {
                    makeNGResponse(PROTOCOLNG_ILLEGAL, requestCode);
                }

                break;
            }

        case REQUEST_SET_WIFI_PASSWORD:
            {
                //set password
                if (wifi_set_key((char *)&commandData[1]) == 0)
                {
                    makeSendResponse(RESPONSE_SET_WIFI_PASSWORD, NULL, 0);
                }
                else
                {
                    makeNGResponse(PROTOCOLNG_ILLEGAL, requestCode);
                }

                break;
            }

        case REQUEST_SET_MAX_CLIENT:
            {
                unsigned char max_client = commandData[1];

                //set max client
                if (wifi_set_max_user(max_client) == 0)
                {
                    makeSendResponse(RESPONSE_SET_MAX_CLIENT, NULL, 0);
                }
                else
                {
                    makeNGResponse(PROTOCOLNG_ILLEGAL, requestCode);
                }

                break;
            }

        case REQUEST_WLAN_STATE:
            {
                int ret;
                unsigned int len;
                unsigned char wifistatus;

                len = sizeof(wifistatus);
                ret = cfg_get_para(CFG_ITEM_WIFI_SET, &wifistatus, &len);

                if (ret != 0)
                {
                    log_e(LOG_ASSIST, "wifi status read failed");
                    break;
                }

                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_U8;
                data[1] = wifistatus;
                makeSendResponse(RESPONSE_WLAN_STATE, data, 2);
                break;
            }

        case REQUEST_SSID:
            {
                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_STRING;
                wifi_get_ssid((char *)(data + 1));
                makeSendResponse(RESPONSE_SSID, data, strlen((char *)data));
                break;
            }

        case REQUEST_WIFI_PASSWORD:
            {
                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_STRING;
                wifi_get_key((char *)(data + 1));
                makeSendResponse(RESPONSE_WIFI_PASSWORD, data, strlen((char *)data));
                break;
            }

        case REQUEST_MAX_CLIENT:
            {
                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_U8;
                wifi_get_max_user(data + 1);
                makeSendResponse(RESPONSE_WIFI_PASSWORD, data, 2);
                break;
            }

        //�ۺ�APP
        case REQUEST_SHELL:
            {
                int rsplen;

                shell_cmd_exec((char *)(commandData + 1), (char *)(data + 1), sizeof(data) - 1);

                if ((rsplen = strlen((char *)(data + 1))) > 0)
                {
                    data[0] = DATA_TYPE_STRING;

                    log_i(LOG_ASSIST, "Send shell rsp: %s", data + 1);
                    makeSendResponse(RESPONSE_SHELL, data, rsplen + 1);
                }
                else
                {
                    makeSendResponse(RESPONSE_SHELL, NULL, 0);
                }

                break;
            }

        case REQUEST_LOG:
            {
                break;
            }

        case REQUEST_DOWNLOAD_START:
            {
                unsigned char *filename = commandData + 1;
                unsigned int total;
                struct stat file_state;
                int ret;

                int filenamelen = strlen((char *)filename);
                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_U32;

                if (filename[filenamelen - 1] == '/')
                {
                    getSDFilename(0, 0, NULL, &total);
                }
                else
                {
                    ret = stat((char *)filename, &file_state);

                    if (ret != 0)
                    {
                        log_e(LOG_ASSIST, "stat file error:%d", errno);
                        total = 0;
                    }
                    else
                    {
                        total = file_state.st_size;
                    }
                }

                memcpy(data + 1, (unsigned char *)&total, sizeof(total));
                makeSendResponse(RESPONSE_DOWNLOAD_START, data, 5);
                break;
            }

        case REQUEST_DOWNLOAD_DATA:
            {
                unsigned int datasize;

                processDataDownload(commandData + 1, data, &datasize);
                makeSendResponse(RESPONSE_DOWNLOAD_DATA, data, datasize);
                break;
            }

        case REQUEST_UPLOAD_START:
            {
                unsigned int *namelen = (unsigned int *)(commandData + 1);
                unsigned int *datasize;
                int ret;
                char *ptr;

                memset(&filebuff, 0, sizeof(filebuff));
                memcpy(filebuff.filename, commandData + 1 + sizeof(unsigned int), *namelen);
                datasize = (unsigned int *)(commandData + 1 + sizeof(unsigned int) + (*namelen));
                filebuff.filesize = *datasize;

                if (strncmp((char *)filebuff.filename, COM_SDCARD_DIR, strlen(COM_SDCARD_DIR))
                    && strncmp((char *)filebuff.filename, COM_USRDATA_DIR, strlen(COM_USRDATA_DIR))
                    && strncmp((char *)filebuff.filename, "/sys/", strlen("/sys/")))
                {
                    log_e(LOG_ASSIST, "can't operate the dir :%s", (char *)filebuff.filename);
                    makeNGResponse(PROTOCOLNG_ILLEGAL, requestCode);
                    break;
                }

                if ((ptr = strstr((char *)filebuff.filename, "/sys/")) != NULL)
                {
                    if (ptr == (char *)filebuff.filename)
                    {
                        unsigned char changedir[256];
                        memset(changedir, 0, sizeof(0));
                        strcpy((char *)changedir, ptr + strlen("/sys"));
                        memset(filebuff.filename, 0, sizeof(filebuff.filename));
                        strcpy((char *)filebuff.filename, COM_APP_PKG_DIR);
                        strcat((char *)filebuff.filename, (char *)changedir);
                    }
                }

                if (path_exists((char *)filebuff.filename))
                {
                    if (!file_exists((char *)filebuff.filename))
                    {
                        log_e(LOG_ASSIST, "exist path,Create file failed :%s", (char *)filebuff.filename);
                        makeNGResponse(PROTOCOLNG_NOSUPPORT, requestCode);
                        break;
                    }
                    else
                    {
                        log_o(LOG_ASSIST, "delete file :%s", (char *)filebuff.filename);
                        file_delete((char *)filebuff.filename);
                    }
                }

                ret = dir_make_path((char *)filebuff.filename, S_IRUSR | S_IWUSR | S_IXUSR, true);

                if (ret != 0)
                {
                    log_e(LOG_ASSIST, "Create file failed :%s", (char *)filebuff.filename);
                    makeNGResponse(PROTOCOLNG_NOSUPPORT, requestCode);
                    break;
                }

                makeSendResponse(RESPONSE_UPLOAD_START, NULL, 0);
                break;
            }

        case REQUEST_UPLOAD_DATA:
            {
                unsigned int *namelen = (unsigned int *)(commandData + 1);
                unsigned char *name = (unsigned char *)namelen + sizeof(unsigned int);
                int fd;
                unsigned int wsize;

                //if(0 ==strncmp(name,filebuff.filename,*namelen))
                {
                    unsigned int *startpos = (unsigned int *)(name + *namelen);
                    unsigned int *datalen  = startpos + 1;
                    unsigned char *data     = (unsigned char *)datalen + sizeof(unsigned int);

                    //log_o(LOG_ASSIST, "Start upload data: startpos:%d, datalen:%d", *startpos, *datalen);
                    //memcpy(filebuff.data + (*startpos), data, *datalen);

                    //log_o(LOG_ASSIST, "filesize=%d,filename:%s", filebuff.filesize, (char*)filebuff.filename);

                    fd = open((char *)filebuff.filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                    wsize = pwrite(fd, data, *datalen, (*startpos - 1));

                    if (wsize < *datalen)
                    {
                        log_e(LOG_ASSIST, "Write file data failed");
                        makeNGResponse(PROTOCOLNG_NOSUPPORT, requestCode);
                        close(fd);
                        break;
                    }
                    else
                    {
                        log_i(LOG_ASSIST, "Write data success startpos:%d ,len:%d", *startpos, *datalen);
                    }

                    if (((*startpos) - 1) + (*datalen) == filebuff.filesize)
                    {
                        log_o(LOG_ASSIST, "over !! filepath:%s", (char *)filebuff.filename);
                        memset(&filebuff, 0, sizeof(filebuff));
                    }

                    close(fd);
                }

                makeSendResponse(RESPONSE_UPLOAD_DATA, NULL, 0);
                break;
            }

        case REQUEST_UDP_DOWNLOAD_START:
            {
                unsigned int total;
                struct stat file_state;

                unsigned int *namelen = (unsigned int *)(commandData + 1);
                unsigned int *datasize;
                int ret;

                memset(&filebuff, 0, sizeof(filebuff));
                memcpy((char *)filebuff.filename, commandData + 1 + sizeof(unsigned int), *namelen);
                datasize = (unsigned int *)(commandData + 1 + sizeof(unsigned int) + (*namelen));
                udpfile_len = *datasize;

                memset(data, 0, sizeof(data));
                data[0] = DATA_TYPE_U32;

                if (filebuff.filename[*namelen - 1] == '/')
                {
                    getSDFilename(0, 0, NULL, &total);
                }
                else
                {
                    if (file_isusing((char *)filebuff.filename))
                    {
                        log_e(LOG_ASSIST, "%s is using", filebuff.filename);
                        makeSendResponse(RESPONSE_NO_DOWNLOAD_START, data, 1);
                        break;
                    }
                    else
                    {
                        ret = stat((char *)filebuff.filename, &file_state);

                        if (ret != 0)
                        {
                            log_e(LOG_ASSIST, "stat file error:%d", errno);
                            total = 0;
                        }
                        else
                        {
                            total = file_state.st_size;
                        }
                    }
                }

                log_o(LOG_ASSIST, "start file: %s", filebuff.filename);

                udpfile_total = total;
                udpfile_start = 1;

                memcpy(data + 1, (unsigned char *)&total, sizeof(total));
                makeSendResponse(RESPONSE_UDP_DOWNLOAD_START, data, 5);
                break;
            }

        case REQUEST_UDP_DOWNLOAD_DATA:
            {
                unsigned int datasize;
                processDataUDPDownload(commandData + 1, data, &datasize);
                makeSendResponse(RESPONSE_UDP_DOWNLOAD_DATA, data, datasize);
                break;
            }


        default:
            {
                makeNGResponse(PROTOCOLNG_NOSUPPORT, requestCode);
            }
    }
}

void makeNGResponse(PROTOCOLNG_TYPE type, unsigned short reqCode)
{
    unsigned short resCode = reqCode | 0x4000;
    unsigned char data[3];

    switch (type)
    {
        case PROTOCOLNG_NOSUPPORT:
            data[0] = DATA_TYPE_U16;
            data[1] = 0x01;
            data[2] = 0x00;
            makeSendResponse(resCode, data, 3);
            break;

        case PROTOCOLNG_ILLEGAL:
            data[0] = DATA_TYPE_U16;
            data[1] = 0x02;
            data[2] = 0x00;
            makeSendResponse(resCode, data, 3);
            break;
    }
}

void makeSendResponse(short responseCode, unsigned char *data, int datalen)
{
    unsigned char responsestr[2048] = {0};
    int len = 0;

    if (data == NULL)
    {
        len = 0;

        responsestr[len ++] = responseCode & 0x00ff;
        responsestr[len ++] = responseCode >> 8;

        responsestr[len ++] = 0;
        responsestr[len ++] = 0;
    }
    else
    {
        len = 0;
        responsestr[len ++] = responseCode & 0x00ff;
        responsestr[len ++] = responseCode >> 8;
        responsestr[len ++] = datalen & 0xff;
        responsestr[len ++] = datalen >> 8;

        memcpy(&responsestr[len], data, datalen);
        len += datalen;
    }

    memcpy(&response_data[response_data_len], responsestr, len);
    response_data_len += len;
}

/****************************************************************
function:     assit_timer_msg_proc
description:  process the inner message from timer
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
void assit_timer_msg_proc(unsigned int timer_msgid)
{
    int ret = 0;

    if (timer_msgid == TIMER_ASSIST_RECREATE)
    {
        log_e(LOG_ASSIST, "assist recreate server socket");
        ret = startCreateServerSocket();

        if (ret != 0)
        {
            if (sock_tcp_fd > 0)
            {
                close(sock_tcp_fd);
                sock_tcp_fd = -1;
            }

            tm_start(recreate_timer, RECREATE_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
        }
    }
    else if (timer_msgid >= TIMER_ASSIST_TIMEOUT_START && timer_msgid <= TIMER_ASSIST_TIMEOUT_END)
    {
    }
}

int app_socket_test(int argc, const char **argv)
{
    int i;
    unsigned char data[] = {0x7e, 0x10, 0x00 , 0x00 , 0x00 , 0x01 , 0x01 , 0x84 , 0x05 , 0x00 , 0x03 , 0x40 , 0x08 , 0x00 , 0x00 , 0xca , 0x7e};
    int sendlen;

    for (i = 0; i < MAX_CLIENT_NUM; i++)
    {
        if (client_fd[i] != -1)
        {
            //log
            log_i(LOG_ASSIST, "test  Start send fd:%d", client_fd[i]);
            log_buf_dump(LOG_ASSIST, "send response:", data, 17);

            //����
            sendlen = send(client_fd[i], data, 17, 0);

            if (sendlen < 17)
            {
                log_e(LOG_ASSIST, "send response failed!!!!!");
            }
            else
            {
                log_e(LOG_ASSIST, "send response success");
            }
        }
    }

    shellprintf(" ok\r\n");
    return 0;
}

/****************************************************************
function:     assist_init
description:  initiaze tbox assist module
input:        INIT_PHASE phase, init phase;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int assist_init(INIT_PHASE phase)
{
    int ret = 0;
    int i = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            {
                for (i = 0; i < MAX_CLIENT_NUM; i++)
                {
                    clients[i].fd = -1;
                }

                break;
            }

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            {
                shell_cmd_register("testsend",      app_socket_test,    "send socket");
                ret = tm_create(TIMER_REL, TIMER_ASSIST_RECREATE, MPU_MID_ASSIST, &recreate_timer);

                if (ret != 0)
                {
                    log_e(LOG_ASSIST, "create timer TIMER_ASSIST_RECREATE failed ret=0x%08x", ret);
                    return ret;
                }

                break;
            }
    }

    assist_shell_init(phase);
    return 0;
}

/****************************************************************
function:     assist_main
description:  app tbox-assist module main function
input:        none
output:       none
return:       NULL
****************************************************************/
void *assist_main(void)
{
    int max_fd, tcom_fd, ret;
    TCOM_MSG_HEADER msghdr;
    fd_set write_set;
    fd_set select_read_set;
    static MSG_RX rx_msg[MAX_CLIENT_NUM];
    unsigned int sockudplen = sizeof(struct sockaddr_in);


    short i = 0;
    struct sockaddr_in cli_addr;
    int new_conn_fd = -1;

    prctl(PR_SET_NAME, "ASSIST");

    FD_ZERO(&write_set);
    FD_ZERO(&select_read_set);

    memset(&cli_addr, 0, sizeof(cli_addr));

    for (i = 0; i < MAX_CLIENT_NUM; i++)
    {
        msg_init_rx(&rx_msg[i], tcp_recv_buf[i], sizeof(tcp_recv_buf[i]));
    }

    tcom_fd = tcom_get_read_fd(MPU_MID_ASSIST);

    if (tcom_fd  < 0)
    {
        log_e(LOG_ASSIST, "tcom_get_read_fd failed");
        return NULL;
    }

    ret = startCreateServerSocket();

    if (ret != 0)
    {
        if (sock_tcp_fd > 0)
        {
            close(sock_tcp_fd);
            sock_tcp_fd = -1;
        }

        tm_start(recreate_timer, RECREATE_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
    }

    startCreateUDPServerSocket();

    while (1)
    {
        FD_ZERO(&select_read_set);
        FD_SET(tcom_fd, &select_read_set);

        if (sock_tcp_fd > 0)
        {
            FD_SET(sock_tcp_fd, &select_read_set);
        }

        max_fd = tcom_fd > sock_tcp_fd ? tcom_fd : sock_tcp_fd;

        for (i = 0; i < MAX_CLIENT_NUM; i++)
        {
            if (clients[i].fd <= 0)
            {
                continue;
            }

            FD_SET(clients[i].fd, &select_read_set);

            if (max_fd < clients[i].fd)
            {
                max_fd = clients[i].fd;
            }

            log_i(LOG_ASSIST, "client_fd[%d]=%d", i, clients[i].fd);
        }

        if (sock_udp_fd > 0)
        {
            FD_SET(sock_udp_fd, &select_read_set);
        }

        if (max_fd < sock_udp_fd)
        {
            max_fd = sock_udp_fd;
        }

        //log_i(LOG_ASSIST,"max_fd=%d, sock_tcp_fd=%d, tcom_fd=%d",max_fd,sock_tcp_fd,tcom_fd);

        /* monitor the incoming data */
        ret = select(max_fd + 1, &select_read_set, NULL, NULL, NULL);

        /* the file deccriptor is readable */
        if (ret > 0)
        {
            if (FD_ISSET(tcom_fd, &select_read_set))
            {
                ret = tcom_recv_msg(MPU_MID_ASSIST, &msghdr, assist_msgbuf);

                if (ret != 0)
                {
                    log_e(LOG_ASSIST, "tcom_recv_msg failed,ret:0x%08x", ret);
                    continue;
                }

                if (MPU_MID_ASSIST == msghdr.sender)
                {
                    log_o(LOG_ASSIST, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>msghdr id is :%d\r\n", msghdr.msgid);
                }
                else if (MPU_MID_TIMER == msghdr.sender)
                {
                    assit_timer_msg_proc(msghdr.msgid);
                }
                else if (MPU_MID_MID_PWDG == msghdr.msgid)
                {
                    pwdg_feed(MPU_MID_ASSIST);
                }
            }

            if (FD_ISSET(sock_tcp_fd, &select_read_set))
            {
                socklen_t len = sizeof(cli_addr);

                new_conn_fd = accept(sock_tcp_fd, (struct sockaddr *)&cli_addr, &len);
                log_i(LOG_ASSIST, "new client comes ,fd=%d\n", new_conn_fd);

                if (new_conn_fd < 0)
                {
                    log_e(LOG_ASSIST, "Fail to accept");
                    continue;
                }
                else
                {
                    for (i = 0; i < MAX_CLIENT_NUM; i++)
                    {
                        if (clients[i].fd == -1)
                        {
                            clients[i].fd = new_conn_fd;
                            clients[i].addr = cli_addr;
                            clients[i].lasthearttime = tm_get_time();
                            log_o(LOG_ASSIST, "add client_fd[%d]=%d", i, clients[i].fd);
                            break;
                        }
                    }

                    if (i >= MAX_CLIENT_NUM)
                    {
                        close(new_conn_fd);
                    }
                }
            }

            {
                for (i = 0; i < MAX_CLIENT_NUM; i++)
                {
                    int num = 0;

                    if (-1 == clients[i].fd)
                    {
                        continue;
                    }

                    if (tm_get_time() - clients[i].lasthearttime > 30000)
                    {
                        close(clients[i].fd);
                        clients[i].fd = -1;
                    }

                    if (FD_ISSET(clients[i].fd, &select_read_set))
                    {
                        log_i(LOG_ASSIST, "start read Client(%d) :%d\n", i, clients[i].fd);

                        if (rx_msg[i].used >= rx_msg[i].size)
                        {
                            rx_msg[i].used =  0;
                        }

                        num = recv(clients[i].fd, (rx_msg[i].data + rx_msg[i].used), rx_msg[i].size - rx_msg[i].used, 0);

                        if (num > 0)
                        {
                            clients[i].lasthearttime = tm_get_time();
                            rx_msg[i].used += num;
                            msg_decodex(&rx_msg[i], TcpProcessProtocol, &clients[i].fd);
                        }
                        else
                        {
                            if (num == 0 && (EINTR != errno))
                            {
                                log_e(LOG_ASSIST, "TCP client disconnect!!!!");
                            }

                            log_e(LOG_ASSIST, "Client(%d) exit\n", clients[i].fd);
                            close(clients[i].fd);
                            clients[i].fd = -1;
                        }
                    }
                }
            }

            {
                if (FD_ISSET(sock_udp_fd, &select_read_set))
                {
                    int num = 0;
                    memset(udp_recv_buf, 0, MAX_RECV_BUF_SIZE * 4);

                    num = recvfrom(sock_udp_fd, udp_recv_buf, sizeof(udp_recv_buf), 0,
                                   (struct sockaddr *)&udp_addr, &sockudplen);

                    if (num > 0)
                    {
                        UdpProcessProtocol(udp_recv_buf, num, sock_udp_fd);
                    }
                    else
                    {
                        log_e(LOG_ASSIST, "udp recv app failed, ret:%s", strerror(errno));

                        if ((-1 == num) && (errno != EINTR))
                        {
                            close(sock_udp_fd);
                            sock_udp_fd = -1;
                            startCreateUDPServerSocket();
                        }
                    }
                }
            }
        }
        else if (0 == ret)   /* timeout */
        {
            continue;   /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }

            log_e(LOG_ASSIST, "assist_main exit, error:%s", strerror(errno));
            break;  /* thread exit abnormally */
        }
    }

    return NULL;
}

/****************************************************************
function:     assist_run
description:  startup data communciation module
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int assist_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&assist_tid, &ta, (void *)assist_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_ASSIST, "pthread_create failed, error:%s", strerror(errno));
        return ret;
    }

    return 0;
}

