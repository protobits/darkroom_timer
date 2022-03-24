
#include <fw_hal.h>
#include <stdbool.h>
#include "alarm.h"
#include "common.h"

alarm_handler_t alarm_handler = NULL;
void* alarm_handler_arg = NULL;
uint32_t system_time_ms = 0;
uint32_t alarm_t = 0;
bool alarm_paused = false;

void alarm_init(void)
{
    /* Configure systick timer */

    TIM_Timer0_Config(HAL_State_OFF, TIM_TimerMode_16BitAutoNoInt, 1000);
    EXTI_Timer0_SetIntState(HAL_State_ON);
    TIM_Timer0_SetRunState(HAL_State_ON);
}

void alarm_isr(void)
{
    system_time_ms++;

    if (alarm_handler && alarm_t == 0)
    {
        (*alarm_handler)(alarm_handler_arg);
        alarm_handler = NULL;
    }
    else if (!alarm_paused)
    {
        alarm_t--;
    }
}

void alarm_set(uint32_t t, alarm_handler_t alarm, void* arg)
{
    alarm_t = t;    
    alarm_handler_arg = arg;
    alarm_paused = false;
    alarm_handler = alarm;
}

void alarm_pause(bool pause)
{
    alarm_paused = pause;
}

void alarm_reset(void)
{
    alarm_handler = NULL;
    alarm_paused = false;
    alarm_t = 0;
}

uint32_t alarm_get_remaining(void)
{    
    return alarm_t;
}