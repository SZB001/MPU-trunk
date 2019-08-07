/****************************************************************
 file:         at_main.c
 description:  the header file of at main function implemention
 date:         2019/6/4
 author:       liuquanfu
 ****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "init.h"
#include "tcom_api.h"
#include "init.h"
#include "gpio.h"
#include "timer.h"
#include "pm_api.h"
#include "at_api.h"
#include "cfg_api.h"
#include "dev_api.h"
#include "shell_api.h"
#include "scom_api.h"
#include "signal.h"
#include "ble.h"
#include "pwdg.h" 
#include "com_app_def.h"
#include "init.h"
#include "bt_interface.h"
#include "btsock.h"
#include "cm256_if.h"
#include "hz_bt_usrdata.h"
#include "hozon_PP_api.h"
#include <unistd.h>
#include<sys/types.h>

#include<dirent.h>

static pthread_t ble_tid; /* thread id */
BLE_MEMBER				g_BleMember;
static unsigned char	g_pucbuf[TCOM_MAX_MSG_LEN];
static int				g_iBleSleep = 0; 
BT_DATA              	g_stBt_Data;
BLE_CONTR				g_BleContr;
typedef struct
{
	uint8_t msg_type;
	uint8_t cmd;
	uint8_t result; // 0表示成功  1表示失败
	uint8_t failtype;
}__attribute__((packed))  PrvtProt_respbt_t; /*resp bt结构体*/


extern void ApiTraceBuf(unsigned char *buf, unsigned int len);
/******************************************************************************
* Function Name  : PrintBuf
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
void PrintBuf(unsigned char *Buf, unsigned long Len)
{
    int i;

    if (!Len)
    {
        return;
    }

    for(i = 0; i < Len; i++)
    {
        if(i && ((i & 0x0f) == 0))
        {
            printf("\n");
        }

		printf("%02X ", *((unsigned char volatile *)((unsigned long)Buf+i)));

        if ((i + 1) % 4 == 0)
        {
            printf(" ");
        }

        if ((i + 1) % 8 == 0)
        {
            printf(" ");
        }
    }

    printf("\n");
}
#ifdef DEBUG_LQF
void ApiTraceBuf(unsigned char *buf, unsigned int len)
{
	PrintBuf(buf,len);
}

#else
void ApiTraceBuf(unsigned char *buf, unsigned int len)
{

}
#endif
/******************************************************************************
* Function Name  : ApiBLETraceBuf
* Description    :   
* Input          :  
* Return         : NONE
******************************************************************************/
void ApiBLETraceBuf(unsigned char *buf, unsigned int len)
{
	ApiTraceBuf(buf, len);
}

