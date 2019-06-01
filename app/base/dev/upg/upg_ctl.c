#include <zlib.h>
#include <stdlib.h>
#include "com_app_def.h"
#include "tcom_api.h"
#include "dev_api.h"
#include "dev_rw.h"
#include "md5.h"
#include "dev.h"
#include "dir.h"
#include "file.h"
#include "pm_api.h"
#include "shm.h"
#include "timer.h"
#include "scom_msg_def.h"
#include "scom_tl.h"
#include "scom_api.h"
#include "upg_ctl.h"

static unsigned char rawData[1024 * 1024 * 4];
static unsigned char upg_file_data[2 * 1024 * 1024];
//extern void tbox_self_upgrade_report(void);


/****************************************************************
function:     upg_scom_msg_proc
description:  process spi msg about upgrade
input:        unsigned char *msg, spi message;
              unsigned int len, message length
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int upg_ctl_scom_msg_proc(unsigned char *msg, unsigned int len)
{
    int ret;
    char *app_path = NULL;
    unsigned int file_len = 0;
    SCOM_TL_NG_RES_MSG res;
    SCOM_TL_MSG_HDR *tl_hdr = (SCOM_TL_MSG_HDR *)msg;

    if (len < sizeof(SCOM_TL_MSG_HDR))
    {
        log_e(LOG_DEV, "invalid message,len:%u", len);
        return -1;
    }

    if (((tl_hdr->msg_type & 0xf0) >> 4) != SCOM_TL_CHN_MGR)
    {
        log_e(LOG_DEV, "invalid message,msgtype:%u, fct:%u", tl_hdr->msg_type, SCOM_TL_CHN_MGR);
        return -1;
    }

    switch ((tl_hdr->msg_type))
    {
        case SCOM_TL_CMD_UPG_MCU_REQ:
            scom_tl_stop_server();
            app_path = COM_APP_CUR_IMAGE_DIR"/"COM_MCU_APP;
            log_o(LOG_DEV, "start upgrading mcu, upgrade times = %d, start time:%u",
                  msg[sizeof(SCOM_TL_MSG_HDR)], (unsigned int)tm_get_time());

            file_len = sizeof(upg_file_data);
			memset(upg_file_data, 0, sizeof(upg_file_data));
            ret = file_read(app_path, upg_file_data , &file_len);

            log_o(LOG_DEV, "rx_data_length = %d", file_len);

            if ((ret != 0) || (file_len <= APP_BIN_LEN))
            {
                log_e(LOG_DEV, "read mcu app failed, ret:0x%08x", ret);
                res.result = SCOM_TL_RET_READ_FAILED;
                res.reason = 0;
                scom_tl_send_msg(SCOM_TL_CMD_UPG_MCU_RES, (unsigned char *)&res, sizeof(res));
                return ret;
            }

            ret = scom_tl_start_server(upg_file_data, file_len);

            if (ret != 0)
            {
                log_e(LOG_DEV, "start upgrade mcu, start scom server failed:%u", ret);
                res.result = SCOM_TL_RET_SERVER_START_FAILED;
                res.reason = 0;
                scom_tl_send_msg(SCOM_TL_CMD_UPG_MCU_RES, (unsigned char *)&res, sizeof(res));
                return ret;
            }

            res.result = SCOM_TL_RET_OK;
            res.reason = 0;
            scom_tl_send_msg(SCOM_TL_CMD_UPG_MCU_RES, (unsigned char *)&res, sizeof(res));
            break;

        case SCOM_TL_CMD_UPG_MCU_FIN:
            log_o(LOG_DEV, "finish upgrading mcu,end time:%u", (unsigned int)tm_get_time());
            scom_tl_stop_server();
            
			//tbox_self_upgrade_report();//by liujian

            res.result = SCOM_TL_RET_OK;
            res.reason = 0;
            scom_tl_send_msg(SCOM_TL_CMD_UPG_MCU_FIN_RES, (unsigned char *)&res, sizeof(res));
            break;

        case SCOM_TL_CMD_UPG_MCU_VER:
            scom_forward_msg(MPU_MID_DEV, DEV_SCOM_MSG_MCU_VER,
                             msg + sizeof(SCOM_TL_MSG_HDR), len - sizeof(SCOM_TL_MSG_HDR));
            break;

        case SCOM_TL_CMD_MCU_BTL_VER:
            scom_forward_msg(MPU_MID_DEV, DEV_SCOM_MSG_MCU_BTL_VER,
                             msg + sizeof(SCOM_TL_MSG_HDR), len - sizeof(SCOM_TL_MSG_HDR));
            break;

        default:
            log_e(LOG_DEV, "invalid message,msgtype:%u", tl_hdr->msg_type);
            break;
    }

    return 0;
}

/****************************************************************
 function:     upg_ctl_init
 description:  init buff
 input:        none
 output:       none
 return:       none
 *****************************************************************/
