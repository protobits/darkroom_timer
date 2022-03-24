
#include <fw_hal.h>
#include "input.h"
#include "gpio.h"
#include "alarm.h"

uint8_t button_states_now = 0;

__XDATA uint8_t prev_button_states = 0;
__XDATA uint8_t button_states = 0;
__XDATA uint8_t changed_buttons = 0;
__XDATA uint32_t last_changed = 0;
__XDATA uint32_t button_read_t = 0;

void input_sample(void)
{
    uint8_t buttons = 0;
    
    /* Configure shared button inputs as quasi-bidirectional IO for reading */
    
    GPIO_P3_SetMode(GPIO_Pin_4, GPIO_Mode_InOut_QBD);
    GPIO_P1_SetMode(GPIO_Pin_7, GPIO_Mode_InOut_QBD);
    GPIO_P3_SetMode(GPIO_Pin_6, GPIO_Mode_InOut_QBD);
    GPIO_P1_SetMode(GPIO_Pin_1, GPIO_Mode_InOut_QBD);

    /* Force a logic how, to read as active-low inputs */

    GPIO_BUTTON1 = 1;
    GPIO_BUTTON2 = 1;
    GPIO_BUTTON3 = 1;
    GPIO_BUTTON4 = 1;

    /* QBD needs a small delay before it generates the pull-up so we
     * read the trigger before reading any button */

    buttons |= (!GPIO_TRIGGER ? BUTTON_TRIGGER : 0);
    buttons |= (!GPIO_BUTTON1 ? BUTTON_CANCEL : 0);
    buttons |= (!GPIO_BUTTON2 ? BUTTON_OK : 0);
    buttons |= (!GPIO_BUTTON3 ? BUTTON_MINUS : 0);
    buttons |= (!GPIO_BUTTON4 ? BUTTON_PLUS : 0);
    button_states_now = buttons;
    button_read_t = system_time_ms;

    /* Return shared inputs as outputs to drive segments */

    GPIO_P1_SetMode(GPIO_Pin_7, GPIO_Mode_Output_PP);
    GPIO_P3_SetMode(GPIO_Pin_4 | GPIO_Pin_6, GPIO_Mode_Output_PP);
    GPIO_P1_SetMode(GPIO_Pin_1, GPIO_Mode_Output_PP);
}

void input_update(void)
{
    button_states = button_states_now;
    changed_buttons = button_states ^ prev_button_states;
    prev_button_states = button_states;

    /* measure time to any buttons being pressed */
    
    if (changed_buttons & button_states)
    {
        last_changed = button_read_t;
    }
}

uint32_t input_changed_time(void)
{
    return system_time_ms - last_changed; // TODO: handle wraparound
}
