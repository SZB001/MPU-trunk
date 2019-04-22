#ifndef __UDS_FILE__
#define __UDS_FILE__

#include "stdio.h"
#include <stdint.h>

#define    maxNumberOfBlockLength     0x0402

typedef enum
{

    TransferStart = 0,
    TransferEnd,

} TRANSFERDATASTATUS;

typedef struct
{

    uint32_t  ProgrammemoryAddress;
    uint32_t  ProgrammemorySize;
    uint32_t  ErasememoryAddress;
    uint32_t  ErasememorySize;
    uint8_t   blockSequenceCounter;
    TRANSFERDATASTATUS TransferStatus;
    uint8_t   data[80 * 1024];
    uint32_t  datasize;

} TRANSFERDATACOFIG;


extern TRANSFERDATACOFIG TransferDataConfig;

#endif
