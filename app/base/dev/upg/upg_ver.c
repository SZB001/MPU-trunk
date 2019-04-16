#include "com_app_def.h"
#include "dev_api.h"
#include "file.h"
#include "shm.h"
#include "at_api.h"
#include "timer.h"
#include <assert.h>

static char mcu_blt_ver[APP_VER_LEN] = {0};
static char upg_mcu_ver[APP_VER_LEN] = {0};
static void *startup_status_addr = NULL;

/****************************************************************
 function:     upg_get_pkg_ver
 description:  get pkg version
 input:        none
 output:       unsigned char *ver
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
int upg_get_pkg_ver(unsigned char *ver, unsigned int size)
{
    int ret;
    unsigned int len;
    assert(ver);
    assert(size > 0);

    if (false == file_exists(COM_APP_CUR_IMAGE_DIR"/"COM_PKG_VER))
    {
        log_e(LOG_DEV, "file is not exists");
        return DEV_FILE_NOT_EXIST;
    }

    len = size;
    ret = file_read(COM_APP_CUR_IMAGE_DIR"/"COM_PKG_VER, ver, &len);

    if (ret != 0)
    {
        log_e(LOG_DEV, "read pkg version failed, ret:0x%08x", ret);
    }

    return ret;
}

/****************************************************************
 function:     upg_get_fw_ex_ver
 description:  get firmware extended version
 input:        unsigned char *ver
               unsigned int size
 output:       unsigned char *ver
 return:       positive indicates the length of the ex version string;
               negative indicates failed
 *****************************************************************/
int upg_get_fw_ex_ver(char *ver, unsigned int size)
{
    assert(ver);
    assert(size > 0);
    int ret, i;
    char intest_ver[TBOX_FW_VER_LEN];
    unsigned int len = sizeof(intest_ver);
    memset(intest_ver, 0, sizeof(intest_ver));

    if (file_exists(COM_FW_EX_VER_FILE))
    {
        ret = file_read(COM_FW_EX_VER_FILE, (unsigned char *)intest_ver, &len);

        if (ret != 0)
        {
            return -2;
        }
        else
        {
            for (i = 0; i < len; i++)
            {
                if ('\r' == intest_ver[i] || '\n' == intest_ver[i])
                {
                    intest_ver[i] = '\0';
                    break;
                }
            }

            strncpy(ver, intest_ver, size);
            return strlen(ver);
        }
    }
    else
    {
        return -1;
    }
}

