/*
 * util.h
 *
 *  Created on: Oct 22, 2015
 *      Author: yunba
 */

#ifndef UTIL_H_
#define UTIL_H_


typedef struct {
	char *appkey;
	char *deviceid;
	char *alias;
	char *topic;
	unsigned short aliveinterval;
} USER_PARM_t;


int8_t init_user_parm(USER_PARM_t *user_parm);
int8_t load_user_parm(USER_PARM_t *user_parm);
void user_parm_free(USER_PARM_t *user_parm);

void setup_wifi_monitor(os_timer_func_t * fn, void *arg, uint32_t msec);
void wifi_monitor_close(void);

#endif /* UTIL_H_ */
