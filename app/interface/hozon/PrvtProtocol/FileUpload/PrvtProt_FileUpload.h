/******************************************************
�ļ�����	PrvtProt_FileUpload.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/10/21		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_FILEUPLOAD_H
#define		_PRVTPROT_FILEUPLOAD_H

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/
#define PP_FILEUPLOAD_PACKNUM       60
#define PP_FILEUPLOAD_BUFNUM        2

#define PP_FILEUPLOAD_PATH      "/media/sdcard/fileUL/"
/**********�곣������*********/


/***********�꺯��***********/


/*******************************************************
description�� struct definitions
*******************************************************/



/*******************************************************
description�� typedef definitions
*******************************************************/
/*****struct definitions*****/
typedef struct
{
    int len;
	uint8_t data[1024];
}__attribute__((packed))  PP_FileUpload_Pack_t;


typedef struct
{
	uint8_t successflag;
    uint8_t cnt;
	PP_FileUpload_Pack_t pack[PP_FILEUPLOAD_PACKNUM];
}__attribute__((packed))  PP_FileUpload_Buf_t;

typedef struct
{
	uint64_t tasktime;
    uint8_t index;
	PP_FileUpload_Buf_t buffer[PP_FILEUPLOAD_BUFNUM];
}__attribute__((packed))  PP_FileUpload_t;

/******enum definitions******/

/******enum definitions******/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void InitPP_FileUpload_Parameter(void);
extern void PP_FileUpload_run(void);
#endif 
