#include <stdbool.h>
#include <fw_hal.h>
#include "gpio.h"
#include "input.h"

#define MIN_BRIGHTNESS 0xF

/* segment bit encoding is [ 0 <A> .. <G> ] */

static const uint8_t numeric_segments[] =
{
    0b01111110, /* 0 */
    0b00110000, /* 1 */
    0b01101101, /* 2 */
    0b01111001, /* 3 */
    0b00110011, /* 4 */
    0b01011011, /* 5 */
    0b01011111, /* 6 */
    0b01110000, /* 7 */
    0b01111111, /* 8 */
    0b01111011, /* 9 */
};

enum SegmentSymbols { SYMBOL_DASH };

static const uint8_t symbol_segments[] =
{
    0b00000001, /* - */
};

static const uint8_t letter_segments[] =
{
    0b01110111, /* A */
    0b00011111, /* B */
    0b00001101, /* C */
    0b00111101, /* D */
    0b01001111, /* E */
    0b01000111, /* F */
    0b01011111, /* G */
    0b00010111, /* H */
    0b00110000, /* I */
    0b01111100, /* J */
    0b00000000, /* K */
    0b00001110, /* L */
    0b00000001, /* M */
    0b00010101, /* N */
    0b00011101, /* O */
    0b01100111, /* P */
    0b00000001, /* Q */
    0b00000101, /* R */
    0b01011011, /* S */
    0b00001111, /* T */
    0b00011100, /* U */
};

__XDATA int current_digit = 0;

__XDATA bool update_lcd = true;
__XDATA uint8_t next_segments[3]; /* D1, D2, D3 */
__XDATA uint8_t next_dots = 0; /* 0b<D1><D2><D3> */

__XDATA uint8_t active_segments[3]; /* D1, D2, D3 */
__XDATA uint8_t active_dots = 0; /* 0b<D1><D2><D3> */

static void lcd_render_segments(uint8_t segments)
{
    GPIO_A = (segments & (1 << 6));
    GPIO_B = (segments & (1 << 5));
    GPIO_C = (segments & (1 << 4));
    GPIO_D = (segments & (1 << 3));
    GPIO_E = (segments & (1 << 2));
    GPIO_F = (segments & (1 << 1));
    GPIO_G = (segments & (1 << 0));
}

static void lcd_render_dot(bool dot)
{
    GPIO_DP = dot;
}

void lcd_render(void)
{
    if (update_lcd && current_digit == 0)
    {
        /* copy new data before start of new rendering */
        
        for (int i = 0; i < 3; i++)
        {
            active_segments[i] = next_segments[i];
        }
        
        active_dots = next_dots;

        update_lcd = false;
    }

    /* deactivate previous digit */

    switch (current_digit)
    {
        case 0:
            PWMA_SetPinOutputState(PWM_Pin_3N, HAL_State_OFF);
            GPIO_D3 = 1;
            break;
        case 1:
            PWMA_SetPinOutputState(PWM_Pin_4, HAL_State_OFF);
            GPIO_D1 = 1;
            break;
        case 2:
            PWMA_SetPinOutputState(PWM_Pin_3, HAL_State_OFF);
            GPIO_D2 = 1;
            break;
    }

    /* read inputs now that the digit is off */

    input_sample();

    /* render segments and dot for this digit */

    lcd_render_segments(active_segments[current_digit]);
    lcd_render_dot(active_dots & (1 << (2 - current_digit)));

    /* activate current digit */

    switch (current_digit)
    {
        case 0:
            PWMA_SetPinOutputState(PWM_Pin_4, HAL_State_ON);
            break;
        case 1:
            PWMA_SetPinOutputState(PWM_Pin_3, HAL_State_ON);
            break;
        case 2:
            PWMA_SetPinOutputState(PWM_Pin_3N, HAL_State_ON);
            break;
    }

    current_digit++;
    if (current_digit == 3) current_digit = 0;
}

/* Draw a fractional number formed as <integer>.<decimal>, where decimal is in [0,99] interval */

void lcd_draw_fractional(uint16_t integer, uint16_t decimal)
{
    EXTI_Timer1_SetIntState(false);

    if (integer > 999)
    {
        next_segments[0] = next_segments[1] = next_segments[2] = symbol_segments[SYMBOL_DASH];
        next_dots = 0;
    }
    else if (integer > 99)
    {
        for (int i = 0; i < 3; i++)
        {
            next_segments[2 - i] = numeric_segments[integer % 10];
            integer /= 10;            
        }
        next_dots = 0;
    }
    else if (integer > 9)
    {
        for (int i = 0; i < 2; i++)
        {
            next_segments[1 - i] = numeric_segments[integer % 10];
            integer /= 10;
        }
        next_segments[2] = numeric_segments[decimal / 10];
        next_dots = 0b010;
    }
    else
    {
        next_segments[0] = numeric_segments[integer % 10];
        next_segments[1] = numeric_segments[decimal / 10];
        next_segments[2] = numeric_segments[decimal % 10];
        next_dots = 0b100;
    }

    update_lcd = true;
    EXTI_Timer1_SetIntState(true);
}

