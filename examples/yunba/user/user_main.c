/*
 * user_main.c
 *
 *  Created on: Oct 22, 2015
 *      Author: yunba
 */
#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "util.h"
#if defined(AT_MODE)
#include "user_at.h"
#endif


/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
#if defined(LIGHT_DEVICE)
	user_light_init();
#elif defined(PLUG_DEVICE)
	user_plug_init();
#endif
//	sem_yunba = xSemaphoreCreateMutex();
//	xSemaphoreTake(sem_yunba, portMAX_DELAY);

	setup_wifi();
}

