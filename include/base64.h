#ifndef BASE64_H
    #define BASE64_H

    int decode64(char *string,  unsigned char *outbuf,  int *outlen);


    void encode64(unsigned char *inbuf, int ilen, char *outbuf, int outlen);


#endif
