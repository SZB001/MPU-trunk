
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include <ctype.h>
#include "com_app_def.h"
#include "mid_def.h"
#include "init.h"
#include "log.h"
#include "shell_api.h"
#include "tcom_api.h"
#include "fota.h"
#include "xml.h"

#include "fota.h"

#define GET_WORD(data)      GET_HWBE(data)
#define GET_DWORD(data)     GET_WDBE(data)

#define GET_BYTE(data)      (*(data))
#define GET_HWBE(data)      (GET_BYTE(data) * 0x100 + GET_BYTE((data) + 1))
#define GET_HWLE(data)      (GET_BYTE(data) + GET_BYTE((data) + 1) * 0x100)
#define GET_WDBE(data)      (GET_HWBE(data) * 0x10000 + GET_HWBE((data) + 2))
#define GET_WDLE(data)      (GET_HWLE(data) + GET_HWBE((data) + 2) * 0x10000)

static int hex2bin(char *hex, uint8_t *bin)
{
    int cnt;

    for (cnt = 0; isxdigit(hex[0]) && isxdigit(hex[1]); cnt++, hex += 2)
    {
        uint8_t hi = isdigit(hex[0]) ? hex[0] - '0' : toupper(hex[0]) - 'A' + 10;
        uint8_t lo = isdigit(hex[1]) ? hex[1] - '0' : toupper(hex[1]) - 'A' + 10;
        *bin++ = hi * 16 + lo;
    }

    if (*hex != '\0' && *hex != ' ')
    {
        return -1;
    }

    return cnt;
}

static const unsigned int crc32tab[] =
{
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
    0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
    0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
    0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
    0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
    0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
    0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
    0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
    0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
    0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
    0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
    0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
    0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
    0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
    0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
    0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
    0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
    0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
    0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
    0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
    0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
    0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
    0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
    0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
    0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
    0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
    0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
    0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
    0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
    0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
    0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
    0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
    0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
    0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
    0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
    0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
    0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
    0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
    0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
    0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
    0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
    0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
    0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
    0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
    0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
    0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
    0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
    0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
    0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
    0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
    0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
    0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};

extern int fota_uds_request(int bc, int sid, int sub, uint8_t *data, int len, int timeout);
extern int fota_uds_result(uint8_t *buf, int siz);

#if 0
static unsigned int crc32(const unsigned char *buf, unsigned int size)
{
    unsigned int i, crc;
    crc = 0xFFFFFFFF;


    for (i = 0; i < size; i++)
    {
        crc = crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    }

    return crc;
}
#endif

unsigned int crc16tab[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};


static unsigned short crc16(const unsigned char *buf, unsigned int size)
{
    unsigned int i;
    unsigned short crc, temp;
    crc = 0xFFFF;


    for (i = 0; i < size; i++)
    {
        temp = (crc >> 8) ^ buf[i];
        crc = (crc << 8) ^ crc16tab[temp];
    }

    return crc;
}



static int cutspace(char *str)
{
    char *h = str, *t = str + strlen(str);

    while (isspace(*h))
    {
        h++;
    }

    while (isspace(*(t - 1)))
    {
        t--;
    }

    *t = 0;

    if (str != h && strlen(h) > 0)
    {
        strcpy(str, h);
    }

    return strlen(str);
}

static int checksum(uint8_t *data, int len)
{
    int i, cs;

    for (cs = 0, i = 0; i < len - 1; i++)
    {
        cs += data[i];
    }

    cs = (~cs) & 0xff;

    return data[i] - cs;
}