void upg_ctl_init(INIT_PHASE phase)
{
	switch (phase)
    {
        case INIT_PHASE_INSIDE:
			memset(rawData, 0, sizeof(rawData));
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            upg_set_status( DEV_UPG_IDLE );
            break;

        default:
            break;
    }

    return;
}

/****************************************************************
 function:     upg_ctl_copy
 description:  copy images from one directory to other directory
 input:        const char *src_dir, source directory;
 const char *dst_dir, destination directory;
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int upg_ctl_copy(const char *src_dir, const char *dst_dir)
{
    int ret;

    if (!dir_exists(src_dir))
    {
        return 0;
    }

    if (dir_is_empty(src_dir))
    {
        return 0;
    }

    /* remove the files and directory in dst_dir */
    ret = dir_remove_path(dst_dir);

    if (ret != 0)
    {
        log_e(LOG_DEV, "remove data dir failed,path:%s, ret:0x%08x",
              dst_dir, ret);
        return ret;
    }

    /* create dst data directory */
    ret = dir_make_path(dst_dir, S_IRUSR | S_IWUSR | S_IXUSR, false);

    if (0 != ret)
    {
        return ret;
    }

    /* copy the file and directory from src_dir to dst_dir */
    ret = dir_copy_path(src_dir, dst_dir);

    if (ret != 0)
    {
        log_e(LOG_DEV, "copy images failed, src path:%s,dst path:%s, ret:0x%08x",
              src_dir, dst_dir, ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     upg_ctl_chk_mpu_app_ver
 description:  check mpu app image version
 input:        char *upg_ver, the request imge version to upgrade
 output:       none
 return:       0 indicates success;
               others indicates failed
*****************************************************************/
unsigned int upg_ctl_chk_mpu_app_ver(unsigned char *upg_ver)
{
    int ret;
    unsigned int size;
    unsigned char run_ver[APP_VER_LEN];

    size = sizeof(run_ver);
    ret = upg_get_app_ver(run_ver, size);

    if ((0 == ret) && (0 == strcmp((char *)run_ver, (char *)upg_ver)))
    {
        log_e(LOG_DEV, "mpu app the same version, run_ver:%s, upg_ver:%s",
              (char *)run_ver, (char *)upg_ver);
        return DEV_SAME_VER;
    }

    return 0;
}

/****************************************************************
 function:     upg_ctl_chk_mcu_app_ver
 description:  check mcu app image version
 input:        char *upg_ver, the request imge version to upgrade
 output:       none
 return:       0 indicates success;
               others indicates failed
*****************************************************************/
unsigned int upg_ctl_chk_mcu_app_ver(unsigned char *upg_ver)
{
    int ret;
    unsigned int size;
    unsigned char run_ver[APP_VER_LEN];

    size = sizeof(run_ver);
    ret = upg_get_mcu_run_ver(run_ver, size); /* include compile time */

    if ((0 == ret) && (0 == strncmp((char *)run_ver, (char *)upg_ver,
                                    strlen((char *)upg_ver))))
    {
        log_e(LOG_DEV, "mcu app the same version, run_ver:%s, upg_ver:%s",
              (char *)run_ver, (char *)upg_ver);
        return DEV_SAME_VER;
    }

    ret = upg_get_mcu_upg_ver(run_ver, size);

    if ((0 == ret) && (0 == strcmp((char *)run_ver, (char *)upg_ver)))
    {
        log_e(LOG_DEV, "mcu upg app the same version, run_ver:%s, upg_ver:%s",
              (char *)run_ver, (char *)upg_ver);
        return DEV_SAME_VER;
    }

    return 0;
}

/****************************************************************
 function:     upg_ctl_pkg_proc
 description:  uncompress PKG and save to file
 input:        unsigned char *data
 unsigned char cmd_type
 unsigned int offset
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
unsigned int upg_ctl_save(unsigned char cmd_type, unsigned char *data)
{
    int ret;
    char *path = NULL;
    unsigned int length = 0;
    unsigned int attribute = 0;
    unsigned int content_length = 0;
    unsigned long len = sizeof(rawData);
    unsigned char upg_ver[256];

    /*whether the data is compress */
    memcpy((unsigned char *) &attribute, data, sizeof(attribute));

    /*the length of data*/
    memcpy((unsigned char *) &length, &data[sizeof(attribute)], sizeof(length));

    /*if data is compress*/
    if (0 != attribute)
    {
    	if (Z_OK != uncompress(rawData, &len, &data[sizeof(attribute) + sizeof(length)],length))
        {
            log_e(LOG_DEV, "uncompress file failed, len:%u, attr:%u", length, attribute);
            return DEV_UNCOMPRESS_FAILED;
        }
    }
    else
    {
        memcpy(rawData, &data[sizeof(attribute) + sizeof(length)], length);
    }

    /*the length of version*/
    memcpy((unsigned char *) &length, &rawData[0], sizeof(length));

    #if 0
    if (length > APP_VER_LEN)
    {
        memcpy(upg_ver, &rawData[sizeof(length)], APP_VER_LEN);
        log_e(LOG_DEV, "app upg_ver length > %d", APP_VER_LEN);
    }
    #endif

    memset(upg_ver, 0, sizeof(upg_ver));
    memcpy(upg_ver, &rawData[sizeof(length)], length);

    /*the length of file content*/
    memcpy((unsigned char *) &content_length, &rawData[sizeof(length) + length],
           sizeof(content_length));

    if (content_length < APP_BIN_LEN)
    {
        log_e(LOG_DEV, "invalid update file");
        return DEV_DATA_CHECK_FAILED;
    }

    switch (cmd_type)
    {
        case MCU_APP_BIN:
            ret = upg_ctl_chk_mcu_app_ver(upg_ver);

            if (ret != 0)
            {
                return ret;
            }

            ret = file_write_atomic(COM_APP_UPG_IMAGE_DIR"/"COM_MCU_VER, upg_ver,
                                    strlen((char *) upg_ver), S_IRUSR | S_IWUSR | S_IXUSR);

            if (ret != 0)
            {
                log_e(LOG_DEV, "write mcu upg_ver failed, ret:0x%08x", ret);
                return ret;
            }

            path = COM_APP_UPG_IMAGE_DIR"/"COM_MCU_APP;

            break;

        case MCU_BTL_BIN:
            path = COM_APP_UPG_IMAGE_DIR"/"COM_MCU_BOOT;
            break;

        case MCU_CFG_FILE:
            path = (char *)upg_ver;
            break;

        case MPU_APP_BIN:
            ret = upg_ctl_chk_mpu_app_ver(upg_ver);

            if (ret != 0)
            {
                return ret;
            }

            path = COM_APP_UPG_IMAGE_DIR"/"COM_APP_IMAGE;
            break;

        case MPU_SHELL_FILE:
            path = COM_APP_UPG_IMAGE_DIR"/"COM_SCRIPT;
            break;

        case MPU_FIRMWARE:
            path = COM_APP_UPG_FW_DIR"/"COM_FW_UPDATE;
            break;

        default:
            return DEV_INVALID_TYPE;
    }

    if (file_exists(path))
    {
        file_delete(path);
    }

    ret = dir_make_path(path, S_IRUSR | S_IWUSR | S_IXUSR, true);

    if (ret != 0)
    {
        log_e(LOG_DEV, "make file failed,path:%s, ret:0x%08x", path, ret);
        return ret;
    }

    ret = file_write_atomic(path, &rawData[sizeof(length) + length + sizeof(content_length)],
                            content_length, S_IRUSR | S_IWUSR | S_IXUSR);

    if (0 != ret)
    {
        log_e(LOG_DEV, "save file failed, file type = %d !", cmd_type);
        return ret;
    }

    if (MPU_SHELL_FILE == cmd_type)
    {
        system(COM_APP_UPG_IMAGE_DIR"/"COM_SCRIPT);
    }

    log_o(LOG_DEV, "save %s successful", path);

    return 0;
}

/****************************************************************
 function:     upg_ctl_fwsave
 description:  uncompress PKG and save to file
 input:        none
 output:       none
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
unsigned int upg_ctl_fwsave(int fd, unsigned int offset)
{
	int ret,i;
	int fw_fd;
	int r_pos = 0;
	unsigned int attribute;
	unsigned int ver_len;
	unsigned int content_length;
	unsigned int lump_len;
	unsigned int lump_cnt;
	unsigned int length = 0;
	unsigned int w_pos = 0;
	unsigned long uncompress_len = sizeof(rawData);
	const char *fwpath = COM_APP_UPG_FW_DIR"/"COM_FW_UPDATE;

	/*whether the data is compress */
	r_pos = offset;
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		return DEV_FILE_READ_FAILED;
	}
	ret= dev_read(fd, (unsigned char *)&attribute, sizeof(attribute));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		return DEV_FILE_READ_FAILED;
	}
	
	/*the length of version*/
	r_pos = r_pos + sizeof(attribute);
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		return DEV_FILE_READ_FAILED;
	}
	ret= dev_read(fd, (unsigned char *)&ver_len, sizeof(ver_len));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		return DEV_FILE_READ_FAILED;
	}

	/*the length of content data*/
	r_pos = r_pos + sizeof(ver_len) + ver_len;
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		return DEV_FILE_READ_FAILED;
	}
	ret= dev_read(fd, (unsigned char *)&content_length, sizeof(content_length));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		return DEV_FILE_READ_FAILED;
	}

	/*the length of  one lump*/
	r_pos = r_pos + sizeof(content_length);
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		return DEV_FILE_READ_FAILED;
	}
	ret= dev_read(fd, (unsigned char *)&lump_len, sizeof(lump_len));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		return DEV_FILE_READ_FAILED;
	}

	/*the count of lumps*/
	r_pos = r_pos + sizeof(lump_len);
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		return DEV_FILE_READ_FAILED;
	}
	ret= dev_read(fd, (unsigned char *)&lump_cnt, sizeof(lump_cnt));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		return DEV_FILE_READ_FAILED;
	}

	log_o(LOG_DEV, "attribute:%d \r\n ver_len:%d\r\n content_length:%d\r\n lump_len:%d\r\n lump_cnt:%d\r\n", 
		attribute,ver_len,content_length,lump_len,lump_cnt);

	if(file_exists(fwpath))
	{
		file_delete(fwpath);
	}

	if (dir_exists("/usrdata/cache"))
    {
        ret = dir_remove_path("/usrdata/cache");
        if (ret != 0)
        {
            log_e(LOG_DEV, "remove fota cache dir failed, path:%s, ret:0x%08x",
                  "/usrdata/cache", ret);
        }
    }
    
	ret = dir_make_path(fwpath, S_IRUSR | S_IWUSR | S_IXUSR, true);
    if (ret != 0)
    {
        log_e(LOG_DEV, "make file failed,path:%s, ret:0x%08x", fwpath, ret);
        return ret;
    }
	fw_fd = open( fwpath, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR );
	if( fw_fd < 0 )
	{
		log_e(LOG_DEV, "open %s failed, error:%s", fwpath,strerror(errno));
		return DEV_FILE_OPEN_FAILED;
	}
	
	r_pos = r_pos + sizeof(lump_cnt);
	length = 0;
	for(i = 0; i < lump_cnt; i++)
	{
		r_pos = r_pos + length;
		if(r_pos != lseek(fd, r_pos, SEEK_SET))
		{
			log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
			return DEV_FILE_READ_FAILED;
		}
		ret= dev_read(fd, (unsigned char *)&length, sizeof(length));
		if(0 != ret)
		{
			log_e(LOG_DEV, "read file failed, ret = %d", ret);
			return DEV_FILE_READ_FAILED;
		}
		
		r_pos = r_pos + sizeof(length);
		if(r_pos != lseek(fd, r_pos, SEEK_SET))
		{
			log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
			return DEV_FILE_READ_FAILED;
		}
		ret= dev_read(fd, upg_file_data, length);
		if(0 != ret)
		{
			log_e(LOG_DEV, "read file failed, ret = %d", ret);
			return DEV_FILE_READ_FAILED;
		}

		/*if data is compress*/
		if (0 != attribute)
		{
			if (Z_OK != uncompress(rawData, &uncompress_len, upg_file_data, length))
			{
				log_e(LOG_DEV, "uncompress file failed, cnt: %d, length:%u", i,length);
				close(fw_fd);
				return DEV_UNCOMPRESS_FAILED;
			}
		}
		else
		{
			memcpy(rawData, upg_file_data, length);
			uncompress_len = length;
		}
		
		lseek (fw_fd, w_pos, SEEK_SET);
		ret = dev_write(fw_fd, rawData, uncompress_len);
		if(ret != 0)
		{
			log_e(LOG_DEV, "save file failed, ret:%d", ret);
			close(fw_fd);
			file_delete(fwpath);
			return ret;
		}
		
		w_pos += uncompress_len;
	}
	
	return close(fw_fd);
}