/****************************************************************
 function:     BleSendMsg
 description:  Send a message as soon as you receive the data
 input:        blemsg:0 recv message    1 send message  
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
int BleSendMsg(unsigned short usMid, int iDate)
{
	TCOM_MSG_HEADER stMsg;
	int iMsgData = 1;

	if (0 != iDate)
	{
		iMsgData = iDate;
	}

	stMsg.msgid	 = usMid;
	stMsg.sender	 = MPU_MID_BLE;
	stMsg.receiver = MPU_MID_BLE;
	stMsg.msglen	 = sizeof(iMsgData);

	return tcom_send_msg(&stMsg, &iMsgData);
}
/****************************************************************
 function:     BleSendMsgToApp
 description:  Send a message as soon as you receive the data
 input:        blemsg:0 recv message    1 send message  2�Ͽ�����
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
int BleSendMsgToApp(unsigned short usMid)
{

	TCOM_MSG_HEADER stMsg;
	int iMsgData = 1;

	stMsg.msgid	 = usMid;
	stMsg.sender	 = MPU_MID_BLE;
	//stMsg.receiver = MPU_MID_AICHI;
	stMsg.receiver = MPU_MID_BLE;
	stMsg.msglen	 = sizeof(iMsgData);

	return tcom_send_msg(&stMsg, &iMsgData);
}


/****************************************************************
 function:     BleSleepHandler
 description:   
 input:        none
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
static int BleSleepHandler(PM_EVT_ID id)
{
    return g_iBleSleep;// 
}

/******************************************************************************
* Function Name  : hz_so_test
* Description    :  init
* Input          :  
* Return         : NONE
******************************************************************************/
int hz_so_test(void)
{
		   //char	 plaintext[128]="zxy30555 0123456789 hozon";
	char	plaintext[128]="zxy30555 0123456789 hozon h1232";
		  
		   char    chiperPath[128]="\0";
		   char    afterdecrypt[128]="\0";
		   char    key[] ="1832521868741387492628667237737729841724568366976937763254826565";
			//char key[] ="8888598527857736162464368777871656575125719259765892327872456299";
		   int ret=0;
		   int clen=0;
		   int mlen=strlen(plaintext);
		  //int size=  256;
		  printf("\n\nplaintext is this[%d]\n\n",mlen);
	       char ver[20]="\0";
		   ret=showversion(ver);
		   printf("\nthe version +++++++++++++++++++++++[%s]\n ", ver);
		   //ApiBLETraceBuf(plaintext, strlen(plaintext));
	
		   ret=HzBtSymEncrypt(plaintext, strlen(plaintext), chiperPath, &clen, key, 1);
			  //printf("----------[%d]\n",chiperPath[0]);		  
	
	
		   if(ret==1){
			  if(chiperPath==NULL){
				  printf("\n\nchiper is null\n\n");
				  return 1;
			   }
		   }   
			 // printf("frist \n\n%d\n\n",clen);
			  //ApiBLETraceBuf(chiperPath, clen);
			  printf("++++[%d]\n\n",clen);
	
	
		   ret=HzBtSymEncrypt(chiperPath, clen, afterdecrypt, &mlen, key, 0);
		   //mlen=strlen(plaintext);
	
		   if(ret==1){
			  printf("\ndecrypt success\n");
			  printf("[%s]\n", afterdecrypt);
	
			  //ApiBLETraceBuf(afterdecrypt, mlen);
			  printf("\n\n%d\n\n",mlen);
	
			}
		   else return 1;
		
	
		   return 0;

#if 0
	 //char  plaintext[128]="zxy30555@163.com1";
	 char	 plaintext[128]="\x0A\x04\x08\x01\x10\x03\x12\x0C\x08\x13\x10\x07\x18\x0F\x20\x12\x28\x30\x30\x31\x18\x01\x22\x01\x00";
	// char	 chiperPath[128]="\x56\x21\xD7\xF1\x02\xE6\xCA\x72\x43\x28\x85\x42\x0A\x3A\x8C\x60\x1C\x6C\x2A\x4E\xFC\x6D\xD8\xFF\x12\xFD\xDA\xD5\xC2\xDC\xA4\x9D";
	
	 char	 chiperPath[128]="\x60\x47\x0F\xD9\x4E\x84\xBC\xF4\xC9\xE3\x80\x46\xEC\xD2\xCC\x58\xBA\x0C\xEB\x71\xF4\xB7\x64\x58\x0F\x5F\xDD\x09\xBF\xAB\x81\xF6";
	// char	 chiperPath[128]="\x03\xD6\x84\x07\x43\x3F\x07\x39\x27\x28\x0A\x09\x1C\xEF\x03\x2F\xA7\xC5\x25\x46\xBD\xB6\x37\x88\x0C\x5B\xCA\x24\x14\xC9\x13\x1F";
	 //char    key[] ="1832521868741387492628667237737729841724568366976937763254826565";
	 char	 key[] ="8888598527857736162464368777871656575125719259765892327872456299";
	 int ret=0;
	 int clen=0;
	 
	ret = HzBtCertcfg(COM_APP_PEM_ROOT_DIR, COM_APP_PEM_TPONE_DIR);

	if(ret)
	{
		log_i(LOG_BLE, "HzBtCertcfg sucess \r\n");
	}
	else
	{
		log_e(LOG_BLE, "HzBtCertcfg Fail \r\n");
		return -1;
	}			
	 printf("\n\nplaintext is this\n\n");
	
	 ApiBLETraceBuf((unsigned char *)plaintext, 25);
	 
	// ret=HzBtSymEncrypt(plaintext, 25, chiperPath, &clen, key, 0);
	ret = HzBtSymEncrypt(chiperPath, 32, plaintext, &clen, key, 0); //ency:zxy30555 0123456789 zxy3055
	 
	if(ret==1){
	   if(chiperPath==NULL){
		   printf("\n\nchiper is null\n\n");
		   return 1;
		}
	   printf("frist \n\n%d\n\n",clen);
	   ApiBLETraceBuf((unsigned char *)plaintext, clen);
	   printf("\n\n%d\n\n",clen);
	   
	 }

	log_e(LOG_BLE, "protocol_header__unpack Fai22l \r\n");

       ProtocolHeader *request =  protocol_header__unpack(NULL, clen, (const uint8_t *)plaintext);
		if (NULL== request)
		{
			log_e(LOG_BLE, "protocol_header__unpack Fail \r\n");
			return YT_ERR;
		}

		log_i(LOG_BLE, "request->head->protocol_version = %d\r\n",request->head->protocol_version);
		log_i(LOG_BLE, "request->head->msg_type = %d\r\n",request->head->msg_type);
		log_i(LOG_BLE, "request->head->msg_type = %d\r\n",request->timestamp->year);
		log_i(LOG_BLE, "request->head->month = %d\r\n",request->timestamp->month);
		log_i(LOG_BLE, "request->head->day = %d\r\n",request->timestamp->day);
		log_i(LOG_BLE, "request->head->hour = %d\r\n",request->timestamp->hour);
		log_i(LOG_BLE, "request->head->minute = %d\r\n",request->timestamp->minute);
		log_i(LOG_BLE, "request->head->second = %d\r\n",request->timestamp->second);
		log_i(LOG_BLE, "request->msgcarrierlen = %d\r\n",request->msgcarrierlen);
		log_i(LOG_BLE, "request->msgcarrier.len = %d\r\n",request->msgcarrier.len);
		log_i(LOG_BLE, "request->msgcarrier[0] = %x\r\n",request->msgcarrier.data[0]);
		log_i(LOG_BLE, "request->msgcarrier[1] = %x\r\n",request->msgcarrier.data[1]);
		log_i(LOG_BLE, "request->msgcarrier[2] = %x\r\n",request->msgcarrier.data[2]);
		log_i(LOG_BLE, "request->msgcarrier.data[request->msgcarrierlen-1] = %x\r\n",request->msgcarrier.data[request->msgcarrierlen-1]);

		unsigned char tmp[20] = {0};
		memcpy(tmp, request->msgcarrier.data,request->msgcarrierlen);

		ACK *request1 =  ack__unpack(NULL, request->msgcarrierlen, tmp);

		log_i(LOG_BLE, "request1.ack_state = %d\r\n", request1->ack_state);
		log_i(LOG_BLE, "request1.msg_type = %d\r\n", request1->msg_type);

		if (request)
	    {
	        protocol_header__free_unpacked(request, NULL);
	    }

		if (request1)
	    {
	        ack__free_unpacked(request1, NULL);
	    }
#endif

#if 0

	char	plaintext[128]="zxy30555@163.com1";
	//char	plaintext[128]="\x0A\x04\x08\x01\x10\x03\x12\x0C\x08\x13\x10\x07\x18\x0F\x20\x12\x28\x30\x30\x31\x18\x01\x22\x01\x00";
	char    chiperPath[128]="\0";
	char    afterdecrypt[128]="\0";
	char    key[] ="1832521868741387492628667237737729841724568366976937763254826565";
	//char    key[] ="8888598527857736162464368777871656575125719259765892327872456299";
	int ret=0;
	int clen=0;
	int mlen=0;
				   
	printf("\n\nplaintext is this\n\n");
	ret = HzBtCertcfg(COM_APP_PEM_ROOT_DIR, COM_APP_PEM_TPONE_DIR);

	ApiBLETraceBuf(plaintext, strlen(plaintext));
	
	ret=HzBtSymEncrypt(plaintext, strlen(plaintext), chiperPath, &clen, key, 1);
	
   if(ret==1){
	  if(chiperPath==NULL){
		  printf("\n\nchiper is null\n\n");
		  return 1;
	   }
	  printf("frist \n\n%d\n\n",clen);
	  ApiBLETraceBuf(chiperPath, clen);
	  printf("\n\n%d\n\n",clen);
	  
	}
	else return 1;
		   
   ret=0;
   ret=HzBtSymEncrypt(chiperPath, clen, afterdecrypt, &mlen, key, 0);
   if(ret==1){
	  printf("\ndecrypt success\n");
	  printf("[%s]\n", afterdecrypt);

	  ApiBLETraceBuf(afterdecrypt, mlen);
	  printf("\n\n%d\n\n",mlen);

	}
   else return 1;
	
#endif		   
	
  return 0;

}

