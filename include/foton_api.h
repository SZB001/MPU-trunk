#ifndef __FOTON_API_H__
#define __FOTON_API_H__

extern int ft_set_addr(const char *url, uint16_t port);
extern int ft_set_vin(const char *vin);
extern int ft_set_datintv(uint16_t period);
extern int ft_set_regintv(uint16_t period);
extern int ft_set_timeout(uint16_t timeout);
extern int ft_init(INIT_PHASE phase);
extern int ft_run(void);

#endif