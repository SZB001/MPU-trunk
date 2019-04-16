/**
 * @Title: dsu_comm.c
 * @author yuzhimin
 * @date Nov 7, 2017
 * @version V1.0
 */

#include "dsu_comm.h"
#include "dsu_file.h"

int dsu_build_head_inx(unsigned char *header, unsigned int size, unsigned int sn)
{
    assert(header);
    assert(size >= DSU_INX_HEADER_SIZE);

    unsigned short cs = 0;
    unsigned int len;
    MD5_CTX mdContext;
    unsigned char authkey[256];
    unsigned char MD5Key[MD5_LENGTH];

    /* add inx header */
    memcpy(header, "INTEST 2.1", 10);

    header[10] = (unsigned char) sn;
    header[11] = sn >> 8;
    header[12] = sn >> 16;
    header[13] = sn >> 24;

    /* inz */
    header[14] = 2;
    len = sizeof(authkey);
    cfg_get_para(CFG_ITEM_DSU_AUTHKEY, (unsigned char *) &authkey, &len);
    len = strlen((char const *) authkey);

    if (len)
    {
        MD5Init(&mdContext);
        MD5Update(&mdContext, authkey, len);
        MD5Final(&mdContext);
        memcpy(MD5Key, mdContext.digest, MD5_LENGTH);
        log_buf_dump(LOG_DSU, "key MD5:", &mdContext.digest[0], MD5_LENGTH);
    }
    else
    {
        /* fix: clear hiskey */
        memset(MD5Key, 0, 16);
    }

    memcpy(&header[15], MD5Key, MD5_LENGTH);

    for (len = 0; len < 31; len++)
    {
        cs += header[len];
    }

    header[31] = cs;
    header[32] = cs >> 8;

    return DSU_INX_HEADER_SIZE;
}

int dsu_build_head_inz(unsigned char *header, unsigned int size, unsigned int sn)
{
    assert(header);
    assert(size >= DSU_INZ_HEADER_SIZE);

    /* add inz header */
    memcpy(header, "INTEST INZ V001", 15);

    /* Terminal address */
    header[15] = (unsigned char) sn;
    header[16] = sn >> 8;
    header[17] = sn >> 16;
    header[18] = sn >> 24;

    /* suggest buffer size */
    header[19] = (unsigned char) DSU_ZBUF_SIZE;
    header[20] = (unsigned char)(DSU_ZBUF_SIZE >> 8);
    header[21] = (unsigned char)(DSU_ZBUF_SIZE >> 16);
    header[22] = (unsigned char)(DSU_ZBUF_SIZE >> 24);

    return DSU_INZ_HEADER_SIZE;
}

int dsu_build_head_inr(unsigned char *header, unsigned int size)
{
    assert(header);
    assert(size >= DSU_INR_HEADER_SIZE);

    /* add inr header */
    memcpy(header, "INTEST", 6);
    header[6] = 0x08;

    return DSU_INR_HEADER_SIZE;
}

int dsu_build_head_iwd(unsigned char *header, unsigned int size, char *ver,
                       unsigned int sn, RTCTIME *time, unsigned char clk)
{
    assert(header);
    assert(size >= DSU_IWD_HEADER_SIZE);

    /* add iwd header */
    memcpy(header, ver, 15);

    /* Terminal address */
    header[15] = (unsigned char) sn;
    header[16] = sn >> 8;
    header[17] = sn >> 16;
    header[18] = sn >> 24;

    /* start time */
    header[19] = (time->year - 2000) & 0xFF;
    header[20] = time->mon;
    header[21] = time->mday;
    header[22] = time->hour;
    header[23] = time->min;
    header[24] = time->sec;
    /* clock source of  systicks */
    header[25] = clk;

    return DSU_IWD_HEADER_SIZE;
}

