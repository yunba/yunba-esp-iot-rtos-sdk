/*
 * util.c
 *
 *  Created on: Oct 22, 2015
 *      Author: yunba
 */
#include "esp_common.h"
#include "util.h"

LOCAL os_timer_t wifi_monitor_timer;

int8_t init_user_parm(USER_PARM_t *user_parm) {
	uint8_t ret = 0;
	user_parm->appkey = (char *) malloc(54);
	user_parm->deviceid = (char *) malloc(54);
	user_parm->topic = (char *) malloc(54);
	user_parm->alias = (char *) malloc(54);

	if (user_parm->appkey == NULL || user_parm->deviceid == NULL
			|| user_parm->topic == NULL || user_parm->alias == NULL) {
		user_parm_free(user_parm);
		ret = -1;
	}
	return ret;
}

void user_parm_free(USER_PARM_t *user_parm) {
	if (user_parm->appkey == NULL)
		free(user_parm->appkey);
	if (user_parm->deviceid == NULL)
		free(user_parm->deviceid);
	if (user_parm->topic == NULL)
		free(user_parm->topic);
	if (user_parm->alias == NULL)
		free(user_parm->alias);
}

int8_t load_user_parm(USER_PARM_t *user_parm) {
	//TODO: need these parameters to flash.
	strcpy(user_parm->appkey, "5697113d4407a3cd028abead");
	strcpy(user_parm->deviceid, "1234567890");
	strcpy(user_parm->topic, "yunba_smart_plug");
	strcpy(user_parm->alias, "smart_plug_0");
	user_parm->aliveinterval = 30;
	return 0;
}

void setup_wifi_monitor(os_timer_func_t * fn, void *arg, uint32_t msec) {
	os_timer_disarm(&wifi_monitor_timer);
	os_timer_setfn(&wifi_monitor_timer, fn, arg);
	os_timer_arm(&wifi_monitor_timer, msec, true);
}

void wifi_monitor_close(void) {
	os_timer_disarm(&wifi_monitor_timer);
}
