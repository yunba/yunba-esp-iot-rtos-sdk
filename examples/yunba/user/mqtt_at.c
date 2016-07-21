/*
 * mqtt_at.c
 *
 *  Created on: Nov 6, 2015
 *      Author: yunba
 */

#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "MQTTClient.h"
#include "util.h"
#include "at.h"
#include "user_config.h"
#include "driver/uart.h"

static uint8_t *sendbuf;
static uint8_t *readbuf;

MQTTClient client;
Network network;
REG_info reg;
MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

char *broker_addr;
uint16_t broker_port;

extern USER_PARM_t parm;

typedef enum {
	ST_INIT,
	ST_GET_TICK,
	ST_BROKER_READY,
	ST_GET_REG,
	ST_REG_READY,
	ST_MQTT_CONN,
	ST_MQTT_SUB,
	ST_MQTT_SET_ALIAS,
	ST_MQTT_PUBLISH,
	ST_MQTT_PUBLISH2,
	ST_MQTT_STANDBY
} ST_MQTT_AT_t;


ST_MQTT_AT_t mqtt_at_state = ST_INIT;

const char GET_TICK_RESP[] = "+GETTICK=";
const char GET_REG_RESP[] = "+GETREG=";
const char MQTT_INIT_RESP[] = "+MQTTINIT=";
const char MQTT_CONN_RESP[] = "+MQTTCONN=";
const char MQTT_SUB_RESP[] = "+MQTTSUB=";
const char MQTT_PUB_RESP[] = "+MQTTPUB=";

LOCAL void printResp(int8_t *resp)
{
	uart0_sendStr(resp);
}

