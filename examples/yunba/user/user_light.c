#include "esp_common.h"
#include "user_config.h"

#if defined(LIGHT_DEVICE)
#include "user_light.h"

struct light_saved_param light_param;


uint32 pwmio_info[8][3]={ {PWM_0_OUT_IO_MUX,PWM_0_OUT_IO_FUNC,PWM_0_OUT_IO_NUM},
                                  {PWM_1_OUT_IO_MUX,PWM_1_OUT_IO_FUNC,PWM_1_OUT_IO_NUM},
                                  {PWM_2_OUT_IO_MUX,PWM_2_OUT_IO_FUNC,PWM_2_OUT_IO_NUM},
                                  {PWM_3_OUT_IO_MUX,PWM_3_OUT_IO_FUNC,PWM_3_OUT_IO_NUM},
                                  {PWM_4_OUT_IO_MUX,PWM_4_OUT_IO_FUNC,PWM_4_OUT_IO_NUM},
                                  };



uint32
user_light_get_duty(uint8 channel)
{
    return light_param.pwm_duty[channel];
}

/******************************************************************************
 * FunctionName : user_light_set_duty
 * Description  : set each channel's duty params
 * Parameters   : uint8 duty    : 0 ~ PWM_DEPTH
 *                uint8 channel : LIGHT_RED/LIGHT_GREEN/LIGHT_BLUE
 * Returns      : NONE
*******************************************************************************/
void
user_light_set_duty(uint32 duty, uint8 channel)
{
    if (duty != light_param.pwm_duty[channel]) {
        pwm_set_duty(duty, channel);

        light_param.pwm_duty[channel] = pwm_get_duty(channel);
    }
}

/******************************************************************************
 * FunctionName : user_light_get_period
 * Description  : get pwm period
 * Parameters   : NONE
 * Returns      : uint32 : pwm period
*******************************************************************************/
uint32
user_light_get_period(void)
{
    return light_param.pwm_period;
}

/******************************************************************************
 * FunctionName : user_light_set_duty
 * Description  : set pwm frequency
 * Parameters   : uint16 freq : 100hz typically
 * Returns      : NONE
*******************************************************************************/
void
user_light_set_period(uint32 period)
{
    if (period != light_param.pwm_period) {
        pwm_set_period(period);

        light_param.pwm_period = pwm_get_period();
    }
}




void  user_light_init(void)
{
	uint32 pwm_duty_init[PWM_CHANNEL];
	light_param.pwm_period = 1000;
	memset(pwm_duty_init,0,PWM_CHANNEL*sizeof(uint32));
	pwm_init(light_param.pwm_period, pwm_duty_init,PWM_CHANNEL,pwmio_info);


    light_param.pwm_period = 1000;
    light_param.pwm_duty[LIGHT_RED]= APP_MAX_PWM;
    light_param.pwm_duty[LIGHT_GREEN]= APP_MAX_PWM;
    light_param.pwm_duty[LIGHT_BLUE]= APP_MAX_PWM;
    light_param.pwm_duty[LIGHT_COLD_WHITE]= APP_MAX_PWM;
    light_param.pwm_duty[LIGHT_WARM_WHITE]= APP_MAX_PWM;


//    printf("LIGHT P:%d",light_param.pwm_period);
//    printf(" R:%d",light_param.pwm_duty[LIGHT_RED]);
//    printf(" G:%d",light_param.pwm_duty[LIGHT_GREEN]);
//    printf(" B:%d",light_param.pwm_duty[LIGHT_BLUE]);
//    if(PWM_CHANNEL>LIGHT_COLD_WHITE){
//        printf(" CW:%d",light_param.pwm_duty[LIGHT_COLD_WHITE]);
//        printf(" WW:%d\r\n",light_param.pwm_duty[LIGHT_WARM_WHITE]);
//    }else{
//        printf("\r\n");
//    }

    light_set_aim(light_param.pwm_duty[LIGHT_RED],
                    light_param.pwm_duty[LIGHT_GREEN],
                    light_param.pwm_duty[LIGHT_BLUE],
                    light_param.pwm_duty[LIGHT_COLD_WHITE],
                    light_param.pwm_duty[LIGHT_WARM_WHITE],
                    light_param.pwm_period);


    pwm_set_duty(APP_MAX_PWM, LIGHT_RED);

    pwm_set_duty(APP_MAX_PWM, LIGHT_GREEN);

    pwm_set_duty(APP_MAX_PWM, LIGHT_BLUE);

}
#endif
