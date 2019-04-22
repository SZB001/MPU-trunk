
#ifndef AES_H
#define AES_H

void add_pkcs_padding( unsigned char *output, int output_len, int data_len );

void* CipherString(void* input, int length,unsigned char* key);

void* InvCipherString(void* input, int length,unsigned char* key);

int InvCipherFile(char *cipherFile,unsigned char* key,char *plainFile);


#endif
