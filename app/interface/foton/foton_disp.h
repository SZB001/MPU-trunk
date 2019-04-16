#ifndef __FOTON_INFO_H__
#define __FOTON_INFO_H__

typedef struct
{
    const char *name;
    int trans;
    const char *(*strfunc)(int);
} ft_disp_t;

static inline const char *str_vehi_type(const int type)
{
    switch (type)
    {
        case FT_VEHITYPE_ELECT:
            return "电动";

        case FT_VEHITYPE_HYBIRD:
            return "混动";

        case FT_VEHITYPE_GASFUEL:
            return "燃油";

        default:
            return "未知";
    }
}

static inline const char *str_vehi_state(const int state)
{
    switch (state)
    {
        case 0x01:
            return "启动";

        case 0x02:
            return "熄火";

        case 0x03:
            return "其他";

        case 0xfe:
            return "异常";

        case 0xff:
            return "无效";

        default:
            return "未知";
    }
}

static inline const char *str_charge_state(const int state)
{
    switch (state)
    {
        case 0x01:
            return "停车充电";

        case 0x02:
            return "行驶充电";

        case 0x03:
            return "未充电";

        case 0x04:
            return "充电完成";

        case 0xfe:
            return "异常";

        case 0xff:
            return "无效";

        default:
            return "未知";
    }
}

static inline const char *str_vehi_mode(const int mode)
{
    switch (mode)
    {
        case 0x01:
            return "纯电";

        case 0x02:
            return "混动";

        case 0x03:
            return "燃油";

        case 0xfe:
            return "异常";

        case 0xff:
            return "无效";

        default:
            return "未知";
    }
}

static inline const char *str_dcdc_state(const int state)
{
    switch (state)
    {
        case 0x01:
            return "工作";

        case 0x02:
            return "断开";

        case 0xfe:
            return "异常";

        case 0xff:
            return "无效";

        default:
            return "未知";
    }
}

static inline const char *str_shift_state(const int state)
{
    static char str[8];

    switch (state)
    {
        case 'R':
        case 'P':
        case 'N':
        case 'D':
        case '1' ... '6':
            sprintf(str, "%c-挡", state);
            return str;

        default:
            return "未知";
    }
}

static inline const char *str_motor_state(const int state)
{
    switch (state)
    {
        case 0x01:
            return "耗电";

        case 0x02:
            return "发电";

        case 0x03:
            return "关闭";

        case 0x04:
            return "准备";

        case 0xfe:
            return "异常";

        case 0xff:
            return "无效";

        default:
            return "未知";
    }
}

static inline const char *str_engin_state(const int state)
{
    switch (state)
    {
        case 0x01:
            return "启动";

        case 0x02:
            return "关闭";

        case 0xfe:
            return "异常";

        case 0xff:
            return "无效";

        default:
            return "未知";
    }
}

