#include <fw_hal.h>
#include <stdbool.h>
#include "gpio.h"
#include "common.h"
#include "alarm.h"
#include "lcd.h"
#include "input.h"
#include "statemachine.h"
#include "settings.h"

INTERRUPT(UART1_Routine, EXTI_VectUART1)
{
    if (TI)
    {
        UART1_ClearTxInterrupt();
    }
    if (RI)
    {
        UART1_ClearRxInterrupt();

        if (SBUF == 0x7F)
        {
            /* pulse received, reboot into bootloader */
                    
            IAP_CONTR = 0x60;
        }   
    }
}

INTERRUPT(Timer0_Routine, EXTI_VectTimer0)
{
    alarm_isr();
}

INTERRUPT(Timer1_Routine, EXTI_VectTimer1)
{
    lcd_render();
}

void main(void)
{
    uint32_t prev_sys_time = 0;
    
    /* Configure GPIO_TRIGGER as input */

    GPIO_P5_SetMode(GPIO_Pin_4, GPIO_Mode_Input_HIP);

    lcd_init();

    /* Configure relay (initially off) */

    GPIO_RELAY = 0;
    GPIO_P3_SetMode(GPIO_Pin_2, GPIO_Mode_Output_PP);

    /* UART1, baud 115200, baud source Timer2, 1T mode, interrupt on */

    UART1_Config8bitUart(UART1_BaudSource_Timer2, HAL_State_ON, 115200);
    UART1_SetRxState(HAL_State_ON);
    EXTI_Global_SetIntState(HAL_State_ON);
    EXTI_UART1_SetIntState(HAL_State_ON);

    alarm_init();

    /* Activate interrupts */

    EXTI_Global_SetIntState(HAL_State_ON);

    #if 0
    /* load settings */

    load_settings();
    lcd_set_brightness(get_settings()->brightness);
    #endif

    /* main loop */

    while (1)
    {
        if (prev_sys_time == system_time_ms)
        {
            /* ensure time "advances" every loop */
            continue;
        }
        
        /* process input changes */

        input_update();        

        /* process state machine logic */
        
        statemachine_process();

        prev_sys_time = system_time_ms;
    }
}