int s19_load(const char *fpath, image_t *img)
{
    FILE *fs19;
    char lbuf[128];
    int lnum, snum;
    uint8_t *mpos;
    unsigned int  crc;
    unsigned int  len = 0;

    if ((fs19 = fopen(fpath, "r")) == NULL)
    {
        log_e(LOG_FOTA, "load S19 file: \"%s\" fail", fpath);
        return -1;
    }

    memset(img->sect, 0, sizeof(img->sect));

    //crc = crc32(NULL, 0);

    for (lnum = 1, snum = 0, mpos = img->buff; fgets(lbuf, sizeof(lbuf), fs19); lnum++)
    {
        uint32_t addr;
        int type, dlen;
        uint8_t data[64], *dpos;
        section_t *sect = img->sect + snum;

        if (cutspace(lbuf) <= 0)
        {
            continue;
        }

        type = GET_HWBE(lbuf) - 0x5330;

        if (lbuf[0] != 'S' || type < 1 || type > 3)
        {
            log_e(LOG_FOTA, "unsupported line(%d): \"%s\"", lnum, lbuf);
            continue;
        }

        dlen = hex2bin(lbuf + 2, data);

        if (dlen <= 0 || dlen != GET_BYTE(data) + 1)
        {
            log_e(LOG_FOTA, "error format at line(%d): \"%s\"", lnum, lbuf);
            fclose(fs19);
            return -1;;
        }

        if (checksum(data, dlen) != 0)
        {
            log_e(LOG_FOTA, "checksum error at line(%d): \"%s\"", lnum, lbuf);
            fclose(fs19);
            return -1;
        }

        for (addr = 0, dlen -= 2, dpos = data + 1; type + 1 > 0; type--, dlen--, dpos++)
        {
            addr = addr * 256 + GET_BYTE(dpos);
        }

        //log_o(LOG_FOTA, "get new line, addr=0x%08X, dlen=0x%02X", addr, dlen);

        if (mpos + dlen > img->buff + sizeof(img->buff))
        {
            log_e(LOG_FOTA, "image size overflow(max=%dKB) at line(%d): \"%s\"",
                  IMAGE_MAX_SIZE, lnum, lbuf);
            fclose(fs19);
            return -1;
        }

        if (sect->size != 0)
        {
            if (sect->addr + sect->size == addr)
            {
                sect->size += dlen;
            }
            else if (snum + 1 < IMAGE_MAX_SECT)
            {
                log_o(LOG_FOTA, "last section end, addr=%08X, size=%08X(%d)", sect->addr, sect->size, sect->size);
                sect++;
                snum++;
            }
            else
            {
                log_e(LOG_FOTA, "too many sections(max=%d) at line(%d): \"%s\"",
                      IMAGE_MAX_SECT, lnum, lbuf);
                fclose(fs19);
                return -1;
            }
        }

        if (sect->size == 0)
        {
            sect->addr = addr;
            sect->size = dlen;
            sect->data = mpos;
        }

        memcpy(mpos, dpos, dlen);
        mpos += dlen;
        len = len + dlen;
    }

    crc = crc16(img->buff, len);
    //crc = ~crc;
    log_e(LOG_FOTA, "crc = %x, len = %d", crc, len);

    img->scnt = crc;

    if (!feof(fs19))
    {
        log_e(LOG_FOTA, "access error in file %s", fpath);
        fclose(fs19);
        return -1;
    }

    fclose(fs19);
    return 0;
}


int foton_security(uint8_t *seed, int *par, uint8_t *key, int keysz)
{
    #define TOPBIT 0x8000
    #define POLYNOM_1 0x9367
    #define POLYNOM_2 0x2956
    #define BITMASK 0x0080
    #define INITIAL_REMINDER 0xFFFE
    #define MSG_LEN 2 /* seed length in bytes */

    uint8_t n;
    uint8_t i;
    uint16_t remainder;

    if (keysz < 2)
    {
        return -1;
    }

    //log_o(LOG_FOTA, "par  :%02X, %02X, %02X, %02X", par[0], par[1]);
    log_o(LOG_FOTA, "seed :%02X, %02X", seed[0], seed[1]);

    remainder = INITIAL_REMINDER;

    for (n = 0; n < MSG_LEN; n++)
    {
        /* Bring the next byte into the remainder. */
        remainder ^= ((seed[n]) << 8);
        /* Perform modulo-2 division, a bit at a time. */
        for (i = 0; i < 12; i++)
        {
            /* Try to divide the current data bit. */
            if (remainder & TOPBIT)
            {
                if(remainder & BITMASK)
                {
                    remainder = (remainder << 2) ^ POLYNOM_1;
                }
                else
                {
                    remainder = (remainder << 2) ^ POLYNOM_2;
                } 
            }
            else
            {
                remainder = (remainder << 1);
            } 
        } 
    }
    /* The final remainder is the key */
    key[1] = (uint8_t)(remainder);
    key[0] = (uint8_t)(remainder >> 8);

    log_o(LOG_FOTA, "key  :%02X, %02X", key[0], key[1]);
    return 2;
}


int foton_check1(unsigned int crc)
{
    unsigned char data[6];

    data[0] = 0x02;
    data[1] = 0x02;
    data[2] = crc >> 24;
    data[3] = crc >> 16;
    data[4] = crc >> 8;
    data[5] = crc;

    return fota_uds_request(0, 0x31, 0x01, data, 6, 5);
}

int foton_check2(unsigned int crc)
{
    extern void set_upgrade_result_errcode(unsigned int ErrCode);

    unsigned char data[4];
    uint8_t tmp[16];

    data[0] = 0xFF;
    data[1] = 0x01;
    //Only Used In He Zhong Project
    data[2] = crc >> 8;
    data[3] = crc;

    //If CheckProgramming Dependencies Return FF 01 02, Means Success
    //Return FF 01 05, Means Fail
    if (fota_uds_request(0, 0x31, 0x01, data, 4, 5) != 0 ||
       (fota_uds_result(tmp, sizeof(tmp)) != 5 && tmp[0] != 0xff && tmp[1] != 0x01 && tmp[2] != 0x02))
    {
        set_upgrade_result_errcode(0x3100 + tmp[2]);
        log_e(LOG_FOTA, "CheckProgramming Dependencies fail, code = %d, crc = 0x%x%x", tmp[2], tmp[3], tmp[4]);
        return -1;
    }

    return 0;
}


