/**
 * @Title: dsu_shell.c
 * @author yuzhimin
 * @date Nov 27, 2017
 * @version V1.0
 */

#include "dsu_main.h"

extern int disk_stat;

int tmp_max_len[3];

static int dsu_shell_sdsave(int argc, const char **argv)
{
    int ret = 0;

    if (FILE_ST_OPEN == dsu_get_stat(&iwdz_file))
    {
        ret |= dsu_set_opt(&iwdz_file, DSU_FILE_IWDZ, FILE_OPT_CLOSE);
        log_i(LOG_DSU, "close iwdz file");
    }

    if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
    {
        ret |= dsu_set_opt(&inx_file, DSU_FILE_INX, FILE_OPT_CLOSE);
        log_i(LOG_DSU, "close inx file");
    }

    shellprintf(" ok\r\n");

    return ret ? -1 : 0;
}

static int dsu_shell_logmode(unsigned int argc, unsigned char **argv)
{
    unsigned int temp = 0;
    unsigned char mode;
    int ret = 0;

    if (argc != 1)
    {
        shellprintf(" usage: dsusetmode <mode> \r\n");
        shellprintf(" one bit represents a file,BIT0:inx,BIT1:iwdz,BIT2:iwd\r\n");
        return -1;
    }

    sscanf((char const *) argv[0], "%u", &temp);

    mode = (unsigned char)(temp & (DSU_CFG_INX_MASK | DSU_CFG_IWDZ_MASK | DSU_CFG_IWD_MASK));
    ret = cfg_set_para(CFG_ITEM_DSU_CANLOG_MODE, (unsigned char *) &mode, sizeof(unsigned char));

    if (0 != ret)
    {
        shellprintf(" set canlog mode failed\r\n");
        return ret ? -1 : 0;
    }

    shellprintf(" set canlog mode successful 0X%x!\r\n", mode);
    return 0;
}

static int dsu_shell_logtime(int argc, const char **argv)
{
    unsigned int temp = 0;
    unsigned short interval;
    int ret = 0;

    if (argc != 1)
    {
        shellprintf(" usage: dsulogtime <sec> \r\n");
        return -1;
    }

    sscanf((char const *) argv[0], "%u", &temp);
    interval = (unsigned short) temp;
    ret = cfg_set_para(CFG_ITEM_DSU_CANLOG_TIME, (unsigned char *) &interval, sizeof(unsigned short));

    if (0 != ret)
    {
        shellprintf(" set canlog time failed\r\n");
        return ret ? -1 : 0;
    }

    shellprintf(" set canlog time %u successful!\r\n", interval);
    return 0;
}

static int dsu_shell_set_authkey(int argc, const char **argv)
{
    int ret = 0;
    unsigned char authkey[256];

    if (argc != 1)
    {
        shellprintf(" usage: dsusetkey keystring \r\n");
        return -1;
    }

    strncpy((char *) authkey, (char const *) argv[0], sizeof(authkey));
    authkey[sizeof(authkey) - 1] = 0;
    ret = cfg_set_para(CFG_ITEM_DSU_AUTHKEY, (unsigned char *) &authkey, sizeof(authkey));

    if (0 != ret)
    {
        shellprintf(" set keystring failed\r\n");
        return ret ? -1 : 0;
    }

    return 0;
}

static int dsu_shell_clear_authkey(int argc, const char **argv)
{
    int ret = 0;
    unsigned char authkey[256];
    memset(authkey, 0, sizeof(authkey));
    ret = cfg_set_para(CFG_ITEM_DSU_AUTHKEY, (unsigned char *) &authkey, sizeof(authkey));

    if (0 != ret)
    {
        shellprintf(" clear keystring failed\r\n");
        return ret ? -1 : 0;
    }

    return 0;
}

static int dsu_shell_sdhz(int argc, const char **argv)
{
    unsigned int temp = 0;
    unsigned char sdhz;
    int ret = 0;

    if (argc != 1)
    {
        shellprintf(" usage: dsuinxhz <hz>\r\n");
        return -1;
    }

    sscanf((char const *) argv[0], "%u", &temp);

    if ((temp < 1) || (temp > 100))
    {
        shellprintf(" usage: dsuinxhz <hz>,hz must: 1-100 \r\n");
        return -1;
    }

    sdhz = (unsigned char)(temp & 0xFF);
    ret = cfg_set_para(CFG_ITEM_DSU_SDHZ, (unsigned char *) &sdhz, sizeof(unsigned char));

    if (0 != ret)
    {
        shellprintf(" set inxhz failed\r\n");
        return ret ? -1 : 0;
    }

    shellprintf(" set inxhz %u successful!\r\n", sdhz);
    return 0;
}