static inline void ft_disp_vinf(const ft_vehi_t *vehi)
{
    static const ft_disp_t disp_vinf[] =
    {
        {"车辆状态", member_offset(ft_vehi_t, state_tbl), str_vehi_state},
        {"充电状态", member_offset(ft_vehi_t, charge_tbl), str_charge_state},
        {"车    速", -1, 0},
        {"累计里程", -1, 0},
        {"总 电 压", -1, 0},
        {"总 电 流", -1, 0},
        {"电池 SOC", -1, 0},
        {"DCDC状态", member_offset(ft_vehi_t, dcdc_tbl), str_dcdc_state},
        {"档位状态", member_offset(ft_vehi_t, shift_tbl), str_shift_state},
        {"绝缘电阻", -1, 0},
        {"加速踏板", -1, 0},
        {"制动踏板", -1, 0},
        {"驱动状态", -1, 0},
        {"制动状态", -1, 0},
        {"运行模式", member_offset(ft_vehi_t, mode_tbl), str_vehi_mode},
    };

    int i;

    shellprintf(" |-[车辆类型]:     %s(这不是运行模式)\r\n", str_vehi_type(vehi->vehi_type));

    for (i = 0; i < FT_VINF_MAX; i++)
    {
        const dbc_sig_t *sig;

        shellprintf(" |-[%s]:", disp_vinf[i].name);

        if ((sig = dbc_get_signal_from_id(vehi->info[i])) != NULL)
        {
            shellprintf("     %-40s 总线值: %-16.6lf %-8s", sig->name, sig->value, sig->unit);

            if (disp_vinf[i].trans >= 0)
            {
                uint8_t *trans_tbl = (uint8_t *)vehi + disp_vinf[i].trans;
                uint8_t trans_val = trans_tbl[(uint8_t)(sig->value)];

                shellprintf(" 转换值: %s(0x%02X)", disp_vinf[i].strfunc(trans_val), trans_val);
            }
        }
        else
        {
            shellprintf("     ***未定义***");
        }

        shellprintf("\r\n");
    }

    shellprintf(" |-[转 换 表]\r\n");

    shellprintf(" |  |-[车辆状态]:  总线值      转换值\r\n");

    for (i = 0; i < sizeof(vehi->state_tbl); i++)
    {
        if (vehi->state_tbl[i] != 0)
        {
            shellprintf(" |  |              0x%02X        %s(0x%02X)\r\n",
                        i, str_vehi_state(vehi->state_tbl[i]), vehi->state_tbl[i]);
        }
    }

    shellprintf(" |  |-[充电状态]:  总线值      转换值\r\n");

    for (i = 0; i < sizeof(vehi->charge_tbl); i++)
    {
        if (vehi->charge_tbl[i] != 0)
        {
            shellprintf(" |  |              0x%02X        %s(0x%02X)\r\n",
                        i, str_charge_state(vehi->charge_tbl[i]), vehi->charge_tbl[i]);
        }
    }

    shellprintf(" |  |-[DCDC状态]:  总线值      转换值\r\n");

    for (i = 0; i < sizeof(vehi->dcdc_tbl); i++)
    {
        if (vehi->dcdc_tbl[i] != 0)
        {
            shellprintf(" |  |              0x%02X        %s(0x%02X)\r\n",
                        i, str_dcdc_state(vehi->dcdc_tbl[i]), vehi->dcdc_tbl[i]);
        }
    }

    shellprintf(" |  |-[运行模式]:  总线值      转换值\r\n");

    for (i = 0; i < sizeof(vehi->mode_tbl); i++)
    {
        if (vehi->mode_tbl[i] != 0)
        {
            shellprintf(" |  |              0x%02X        %s(0x%02X)\r\n",
                        i, str_vehi_mode(vehi->mode_tbl[i]), vehi->mode_tbl[i]);
        }
    }

    shellprintf(" |  |-[挡    位]:  总线值      转换值\r\n");

    for (i = 0; i < sizeof(vehi->shift_tbl); i++)
    {
        if (vehi->shift_tbl[i] != 0)
        {
            shellprintf(" |  |              0x%02X        %s\r\n",
                        i, str_shift_state(vehi->shift_tbl[i]));
        }
    }

}


static inline void ft_disp_minf(const ft_motor_t *motor)
{
    static const ft_disp_t disp_minf[] =
    {
        {"电机状态", member_offset(ft_motor_t, state_tbl), str_motor_state},
        {"MCU 温度", -1, 0},
        {"电机转速", -1, 0},
        {"电机转矩", -1, 0},
        {"电机温度", -1, 0},
        {"电机电压", -1, 0},
        {"电机电流", -1, 0},
    };

    int i;

    for (i = 0; i < FT_MINF_MAX; i++)
    {
        const dbc_sig_t *sig;

        shellprintf(" |-[%s]:", disp_minf[i].name);

        if ((sig = dbc_get_signal_from_id(motor->info[i])) != NULL)
        {
            shellprintf("     %-40s 总线值: %-16.6lf %-8s", sig->name, sig->value, sig->unit);

            if (disp_minf[i].trans >= 0)
            {
                uint8_t *trans_tbl = (uint8_t *)motor + disp_minf[i].trans;
                uint8_t trans_val = trans_tbl[(uint8_t)(sig->value)];

                shellprintf(" 转换值: %s(0x%02X)", disp_minf[i].strfunc(trans_val), trans_val);
            }
        }
        else
        {
            shellprintf("     ***未定义***");
        }

        shellprintf("\r\n");
    }

    shellprintf(" |-[转 换 表]\r\n");

    shellprintf(" |  |-[电机状态]:  总线值      转换值\r\n");

    for (i = 0; i < sizeof(motor->state_tbl); i++)
    {
        if (motor->state_tbl[i] != 0)
        {
            shellprintf(" |  |              0x%02X        %s(0x%02X)\r\n",
                        i, str_motor_state(motor->state_tbl[i]), motor->state_tbl[i]);
        }
    }
}

