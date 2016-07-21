/*
 * uart_task.c
 *
 *  Created on: Nov 6, 2015
 *      Author: yunba
 */
#include "esp_common.h"
#include "ctype.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "user_at.h"
#include "util.h"
#include "at.h"

xTaskHandle xUartTaskHandle;

LOCAL void uart_task(void *pvParameters)
{
	uint8_t error;
    os_event_t e;
    UART_RX_MGR_t uart_rx_mgr;

    memset(&uart_rx_mgr, 0, sizeof(UART_RX_MGR_t));

    for (;;) {
        if (xQueueReceive(xQueueUart, (void *)&e, (portTickType)portMAX_DELAY)) {
            switch (e.event) {
                case UART_EVENT_RX_CHAR:
					if (uart_rx_mgr.indx >= MAX_RX_LEN) {
						uart_rx_mgr.indx = 0;
					} else {
						uart_rx_mgr.rxBuf[uart_rx_mgr.indx++] = e.param;
						if (e.param == 0x0a) {
							uart_rx_mgr.rxBuf[uart_rx_mgr.indx - 1] = 0x00;
							uart_rx_mgr.indx = 0;
							if (memcmp(uart_rx_mgr.rxBuf, "at", 2) == 0 ||
								memcmp(uart_rx_mgr.rxBuf, "AT", 2) ==0) {
								xQueueSend(xQueueProc, uart_rx_mgr.rxBuf, 100 / portTICK_RATE_MS);
							}
						}
					}
                    break;

                default:
                    break;
            }
        }
    }

    vTaskDelete(NULL);
}

void setup_uart_task(void)
{
	xQueueUart = xQueueCreate(32, sizeof(os_event_t));
	UART_init(xQueueUart);
	xTaskCreate(uart_task, (uint8 const *)"uTask", 256, NULL, tskIDLE_PRIORITY + 2, &xUartTaskHandle);
}
