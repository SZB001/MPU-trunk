#ifndef __PWDG_H__
#define __PWDG_H__

#define PTHREAD_WDG_INTERVAL    3000


int pwdg_timeout(unsigned short mid);

int pwdg_init(unsigned short mid);

void pwdg_feed(unsigned short mid);

unsigned int pwdg_food(void);


#endif