static inline void ft_disp_finf(const ft_fuelcell_t *fuel)
{
    static const ft_disp_t disp_finf[] =
    {
        {"燃料电池电压"           , -1, 0},
        {"燃料电池电流"           , -1, 0},
        {"燃料消耗率"             , -1, 0},            
        {"氢系统最高温度"         , -1, 0},
        {"氢系统最高温度探针代号" , -1, 0},
        {"氢气最高浓度"           , -1, 0},
        {"氢气最高浓度传感器代号" , -1, 0},
        {"氢气最高压力"           , -1, 0},
        {"氢气最高压力传感器代号" , -1, 0},            
        {"高压DC/DC状态", member_offset(ft_fuelcell_t, hvdcdc), str_dcdc_state},
    };
    
    int i;
    const dbc_sig_t *sig;

    for (i = 0; i < FT_FCINF_MAX; i++)
    {
        shellprintf(" |-[%-22s]:", disp_finf[i].name);

        if ((sig = dbc_get_signal_from_id(fuel->info[i])) != NULL)
        {
            shellprintf("     %-40s 总线值: %-16.6lf %-8s", sig->name, sig->value, sig->unit);

            if (disp_finf[i].trans >= 0)
            {
                uint8_t *trans_tbl = (uint8_t *)fuel + disp_finf[i].trans;
                uint8_t trans_val = trans_tbl[(uint8_t)(sig->value)];

                shellprintf(" 转换值: %s(0x%02X)", disp_finf[i].strfunc(trans_val), trans_val);
            }
        }
        else
        {
            shellprintf("     ***未定义***");
        }

        shellprintf("\r\n");
    }

    shellprintf(" |-[温度探针列表]\r\n");
    for (i = 0; i < fuel->temp_cnt; i++)
    {
        const dbc_sig_t *temp;
        temp = dbc_get_signal_from_id(fuel->temp[i]);
        shellprintf(" |-[探针-%d]     %-30s 总线值: %-16.6lf %-8s\r\n", i, temp->name, temp->value, temp->unit);
    }
    
    shellprintf(" |-[转 换 表]\r\n");
    shellprintf(" |  |-[高压DCDC状态]:  总线值      转换值\r\n");

    for (i = 0; i < sizeof(fuel->hvdcdc); i++)
    {
        if (fuel->hvdcdc[i] != 0)
        {
            shellprintf(" |  |                  0x%02X        %s(0x%02X)\r\n",
                        i, str_dcdc_state(fuel->hvdcdc[i]), fuel->hvdcdc[i]);
        }
    }
}

static inline void ft_disp_binf(const ft_batt_t *batt)
{
    int i;
    const dbc_sig_t *sig;

    shellprintf(" |-[电池电压]:");

    if ((sig = dbc_get_signal_from_id(batt->voltage)) != NULL)
    {
        shellprintf("     %-40s 总线值: %-16.6lf %-8s\r\n", sig->name, sig->value, sig->unit);
    }
    else
    {
        shellprintf("     ***未定义***\r\n");
    }

    shellprintf(" |-[电池电流]:");

    if ((sig = dbc_get_signal_from_id(batt->current)) != NULL)
    {
        shellprintf("     %-40s 总线值: %-16.6lf %-8s\r\n", sig->name, sig->value, sig->unit);
    }
    else
    {
        shellprintf("     ***未定义***\r\n");
    }

    shellprintf(" |-[单体信息]  (总共%d个)\r\n", batt->cell_cnt);

    for (i = 0; i < batt->cell_cnt; i++)
    {
        shellprintf(" |  |-[单体-%03d]:", i + 1);

        if ((sig = dbc_get_signal_from_id(batt->cell[i])) != NULL)
        {
            shellprintf("  %-40s 总线值: %-16.6lf %-8s\r\n", sig->name, sig->value, sig->unit);
        }
        else
        {
            shellprintf("  ***未定义***\r\n");
        }
    }

    shellprintf(" |-[温度信息]  (总共%d个)\r\n", batt->temp_cnt);

    for (i = 0; i < batt->temp_cnt; i++)
    {
        shellprintf(" |  |-[温度-%03d]:", i + 1);

        if ((sig = dbc_get_signal_from_id(batt->temp[i])) != NULL)
        {
            shellprintf("  %-40s 总线值: %-16.6lf %-8s\r\n", sig->name, sig->value, sig->unit);
        }
        else
        {
            shellprintf("  ***未定义***\r\n");
        }
    }
}

