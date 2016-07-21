#include "esp_common.h"
#include "ctype.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "user_at.h"
#include "util.h"
#include "uart_task.h"
#include "at.h"
#include "ip_at.h"

#define MAX_CMD_NUM 26
#define CMD_MAX_ARGS 56

xTaskHandle xProcTaskHandle;

uint8_t traceLevel = APP_TRACE_LEVEL_DEFAULT;

const char ATZ_CMD[] = "ATZ";
const char APPKEY_SET_CMD[] = "AT+APPKEY";
const char APPKEY_GET_CMD[] = "AT+APPKEY?";
const char DEVID_SET_CMD[] = "AT+DEVID";
const char DEVID_GET_CMD[] = "AT+DEVID?";
const char GIT_TICK_CMD[] = "AT+GETTICK";
const char GET_REG_CMD[] = "AT+GETREG";
const char MQTT_INIT_CMD[] = "AT+MQTTINIT";
const char MQTT_CONN_CMD[] = "AT+MQTTCONN";
const char MQTT_SETALIAS_CMD[] = "AT+MQTTSETA";
const char MQTT_GETALIAS_CMD[] = "AT+MQTTGETA";
const char MQTT_SUB_CMD[] = "AT+MQTTSUB";
const char MQTT_PUBLISH_CMD[] = "AT+MQTTPUB";
const char MQTT_PUBLISH2_CMD[] = "AT+MQTTPUB2";
const char MQTT_PUBLISH2ALIAS_CMD[] = "AT+MQTTPUB2TOA";
const char MQTT_RECONN_CMD[] = "AT+MQTTRECONN";
const char MQTT_PUBLISH_TO_ALIAS_CMD[] = "AT+MQTTPUBTOA";
const char MQTT_UNSUB_CMD[] = "AT+MQTTUNSUB";
const char MQTT_DISCONN_CMD[] = "AT+MQTTDISCONN";
const char MQTT_GETALIASLIST[] = "AT+MQTTGETAL";
const char MQTT_GETTOPICLIST[] = "AT+MQTTGETTL";
const char CIPSTART_CMD[] = "AT+CIPSTART";
const char CIPSEND_CMD[] = "AT+CIPSEND";
const char CIPCLOSE_CMD[] = "AT+CIPCLOSE";
const char CWSMARTSTART[] = "AT+CWSMARTSTART";
const char CIFSR[] = "AT+CIFSR";

typedef int16_t (*cmdHandler)(uint8_t argc, int8_t *argv[]);

typedef struct {
	const uint8_t* name;
	cmdHandler func;
	const uint8_t argc;
} CMD_HDR_t;

uint8_t uart_rxBuf[MAX_RX_LEN];

static int16_t appkey_set_proc(uint8_t argc, int8_t *argv[]);
static int16_t appkey_get_proc(uint8_t argc, int8_t *argv[]);
static int16_t devid_set_proc(uint8_t argc, int8_t *argv[]);
static int16_t devid_get_proc(uint8_t argc, int8_t *argv[]);
static int16_t get_tick_proc(uint8_t argc, int8_t *argv[]);
static int16_t get_reg_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_init_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_conn_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_sub_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_set_alias_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_get_alias_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_publish_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_reconn_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_publish_to_alias_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_publish2_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_publish2_to_alias_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_unsub_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_disconn_proc(uint8_t argc, int8_t *argv[]);
static int16_t atz_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_get_aliaslist_proc(uint8_t argc, int8_t *argv[]);
static int16_t mqtt_get_topiclist_proc(uint8_t argc, int8_t *argv[]);

static int16_t cip_start_proc(uint8_t argc, int8_t *argv[]);
static int16_t cip_send_proc(uint8_t argc, int8_t *argv[]);
static int16_t cip_close_proc(uint8_t argc, int8_t *argv[]);

static int16_t cw_smart_start_proc(uint8_t argc, int8_t *argv[]);
static int16_t cifsr_proc(uint8_t argc, int8_t *argv[]);