static int dsu_shell_info(int argc, const char **argv)
{
    if (FILE_ST_OPEN == dsu_get_stat(&iwdz_file))
    {
        shellprintf(" opened file:%s\r\n", iwdz_file.name);
    }

    if (FILE_ST_OPEN == dsu_get_stat(&iwd_file))
    {
        shellprintf(" opened file:%s\r\n", iwd_file.name);
    }

    if (FILE_ST_OPEN == dsu_get_stat(&inx_file))
    {
        shellprintf(" opened file:%s\r\n", inx_file.name);
    }

    shellprintf(" max buf size: inx[%d],iwdz[%d],iwd[%d]\r\n",
                tmp_max_len[0], tmp_max_len[1], tmp_max_len[2]);
    shellprintf(" file stat: inx[%u],iwdz[%u],iwd[%u]\r\n",
                inx_file.stat, iwdz_file.stat, iwd_file.stat);
    shellprintf(" disk stat:%d\r\n", disk_stat);
    return 0;
}

static int dsu_shell_hourfile(unsigned int argc, unsigned char **argv)
{
    unsigned int temp = 0;
    unsigned char mode;
    int ret = 0;

    if (argc != 1)
    {
        shellprintf(" usage: dsuhourfile <mode>\r\n");
        shellprintf(" one bit represents a file,BIT0:inx,BIT1:iwdz\r\n");
        return -1;
    }

    sscanf((char const *) argv[0], "%u", &temp);

    mode = (unsigned char)(temp & (DSU_CFG_INX_MASK | DSU_CFG_IWDZ_MASK));
    ret = cfg_set_para(CFG_ITEM_DSU_HOURFILE, (unsigned char *) &mode, sizeof(unsigned char));

    if (0 != ret)
    {
        shellprintf(" set hourfile mode failed\r\n");
        return ret ? -1 : 0;
    }

    shellprintf(" set hourfile mode successful 0X%x!\r\n", mode);
    return 0;
}

static int dsu_shell_loopfile(int argc, const char **argv)
{
    unsigned int temp = 0;
    unsigned char mode;
    int ret = 0;

    if (argc != 1)
    {
        shellprintf(" usage: dsuloop <on/off> 0:off,1:on\r\n");
        return -1;
    }

    sscanf((char const *) argv[0], "%u", &temp);

    if (temp > 1)
    {
        shellprintf(" usage: dsuloop <on/off> 0:off,1:on\r\n");
        return -1;
    }

    mode = (unsigned char)(temp & 0xFF);
    ret = cfg_set_para(CFG_ITEM_DSU_LOOPFILE, (unsigned char *) &mode, sizeof(unsigned char));

    if (0 != ret)
    {
        shellprintf(" set loop failed\r\n");
        return ret ? -1 : 0;
    }

    shellprintf(" set loop %u successful!\r\n", mode);
    return 0;
}


int dsu_shell_init(void)
{
    int ret = 0;
    ret |= shell_cmd_register_ex("dsusave",     "sdsave",   dsu_shell_sdsave,
                                 "Save the current file.");
    ret |= shell_cmd_register_ex("dsulogtime",  "logtime",  dsu_shell_logtime,
                                 "iwd log time when error happen.");
    ret |= shell_cmd_register_ex("dsusetmode",  "logmode",  dsu_shell_logmode,
                                 "log can data to inx,iwd,iwdz.");
    ret |= shell_cmd_register_ex("dsusetkey",   "setkey",   dsu_shell_set_authkey,
                                 "Set the authkey.");
    ret |= shell_cmd_register_ex("dsuclrkey",   "clearkey", dsu_shell_clear_authkey,
                                 "Clear the authkey.");
    ret |= shell_cmd_register_ex("dsuinxhz",    "sdhz",     dsu_shell_sdhz,
                                 "Set inx file sample frequency.");
    ret |= shell_cmd_register_ex("dsuinfo",     "filestat", dsu_shell_info,
                                 "The state of the storage file.");
    ret |= shell_cmd_register_ex("dsuhourfile", "hourfile", dsu_shell_hourfile,
                                 "Set the file record time length.");
    ret |= shell_cmd_register_ex("dsuloop",     "loopfile", dsu_shell_loopfile,
                                 "Set the file loop overlay.");
    return ret;
}

