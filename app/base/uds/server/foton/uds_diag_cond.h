#ifndef UDS_DIAG_COND_H
#define UDS_DIAG_COND_H

#include "dev_api.h"

static __inline bool uds_diag_available(void)
{
    uint8_t ret;

    ret = dev_get_KL15_signal();

    return (ret != 0);
}


#endif