static CMD_HDR_t cmd_tbl[MAX_CMD_NUM] = {
		{ATZ_CMD, atz_proc, 1},
		{APPKEY_SET_CMD, appkey_set_proc, 2},
		{APPKEY_GET_CMD, appkey_get_proc, 1},
		{DEVID_SET_CMD, devid_set_proc, 2},
		{DEVID_GET_CMD, devid_get_proc, 1},
		{GIT_TICK_CMD, get_tick_proc, 1},
		{GET_REG_CMD, get_reg_proc, 1},
		{MQTT_INIT_CMD, mqtt_init_proc, 1},
		{MQTT_CONN_CMD, mqtt_conn_proc, 1},
		{MQTT_SETALIAS_CMD, mqtt_set_alias_proc, 2},
		{MQTT_GETALIAS_CMD, mqtt_get_alias_proc, 1},
		{MQTT_SUB_CMD, mqtt_sub_proc, 2},
		{MQTT_PUBLISH_CMD, mqtt_publish_proc, 3},
		{MQTT_RECONN_CMD, mqtt_reconn_proc, 1},
		{MQTT_PUBLISH_TO_ALIAS_CMD, mqtt_publish_to_alias_proc, 3},
		{MQTT_PUBLISH2_CMD, mqtt_publish2_proc, 3},
		{MQTT_PUBLISH2ALIAS_CMD, mqtt_publish2_to_alias_proc, 3},
		{MQTT_UNSUB_CMD, mqtt_unsub_proc, 2},
		{MQTT_DISCONN_CMD, mqtt_disconn_proc, 1},
		{MQTT_GETALIASLIST, mqtt_get_aliaslist_proc, 2},
		{MQTT_GETTOPICLIST, mqtt_get_topiclist_proc, 2},
		{CIPSTART_CMD, cip_start_proc, 3},
		{CIPSEND_CMD, cip_send_proc, 3},
		{CIPCLOSE_CMD, cip_close_proc, 1},
		{CWSMARTSTART, cw_smart_start_proc, 2},
		{CIFSR, cifsr_proc, 1}
};

enum {
	ERR_OV, ERR
};

UART_RX_MGR_t uart_rx_mgr;
USER_PARM_t parm;

static void print_resp(int8_t *resp) {
	uart0_sendStr(resp);
}

static int16_t appkey_set_proc(uint8_t argc, int8_t *argv[]) {
//	printf("%s, %d, %s\n", __func__, argc, argv[1]);
	strcpy(parm.appkey, (char *) argv[1]);
	save_user_parm(&parm);
	print_resp("OK\r\n");
	return 0;
}

static int16_t appkey_get_proc(uint8_t argc, int8_t *argv[]) {
	char buf[56];
	//TODO:
	sprintf(buf, "+APPKEY:%s\r\nOK\r\n", parm.appkey);
	print_resp(buf);
	return 0;
}

static int16_t devid_set_proc(uint8_t argc, int8_t *argv[]) {
	strcpy(parm.deviceid, argv[1]);
	save_user_parm(&parm);
	print_resp("OK\r\n");
	return 0;
}

static int16_t devid_get_proc(uint8_t argc, int8_t *argv[]) {
	char buf[56];
	//TODO:
	sprintf(buf, "+DEVID:%s\r\nOK\r\n", parm.deviceid);
	print_resp(buf);
	return 0;
}

static int16_t get_tick_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = GET_TICKET;
//	printf("--> %s\n", __func__);
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t get_reg_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = GET_REG;
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_init_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_INIT;
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_conn_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_CONNECT;
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_sub_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_SUB;
	strcpy((char *) msg.payload, (char *) argv[1]);
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_set_alias_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_SET_ALIAS;
	strcpy((char *) msg.payload, (char *) argv[1]);
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_publish_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_PUBLISH;
	strcpy((char *) msg.topic, (char *) argv[1]);
	strcpy((char *) msg.payload, (char *) argv[2]);
	msg.len = strlen((char *) argv[2]) + 1;
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_reconn_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_RECONN;
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_publish_to_alias_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_PUBLISH_ALIAS;
	strcpy((char *) msg.topic, (char *) argv[1]);
	strcpy((char *) msg.payload, (char *) argv[2]);
	msg.len = strlen((char *) argv[2]) + 1;