void lcd_draw_number(int num)
{
    EXTI_Timer1_SetIntState(false);
    
    next_segments[0] = (num < 100 ? 0 : numeric_segments[num / 100]);
    next_segments[1] = (num < 10 ? 0 : numeric_segments[num / 10]);
    next_segments[2] = numeric_segments[num % 10];
    next_dots = 0;
    
    update_lcd = true;
    EXTI_Timer1_SetIntState(true);
}

void lcd_draw_text(const char* text)
{
    EXTI_Timer1_SetIntState(false);
    for (int i = 0; i < 3; i++)
    {
        if (text[i] >= 'a' && text[i] <= 'u')
        {
            next_segments[i] = letter_segments[text[i] - 'a'];
        }
        else if (text[i] >= '0' && text[i] <= '9')
        {
            next_segments[i] = numeric_segments[text[i] - '0'];
        }
        else if (text[i] == ' ')
        {
            next_segments[i] = 0;
        }
    }
    next_dots = 0;
    update_lcd = true;
    EXTI_Timer1_SetIntState(true);
}

void lcd_draw_milliseconds(uint32_t t)
{
    lcd_draw_fractional(t / 1000, (t % 1000) / 10);
}

void lcd_init(void)
{
    /* Configure segments, digits and relay as outputs */

    lcd_render_segments(0);
    lcd_render_dot(0);

    /* digits off */
    
    GPIO_D1 = 1;
    GPIO_D2 = 1;
    GPIO_D3 = 1;

    GPIO_P1_SetMode(GPIO_Pin_All & ~(GPIO_Pin_1), GPIO_Mode_Output_PP);
    GPIO_P3_SetMode(GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6, GPIO_Mode_Output_PP);

    /* Configure timer for LCD rendering */

    TIM_Timer1_Config(HAL_State_OFF, TIM_TimerMode_16BitAuto, 1000);
    EXTI_Timer1_SetIntState(HAL_State_ON);
    EXTI_Timer1_SetIntPriority(EXTI_IntPriority_High);
    TIM_Timer1_SetRunState(HAL_State_ON);

    /* Configure PWM on digit pins */

    PWMA_PWM3_SetPortState(HAL_State_OFF);
    PWMA_PWM3N_SetPortState(HAL_State_OFF);
    PWMA_PWM4_SetPortState(HAL_State_OFF);
    
    PWMA_PWM3_SetPortDirection(PWMB_PortDirOut);
    PWMA_PWM4_SetPortDirection(PWMB_PortDirOut);
    
    PWMA_PWM3_ConfigOutputMode(PWM_OutputMode_PWM_LowIfLess);
    PWMA_PWM4_ConfigOutputMode(PWM_OutputMode_PWM_LowIfLess);
    
    PWMA_PWM3_SetComparePreload(HAL_State_ON);
    PWMA_PWM4_SetComparePreload(HAL_State_ON);
    
    PWMA_PWM3_SetPortState(HAL_State_ON);
    PWMA_PWM3N_SetPortState(HAL_State_ON);
    PWMA_PWM4_SetPortState(HAL_State_ON);

    PWMA_SetPrescaler(11); // 1Mhz
    PWMA_SetPeriod(0xF);
    PWMA_SetCounterDirection(PWM_CounterDirection_Down);
    PWMA_SetAutoReloadPreload(HAL_State_ON);
    PWMA_SetPinOutputState(PWM_Pin_3 | PWM_Pin_3N | PWM_Pin_4, HAL_State_OFF);

    PWMA_PWM3_SetPort(PWMA_PWM3_AlterPort_P14_P15);
    PWMA_PWM4_SetPort(PWMA_PWM4_AlterPort_P16_P17);
    
    PWMA_SetOverallState(HAL_State_ON);
    PWMA_SetCounterState(HAL_State_ON);
}

uint8_t lcd_get_brightness(void)
{
    uint8_t val;
    
    SFRX_ON();
    val = PWMA_ARRL;
    SFRX_OFF();

    return val;
}

void lcd_set_brightness(uint8_t val)
{
    if (val < MIN_BRIGHTNESS)
    {
        PWMA_SetPeriod((uint16_t)val);
    } 
}