static inline void ft_disp_xinf(const int *extr)
{
    int i;

    static const char *xstr[] =
    {
        "最高单体包号",
        "最高单体序号",
        "最高单体电压",
        "最低单体包号",
        "最低单体序号",
        "最低单体电压",
        "最高温度包号",
        "最高温度序号",
        "最高电池温度",
        "最低温度包号",
        "最低温度序号",
        "最低电池温度",
    };

    for (i = 0; i < FT_XINF_MAX; i++)
    {
        const dbc_sig_t *sig;

        shellprintf(" |-[%s]:", xstr[i]);

        if ((sig = dbc_get_signal_from_id(extr[i])) != NULL)
        {
            shellprintf(" %-40s 总线值: %-16.6lf %-8s\r\n", sig->name, sig->value, sig->unit);
        }
        else
        {
            shellprintf(" ***未定义***\r\n");
        }
    }
}

static inline void ft_disp_einf(const ft_engin_t *engin)
{
    static const ft_disp_t disp_einf[] =
    {
        {"引擎状态", member_offset(ft_engin_t, state_tbl), str_engin_state},
        {"曲轴转速", -1, 0},
        {"燃油消耗", -1, 0},
    };

    int i;

    for (i = 0; i < FT_EINF_MAX; i++)
    {
        const dbc_sig_t *sig;

        shellprintf(" |-[%s]:", disp_einf[i].name);

        if ((sig = dbc_get_signal_from_id(engin->info[i])) != NULL)
        {
            shellprintf("     %-40s 总线值: %-16.6lf %-8s", sig->name, sig->value, sig->unit);

            if (disp_einf[i].trans >= 0)
            {
                uint8_t *trans_tbl = (uint8_t *)engin + disp_einf[i].trans;
                uint8_t trans_val = trans_tbl[(uint8_t)(sig->value)];

                shellprintf(" 转换值: %s(0x%02X)", disp_einf[i].strfunc(trans_val), trans_val);
            }
        }
        else
        {
            shellprintf("     ***未定义***");
        }

        shellprintf("\r\n");
    }

    shellprintf(" |-[转 换 表]\r\n");

    shellprintf(" |  |-[引擎状态]:  总线值      转换值\r\n");

    for (i = 0; i < sizeof(engin->state_tbl); i++)
    {
        if (engin->state_tbl[i] != 0)
        {
            shellprintf(" |  |              0x%02X        %s(0x%02X)\r\n",
                        i, str_engin_state(engin->state_tbl[i]), engin->state_tbl[i]);
        }
    }
}

static inline void ft_disp_winf(const int *warn)
{
    int i;

    static const char *wstr[] =
    {
        "温度差异报警",
        "电池高温报警",
        "电池过压报警",
        "电池欠压报警",
        "SOC 过低报警",
        "单体过压报警",
        "单体欠压报警",
        "SOC 过高报警",
        "SOC 跳变报警",
        "电池匹配报警",
        "单体差异报警",
        "绝缘异常报警",
        "DCDC温度报警",
        "制动系统报警",
        "DCDC状态报警",
        "MCU 温度报警",
        "高压互锁报警",
        "电机温度报警",
        "电池过充报警",
    };

    for (i = 0; i < 19; i++)
    {
        const dbc_sig_t *sig;

        shellprintf(" |-[%s]:", wstr[i]);

        if ((sig = dbc_get_signal_from_id(warn[i])) != NULL)
        {
            shellprintf(" %-40s 总线值: %-16.6lf %-8s\r\n", sig->name, sig->value, sig->unit);
        }
        else
        {
            shellprintf(" ***未定义***\r\n");
        }
    }
}

#endif