/****************************************************************
 function:     upg_ctl_proc
 description:  uncompress PKG and save to file
 input:        none
 output:       none
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
int upg_ctl_proc( int fd, unsigned int pos)
{
    int ret, i;	
	int  ver_mid = 0,ver_sid = 0;
    char version[UPG_PKG_VER_LEN] = {0};
	unsigned int  r_pos = 0;
	unsigned int  data_len = 0;
    unsigned int  cmdtotal = 0;
    unsigned int  cmdtype = 0;
    unsigned int  cmdoffset = 0;
    unsigned int  cmdlength = 0;
    unsigned char cmd_table[DEV_UPG_NUM_MAX] = {0};
    const char *fwpath = COM_APP_UPG_FW_DIR"/"COM_FW_UPDATE;

    if (fd < 0)
    {
        log_e(LOG_DEV, "invaild fd, fd = %d",fd);
        return fd;
    }

    /* remove the files and directory in image directory */
    ret = dir_remove_path(COM_APP_UPG_DIR);

    if (ret != 0)
    {
        log_e(LOG_DEV, "remove data dir failed,path:%s, ret:0x%08x",
              COM_APP_UPG_DIR, ret);
        return ret;
    }

    /* create dst directory */
    ret = dir_make_path(COM_APP_UPG_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false);

    if (0 != ret)
    {
        log_e(LOG_DEV, "make dir failed,path:%s, ret:0x%08x",
              COM_APP_UPG_DIR, ret);
        return ret;
    }

    ret = dir_update_start(COM_APP_UPG_DIR);

    if (ret != 0)
    {
        log_e(LOG_DEV, "lock dir:%s, ret:0x%08x", COM_APP_UPG_DIR, ret);
        return ret;
    }

    /* create data directory to save bin file */
    ret = dir_make_path(COM_APP_UPG_IMAGE_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false);

    if (ret != 0)
    {
        log_e(LOG_DEV, "make data dir failed,path:%s, ret:0x%08x",
              COM_APP_UPG_IMAGE_DIR, ret);
        return ret;
    }

	/* read the pkg version */
	r_pos = 1;
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		return DEV_FILE_READ_FAILED;
	}
	ret= dev_read(fd, (unsigned char *)version, sizeof(version));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		return DEV_FILE_READ_FAILED;
	}

    /*write PKG version*/
    ret = file_write_atomic(COM_APP_UPG_IMAGE_DIR"/"COM_PKG_VER, (unsigned char *)version,
                            strlen(version), S_IRUSR | S_IWUSR | S_IXUSR);
    if (ret != 0)
    {
        log_e(LOG_DEV, "write pkg version failed, ret:0x%08x", ret);
        return ret;
    }
    sscanf(version, "Ver.%d.%d", &ver_mid, &ver_sid);

	/*the length of data*/
	r_pos = pos;
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		return DEV_FILE_READ_FAILED;
	}
	ret= dev_read(fd, (unsigned char *)&data_len, sizeof(data_len));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		return DEV_FILE_READ_FAILED;
	}

    /*the number of cmd*/
	r_pos = pos + sizeof(data_len);
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		return DEV_FILE_READ_FAILED;
	}
	ret= dev_read(fd, (unsigned char *)&cmdtotal, sizeof(cmdtotal));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		return DEV_FILE_READ_FAILED;
	}
	
    log_o(LOG_DEV, "table len = %d, cmdtotal = %d", data_len, cmdtotal);
	
	r_pos = pos + sizeof(data_len) + sizeof(cmdtotal);
    for (i = 0; i < cmdtotal; i++)
    {
		/*read the cmd offset*/
		lseek(fd, (r_pos+i*8), SEEK_SET);
		ret= dev_read(fd, (unsigned char *)&cmdoffset, sizeof(cmdoffset));
		if(0 != ret)
		{
			log_e(LOG_DEV, "read file failed, ret = %d", ret);
			return DEV_FILE_READ_FAILED;
		}
		
		/*read the cmd length*/
		lseek(fd, (r_pos+i*8+4), SEEK_SET);
		ret= dev_read(fd, (unsigned char *)&cmdlength, sizeof(cmdlength));
		if(0 != ret)
		{
			log_e(LOG_DEV, "read file failed, ret = %d", ret);
			return DEV_FILE_READ_FAILED;
		}

		/*read the cmd type*/
		if(cmdoffset != lseek(fd, cmdoffset, SEEK_SET))
		{
			log_e(LOG_DEV, "lseek file failed, cmdoffset = %d", cmdoffset);
			return DEV_FILE_READ_FAILED;
		}
		ret= dev_read(fd, (unsigned char *)&cmdtype, sizeof(cmdtype));
		if(0 != ret)
		{
			log_e(LOG_DEV, "read file failed, ret = %d", ret);
			return DEV_FILE_READ_FAILED;
		}

        log_o(LOG_DEV, "i = %d , cmdoffset=%d, cmdlength=%d, cmdtype=%d",
			i, cmdoffset, cmdlength, cmdtype);

        cmd_table[i] = cmdtype;

		if((MPU_FIRMWARE == cmdtype) && (ver_mid >= 3))
		{
			ret = upg_ctl_fwsave(fd, cmdoffset + sizeof(cmdtype));
		}
		else
		{
			int temp_pos = cmdoffset + sizeof(cmdtype);

			if(cmdlength > sizeof(upg_file_data))
			{
				log_e(LOG_DEV, "this cmd data is too long, length = %d", cmdlength);
				return -1;
			}
			
			if(temp_pos != lseek(fd, temp_pos, SEEK_SET))
			{
				log_e(LOG_DEV, "lseek file failed, temp_pos = %d", temp_pos);
				return DEV_FILE_READ_FAILED;
			}

			ret= dev_read(fd, upg_file_data, cmdlength - sizeof(cmdtype));
			if(0 != ret)
			{
				log_e(LOG_DEV, "read file failed, ret = %d", ret);
				return DEV_FILE_READ_FAILED;
			}

        	ret = upg_ctl_save(cmdtype, upg_file_data);
		}
        if (ret != 0)
        {
            log_e(LOG_DEV, "save file:%d failed", cmdtype);
            return ret;
        }
    }

    ret = dir_update_commit(COM_APP_UPG_DIR);

    if (ret != 0)
    {
        log_e(LOG_DEV, "unlock dir:%s, ret:0x%08x", COM_APP_UPG_DIR, ret);
        return ret;
    }
  //  printf("wxw cmdtotal= %d  cmdtype = %d \r\n",cmdtotal,cmdtype);
    if (1 == cmdtotal)
    {
        if ((MPU_SHELL_FILE == cmdtype) || (MCU_CFG_FILE == cmdtype))
        {
            return 0;
        }
    }
    else if (2 == cmdtotal)
    {
        if (MPU_APP_BIN == cmdtype)
        {
            const char mpu_upgrade_flag_file[]="/home/root/foton/mpu_upgrade_flag_file.txt";
            ret = file_write_atomic(mpu_upgrade_flag_file,(unsigned char *) "report",
                                    strlen("report"), S_IRUSR | S_IWUSR | S_IXUSR);
            if (ret != 0)
            {
                log_e(LOG_DEV, "write mpu upgrade flag failed, ret:0x%08x", ret);
               // return ret;
            }

        }
        if ((MPU_SHELL_FILE == cmd_table[1]) && (MCU_CFG_FILE == cmd_table[0]))
        {
            return 0;
        }
    }

    pm_send_evt(MPU_MID_DEV, PM_EVT_RING);
    upg_set_status( DEV_UPG_BUSY );
	
    /* if there is fota update, priority processing */
    for (i = 0; i < cmdtotal; i++)
    {
        if (MPU_FIRMWARE == cmd_table[i])
        {
            ret = pm_send_evt(MPU_MID_DEV, PM_EVT_FOTA_UPDATE_REQ);
            if( ret !=0 )
            {
                file_delete(fwpath);
                log_e(LOG_DEV, "send event failed,ret:%u", ret);
            }
            return ret;
        }
    }

    return pm_send_evt(MPU_MID_DEV, PM_EVT_RESTART_4G_REQ);

}

