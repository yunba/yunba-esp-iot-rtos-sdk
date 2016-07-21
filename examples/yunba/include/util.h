/*
 * util.h
 *
 *  Created on: Oct 22, 2015
 *      Author: yunba
 */

#ifndef UTIL_H_
#define UTIL_H_

#define PRIV_PARAM_START_SEC        0x3c//0x7C
#define PRIV_PARAM_SAVE     0
#define PRIV_YUNBA_PARAM_SAVE  128

/* Special ASCII character */
#define ASCII_NULL_CHAR         0x00
#define ASCII_BS_CHAR           0x08
#define ASCII_LF_CHAR           0x0a
#define ASCII_CR_CHAR           0x0d
#define ASCII_SPACE_CHAR        0x20
#define ASCII_DOT_CHAR          0x2e
#define ASCII_0_CHAR            0x30
#define ASCII_9_CHAR            0x39
#define ASCII_SEMICOLON_CHAR    0x3b
#define ASCII_A_CHAR            0x41
#define ASCII_B_CHAR            0x42
#define ASCII_C_CHAR            0x43
#define ASCII_X_CHAR            0x58
#define ASCII_Y_CHAR            0x59
#define ASCII_Z_CHAR            0x5a
#define ASCII_TILDE_CHAR        0x7e
#define ASCII_EQUAL_CHAR        0x3d
#define ASCII_COMMA_CHAR        0x2c

#define MAX_APPKEY_LEN 54
#define MAX_DEVID_LEN 54
#define MAX_ALIAS_LEN 54
#define MAX_TOPIC_LEN 54

typedef struct {
	uint8_t checksum;
	char appkey[MAX_APPKEY_LEN];
	char deviceid[MAX_DEVID_LEN];
//	char alias[MAX_ALIAS_LEN];
//	char topic[MAX_TOPIC_LEN];
	uint16_t aliveinterval;
} USER_PARM_t;

int8_t init_user_parm(USER_PARM_t *user_parm);
bool load_user_parm(USER_PARM_t *user_parm);
bool save_user_parm(USER_PARM_t *user_parm);
void user_parm_free(USER_PARM_t *user_parm);

void setup_wifi_monitor(os_timer_func_t * fn, void *arg, uint32_t msec);
void wifi_monitor_close(void);
char *strupr_a(char *str);

/*
 *
 *
 */
extern uint8_t traceLevel;

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF            0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO           1
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG            2
#endif

#ifndef  APP_TRACE_LEVEL
#define  APP_TRACE_LEVEL            TRACE_LEVEL_OFF
#endif

#ifndef  APP_TRACE
#define  APP_TRACE                  printf
#endif

#define  APP_TRACE_INFO(x)          ((traceLevel >= TRACE_LEVEL_INFO) ? (void)(APP_TRACE x) : (void)0)
#define  APP_TRACE_DBG(x)           ((traceLevel >= TRACE_LEVEL_DBG)  ? (void)(APP_TRACE x) : (void)0)

#define  APP_TRACE_LEVEL_DEFAULT        TRACE_LEVEL_DBG

#endif /* UTIL_H_ */
