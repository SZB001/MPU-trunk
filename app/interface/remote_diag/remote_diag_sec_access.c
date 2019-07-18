#include  <stdlib.h>
#include  <stdint.h>
#include "remote_diag.h"

#define TOPBIT              (0x8000)
#define POLYNOM_1           (0x8408)
#define POLYNOM_2           (0x8025)
#define BITMASK             (0x0080)
#define INITIAL_REMINDER    (0xFFFE)
#define MSG_LEN             (2) /* seed length in bytes */

void remote_diag_calcKey(char      * seed, char * key, unsigned char len)
{
    uint8_t bSeed[2];
    uint16_t remainder;
    uint8_t n;
    uint8_t i;
    
    bSeed[0] = seed[0]; /* MSB */
    bSeed[1] = seed[1]; /* LSB */
    
    remainder = INITIAL_REMINDER;
    
    for (n = 0; n < MSG_LEN; n++)
    {
        /* Bring the next byte into the remainder. */
        remainder ^= ((bSeed[n]) << 8);
        
        /* Perform modulo-2 division, a bit at a time. */
        for (i = 0; i < 8; i++)
        {
            /* Try to divide the current data bit. */
            if (remainder & TOPBIT)
            {
                if(remainder & BITMASK)
                {
                    remainder = (remainder << 1) ^ POLYNOM_1;
                }
                else
                {
                    remainder = (remainder << 1) ^ POLYNOM_2;
                }
            }
            else
            {
                remainder = (remainder << 1);
            }
        }
    }
    /* The final remainder is the key */
    key[0] = (remainder & 0xff00) >> 8;
    key[1] = (remainder & 0x00ff);
} 



