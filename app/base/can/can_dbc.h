#ifndef __CAN_DBC_H__
#define __CAN_DBC_H__


#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define ABS(d)              ((d) < 0 ? -(d) : (d))

static __inline uint16_t REV16(uint16_t d)
{
    uint16_t out;

    out  = d >> 8;
    out |= d << 8;
    return out;
}

static __inline uint32_t REV32(uint32_t d)
{
    uint32_t out;

    out  = REV16(d >> 16);
    out |= REV16(d) << 16;
    return out;
}

static __inline uint64_t REV64(uint64_t d)
{
    uint64_t out;

    out  = REV32(d >> 32);
    out |= (uint64_t)REV32(d) << 32;
    return out;
}

extern int dbc_init(INIT_PHASE phase);
extern int dbc_load_file(const char *fpath, short baud[]);
extern int dbc_can_callback(uint32_t event, uint32_t par1, uint32_t par2);

extern void dbc_canid_to_mcu(void);
extern void dbc_canid_check(unsigned char no, unsigned char *result);

#endif