/*****************************************************************************
function:     upg_pkg_md5
description:  compute the md5 of the file
input:        const char *path, file path;
output:       unsigned char *md5, the buffer to save md5 value
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int upg_pkg_md5(int fd, int totalsize, unsigned char *md5)
{
    int size, ret, len = 0;
	int readlen;
    MD5_CTX  md5_ctx;
    unsigned char buf[1024] = {0};
	
    /* compute md5 */
    MD5Init(&md5_ctx);

	size = totalsize - 1 - MD5_LENGTH - 1;

	lseek(fd, 1, SEEK_SET);

    while (len < size)
    {
    	readlen = ((size - len) > sizeof(buf)) ? sizeof(buf):(size - len);
        ret = read(fd, buf, readlen);  
        if (ret >= 0)
        {
        	//log_e(LOG_DEV,"ret = %d,  %02x,%02x,%02x,%02x", ret, );
            MD5Update(&md5_ctx, buf, (unsigned int)ret);
            len = len + ret;
        }
        else
        {     
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }
            MD5Final(&md5_ctx);
            log_e(LOG_DEV, "read error:%s", strerror(errno));
            return DEV_FILE_READ_FAILED;
        }
    }

    MD5Final(&md5_ctx);
    memcpy(md5, md5_ctx.digest, MD5_LENGTH);

    return 0;
}


