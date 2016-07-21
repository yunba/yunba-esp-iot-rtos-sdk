#ifndef CLIENT_H_
#define CLIENT_H_

#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "util.h"

struct MSG_t {
	uint8_t payload[64];
	uint8_t len;
};

extern xQueueHandle QueueMQTTClient;
extern USER_PARM_t parm;

#endif /* CLIENT_H_ */