/****************************************************************
 function:     upg_get_fw_ver
 description:  get firmware version
 input:        none
 output:       unsigned char *ver
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
int upg_get_fw_ver(unsigned char *ver, unsigned int size)
{
    assert(ver);
    assert(size > 0);
    unsigned int len;
    at_get_ati((char *) ver, size);
    len = strlen((char *)ver);

    if (size > len)
    {
        upg_get_fw_ex_ver((char *)ver + len, size - len);
    }

    return 0;

}

/****************************************************************
 function:     upg_get_app_ver
 description:  get mpu app version
 input:        none
 output:       unsigned char *ver
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
int upg_get_app_ver(unsigned char *ver, unsigned int size)
{
    unsigned int len;

    assert(ver);
    assert(size > 0);

    len = strlen(COM_APP_MAIN_VER) ;

    if (size <= len)
    {
        log_e(LOG_DEV, "the buffer overflow");
        return DEV_INVALID_PARA;
    }

    strncpy((char *)ver, COM_APP_MAIN_VER, len);
    ver[len] = 0;

    return 0;
}

/****************************************************************
 function:     upg_get_mcu_upg_ver
 description:  get the mcu version which the mpu expected to upgrade
 input:        none
 output:       unsigned char *ver
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
int upg_get_mcu_upg_ver(unsigned char *ver, unsigned int size)
{
    int ret = 0;
    unsigned int len;
    static unsigned char mcu_ver[APP_VER_LEN] = {0};

    assert(ver);
    assert(size > 0);

    if (0 != strlen((char *)mcu_ver))
    {
        memcpy(ver, mcu_ver, size);
        return 0;
    }

    if (false == file_exists(COM_APP_CUR_IMAGE_DIR"/"COM_MCU_VER))
    {
        return DEV_FILE_NOT_EXIST;
    }

    if (false == file_exists(COM_APP_CUR_IMAGE_DIR"/"COM_MCU_APP))
    {
        return DEV_FILE_NOT_EXIST;
    }

    len = size;
    ret = file_read(COM_APP_CUR_IMAGE_DIR"/"COM_MCU_VER, mcu_ver, &len);

    if (ret != 0)
    {
        log_e(LOG_DEV, "read pkg version failed, ret:0x%08x", ret);
    }
    else
    {
        ver[len] = 0;
    }

    memcpy(ver, mcu_ver, len);
    return ret;
}

/****************************************************************
 function:     upg_get_mcu_run_ver
 description:  get the mcu version which is running now
 input:        char *ver
               unsigned int len
 output:       none
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int upg_get_mcu_run_ver(unsigned char *ver, unsigned int size)
{
    memset(ver, 0, size);
    strncpy((char *)ver, upg_mcu_ver, APP_VER_LEN - 1);

    return 0;
}

/****************************************************************
 function:     upg_get_mcu_blt_ver
 description:  get the mcu bootloader version which is running now
 input:        char *ver
               unsigned int len
 output:       none
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int upg_get_mcu_blt_ver(unsigned char *ver, unsigned int size)
{
    memset(ver, 0, size);
    strncpy((char *)ver, mcu_blt_ver, APP_VER_LEN - 1);
    return 0;
}

/****************************************************************
 function:     upg_init_startup
 description:  init the shared memory
 input:        none
 output:       none
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int upg_init_startup(void)
{
    startup_status_addr = shm_create(COM_APP_STARTUP_FILE, O_RDWR, 16);

    if (NULL == startup_status_addr)
    {
        log_e(LOG_DEV, "create shm failed");
        return DEV_FILE_OPEN_FAILED;
    }

    return 0;
}

/****************************************************************
 function:     upg_get_startup
 description:  get startup status data from shared memory
 input:        unsigned int size
 output:       char *status
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int upg_get_startup(char *status, unsigned int size)
{
    int ret;

    ret = shm_read(startup_status_addr, (unsigned char *)status, size);

    if (ret != 0)
    {
        log_e(LOG_DEV, "read startup shm failed, ret:%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     upg_set_startup
 description:  set startup status data to shared memory
 input:        unsigned int size
               char *status
 output:       none
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int upg_set_startup(char *status, unsigned int size)
{
    int ret;

    ret = shm_write(startup_status_addr, (unsigned char *)status, size);

    if (ret != 0)
    {
        log_e(LOG_DEV, "write shm failed, ret:%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     upg_proc_mcu_ver_msg
 description:  proc the mcu version message
 input:        char *ver
               unsigned int len
 output:       none
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int upg_proc_mcu_ver_msg(unsigned char *msg, unsigned int len)
{
    int ret;
    unsigned int size;
    char ver[64];
    char status[16];

    if ((len <= 0) || (len >= APP_VER_LEN))
    {
        log_e(LOG_DEV, "invalid message len, len:%u", len);
        return DEV_INVALID_PARA;
    }

    memset(upg_mcu_ver, 0, sizeof(upg_mcu_ver));
    memcpy(upg_mcu_ver, msg, len);

    size = sizeof(ver);
    memset(ver, 0, size);
    ret = upg_get_mcu_upg_ver((unsigned char *)ver, size);

    /* if no mcu_ver.dat file or the mcu upgraded successfully */
    if ((0 == strncmp(upg_mcu_ver, ver, strlen(ver)) || (ret != 0)) && (tm_get_time() > (15 * 1000)))
    {
        ret = upg_get_startup(status, sizeof(status));

        if (ret != 0)
        {
            log_e(LOG_DEV, "read startup shm failed, ret:%08x", ret);
        }
        else
        {
            if (0 == strncmp(status, "OK", strlen("OK")))
            {
                return 0;
            }
        }

        log_o(LOG_DEV, "begin write startup shm");
        upg_set_startup("OK", strlen("OK") + 1);
        log_o(LOG_DEV, "end write startup shm");

		upg_set_status( DEV_UPG_IDLE );
		
        dev_print_softver_delay();
    }
	
    return 0;
}

/****************************************************************
 function:     upg_proc_btl_ver_msg
 description:  proc the mcu version message
 input:        char *ver
               unsigned int len
 output:       none
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int upg_proc_btl_ver_msg(unsigned char *msg, unsigned int len)
{
    if ((len <= 0) || (len >= APP_VER_LEN))
    {
        log_e(LOG_DEV, "invalid message len, len:%u", len);
        return DEV_INVALID_PARA;
    }

    memset(mcu_blt_ver, 0, sizeof(mcu_blt_ver));
    memcpy(mcu_blt_ver, msg, len);

    return 0;
}

