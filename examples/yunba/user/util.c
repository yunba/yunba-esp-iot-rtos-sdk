/*
 * util.c
 *
 *  Created on: Oct 22, 2015
 *      Author: yunba
 */
#include "esp_common.h"
#include "ctype.h"
#include "util.h"

LOCAL os_timer_t wifi_monitor_timer;

LOCAL uint8_t calc_checksum(USER_PARM_t *user_parm);
LOCAL void load_default_user_param(USER_PARM_t *user_parm);

int8_t init_user_parm(USER_PARM_t *user_parm) {
	int8_t ret = 0;
	memset(user_parm, 0, sizeof(USER_PARM_t));

	ret = load_user_parm(user_parm);
	if (!ret) {
		load_default_user_param(user_parm);
	}
	return 0;
}

LOCAL void load_default_user_param(USER_PARM_t *user_parm) {
	strcpy(user_parm->appkey, "55fceaa34a481fa955f3955f");
	strcpy(user_parm->deviceid, "53a4d91ea32c0bbaffc3667bf224b065");
	user_parm->aliveinterval = 30;
}

LOCAL uint8_t calc_checksum(USER_PARM_t *user_parm) {
	char *p = (char*) user_parm;
	int x;
	char r = 0x5a;
	for (x = 1; x < sizeof(USER_PARM_t); x++)
		r += p[x];
	return r;
}

void user_parm_free(USER_PARM_t *user_parm) {
	;
}

bool load_user_parm(USER_PARM_t *user_parm) {
	bool ret = false;
	uint8_t checksum = 0;
	SpiFlashOpResult flash_ret;

	flash_ret = spi_flash_read(
			(PRIV_PARAM_START_SEC + PRIV_YUNBA_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
			(uint32 *) user_parm, sizeof(USER_PARM_t));
	checksum = calc_checksum(user_parm);
	if (checksum == user_parm->checksum)
		ret = true;
	APP_TRACE_INFO(("load parm: appkey: %s, devid: %s\r\n", user_parm->appkey, user_parm->deviceid));
	return ret;
}

bool save_user_parm(USER_PARM_t *user_parm) {
	bool ret = false;
	SpiFlashOpResult flash_ret;
	//TODO:
	user_parm->aliveinterval = 30;
	user_parm->checksum = calc_checksum(user_parm);
	flash_ret = spi_flash_erase_sector(
	PRIV_PARAM_START_SEC + PRIV_YUNBA_PARAM_SAVE);
	if (flash_ret == SPI_FLASH_RESULT_OK) {
		flash_ret = spi_flash_write(
				(PRIV_PARAM_START_SEC + PRIV_YUNBA_PARAM_SAVE)
						* SPI_FLASH_SEC_SIZE, (uint32 *) user_parm,
				sizeof(USER_PARM_t));
		if (flash_ret == SPI_FLASH_RESULT_OK)
			ret = true;
	}
	return ret;
}

void setup_wifi_monitor(os_timer_func_t * fn, void *arg, uint32_t msec) {
	os_timer_disarm(&wifi_monitor_timer);
	os_timer_setfn(&wifi_monitor_timer, fn, arg);
	os_timer_arm(&wifi_monitor_timer, msec, true);
}

void wifi_monitor_close(void) {
	os_timer_disarm(&wifi_monitor_timer);
}

char *strupr_a(char *str) {
	char *p = str;
	for (; *str != '\0'; str++)
		*str = toupper(*str);
	return p;
}
