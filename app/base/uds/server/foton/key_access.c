#include  <stdlib.h>
#include  <stdint.h>

/*
dwKey1=£¨a*£¨dwSeed1*dwSeed1£©£©+£¨b*£¨dwSeed2*dwSeed2£©£©+£¨c*£¨dwSeed1*dwSeed2£©£©£»
dwKey2=£¨a* dwSeed1£©+£¨b* dwSeed2£©+£¨d*£¨dwSeed1*dwSeed2£©£©£»
dwKey3=£¨a*£¨dwSeed3*dwSeed4£©£©+£¨b*£¨dwSeed4*dwSeed4£©£©+£¨c*£¨dwSeed3*dwSeed4£©£©£»
dwKey4=£¨a*£¨dwSeed3*dwSeed4£©£©+£¨b* dwSeed4£©+£¨d*£¨dwSeed3*dwSeed4£©£©£»
dwNewKeywrd=£¨£¨dwKey1 & 0xFF£©£¼£¼24£©+
£¨£¨dwKey2 & 0xFF£©£¼£¼16£©+
£¨£¨dwKey3 & 0xFF£©£¼£¼ 8£©+
£¨dwKey4 & 0xFF£©£©£»

Level1(0x01/02)? Key Parameters
T©\BOX a=b1h b=a2h c=49h d=09h

Level3(0x03/04)? Key Parameters
T©\BOX a=deh b=08h c=3bh d=5fh
*/

/*
const uint32_t wconst1 = 0xb1a24909;
const uint32_t wconst2 = 0xde083b5f;
*/

#define TOPBIT              (0x8000)
#define POLYNOM_1           (0x8408)
#define POLYNOM_2           (0x8025)
#define BITMASK             (0x0080)
#define INITIAL_REMINDER    (0xFFFE)
#define MSG_LEN             (2) /* seed length in bytes */



const uint32_t wconst1 = 0x0949a2b1;
const uint32_t wconst2 = 0x5f3b08de;

uint32_t saGetKey(uint32_t wSeed, uint8_t mode)
{
    uint8_t Seed[4];
    uint8_t Const[4];
    uint8_t Key[4];
    uint32_t wKey;
    uint32_t wConst = 0;

    if (mode != 2 && mode != 4)
    {
        return 0;
    }

    if (mode == 2)
    {
        wConst = wconst1;
    }
    else
    {
        wConst = wconst2;
    }

    Seed[0] = (uint8_t)((wSeed & 0xff000000) >> 24);
    Seed[1] = (uint8_t)((wSeed & 0x00ff0000) >> 16);
    Seed[2] = (uint8_t)((wSeed & 0x0000ff00) >> 8);
    Seed[3] = (uint8_t)(wSeed & 0x000000ff);

    Const[3] = (uint8_t)((wConst & 0xff000000) >> 24);
    Const[2] = (uint8_t)((wConst & 0x00ff0000) >> 16);
    Const[1] = (uint8_t)((wConst & 0x0000ff00) >> 8);
    Const[0] = (uint8_t)(wConst & 0x000000ff);

    Key[0] = Const[0] * (Seed[0] * Seed[0]) + Const[1] * (Seed[1] * Seed[1]) + Const[2] *
             (Seed[0] * Seed[1]);
    Key[1] = Const[0] * (Seed[0])           + Const[1] * (Seed[1])           + Const[3] *
             (Seed[0] * Seed[1]);
    Key[2] = Const[0] * (Seed[2] * Seed[3]) + Const[1] * (Seed[3] * Seed[3]) + Const[2] *
             (Seed[2] * Seed[3]);
    Key[3] = Const[0] * (Seed[2] * Seed[3]) + Const[1] * (Seed[3])           + Const[3] *
             (Seed[2] * Seed[3]);

    wKey = ((((uint32_t)Key[3]) & 0x000000ff)
            + (((uint32_t)(Key[2]) << 8) & 0x0000ff00)
            + (((uint32_t)(Key[1]) << 16) & 0x00ff0000)
            + (((uint32_t)(Key[0]) << 24) & 0xff000000));
    return wKey;
}


uint16_t calcKey(uint16_t seed)
{
    uint8_t bSeed[2];
    uint16_t remainder;
    uint8_t n;
    uint8_t i;
    
    bSeed[0] = (uint8_t)(seed >> 8); /* MSB */
    bSeed[1] = (uint8_t)seed; /* LSB */
    
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
    return remainder;
} 



