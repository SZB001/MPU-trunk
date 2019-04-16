#include "aes_e.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

#define PLAIN_FILE_OPEN_ERROR -1
#define KEY_FILE_OPEN_ERROR -2
#define CIPHER_FILE_OPEN_ERROR -3
#define OK 1

int CipherFile(char *plainFile,unsigned char* key,char *cipherFile)
{
	mbedtls_aes_context aes;
    FILE *plain,*cipher;
	unsigned char dataBlock[16];
	unsigned char outdata[16];
    int count = 0;
	
    if((plain = fopen(plainFile,"rb")) == NULL)
    {
        return PLAIN_FILE_OPEN_ERROR;
    }
    if((cipher = fopen(cipherFile,"wb")) == NULL)
    {
        return CIPHER_FILE_OPEN_ERROR;
    }
	mbedtls_aes_init( &aes ); 
	mbedtls_aes_setkey_enc( &aes, key, 128 );
	
    //读取文件
    while(!feof(plain))
    {
        //每次读16个字节，并返回成功读取的字节数
        if((count = fread(dataBlock,sizeof(unsigned char),16,plain)) == 16)
        {
			//mbedtls_aes_encrypt( &aes, dataBlock, outdata );
			mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, dataBlock, outdata );			
            fwrite(outdata,sizeof(unsigned char),16,cipher);
        }
    }
    if(count)
    {
        //填充
        memset(dataBlock + count,'\0',15 - count);  //不够16位，按照空格来处理
        //最后一个字符保存包括最后一个字符在内的所填充的字符数量
        dataBlock[15] = 16 - count;
		//mbedtls_aes_encrypt( &aes, dataBlock, outdata );
		mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, dataBlock, outdata );
        fwrite(outdata,sizeof(unsigned char),16,cipher);
    }
    fclose(plain);
    fclose(cipher);
    return OK;
}

int InvCipherFile(char *cipherFile,unsigned char* key,char *plainFile)
{
	mbedtls_aes_context aes;
    FILE *plain,*cipher;
	unsigned char dataBlock[16];
	unsigned char outdata[16];
    int count = 0,times = 0;
    long fileLen;
	
    if((plain = fopen(plainFile,"wb")) == NULL)
    {
        return PLAIN_FILE_OPEN_ERROR;
    }
    if((cipher = fopen(cipherFile,"rb")) == NULL)
    {
        return CIPHER_FILE_OPEN_ERROR;
    }
    
    //产生密匙
	mbedtls_aes_setkey_dec( &aes, key, 128 );
	
    //取文件长度
    fseek(cipher,0,SEEK_END);   //将文件指针置尾
    fileLen = ftell(cipher);    //取文件指针当前位置
    rewind(cipher);             //将文件指针重指向文件头
    //读取文件
    while(1)
    {
        //密文的字节数一定是16的整数倍
        fread(dataBlock,sizeof(unsigned char),16,cipher);
		mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_DECRYPT, dataBlock, outdata );
        times += 16;
        if(times < fileLen)
        {
            fwrite(outdata,sizeof(unsigned char),16,plain);
        }
        else
        {
            break;
        }
    }
    //判断末尾是否被填充
    if(outdata[15] < 16)
    {
        for(count = 16 - outdata[15]; count < 15; count++)
        {
            if(outdata[count] != '\0')
            {
                break;
            }
        }
    }
    if(count == 15) //有填充
    {
        fwrite(outdata,sizeof(unsigned char),16 - outdata[15],plain);
    }
    else //无填充
    {
        fwrite(outdata,sizeof(unsigned char),16,plain);
    }

    fclose(plain);
    fclose(cipher);
    return OK;
}
