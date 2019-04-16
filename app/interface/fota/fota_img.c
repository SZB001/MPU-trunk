#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "init.h"
#include "log.h"
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

static int hex2byt(char *hex)
{
    int hi, lo;
    
    if (!isxdigit(hex[0]) || !isxdigit(hex[1]))
    {
        return 0;
    }

    hi = isdigit(hex[0]) ? hex[0] - '0' : toupper(hex[0]) - 'A' + 10;
    lo = isdigit(hex[1]) ? hex[1] - '0' : toupper(hex[1]) - 'A' + 10;
    return hi * 16 + lo;
}

static int cutspace(char *str)
{
    char *h = str, *t = str + strlen(str);

    while (isspace(*h)) h++;
    while (isspace(*(t - 1))) t--;

    *t = 0;
    if (str != h && strlen(h) > 0)
    {
        strcpy(str, h);
    }

    return strlen(str);
}

static int checksum_hex(uint8_t *data, int len)
{
    int i, cs;

    for (cs = 0, i = 0; i < len - 1; i++)
    {
        cs += data[i];
    }

    cs = ((~cs) + 1) & 0xff;
    return data[i] - cs;
}

static int checksum_s19(uint8_t *data, int len)
{
    int i, cs;

    for (cs = 0, i = 0; i < len - 1; i++)
    {
        cs += data[i];
    }

    cs = (~cs) & 0xff;
    return data[i] - cs;
}


int fota_load_s19(const char *fpath, fota_img_t *img, int wsize)
{
    FILE *file;    
    uint8_t *mpos = img->buff;
    char line_buf[255 * 2 + 4 + 1];
    int  line_num, sect_num = 0;

    if ((file = fopen(fpath, "r")) == NULL)
    {
        log_e(LOG_FOTA, "load S19 file: \"%s\" fail", fpath);
        return -1;
    }

    memset(img->sect, 0, sizeof(img->sect));

    for (line_num = 1; fgets(line_buf, sizeof(line_buf), file); line_num++)
    {
        uint32_t base, type, dlen;
        uint8_t  data[255 + 1], *dpos;
        img_sect_t *sect = img->sect + sect_num;
        
        if (cutspace(line_buf) <= 0)
        {
            continue;
        }

        type = GET_HWBE(line_buf) - 0x5330;
        
        if (line_buf[0] != 'S' || type < 1 || type > 3)
        {
            log_e(LOG_FOTA, "unsupported line(%d): \"%s\"", line_num, line_buf);
            continue;
        }

        dlen = hex2bin(line_buf + 2, data);

        if (dlen <= 0 || dlen != GET_BYTE(data) + 1)
        {
            log_e(LOG_FOTA, "error format at line(%d): \"%s\"", line_num, line_buf);
            fclose(file);
            return -1;;
        }

        if (checksum_s19(data, dlen) != 0)
        {
            log_e(LOG_FOTA, "checksum error at line(%d): \"%s\"", line_num, line_buf);
            fclose(file);
            return -1;
        }
        
        for (base = 0, dlen -= 2, dpos = data + 1; type + 1 > 0; type--, dlen--, dpos++)
        {
            base = base * 256 + GET_BYTE(dpos);
        }

        if (dlen & ((1 << wsize) - 1))
        {
            log_e(LOG_FOTA, "data length(%d) is not matched with word size %d", dlen - 2, 1 << wsize);
            fclose(file);
            return -1;
        }

        if (mpos + dlen > img->buff + sizeof(img->buff))
        {
            log_e(LOG_FOTA, "image size overflow(max=%dKB) at line(%d): \"%s\"", 
                IMAGE_MAX_SIZE, line_num, line_buf);
            fclose(file);
            return -1;
        }

        if (sect->size != 0)
        {
            if (sect->base + (sect->size >> wsize) == base)
            {
                sect->size += dlen;                
            }
            else if (sect_num + 1 < IMAGE_MAX_SECT)
            {
                sect++;
                sect_num++;                
            }
            else
            {
                log_e(LOG_FOTA, "too many sections(max=%d) at line(%d): \"%s\"", 
                    IMAGE_MAX_SECT, line_num, line_buf);
                fclose(file);
                return -1;
            }
        }

        if (sect->size == 0)
        {
            sect->base = base;
            sect->size = dlen;
            sect->data = mpos;            
        }

        memcpy(mpos, dpos, dlen);
        mpos += dlen;
    }


    if (!feof(file))
    {
        log_e(LOG_FOTA, "access error in file %s", fpath);
        fclose(file);
        return -1;
    }
    
    fclose(file);
    return 0;
}

int fota_calc_s19(const char *fpath)
{
    FILE *file;
    char line_buf[255 * 2 + 4 + 1];
    int  size = 0;

    if ((file = fopen(fpath, "r")) == NULL)
    {
        log_e(LOG_FOTA, "load S19 file: \"%s\" fail", fpath);
        return -1;
    }

    for (size = 0; fgets(line_buf, sizeof(line_buf), file);)
    {
        if (cutspace(line_buf) <= 0)
        {
            continue;
        }
        
        if (line_buf[0] != 'S' || line_buf[1] < '1' || line_buf[1] > '3')
        {
            continue;
        }

        /* excluding address & check sum */
        if (hex2byt(line_buf + 2) - (line_buf[1] - '0' + 1) - 1 <= 0)
        {
            continue;
        }

        size += hex2byt(line_buf + 2) - (line_buf[1] - '0' + 1) - 1;
    }


    if (!feof(file))
    {
        log_e(LOG_FOTA, "access error in file %s", fpath);
        fclose(file);
        return -1;
    }

    fclose(file);
    return size;
}


