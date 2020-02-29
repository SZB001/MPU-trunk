
//comm.h
#ifndef _COMM_H__
#define _COMM_H__
 
#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>

#define PATHNAME "/"
#define PROJ_ID 0x6666

#define m_start  0x7E
#define m_end    0x7E
#define gb_index 1  

#define GB_MEMORY_HEAD "GBDATA"
#define GB_MEMORY_HEAD_LENGTH 6
#define CAN_MEMORY_HEAD "CANDTAT"
#define CAN_MEMORY_HEAD_LENGTH 7

#define CAN_FAULTREQUEST "CANDTATfault"
#define CAN_TSPREQUEST_10 "CANDTATTSP_01"
#define CAN_TSPREQUEST_20 "CANDTATTSP_02"
#define CAN_TSPREQUEST_30 "CANDTATTSP_03"
#define CAN_TSPREQUEST_40 "CANDTATTSP_04"
#define CAN_TSPREQUEST_50 "CANDTATTSP_05"
#define CAN_TSPREQUEST_60 "CANDTATTSP_06"
#define CAN_TSPREQUEST_70 "CANDTATTSP_07"
#define CAN_TSPREQUEST_80 "CANDTATTSP_08"
#define CAN_TSPREQUEST_90 "CANDTATTSP_09"
#define CAN_TSPREQUEST_100 "CANDTATTSP_10"
#define CAN_TSPREQUEST_110 "CANDTATTSP_11"
#define CAN_TSPREQUEST_120 "CANDTATTSP_12"

#define SHARE_MEMORY_LENGTH 4096
#define BUF_SIZE 100
#define  BUFFSIZE  512     // 缓冲区大小

#define CMD_LENGTH         6
#define TBOX_VIN_LENGTH    17
#define TBOX_SN_LENGTH     18

union semun
{
 	int val; //信号量初始值                   
 	struct semid_ds *buf;        
 	unsigned short int *array;  
 	struct seminfo *__buf;      
}; 

static int Commsem(void)
{
	key_t key = ftok(PATHNAME,PROJ_ID);
	if(key < 0)
	{
		perror("ftok");
		return -1;
	}
	int semid = 0;
	if((semid = semget(key,1,IPC_CREAT|0666)) < 0)
	{
		perror("semget");
		return -1;
	}
	return semid;
}
 int set_semvalue(int semid)
{
    // 用于初始化信号量，在使用信号量前必须这样做
    union semun num;
 
    num.val = 1;
    if (semctl(semid, 0, SETVAL, num) == -1)
    {
        return 0;
    }
    return 1;
}

int sem_p(int sem_id) 
{
    struct sembuf sem_buf;
    sem_buf.sem_num=0;//信号量编号
    sem_buf.sem_op=-1;//P操作
    sem_buf.sem_flg=SEM_UNDO;//系统退出前未释放信号量，系统自动释放
    if (semop(sem_id,&sem_buf,1)==-1) {
        perror("Sem P operation");
        return -1;
    }
    return 0;
}

int sem_v(int sem_id) 
{
    struct sembuf sem_buf;
    sem_buf.sem_num=0;
    sem_buf.sem_op=1;//V操作
    sem_buf.sem_flg=SEM_UNDO;
    if (semop(sem_id,&sem_buf,1)==-1) {
        perror("Sem V operation");
        return -1;
    }
    return 0;
}

int CommShm(int size,int flags)
{
	key_t key = ftok(PATHNAME,PROJ_ID);
	if(key < 0)
	{
		perror("ftok");
		return -1;
	}
	int shmid = 0;
	if((shmid = shmget(key,size,flags)) < 0)
	{
		perror("shmget");
		return -2;
	}
	return shmid;
}
int DestroyShm(int shmid)
{
	if(shmctl(shmid,IPC_RMID,NULL) < 0)
	{
		perror("shmctl");
		return -1;
	}
	return 0;
}
int CreateShm(int size)
{
	return CommShm(size,IPC_CREAT | 0666);  //创建共享内存
}
int GetShm(int size)
{
	return CommShm(size,IPC_CREAT| 0666);   //如果不存在就创建
}

#endif


