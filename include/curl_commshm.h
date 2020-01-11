
//comm.h
#ifndef _COMM_H__
#define _COMM_H__
 
#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
 
#define PATHNAME "/"
#define PROJ_ID 0x6666
#define SEM_NAME "gb"     //有名信号量名称
#define GB_FILE  0
#define CAN_FILE 1
#define CDM_SEND "sendfile"

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


