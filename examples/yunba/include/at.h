/*
 * File	: at.h
 * This file is part of Espressif's AT+ command set program.
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __AT_H
#define __AT_H

#include "c_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MAX_RX_LEN 128

typedef enum {
	GET_REG,
	GET_TICKET,
//	NET_CONN,
	MQTT_INIT,
	MQTT_CONNECT,
//	MQTT_REG,
	MQTT_SUB,
	MQTT_UNSUB,
	MQTT_SET_ALIAS,
	MQTT_GET_ALIAS,
	MQTT_GET_TOPICLIST,
	MQTT_GET_ALIASLIST,
	MQTT_PUBLISH,
	MQTT_PUBLISH_ALIAS,
	MQTT_PUBLISH2,
	MQTT_PUBLISH2_ALIAS,
	MQTT_DISCONN,
	MQTT_RECONN
} MQTT_AT_CMD;


typedef struct {
	uint8_t payload[64];
	uint8_t topic[51];
	MQTT_AT_CMD cmd;
	uint8_t len;
} MQTT_AT_t;


typedef enum {
	IP_CONNECT_SERVER,
	IP_SEND_PAYLOD,
	IP_CLOSE
} IP_CMD;

typedef struct {
	uint8_t payload[64];
	uint8_t ip_addr[64];
	int port;
	uint8_t len;
	IP_CMD cmd;
} IP_AT_t;

typedef struct {
	uint8_t rxBuf[MAX_RX_LEN];
	uint16_t indx;
} UART_RX_MGR_t;

xQueueHandle xQueueUart;
xQueueHandle xQueueProc;
xQueueHandle xQueueMQTT;
xQueueHandle xQueueIP;

#endif