void getPidByName(pid_t *pid, char *task_name)
 {
     DIR *dir;
     struct dirent *ptr;
     FILE *fp;
     char filepath[50];
     char cur_task_name[50];
     char buf[1024];
 
     dir = opendir("/proc"); 
     if (NULL != dir)
     {
         while ((ptr = readdir(dir)) != NULL)  
         {
             if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
                 continue;
             if (DT_DIR != ptr->d_type)
                 continue;
 
             sprintf(filepath, "/proc/%s/status", ptr->d_name); 
             printf( "/proc/%s/status\r\n", ptr->d_name);
             fp = fopen(filepath, "r");
             if (NULL != fp)
             {
                 if( fgets(buf, 1024-1, fp)== NULL ){
                     fclose(fp);
                     continue;
                 }
                 sscanf(buf, "%*s %s", cur_task_name);
				 printf( "cur_task_name=%s\r\n", cur_task_name);

                 if (!strcmp(task_name, cur_task_name)){
                     sscanf(ptr->d_name, "%d", pid);
                 }
                 fclose(fp);
             }
         }
         closedir(dir);
     }
 }
/******************************************************************************
* Function Name  : Cm256_Init
* Description    :  init
* Input          :  
* Return         : NONE
******************************************************************************/
size_t get_executable_path( char* processdir,char* processname, size_t len)
{
        char* path_end;
        if(readlink("/proc/self/exe", processdir,len) <=0)
                return -1;
        path_end = strrchr(processdir,  '/');
        if(path_end == NULL)
                return -1;
        ++path_end;
        strcpy(processname, path_end);
        *path_end = '\0';
        return (size_t)(path_end - processdir);
}
/******************************************************************************
* Function Name  : Cm256_Init
* Description    :  init
* Input          :  
* Return         : NONE
******************************************************************************/
int check_server_socket(void)
{
#if 0
	///usrdata/bt/bt-daemon-socket
	if (!file_exists(COM_APP_DATA_BT_DIR"/"COM_APP_SERVER_DATA ))
	{
		log_e(LOG_BLE, "%s is not exist", COM_APP_DATA_BT_DIR"/"COM_APP_SERVER_DATA);
		return -1;
	}
#endif


	char path[256];
	char processname[1024];
	int pid ;
	printf("check_server_socket\r\n");
	get_executable_path(path, processname, sizeof(path));
	printf("directory:%s\nprocessname:%s\n", path, processname);

	getPidByName((pid_t *)&pid, "MAIN");
	printf(" MAIN pid = %d\r\n", pid); 

   getPidByName((pid_t *)&pid, "bsa_server");
   printf(" bsa_server pid = %d\r\n", pid); 
	
	//printf("pid=%d\n", getpid());
	return 0;
}