/****************************************************************
 function:     upg_app_start
 description:  when PKG is download,begin to upgrade
 input:        unsigned char *data
               unsigned int len
 output:       none
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int upg_app_start(char *path)
{
    int ret;
	int fd;
	
    char status[16] = {0};
	unsigned char start,end;
	unsigned int  r_pos = 0;
	unsigned int  info_len = 0;
	unsigned int  total_len = 0;
	unsigned char orgmd5[MD5_LENGTH];
	unsigned char clamd5[MD5_LENGTH];
	unsigned char mode;
    unsigned int  mode_len;
	static unsigned char update_status = 0;

	if(1 == update_status)
	{
		log_e(LOG_DEV, "devices is updating");
		return DEV_UPDATING;
	}
	update_status = 1;
	
    mode_len = sizeof(mode);
    ret = st_get(ST_ITEM_PM_MODE, &mode, &mode_len);
	if( ret != 0 )
	{
		log_e(LOG_DEV , "get pm mode failed:%u", ret);
		update_status = 0;
		return ret;
	}

	if( PM_EMERGENCY_MODE == mode ) 
    {
        log_e(LOG_DEV, "TBOX is in emergency mode");
		update_status = 0;
    	return DEV_IN_EMERGENCY;		
	}

    ret = upg_get_startup(status, sizeof(status));
    if (ret != 0)
    {
        log_e(LOG_DEV, "read startup shm failed, ret:%08x", ret);
		update_status = 0;
        return ret;
    }
    else
    {
        if (0 != strncmp(status, "OK", strlen("OK")))
        {
            unsigned char ver[64];
            upg_get_mcu_upg_ver(ver, sizeof(ver));
            log_e(LOG_DEV, "TBOX is self-checking, current:%s", ver);
			update_status = 0;
            return DEV_MCU_UPDATING;
        }
    }
	
	total_len = file_size(path);
	if(total_len < 32)
	{
		log_e(LOG_DEV, "file is too small, size = %d", total_len);
		ret = DEV_FILE_READ_FAILED;
	}
	
	fd = file_open_read(path);
	if( fd < 0 )
	{
		log_e(LOG_DEV, "open PKG file failed, fd = %d", fd);
		update_status = 0;
		return fd;
	}

	/* read the start flag */
	r_pos = 0;
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		ret = DEV_FILE_READ_FAILED;
		goto exit;
	}
	
	ret= dev_read(fd, &start, sizeof(start));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		ret = DEV_FILE_READ_FAILED;
		goto exit;
	}

	/* read the information length */
	r_pos = 1 + UPG_PKG_VER_LEN;
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		ret = DEV_FILE_READ_FAILED;
		goto exit;
	}
	
	ret= dev_read(fd, (unsigned char *)&info_len, sizeof(info_len));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		ret = DEV_FILE_READ_FAILED;
		goto exit;
	}

	/* read original md5 */
	r_pos = total_len- 1 - MD5_LENGTH;
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		ret = DEV_FILE_READ_FAILED;
		goto exit;
	}
	
	ret= dev_read(fd, orgmd5, sizeof(orgmd5));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		ret = DEV_FILE_READ_FAILED;
		goto exit;
	}

	/* read the end */	
	r_pos = total_len - 1;
	if(r_pos != lseek(fd, r_pos, SEEK_SET))
	{
		log_e(LOG_DEV, "lseek file failed, r_pos = %d", r_pos);
		ret = DEV_FILE_READ_FAILED;
		goto exit;
	}
	ret= dev_read(fd, &end, sizeof(end));
	if(0 != ret)
	{
		log_e(LOG_DEV, "read file failed, ret = %d", ret);
		ret = DEV_FILE_READ_FAILED;
		goto exit;
	}
		
    if ((start != 0xAA) || (end != 0x55))
    {
        log_e(LOG_DEV, "The head(0xAA) != %d or The end(0x55) != %d incorrect!", start, end);		
        ret = DEV_DATA_CHECK_FAILED;
		goto exit;
    }

	ret = upg_pkg_md5(fd, total_len, clamd5);
	if(0 != ret)
	{
		log_e(LOG_DEV, "make MD5 errror: %d", ret);
		goto exit;
	}

    if (memcmp(orgmd5, clamd5, MD5_LENGTH))
    {
        log_e(LOG_DEV, "MD5 check SUM incorrect!");
        ret = DEV_DATA_CHECK_FAILED;
		goto exit;
    }
	
	r_pos = 1 + UPG_PKG_VER_LEN + sizeof(info_len) + info_len;
    ret = upg_ctl_proc(fd,r_pos);

