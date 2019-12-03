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
	bt_ack_t cmd_state; // 0��ʾ�ɹ�  1��ʾʧ��
	uint8_t failtype;
	bt_vihe_info_t state;
	
}__attribute__((packed))  PrvtProt_respbt_t;  


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
 input:        
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
int BleSendMsgToApp(unsigned short usMid)
{
	TCOM_MSG_HEADER stMsg;
	int iMsgData = 1;

	stMsg.msgid	 = usMid;
	stMsg.sender	 = MPU_MID_BLE;
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
* Function Name  : get_executable_path
* Description    :   
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
* Function Name  : check_server_socket
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
 function:     BleShellGetUuid
 description:  get ble mac
 input:        
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
int BleGetMac(unsigned char *Mac)
{
	unsigned char ucLen = 0;
	stBtApi.GetMac(Mac, &ucLen);
	if(6 == ucLen)
	{
		return YT_OK;
	}
	else
	{
		return YT_ERR;
	}
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
 function:     BleShellGetUuid
 description:  get ble uuid
 input:        
 output:       none
 return:       0 indicates success,others indicates failed
 *****************************************************************/
static int BleShellBleEn(int argc, const char **argv)
{  
    if (argc != 1)
    {
        shellprintf(" usage:setble on/off\r\n");
        return AT_INVALID_PARA;
    }

    if (0 == strncmp("on", argv[0], 2))
    {
		unsigned char ble_enable = 1;
		cfg_set_para(CFG_ITEM_EN_BLE, (unsigned char *)&ble_enable, 1);

    }
    else if (0 == strncmp("off", argv[0], 3))
    {
		unsigned char ble_enable = 0;
		cfg_set_para(CFG_ITEM_EN_BLE, (unsigned char *)&ble_enable, 1);
    }
    else
    {
        shellprintf(" usage:setble on/off\r\n");
        return -1;
    }

	shellprintf(" set ble ok\r\n");
	shellprintf(" please restart terminal!\r\n");
	//system("reboot");
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
	 shell_cmd_register_ex("bletest",    "bletest",   BleShellSetTest, "bletest");
	 shell_cmd_register_ex("blegetmac",    "blegetmac",  BleShellGetMac, "blegetmac");
	 shell_cmd_register_ex("blegetname",   "blegetname",  BleShellGetName, "blegetname");
	 shell_cmd_register_ex("bleen",   "bleen",  BleShellBleEn, "blegetname on/off");
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

	switch (phase)
	{
		case INIT_PHASE_INSIDE:

			memset((char *)&g_BleMember, 0, sizeof(BLE_MEMBER));
            g_BleMember.ucConnStatus = BLE_MSG_DISCONNECT;
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
			unsigned char ble_enable = 1;
			cfg_set_para(CFG_ITEM_EN_BLE, (unsigned char *)&ble_enable, 1);
			break;

		default:
			break;
	}
	
	return iRet;
}
/******************************************************************************
* Function Name  : start_ble
* Description    :  init
* Input          :  
* Return         : NONE
******************************************************************************/
int start_ble(void)
{
	unsigned int len = 1;
	unsigned char vin[20] = {0};
	unsigned char tmp[250] = {0};
	unsigned char tmp_len = 0;
	unsigned char ble_en = 0;

	cfg_get_para(CFG_ITEM_EN_BLE, &ble_en, &len);
	if (0 == ble_en)
	{
	 	return -1;
	}

	len = 18;
	cfg_get_user_para(CFG_ITEM_GB32960_VIN, vin, &len);
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
	unsigned int ulConnStartTime = 0;
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

		if (BLE_MSG_CONNECT == g_BleMember.ucConnStatus)
        {
        	if (BT_AUTH_SUCCESS != bt_get_auth_flag())
			{
				if (IS_TIME_OUT(ulConnStartTime, 30000))
				{
					stBtApi.LinkDrop();
					g_BleMember.ucConnStatus = BLE_MSG_DISCONNECT;
					log_e(LOG_BLE, "stBtApi.LinkDrop");
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
						bt_vihe_info_t 	  vihe_info;
						memcpy((char *)&respbt, g_pucbuf, msgheader.msglen);
						log_i(LOG_BLE, "respbt.cmd = %d", respbt.cmd);
						log_i(LOG_BLE, "msgheader.msglen = %d", msgheader.msglen);
					    log_i(LOG_BLE, "respbt.execution_result = %d", respbt.cmd_state.execution_result);
						log_i(LOG_BLE, "respbt.cmd_state.state = %d", respbt.cmd_state.state);
						log_i(LOG_BLE, "respbt.msg_type = %d", respbt.msg_type);
	 					if ((g_hz_protocol.hz_send.ack.msg_type ==  (respbt.msg_type)) && (g_hz_protocol.hz_send.ack.state == respbt.cmd))
	 					{
	 						bt_send_cmd_pack(respbt.cmd_state,vihe_info, g_stBt_Data.aucTxPack, &g_stBt_Data.ulTxLen);
							stBtApi.Send(g_stBt_Data.aucTxPack, &g_stBt_Data.ulTxLen);
	 					}
						if (APPLICATION_HEADER__MESSAGE_TYPE__ACK == g_hz_protocol.hz_send.msg_type) 
						{
						}
						else if (APPLICATION_HEADER__MESSAGE_TYPE__Vehicle_Infor == g_hz_protocol.hz_send.msg_type) 
						{
						    log_i(LOG_BLE, "g_hz_protocol.hz_send.msg_type = %d", g_hz_protocol.hz_send.msg_type);
						   	bt_send_cmd_pack(respbt.cmd_state, respbt.state, g_stBt_Data.aucTxPack, &g_stBt_Data.ulTxLen);
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
						    if (APPLICATION_HEADER__MESSAGE_TYPE__SECURITY_FUNC_RESPONSE == g_hz_protocol.hz_send.msg_type)
						    {
						    	stBtApi.Send(g_stBt_Data.aucTxPack, &g_stBt_Data.ulTxLen);
						    }
							else if (APPLICATION_HEADER__MESSAGE_TYPE__ACK == g_hz_protocol.hz_send.msg_type) 
							{
							    log_i(LOG_BLE, "g_hz_protocol.type=%d\r\n", g_hz_protocol.type);
								log_i(LOG_BLE, "g_hz_protocol.hz_send.ack.state=%d\r\n", g_hz_protocol.hz_send.ack.state);
								PP_rmtCtrl_BluetoothCtrlReq(g_hz_protocol.type, g_hz_protocol.hz_send.ack.state);
							}
							else if (APPLICATION_HEADER__MESSAGE_TYPE__Vehicle_Infor == g_hz_protocol.hz_send.msg_type) 
							{
							    log_i(LOG_BLE, "g_hz_protocol.type=%d\r\n", g_hz_protocol.type);
								log_i(LOG_BLE, "g_hz_protocol.hz_send.ack.state=%d\r\n", g_hz_protocol.hz_send.ack.state);
								PP_rmtCtrl_BluetoothCtrlReq(g_hz_protocol.type, 0);
							}
						}
					}
					else if (BLE_MSG_CONNECT == msgheader.msgid)
					{
						g_BleMember.ucConnStatus = BLE_MSG_CONNECT;
						log_i(LOG_BLE, "BLE_MSG_CONNECT");
					}
					else if (BLE_MSG_DISCONNECT == msgheader.msgid)
					{
						memset((char *)&g_BleMember, 0, sizeof(BLE_MEMBER));
						g_BleMember.ucConnStatus = BLE_MSG_DISCONNECT;
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
                          //
                          //if(0 != stBtApi.Open())
                          //{
                          		//
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