void messageArrived(MessageData* data)
{
	if (data->message->payloadlen > 256 || data->topicName->lenstring.len > 100)
		return;

	uint8_t *buf = (uint8_t *)zalloc(256);
	if (buf != NULL) {
		int8_t *topic = (int8_t *)zalloc(100);

		if (!topic) APP_TRACE_INFO(("can't get mem\n"));

		memset(buf, 0, 256 * sizeof(uint8_t));
		memcpy(buf, data->message->payload, data->message->payloadlen);
		APP_TRACE_INFO(("message arrive:\nmsg: %s\n", buf));
		printResp("+MSG=");
		printResp((int8_t *)buf);
		printResp(",");
		memset(topic, 0, 100);
		memcpy(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
		printResp((int8_t *)topic);
		printResp("\r\n");
		APP_TRACE_INFO(("topic: %s\n", topic));
		free(topic);
		free(buf);
	}
}

static void extMessageArrive(EXTED_CMD cmd, int status, int ret_string_len, char *ret_string)
{
	uint8_t *buf = (uint8_t *)malloc(200);
	if (buf) {
			memset(buf, 0, 200);
			if (ret_string_len <= 200) {
				memcpy(buf, ret_string, ret_string_len);
				APP_TRACE_INFO(("%s, cmd:%d, status:%d, payload: %s\n", __func__, cmd, status, buf));

				switch (cmd) {
				case GET_ALIAS_ACK:
					printResp("+ALIAS=");
					printResp((int8_t *)buf);
					printResp("\r\n");
					break;

				case GET_TOPIC_LIST2_ACK:
					printResp("+TOPICLIST=");
					printResp((int8_t *)buf);
					printResp("\r\n");
					break;

				case GET_ALIASLIST2_ACK:
					printResp("+ALIASLIST=");
					printResp((int8_t *)buf);
					printResp("\r\n");
					break;

				default:
					break;
				}
			}
			free(buf);
	}
}

static int mqtt_reconnet(void)
{
	int rc;
	MQTTDisconnect(&client);
	network.disconnect(&network);
	if ((rc = NetworkConnect(&network, broker_addr, broker_port)) != 0)
		APP_TRACE_INFO(("Return code from network connect is %d\n", rc));
	else {
		APP_TRACE_INFO(("net connect: %d\n", rc));
		rc = MQTTConnect(&client, &connectData, true);
	}
	return rc;
}

static void mqttConnectLost(char *reaseon)
{
	int rc = mqtt_reconnet();
	APP_TRACE_INFO(("mqtt connect lost, reconnect:%d\n", rc));
}

static int get_ip_pair(const char *url, char *addr, uint16_t *port)
{
	char *p = strstr(url, "tcp://");
	if (p) {
		p += 6;
		char *q = strstr(p, ":");
		if (q) {
			int len = strlen(p) - strlen(q);
			if (len > 0) {
				memcpy(addr, p, len);
				//sprintf(addr, "%.*s", len, p);
				*port = atoi(q + 1);
				return SUCCESS;
			}
		}
	}
	return FAILURE;
}



int mqtt_at_setalias(int8_t *alias)
{
	MQTTSetAlias(&client, alias);
}

int mqtt_subscribe(int8_t *topic)
{
	APP_TRACE_INFO(("sub: %s\n", topic));
	return MQTTSubscribe(&client, topic, QOS2, messageArrived);
}

int mqtt_unsubscribe(int8_t *topic)
{
	APP_TRACE_INFO(("unsub: %s\n", topic));
	return MQTTUnsubscribe(&client, topic);
}


int mqtt_connect()
{
	connectData.MQTTVersion = YUNBA_MQTT_VER;
	connectData.clientID.cstring = reg.client_id;
	connectData.username.cstring = reg.username;
	connectData.password.cstring = reg.password;
	connectData.keepAliveInterval = parm.aliveinterval;

	return	MQTTConnect(&client, &connectData, true);
}

int mqtt_publish(int8_t *topic, int8_t *payload, uint16_t len)
{
	MQTTMessage M;
	M.payload = (void *)payload;
	M.qos = QOS2;
	M.payloadlen = (size_t)len;
	APP_TRACE_INFO(("%s, %s, %d\r\n", __func__, payload, len));
	return MQTTPublish(&client, topic, &M);
}

int mqtt_publish_alias(int8_t *alias, int8_t *payload, uint16_t len)
{
	MQTTMessage M;
	M.payload = (void *)payload;
	M.qos = QOS2;
	M.payloadlen = (size_t)len;
	APP_TRACE_INFO(("%s, %s, %d\r\n", __func__, payload, len));
	return MQTTPublishToAlias(&client, alias, payload, len);
}

int mqtt_publish2_alias(int8_t *alias, int8_t *payload, uint16_t len)
{
	int rc;
    cJSON *apn_json, *aps;
    cJSON *Opt = cJSON_CreateObject();
    cJSON_AddStringToObject(Opt,"time_to_live",  "10");

    rc = MQTTPublish2ToAlias(&client, alias, payload, len, Opt);

    cJSON_Delete(Opt);
    return rc;
}

int mqtt_publish2(int8_t *topic, int8_t *payload, uint16_t len)
{
	int rc;
    cJSON *apn_json, *aps;
    cJSON *Opt = cJSON_CreateObject();
    cJSON_AddStringToObject(Opt,"time_to_live",  "10");

    rc = MQTTPublish2(&client, topic, payload, len, Opt);

    cJSON_Delete(Opt);
    return rc;
}

int mqtt_disconnect(void)
{
	MQTTDisconnect(&client);
	network.disconnect(&network);
	return 0;
}

//
LOCAL int mqtt_get_broker()
{
	int rc = FAILURE;
	char *url = (char *)malloc(128);
	if (url) {
		memset(url, 0, 128);
		if (MQTTClient_get_host_v2(parm.appkey, url) == SUCCESS) {
			APP_TRACE_INFO(("--->get broker: %s, %s\n", url, parm.appkey));
			memset(broker_addr, 0, 28 * sizeof(int8_t));
			broker_port = 0;
			get_ip_pair(url, broker_addr, &broker_port);
			APP_TRACE_INFO(("--->broker: %s, %d\n", broker_addr, broker_port));
			 rc = SUCCESS;
		}
	}
	return rc;
}

LOCAL int mqtt_get_broker_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+GETTICK=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_get_reg_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+GETREG=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_init_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTINIT=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_conn_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTCONN=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_sub_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTSUB=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_set_alias_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTSETA=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_get_alias_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTGETA=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_publish_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTPUB=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_publish_alias_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTPUBTOA=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_publish2_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTPUB2=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_publish2_alias_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTPUB2TOA=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_unsub_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTUNSUB=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_disconn_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTDISCONN=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_reconn_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTRECONN=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_get_topiclist_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTGETTL=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL int mqtt_get_aliaslist_resp(int8_t rc)
{
	char buf[56];
	sprintf(buf, "+MQTTGETAL=%d\r\n", rc);
	printResp(buf);
	return 0;
}

LOCAL void mqtt_client_task(void *pvParameters)
{
	int rc;
	MQTT_AT_t msg;
	APP_TRACE_INFO(("---->%s\n", __func__));
	for (;;) {
			if(xQueueReceive(xQueueMQTT, &msg, 100/portTICK_RATE_MS ) == pdPASS) {
				switch (msg.cmd) {

				case GET_REG:
					APP_TRACE_INFO(("%s, %s\n", parm.appkey, parm.deviceid));
					rc = MQTTClient_setup_with_appkey_v2(parm.appkey, parm.deviceid, &reg);
					if (rc == SUCCESS) {
						MQTTSetCallBack(&client, messageArrived, extMessageArrive, mqttConnectLost);
						mqtt_at_state = ST_REG_READY;

						APP_TRACE_INFO(("get reg info: cid:%s, username:%d, password:%s, devid:%s\n",
					    		reg.client_id, reg.username, reg.password, reg.device_id));
					}
					mqtt_get_reg_resp(rc);
					break;

				case GET_TICKET:
					rc = mqtt_get_broker();
					if (rc == SUCCESS) {
						mqtt_at_state = ST_BROKER_READY;
					}
					mqtt_get_broker_resp(rc);
					break;

				case MQTT_INIT:
					APP_TRACE_INFO(("mqtt init: %s, %d\r\n", broker_addr, broker_port));
					rc = NetworkConnect(&network, broker_addr, broker_port);
					if (rc == SUCCESS) {
						APP_TRACE_INFO(("net connect: %d\n", rc));
		    			mqtt_at_state = ST_MQTT_CONN;
		    		}
					mqtt_init_resp(rc);
					break;

				case MQTT_CONNECT:
					if (mqtt_at_state == ST_MQTT_CONN) {
			    		connectData.MQTTVersion = YUNBA_MQTT_VER;
						connectData.clientID.cstring = reg.client_id;
						connectData.username.cstring = reg.username;
						connectData.password.cstring = reg.password;
						connectData.keepAliveInterval = parm.aliveinterval;
						rc = MQTTConnect(&client, &connectData, true);
						if (rc == SUCCESS) {
							APP_TRACE_INFO(("MQTT Connected\n"));
							mqtt_at_state = ST_MQTT_SUB;
						}
						mqtt_conn_resp(rc);
					}
					break;

				case MQTT_SUB:
					APP_TRACE_INFO(("mqtt sub: topic:%s\r\n", msg.payload));
					if (mqtt_at_state == ST_MQTT_SUB || mqtt_at_state == ST_MQTT_STANDBY) {
						rc = mqtt_subscribe(msg.payload);
						if (rc == SUCCESS) {
							mqtt_at_state = ST_MQTT_STANDBY;
						}
						mqtt_sub_resp(rc);
					}
					break;

				case MQTT_UNSUB:
					APP_TRACE_INFO(("unsub, %s\r\n", msg.payload));
					if (mqtt_at_state == ST_MQTT_SUB || mqtt_at_state == ST_MQTT_STANDBY) {
						rc = mqtt_unsubscribe(msg.payload);
						mqtt_unsub_resp(rc);
					}
					break;

				case MQTT_SET_ALIAS:
					APP_TRACE_INFO(("mqtt setalias: alias:%s\r\n", msg.payload));
					if (mqtt_at_state == ST_MQTT_SUB || mqtt_at_state == ST_MQTT_STANDBY) {
						rc = MQTTSetAlias(&client, msg.payload);
						mqtt_set_alias_resp(rc);
					}
					break;

				case MQTT_GET_ALIAS:
					APP_TRACE_INFO(("mqtt get alias\r\n"));
					if (mqtt_at_state == ST_MQTT_SUB || mqtt_at_state == ST_MQTT_STANDBY) {
						rc = MQTTGetAlias(&client, "");
						mqtt_get_alias_resp(rc);
						APP_TRACE_INFO(("get alias ret:%d\r\n", rc));
					}
					break;

				case MQTT_GET_TOPICLIST:
					APP_TRACE_INFO(("mqtt get topiclist\r\n"));
					if (mqtt_at_state == ST_MQTT_SUB || mqtt_at_state == ST_MQTT_STANDBY) {
						rc = MQTTGetTopiclist(&client, msg.payload);
						mqtt_get_topiclist_resp(rc);
						APP_TRACE_INFO(("get topiclist ret:%d\r\n", rc));
					}
					break;

				case MQTT_GET_ALIASLIST:
					APP_TRACE_INFO(("mqtt get aliaslist\r\n"));
					if (mqtt_at_state == ST_MQTT_SUB || mqtt_at_state == ST_MQTT_STANDBY) {
						rc = MQTTGetAliaslist(&client, msg.payload);
						mqtt_get_aliaslist_resp(rc);
						APP_TRACE_INFO(("get aliaslist ret:%d\r\n", rc));
					}
					break;

				case MQTT_PUBLISH:
					APP_TRACE_INFO(("publish: topic:%s, %d\r\n", msg.payload, msg.len));
					if (mqtt_at_state == ST_MQTT_STANDBY) {
						rc = mqtt_publish(msg.topic, msg.payload, msg.len);
						mqtt_publish_resp(rc);
					}
					break;

				case MQTT_RECONN:
					APP_TRACE_INFO(("mqtt disconnect\r\n"));
					if (mqtt_at_state == ST_MQTT_STANDBY || mqtt_at_state == ST_MQTT_SUB) {
						rc = mqtt_reconnet();
						mqtt_reconn_resp(rc);
					}

					break;

				case MQTT_PUBLISH_ALIAS:
					APP_TRACE_INFO(("publish to alias, %s, %s\r\n", msg.topic, msg.payload));
					if (mqtt_at_state == ST_MQTT_STANDBY) {
						rc = mqtt_publish_alias(msg.topic, msg.payload, msg.len);
						mqtt_publish_alias_resp(rc);
					}
					break;

				case MQTT_PUBLISH2:
					APP_TRACE_INFO(("publish2\r\n"));
					if (mqtt_at_state == ST_MQTT_STANDBY) {
						rc = mqtt_publish2(msg.topic, msg.payload, msg.len);
						mqtt_publish2_resp(rc);
					}
					break;

				case MQTT_PUBLISH2_ALIAS:
					APP_TRACE_INFO(("publish2 to alias, %s, %s\r\n", msg.topic, msg.payload));
					if (mqtt_at_state == ST_MQTT_STANDBY) {
						rc = mqtt_publish2_alias(msg.topic, msg.payload, msg.len);
						mqtt_publish2_alias_resp(rc);
					}
					break;

				case MQTT_DISCONN:
					APP_TRACE_INFO(("disconn\r\n"));
					if (mqtt_at_state == ST_MQTT_SUB || mqtt_at_state == ST_MQTT_STANDBY) {
						rc = mqtt_disconnect();
						mqtt_disconn_resp(rc);
					}
					break;

				default:
					break;
				}
				APP_TRACE_INFO(("mqtt task--> cmd:%d, st:%d\n", msg.cmd, mqtt_at_state));
			}
			MQTTYield(&client, 100);
		}

    free(reg.client_id);
    free(reg.device_id);
    free(reg.password);
    free(reg.username);
    free(sendbuf);
    free(readbuf);

    free(broker_addr);
    user_parm_free(&parm);
    vQueueDelete(xQueueMQTT);
	vTaskDelete(NULL);
}

void mqtt_at_init(void)
{
    sendbuf = (uint8_t *)malloc(200);
    readbuf = (uint8_t *)malloc(200);
    NetworkInit(&network);
    MQTTClientInit(&client, &network, 30000, sendbuf, 200 * sizeof(uint8_t), readbuf, 200 * sizeof(uint8_t));

    reg.client_id = (char *)malloc(56);
    reg.device_id = (char *)malloc(56);
    reg.password = (char *)malloc(56);
    reg.username = (char *)malloc(56);

    broker_addr = (char *)malloc(28);

    xQueueMQTT = xQueueCreate(32, sizeof(MQTT_AT_t));

    xTaskCreate(mqtt_client_task,
    		"mqtt_client_task",
    		384,
    		NULL,
    		2,
    		NULL);
}