//	APP_TRACE_INFO(("publishalias...%s, %s, %d\n", msg.topic, msg.payload, msg.len));
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_publish2_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_PUBLISH2;
	strcpy((char *) msg.topic, (char *) argv[1]);
	strcpy((char *) msg.payload, (char *) argv[2]);
	msg.len = strlen((char *) argv[2]) + 1;
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_publish2_to_alias_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_PUBLISH2_ALIAS;
	strcpy((char *) msg.topic, (char *) argv[1]);
	strcpy((char *) msg.payload, (char *) argv[2]);
	msg.len = strlen((char *) argv[2]) + 1;
//	APP_TRACE_INFO(("publish2alias...%s, %d, %s\n", msg.topic, msg.len, msg.payload));
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_unsub_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_UNSUB;
	strcpy((char *) msg.payload, (char *) argv[1]);
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_disconn_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_DISCONN;
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t atz_proc(uint8_t argc, int8_t *argv[]) {
	system_restart();
	return 0;
}

static int16_t mqtt_get_alias_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_GET_ALIAS;
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_get_aliaslist_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_GET_ALIASLIST;
	strcpy((char *) msg.payload, (char *) argv[1]);
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t mqtt_get_topiclist_proc(uint8_t argc, int8_t *argv[]) {
	MQTT_AT_t msg;
	msg.cmd = MQTT_GET_TOPICLIST;
	strcpy((char *) msg.payload, (char *) argv[1]);
	xQueueSend(xQueueMQTT, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t cip_start_proc(uint8_t argc, int8_t *argv[]) {
	IP_AT_t msg;
	msg.cmd = IP_CONNECT_SERVER;
	strcpy((char *) msg.ip_addr, (char *) argv[1]);
	msg.port = atoi((char *) argv[2]);
	xQueueSend(xQueueIP, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t cip_send_proc(uint8_t argc, int8_t *argv[]) {
	uint16_t i = 0;
	IP_AT_t msg;
	msg.cmd = IP_SEND_PAYLOD;
	msg.len = atoi((char *) argv[1]);
	strcpy((char *) msg.payload, (char *) argv[2]);
	APP_TRACE_INFO(("ip send: len:%d, %s\n", msg.len, msg.payload));

	xQueueSend(xQueueIP, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t cip_close_proc(uint8_t argc, int8_t *argv[]) {
	IP_AT_t msg;
	msg.cmd = IP_CLOSE;
	xQueueSend(xQueueIP, &msg, 100 / portTICK_RATE_MS);
	return 0;
}

static int16_t cw_smart_start_proc(uint8_t argc, int8_t *argv[])
{
	//TODO:
	return 0;
}

static int16_t cifsr_proc(uint8_t argc, int8_t *argv[])
{
	int8_t buf[56];
	struct ip_info ip_config;
	bool ret = false;

	ret = wifi_get_ip_info(STATION_IF, &ip_config);
	if (ret) {
		sprintf(buf, "+CIFSR:STAIP,\"%d.%d.%d.%d\"\r\n", IP2STR(&ip_config.ip));
		uart0_sendStr(buf);
		ret = wifi_get_macaddr(STATION_IF, buf);
		if (ret) {
			sprintf(buf, "+CIFSR:STAMAC:\""MACSTR"\"\r\n",
					(uint8_t)buf[0], (uint8_t)buf[1], (uint8_t)buf[2], (uint8_t)buf[3], (uint8_t)buf[4], (uint8_t)buf[5]);
			uart0_sendStr(buf);
		}
		uart0_sendStr("OK\r\n");
	} else {
		uart0_sendStr("ERROR\r\n");
	}
	return 0;
}

static int8_t parse_cmd_line(uint8_t *cmdLine, int8_t *argv[]) {
	uint8_t argc; /* Counts cmd line args */

	for (argc = 0; argc < CMD_MAX_ARGS; argc++) {
		argv[argc] = NULL;
	}

	argc = 0;
	while (argc < CMD_MAX_ARGS) {
		/* Find beginning of argument string - skip white space. */
		while (*cmdLine == ASCII_SPACE_CHAR) {
			cmdLine++;
		}

		if ((*cmdLine == ASCII_NULL_CHAR) || (*cmdLine == ASCII_LF_CHAR)
				|| (*cmdLine == ASCII_CR_CHAR)) { /* end of line or empty field, no more args  */
			*cmdLine = ASCII_NULL_CHAR; /* make sure the line is terminated */
			return (argc);
		}

		argv[argc++] = cmdLine; /* Ptr to argument string */

		/* find end of argument string */
		while ((*cmdLine != ASCII_NULL_CHAR) && (*cmdLine != ASCII_LF_CHAR)
				&& (*cmdLine != ASCII_COMMA_CHAR)
				&& (*cmdLine != ASCII_EQUAL_CHAR) && (*cmdLine != ASCII_CR_CHAR)) {
			cmdLine++;
		}

		if ((*cmdLine == ASCII_NULL_CHAR) || (*cmdLine == ASCII_LF_CHAR)
				|| (*cmdLine == ASCII_CR_CHAR)) { /* end of line, no more args  */
			*cmdLine = ASCII_NULL_CHAR; /* make sure the line is terminated */
			return (argc);
		}

		*cmdLine++ = ASCII_NULL_CHAR; /* terminate current arg   */
	} /* end of while loop */

	return (argc);
}

LOCAL uint16_t get_special_parse_len(int8_t *argv[], uint8_t wo_len) {
	uint8_t i = 0;
	uint8_t len = 0;

	for (; i < wo_len; i++)
		len += strlen((char *) argv[i]) + 1;
	return len ;
}

LOCAL void at_proc_task(void *pvParameters) {
	uint8_t i;
	UART_RX_MGR_t uart_rx_msg;
	int8_t *argv[CMD_MAX_ARGS];
	uint8_t argc;

	init_user_parm(&parm);
	memset(&uart_rx_msg, 0, sizeof(UART_RX_MGR_t));
	for (;;) {
		if (xQueueReceive(xQueueProc, &uart_rx_msg,
				100 / portTICK_RATE_MS) == pdPASS) {
			int8_t cmd_found = 0;
			memset(uart_rxBuf, 0x00, sizeof(uart_rxBuf));
			strcpy(uart_rxBuf, uart_rx_msg.rxBuf);
			argc = parse_cmd_line(uart_rx_msg.rxBuf, argv);
//			APP_TRACE_INFO(("parse...%s, %d, %s\n", cmd, argc, argv[0]));
			if (argc != 0) {
				for (i = 0; i < MAX_CMD_NUM; i++) {
					char *cmd_name = strupr_a((char *) argv[0]);
					if (strcmp((char *) cmd_name, (char *) cmd_tbl[i].name) == 0) {
						//TODO: special character.
						if (argc > cmd_tbl[i].argc && cmd_tbl[i].argc > 1) {
							uint16_t len = get_special_parse_len(argv, (cmd_tbl[i].argc - 1));
							argc = cmd_tbl[i].argc;
							strncpy(argv[cmd_tbl[i].argc - 1], uart_rxBuf + len, strlen(uart_rxBuf) - len);
							APP_TRACE_INFO(("special command process, %s\n", argv[0]));
						}

						if (argc == cmd_tbl[i].argc)
							cmd_found = 1;
						break;
					}
				}

				if (cmd_found) {
					if (cmd_tbl[i].func) {
						cmd_tbl[i].func(argc, argv);
					}
				} else
					print_resp("ERROR\r\n");
			}
		}
	}
	vTaskDelete(NULL);
}

void setup_at_proc_task(void) {
	xQueueProc = xQueueCreate(32, sizeof(UART_RX_MGR_t));
	xTaskCreate(at_proc_task, (uint8 const * )"atTask", 384, NULL,
			tskIDLE_PRIORITY + 2, &xProcTaskHandle);
}

void setup_at(void) {
	setup_uart_task();
	setup_at_proc_task();
	mqtt_at_init();
	ip_at_init();
}
