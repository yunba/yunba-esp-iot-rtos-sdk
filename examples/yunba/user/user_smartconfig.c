#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "smartconfig.h"
#include "user_config.h"
#include "util.h"
#include "mqtt_client.h"

#include "user_light.h"
#include "user_at.h"

LOCAL os_timer_t client_timer;

void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
    printf("smartconfig_done:%d\n", status);
    switch(status) {
        case SC_STATUS_WAIT:
            printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            printf("SC_STATUS_GETTING_SSID_PSWD\n");
            sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = pdata;

	        wifi_station_set_config(sta_conf);
	        wifi_station_disconnect();
	        wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};

                memcpy(phone_ip, (uint8*)pdata, 4);
                printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);

            }
            smartconfig_stop();
#if defined(LIGHT_DEVICE)
            light_set_aim(APP_MAX_PWM, APP_MAX_PWM, APP_MAX_PWM, 0, APP_MAX_PWM, 1000);
#endif
#if defined(AT_MODE)
            setup_at();
#else
            xTaskCreate(yunba_mqtt_client_task,
            		"yunba_mqtt_client_task",
            		384,//configMINIMAL_STACK_SIZE * 7,
            		NULL,
            		2,
            		NULL);
#endif
            break;
    }
}

void smartconfig_task(void *pvParameters)
{
#if defined(LIGHT_DEVICE)
	light_set_aim(APP_MAX_PWM, 0, 0, 0, APP_MAX_PWM, 1000);
#endif
    printf("smartconfig_task start\n");
    smartconfig_start(smartconfig_done);

    vTaskDelete(NULL);
}

LOCAL void check_wifi_conn(void *parm)
{
	uint8_t status = wifi_station_get_connect_status();
	switch (status) {
	case STATION_GOT_IP:
#if defined(AT_MODE)
		setup_at();
#else
		xTaskCreate(yunba_mqtt_client_task,
				"yunba_mqtt_client_task",
				384,//configMINIMAL_STACK_SIZE * 7,
				NULL,
				2,
				NULL);
#endif
        break;

	case STATION_CONNECTING:
	case STATION_IDLE:
	case 255:
		os_timer_arm(&client_timer, 5000, 0);
		break;

	default:
		wifi_set_opmode(STATION_MODE);
		xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL, 10, NULL);
		break;
	}
	printf("wifi conn status:%d\n", status);
}

void setup_wifi(void)
{
    struct station_config *sta_config5 = (struct station_config *)zalloc(sizeof(struct station_config)*5);
    int ret = wifi_station_get_config(sta_config5);
    printf("get wifi config, %s, %s, %d\n", sta_config5->ssid, sta_config5->password, ret);
    wifi_set_opmode(STATION_MODE);
    if(strlen(sta_config5->ssid) != 0) {
        os_timer_disarm(&client_timer);
        os_timer_setfn(&client_timer, (os_timer_func_t *)check_wifi_conn, NULL);
        os_timer_arm(&client_timer, 6000, 0);
    } else
    	xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL, 10, NULL);
    free(sta_config5);
}
