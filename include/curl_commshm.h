
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
#define SEM_NAME "/gb"     //有名信号量名称
#define GB_FILE  0
#define CAN_FILE 1
#define CDM_SEND "sendfile"

#define CMD_LENGTH         6
#define TBOX_VIN_LENGTH    17
#define TBOX_SN_LENGTH     18

/*
static int Commsem()
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
}*/
int sem_p(int sem_id) 
{
    struct sembuf sem_buf;
    sem_buf.sem_num=0;//信号量编号
    sem_buf.sem_op=-1;//P操作
    sem_buf.sem_flg=SEM_UNDO;//系统退出前未释放信号量，系统自动释放
    if (semop(sem_id,&sem_buf,1)==-1) {
        perror("Sem P operation");
        exit(1);
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
        exit(1);
    }
    return 0;
}

static int CommShm(int size,int flags)
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
	return CommShm(size,IPC_CREAT);   //如果不存在就创建
}

#endif