/****************************************************************
 function:     BleShellGetName
 description:  get name
 input:        none
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
static int BleShellGetName(int argc, const char **argv)
{
	unsigned char tmp[250] = {0};
	unsigned char tmp_len = 0;
    
	stBtApi.GetName(tmp, &tmp_len);		
    shellprintf(" ble name: %s\r\n", tmp);
    return YT_OK;
}


/****************************************************************
 function:     BleShellGetUuid
 description:  get ble uuid
 input:        
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
static int BleShellGetMac(int argc, const char **argv)
{
    unsigned char aucMac[250] = {0};
	unsigned char ucLen = 0;
	stBtApi.GetMac(aucMac, &ucLen);
	log_i(LOG_BLE, "BleShellGetMac2= %d\r\n",ucLen);
	printf("Mac = [%x:%x:%x:%x:%x:%x]\r\n",aucMac[0],aucMac[1],aucMac[2],aucMac[3],aucMac[4],aucMac[5]);
    //PRINTFBUF(aucMac, ucLen);
    return 0;
}

/****************************************************************
 function:     BleShellSetTest
 description:  test
 input:         
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
static int BleShellSetTest(int argc, const char **argv)
{
    unsigned char aucBuff[500] = {0};
	unsigned int ulLen = 0;
	unsigned char ucLen = 0;
	//unsigned short usIndex;
	int ulState = 0;
	//char l = '"';

	 if (argc != 1 || sscanf(argv[0], "%d", &ulLen) != 1)
	 {
	
	 	shellprintf("argc = %d\r\n",argc);
	 
	 }

	 switch(ulLen)
	 {
		case 1:
			
			ulState = stBtApi.GetMac(aucBuff, &ucLen);
			shellprintf("ulState = %d\r\n",ulState);
		    ApiTraceBuf(aucBuff, 6);
		 	break;	
		case 2:
			shellprintf("ulState = %d\r\n",ulState);
		    stBtApi.Send(aucBuff, &ulLen);
		 	break;	
		case 3:
			hz_so_test();
		 	break;	
		case 4:
			stBtApi.LinkDrop();
		 	break;	
		case 5:
			test_bt_hz();
			shellprintf("test_bt_hzr\n");
			break;	
		case 6:
			ulLen = strlen("LQF1234567890123456789");
			memcpy(aucBuff,"LQF1234567890123456789",ulLen);
			stBtApi.Send(aucBuff, &ulLen);
		    shellprintf("stBtApi.Send\n");
			break;
       case 7:
			check_server_socket();
			break;
        default:
			break;
	 }
	 
	//stBtApi.Send("\x31\x32\x33", 3);
	shellprintf("ucLen = %d\r\n",ucLen);
    return 0;
}

/****************************************************************
 function:     BleShellInit
 description:  initiaze ble shell
 input:        none
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
int BleShellInit(void)
{
    int ret = 0;
	shell_cmd_register_ex("bletest", 			"bletest",			BleShellSetTest, "bletest");
	shell_cmd_register_ex("blegetmac", 			"blegetmac",		BleShellGetMac, "blegetmac");
	shell_cmd_register_ex("blegetname", 		"blegetname",		BleShellGetName, "blegetname");
    return ret;
}
/****************************************************************
 function:     ble_init
 description:  initiaze ble module
 input:        none
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
int ble_init(INIT_PHASE phase)
{
	int iRet = 0;
	//unsigned char ucLen = 0;

	//log_o(LOG_BLE, "init ble thread");
	switch (phase)
	{
		case INIT_PHASE_INSIDE:

			memset((char *)&g_BleMember, 0, sizeof(BLE_MEMBER));

		    BT_Interface_Init();
            reset_hz_data();
			break;

		case INIT_PHASE_RESTORE:

			break;

		case INIT_PHASE_OUTSIDE:
            pm_reg_handler(MPU_MID_BLE, BleSleepHandler);  
			
			if (0 != tm_create(TIMER_REL, BLE_MSG_ID_TIMER_HEARTER, MPU_MID_BLE, &g_BleMember.Retimer))
            {
                log_e(LOG_BLE, "create ble timer failed!");
                return -1;
            }  

			tm_start(g_BleMember.Retimer, BLE_TIMER_CMD, TIMER_TIMEOUT_REL_PERIOD);
			
			BleShellInit();
			break;

		default:
			break;
	}
	
	return iRet;
}

int start_ble(void)
{
	unsigned int len = 18;
	unsigned char vin[20] = {0};
	unsigned char tmp[250] = {0};
	unsigned char tmp_len = 0;
	cfg_get_para(CFG_ITEM_GB32960_VIN, vin, &len);
	int iRet = -1;
	
	if (strlen((char *)vin) < 17)
	{
		//memcpy(tmp, "HZ01234567891234567", strlen("HZ01234567891234567"));
		memcpy(tmp, "HZ000000000000000", strlen("HZ000000000000000"));
	}
	else
	{
		memcpy(tmp, "HZ", 2);
		memcpy(tmp+2, vin, len);
	}

    //memset(tmp, 0 , sizeof(tmp));
	//memcpy(tmp, "HZ01234567891234567", strlen("HZ01234567891234567"));
     
	tmp_len = strlen((char *)tmp);
	stBtApi.SetName(tmp, &tmp_len);

	log_e(LOG_BLE, "SetName = %s",tmp);

	iRet = stBtApi.Init();
    if (-1 == iRet)
    {
    	log_e(LOG_BLE, "ble init fail*************");
    }
	
	iRet = stBtApi.Open();
	if (-1 == iRet)
    {
    	log_e(LOG_BLE, "ble open fail**************");
    }
	
	return iRet;
}
/****************************************************************
 function:     ble_main
 description:   
 input:        none
 output:       none
 return:       NULL
 ****************************************************************/