exit:
	update_status = 0;
	close(fd);
    return ret;
}

/****************************************************************
 function:     upg_fw_start
 description:  when firmware is download,begin to firmware
 input:        char *path, file path;
 output:       none
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int upg_fw_start(char *path)
{
    int ret;
    char *fota_path = NULL;
	unsigned char mode;
    unsigned int  mode_len;
	char status[16] = {0};

	
	mode_len = sizeof(mode);	
	ret = st_get(ST_ITEM_PM_MODE, &mode, &mode_len);
	if( ret != 0 )
	{
		log_e(LOG_DEV , "get pm mode failed:%u", ret);	
		return ret;
	}

	if( PM_EMERGENCY_MODE == mode ) 
	{
		log_e(LOG_DEV, "TBOX is in emergency mode");	
		return DEV_IN_EMERGENCY;		
	}

	ret = upg_get_startup(status, sizeof(status));

	if (ret != 0)
	{
		log_e(LOG_DEV, "read startup shm failed, ret:%08x", ret);
		return ret;
	}
	else
	{
		if (0 != strncmp(status, "OK", strlen("OK")))
		{
			unsigned char ver[64];
			upg_get_mcu_upg_ver(ver, sizeof(ver));
			log_e(LOG_DEV, "TBOX is self-checking, current:%s", ver);
			return DEV_MCU_UPDATING;
		}
	}

    fota_path = COM_APP_UPG_FW_DIR"/"COM_FW_UPDATE;

    if (!file_exists(path))
    {
        log_e(LOG_DEV, "file is not exist,file:%s", path);
        return DEV_FILE_NOT_EXIST;
    }

    if (dir_exists("/usrdata/cache"))
    {
        ret = dir_remove_path("/usrdata/cache");
        if (ret != 0)
        {
            log_e(LOG_DEV, "remove fota cache dir failed, path:%s, ret:0x%08x",
                  "/usrdata/cache", ret);
        }
    }

    if (0 != strncmp(path, fota_path, strlen(fota_path)))
    {
        ret = file_copy(path, fota_path);

        if (ret != 0)
        {
            file_delete(fota_path);
            file_delete(path);
            log_e(LOG_DEV, "copy file failed,ret:%u", ret);
            return ret;
        }
        
        file_delete(path);
    }

    pm_send_evt( MPU_MID_DEV, PM_EVT_RING );
	upg_set_status( DEV_UPG_BUSY );
    ret = pm_send_evt(MPU_MID_DEV, PM_EVT_FOTA_UPDATE_REQ);

    if (ret != 0)
    {
        file_delete(fota_path);
        log_e(LOG_DEV, "send event failed,ret:%u", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
function:     upg_is_mcu_exist
description:  check mcu app exist
input:        none
output:       none
return:       0 indicates not exist;
              1 indicates exist
*****************************************************************/
int upg_is_mcu_exist(void)
{
    char *app_path = NULL;
    int size;

    app_path = COM_APP_CUR_IMAGE_DIR"/"COM_MCU_APP;

    size = file_size(app_path);

    return (file_exists(app_path) && (size > APP_BIN_LEN));
}