int fota_load_bin(const char *fpath, fota_img_t *img, uint32_t base)
{
    FILE *file;
    uint8_t *mpos;

    if ((file = fopen(fpath, "r")) == NULL)
    {
        log_e(LOG_FOTA, "load BIN file: \"%s\" fail", fpath);
        return -1;
    }

    memset(img->sect, 0, sizeof(img->sect));
    mpos = img->buff;
    img->sect[0].base = base;
    
    while (!feof(file))
    {
        int len = fread(mpos, 1, 1024, file);

        if (ferror(file))
        {
            log_e(LOG_FOTA, "load BIN file: \"%s\" fail", fpath);
            fclose(file);
            return -1;
        }

        if (img->sect[0].size + len > IMAGE_MAX_SIZE * 1024)
        {
            log_e(LOG_FOTA, "BIN file: \"%s\" is too large to load(max %d KB)", fpath, IMAGE_MAX_SIZE);
            fclose(file);
            return -1;
        }

        img->sect[0].size += len;
        mpos += len;
    }

    fclose(file);
    return 0;
}

int fota_calc_bin(const char *fpath)
{
    FILE *file;
    int size;

    if ((file = fopen(fpath, "r")) == NULL)
    {
        log_e(LOG_FOTA, "load BIN file: \"%s\" fail", fpath);
        return -1;
    }

    if (fseek(file, 0, SEEK_SET) != 0 || (size = ftell(file)) < 0)
    {
        log_e(LOG_FOTA, "access error in file %s", fpath);
        fclose(file);
        return -1;
    }

    fclose(file);
    return size;
}

int fota_load_hex(const char *fpath, fota_img_t *img)
{
    FILE *file;
    char line_buf[255 * 2 + 6 * 2 + 1];
    int  line_num, sect_num = 0;
    uint8_t *mpos = img->buff;
    uint32_t seg = 0;

    if ((file = fopen(fpath, "r")) == NULL)
    {
        log_e(LOG_FOTA, "load HEX file: \"%s\" fail", fpath);
        return -1;
    }

    memset(img->sect, 0, sizeof(img->sect));

    for (line_num = 1; fgets(line_buf, sizeof(line_buf), file); line_num++)
    {
        int type, dlen, off;
        uint8_t data[255 + 5], *dpos;
        img_sect_t *sect = img->sect + sect_num;

        if (cutspace(line_buf) <= 0)
        {
            continue;
        }

        type = line_buf[8] - '0';
        
        if (line_buf[0] != ':' || (type != 2 && type != 4 && type != 0))
        {
            log_e(LOG_FOTA, "unsupported line(%d): \"%s\"", line_num, line_buf);
            continue;
        }
        
        dlen = hex2bin(line_buf + 1, data);

        if (dlen <= 0 || dlen != GET_BYTE(data) + 5)
        {
            log_e(LOG_FOTA, "error format at line(%d): \"%s\"", line_num, line_buf);
            fclose(file);
            return -1;;
        }

        if (checksum_hex(data, dlen) != 0)
        {
            log_e(LOG_FOTA, "checksum error at line(%d): \"%s\"", line_num, line_buf);
            fclose(file);
            return -1;
        }

        if (type != 0)
        {
            seg = GET_HWBE(data + 4) << (1 << type);
            continue;
        }

        off  = GET_HWBE(data + 1);
        dlen = GET_BYTE(data);
        dpos = data + 4;

        if (mpos + dlen > img->buff + sizeof(img->buff))
        {
            log_e(LOG_FOTA, "image size overflow(max=%dKB) at line(%d): \"%s\"", 
                IMAGE_MAX_SIZE, line_num, line_buf);
            fclose(file);
            return -1;
        }
        
        if (sect->size != 0)
        {
            if (sect->base + sect->size == seg + off)
            {
                sect->size += dlen;                
            }
            else if (sect_num + 1 < IMAGE_MAX_SECT)
            {
                sect++;
                sect_num++;                
            }
            else
            {
                log_e(LOG_FOTA, "too many sections(max=%d) at line(%d): \"%s\"", 
                    IMAGE_MAX_SECT, line_num, line_buf);
                fclose(file);
                return -1;
            }
        }

        if (sect->size == 0)
        {
            sect->base = seg + off;
            sect->size = dlen;
            sect->data = mpos;            
        }

        memcpy(mpos, dpos, dlen);
        mpos += dlen;
    }


    if (!feof(file))
    {
        log_e(LOG_FOTA, "access error in file %s", fpath);
        fclose(file);
        return -1;
    }
        
    fclose(file);
    return 0;
}

int fota_calc_hex(const char *fpath)
{
    FILE *file;
    char line_buf[255 * 2 + 6 * 2 + 1];
    int size;

    if ((file = fopen(fpath, "r")) == NULL)
    {
        log_e(LOG_FOTA, "load HEX file: \"%s\" fail", fpath);
        return -1;
    }
    
    for (size = 0; fgets(line_buf, sizeof(line_buf), file);)
    {
        if (cutspace(line_buf) <= 0)
        {
            continue;
        }
        
        if (line_buf[0] != ':' || line_buf[8] != '0')
        {
            continue;
        }
        
        /* excluding address & check sum */
        if (hex2byt(line_buf + 1) <= 0)
        {
            continue;
        }

        size += hex2byt(line_buf + 1);
    }


    if (!feof(file))
    {
        log_e(LOG_FOTA, "access error in file %s", fpath);
        fclose(file);
        return -1;
    }
        
    fclose(file);
    return size;
}

void fota_show_img(fota_img_t *img)
{
    img_sect_t *sect;
    int i = 0;

    FOR_EACH_IMGSECT(sect, img, 1)
    {
        log_e(LOG_FOTA, "image section %d, base=%08X, size=%08X(%d)", 
            ++i, sect->base, sect->size, sect->size);
    }
}