static void *ble_main(void)
{
    int iTcom_fd, iMax_fd;
	unsigned char ucCnt = 0;
	unsigned char aucTest[1024] = {0};
	unsigned int TestLen = 0;
    fd_set fds;
    TCOM_MSG_HEADER msgheader;
	unsigned int ulStartTime = 0;
	int iRet = -1;
	
	start_ble();

	iTcom_fd = tcom_get_read_fd(MPU_MID_BLE);

    if (iTcom_fd < 0)
    {
        log_e(LOG_BLE, "get ble recv fd failed");
        return NULL;
    }

	log_o(LOG_BLE, "ble_main******************************\r\n");
    iMax_fd = iTcom_fd;

	 while (1)
    {
        if (BLE_RECV_STATUS == g_BleMember.ucTransStatus)
        {
        	if (IS_TIME_OUT(ulStartTime, 400))
        	{
        	    log_i(LOG_BLE, "send self***********\r\n");
        	    if (YT_OK == ApiCheckLen())
        	    {
					g_BleMember.ucTransStatus = BLE_RECV_FINISH_STATUS;
					BleSendMsgToApp(BLE_MSG_RECV_TO_APP);
        	    }  
				else
				{
				    log_i(LOG_BLE, "send self\r\n");
					memset(aucTest, 0, sizeof(aucTest));
					TestLen = sizeof(aucTest);
					iRet = stBtApi.Recv(aucTest, &TestLen);
					//PRINTFBUF(aucTest, TestLen);
					if (YT_ERR == iRet)
					{
					    log_i(LOG_BLE, "send self1\r\n");
						memset((unsigned char *)&aucTest, 0 , sizeof(aucTest));
					}

                    log_i(LOG_BLE, "send self4=%d\r\n",TestLen);
					log_i(LOG_BLE, "send self2\r\n");
					g_BleMember.ucTransStatus = BLE_RECV_FINISH_STATUS;
					iRet = stBtApi.Send(aucTest, &TestLen);
					BleSendMsg(BLE_MSG_SEND_TYPE, 1);
					log_i(LOG_BLE, "send self3\r\n");
					
					if (YT_OK != iRet)
					{
						//log_e(LOG_AICHI, "self1ret = %d\r\n", iRet);
					}
				}
        	}
        }
		
        FD_ZERO(&fds);
        FD_SET(iTcom_fd, &fds);

        iRet = select(iMax_fd + 1, &fds, NULL, NULL, NULL);

        if (iRet)
        {
            if (FD_ISSET(iTcom_fd, &fds))
            {
                iRet = tcom_recv_msg(MPU_MID_BLE, &msgheader, g_pucbuf);

                if (0 != iRet)
                {
                    log_e(LOG_BLE, "tcom_recv_msg failed,ret:0x%08x", iRet);
                    continue;
                }

                if (MPU_MID_TIMER == msgheader.sender)
                {
                    if (BLE_MSG_ID_TIMER_HEARTER == msgheader.msgid)
                    {
						if(0 == (ucCnt % 50))
						{
							//log_i(LOG_BLE, "***********2333\r\n");
						}
						
                        ucCnt++;
						if(ucCnt >= 3)
						{
							g_iBleSleep = 1;
						}
                    }
                }
				else if (MPU_MID_FCT == msgheader.sender)
				{
						
				}
				else if (MPU_MID_HOZON_PP == msgheader.sender)
				{
				    if (BLE_MSG_CONTROL == msgheader.msgid)
				    {
				    	PrvtProt_respbt_t respbt;
						memcpy((char *)&respbt, g_pucbuf, msgheader.msglen);
						log_e(LOG_BLE, "respbt.cmd = %d", respbt.cmd);
						log_e(LOG_BLE, "msgheader.msglen = %d", msgheader.msglen);
						log_e(LOG_BLE, "respbt.result = %d", respbt.result);
						log_e(LOG_BLE, "respbt.msg_type = %d", respbt.msg_type);
	 					if ((g_hz_protocol.hz_send.ack.msg_type ==  (respbt.msg_type)) && (g_hz_protocol.hz_send.ack.state == respbt.cmd))
	 					{
	 						bt_send_cmd_pack(respbt.result, g_stBt_Data.aucTxPack, &g_stBt_Data.ulTxLen);
							stBtApi.Send(g_stBt_Data.aucTxPack, &g_stBt_Data.ulTxLen);
	 					}
				    }
				
				}
				else if (MPU_MID_BLE == msgheader.sender)
				{
					ucCnt = 0;
					g_iBleSleep = 0;
					
					if (BLE_MSG_SEND_TYPE == msgheader.msgid)
					{
						log_i(LOG_BLE, "msgheader.sender = MPU_MID_BLE");
						log_i(LOG_BLE, "BLE_MSG_SEND_TYPE");
						g_BleMember.ucTransStatus = BLE_INIT_STATUS;
					}
					else if (BLE_MSG_RECV_TYPE == msgheader.msgid)
					{
					    //stBtApi.Recv();
						log_i(LOG_BLE, "msgheader.sender = MPU_MID_BLE");
						log_i(LOG_BLE, "BLE_MSG_RECV_TYPE");
						ulStartTime = tm_get_time();
						
						//if(BLE_INIT_STATUS == g_BleMember.ucTransStatus)
						//{
						    
							g_BleMember.ucTransStatus = BLE_RECV_STATUS;
						//}
					}
					else if (BLE_MSG_RECV_TO_APP == msgheader.msgid)
					{
						log_i(LOG_BLE, "LOG_BLE2\r\n");
						g_stBt_Data.ulRxLen = sizeof(g_stBt_Data.aucRxPack);
						stBtApi.Recv(g_stBt_Data.aucRxPack, &g_stBt_Data.ulRxLen);
						log_i(LOG_BLE, "g_stBt_Data.ulRxLen=%d\r\n", g_stBt_Data.ulRxLen);
						ApiBLETraceBuf(g_stBt_Data.aucRxPack,  g_stBt_Data.ulRxLen);	
						iRet = hz_protocol_process(g_stBt_Data.aucRxPack,&g_stBt_Data.ulRxLen, g_stBt_Data.aucTxPack,&g_stBt_Data.ulTxLen) ;
						if (0 == iRet)
						{
						    if (BT_AH_MSG_TYPE_SE_FUN_RESP == g_hz_protocol.hz_send.msg_type)
						    {
						    	stBtApi.Send(g_stBt_Data.aucTxPack, &g_stBt_Data.ulTxLen);
						    }
							else if (BT_AH_MSG_TYPE_ACK == g_hz_protocol.hz_send.msg_type) 
							{
								PP_rmtCtrl_BluetoothCtrlReq(g_hz_protocol.type, g_hz_protocol.hz_send.ack.state);
							}
						}
					}
					else if (BLE_MSG_DISCONNECT == msgheader.msgid)
					{
						memset((char *)&g_BleMember, 0, sizeof(BLE_MEMBER));
		    			g_BleMember.ucTransStatus = BLE_INIT_STATUS;
						reset_hz_data();
						//BleSendMsgToApp(AICHI_MSG_DISCONFIG_TYPE);
						log_i(LOG_BLE, "1BLE_MSG_DISCONNECT");
					}
				}
                else if (MPU_MID_PM == msgheader.sender)
                {
                
                    if (PM_MSG_SLEEP == msgheader.msgid)
                    { 
                       
                       g_iBleSleep = 1;
                       // if (1 == g_BleContr.ucSleepCloseBle)
                       {
                          //stBtApi.Close();
                       }
                    }
                    else if (PM_MSG_RUNNING == msgheader.msgid)
                    {
                        //at_wakeup_proc();
                      // if (1 == g_BleContr.ucSleepCloseBle)
                       {
                          //������
                          //if(0 != stBtApi.Open())
                          //{
                          		//������ʧ��
                          //}
                       }
                    }
                    else if(PM_MSG_OFF == msgheader.msgid)
                    {
                        //at_wakeup_proc();
                       // disconnectcall();
                    }
                }
                else if (MPU_MID_MID_PWDG == msgheader.msgid)
                {
					  pwdg_feed(MPU_MID_BLE);
                }
				else if(BLE_MSG_ID_CHECK_TIMEOUT == msgheader.msgid)
				{
					//tm_ble_timeout();
				}
				else if (BLE_MSG_ID_TIMER_HEARTER == msgheader.msgid)
                {
                   // pwdg_feed(MPU_MID_BLE);
                }

                continue;
            }

        }
        else if (0 == iRet)  /* timeout */
        {
            continue; /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }

            break; /* thread exit abnormally */
        }
    }
    
    return NULL;
}

/****************************************************************
 function:     gps_run
 description:  startup GPS module
 input:        none
 output:       none
 return:       positive value indicates success;
 -1 indicates failed
 *****************************************************************/
int ble_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&ble_tid, &ta, (void *) ble_main, NULL);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