/****************************************************************
function:     upg_set_status
description:  set tbox upgrade status
input:        DEV_UPG_IDLE indicates tbox is not in upgrade status
              DEV_UPG_BUSY indicates tbox is in upgrade status
output:       none
return:       0 indicates upgrade success;
              others indicates upgrade failed
*****************************************************************/
int upg_set_status( DEV_UPG_STATUS status )
{
	unsigned char upg_status = status;
	unsigned int  ilen = sizeof(upg_status);

	st_set(ST_ITEM_UPG_STATUS, &upg_status, ilen);

	return 0;
}
unsigned int read_report_url_file(char *url)
{
    const char url_file[]="/home/root/foton/url_report.txt";
    int ret = 0;
    unsigned int total_len = 0;
    if (!file_exists(url_file))
    {
	    return -1;
    }
    total_len = file_size(url_file);
    if(total_len == 0)
    {
        return -1;
    }
    ret = file_read(url_file, (unsigned char *)url, &total_len);

    if (ret != 0)
    {
	    return -1;
    }
   return 0;

}
static unsigned char mpu_self_upgrade_finish_report_flag = 0;
const char mpu_selfupgrade_flag_file[]="/home/root/foton/mpu_upgrade_flag_file.txt";

void mpu_self_file_exists(void)
{   
    if (!file_exists(mpu_selfupgrade_flag_file))
    { 
        log_e(LOG_DEV, "mpu self report file no exist");
        mpu_self_upgrade_finish_report_flag = 0;
    }
    else
    {
        mpu_self_upgrade_finish_report_flag = 1;
    }
}
extern void hu_cmd_fota_selupgrade_finish(void);

void selfupgrade_mpu_only_report(void)
{
    if (1 == mpu_self_upgrade_finish_report_flag)
    {
        mpu_self_upgrade_finish_report_flag = 0;
        //hu_cmd_fota_selupgrade_finish();//finish report
        file_delete(mpu_selfupgrade_flag_file);
        log_e(LOG_DEV, "repot mpu self upgrade finish ,delete file");
    }
}





