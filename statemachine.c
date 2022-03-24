#include <fw_hal.h>
#include <stdbool.h>
#include "common.h"
#include "statemachine.h"
#include "lcd.h"
#include "input.h"
#include "alarm.h"
#include "relay.h"
#include "fixedmath.h"

#define RAPID_INCREASE_PERIOD 100

__XDATA State current_state = STATE_START;
__XDATA State current_state_delayed = STATE_START;

__XDATA uint32_t countdown_base = 16000;
__XDATA uint32_t strip_base = 8000;
__XDATA uint8_t strip_count = 7;
__XDATA uint8_t strip_increment = 3;
__XDATA uint8_t strip_index = 0;
__XDATA bool f_mode = false;
__XDATA int16_t f_mode_index = 0;
__XDATA uint32_t f_mode_base = 0;

typedef enum { STRIP_INCREMENTAL, STRIP_ABSOLUTE, STRIP_MODE_COUNT } StripMode;
__XDATA StripMode strip_mode = STRIP_INCREMENTAL;

/* Get fixed point representation of exposure increment factor */

static const uint16_t exposure_factors[] =
{
  8192, /* 2 ** (1/1) == 2 */
  5160, /* 2 ** (1/3) */
  4598, /* 2 ** (1/6) */
  4424, /* 2 ** (1/9) */
  4340, /* 2 ** (1/12) */
};

static const uint16_t inv_exposure_factors[] =
{
  2048, /* 1 / 2 ** (1/1) == 2 */
  3251, /* 1 / 2 ** (1/3) */
  3649, /* 1 / 2 ** (1/6) */
  3792, /* 1 / 2 ** (1/9) */
  3866, /* 1 / 2 ** (1/12) */
};

static const char* const exposure_factors_txt[] =
{
  " f1",
  "3rd",
  "6th",
  "9th",
  "12t",
};

/* Compensate base exposure by n/strip_increment stops */

static uint32_t compensate_exposure(uint32_t base, int16_t idx)
{
  if (idx == 0)
  {
    return base;
  }
  
  bool up = (idx > 0);
  uint16_t abs_idx = idx > 0 ? idx : -idx;
  
  const uint16_t* factors = (up ? exposure_factors : inv_exposure_factors);
  uint32_t factor = factors[strip_increment / 3];
  for (int i = 1; i < abs_idx; i++)
  {
    factor = FIXED2UINT(factors[strip_increment / 3] * factor);    
  }
  
  return FIXED2UINT(base * factor);
}

/* round milliseconds to a value that does not hide digits in display */

uint32_t round_milliseconds(uint32_t val)
{
  if (val < 100000)
  {
    /* less than 100s, keep one decimal */
    
    return ((val + 50) / 100) * 100;
  }
  else
  {
    /* more than 99s, keep no decimals */
    
    return (val / 1000) * 1000;
  }
}

void statemachine_enter_delayed_handler(void* arg)
{
  State* state = arg;
  current_state_delayed = *state;
}

uint32_t compute_next_strip_time(void)
{
  uint32_t next_t;
  
  if (strip_index == 0)
  {
    next_t = strip_base;
  }
  else
  {
    next_t = compensate_exposure(strip_base, strip_index);
    if (strip_mode == STRIP_INCREMENTAL)
    {
      next_t -= compensate_exposure(strip_base, strip_index - 1);
    }
  }

  return round_milliseconds(next_t);
}

void statemachine_enter_delayed(uint32_t t, State state)
{
  static State target_state;

  target_state = state;
  alarm_set(t, statemachine_enter_delayed_handler, &target_state);
}

void statemachine_enter(State state)
{
  switch (state)
  {
    case STATE_SPLASH:
      lcd_draw_text("drt");
      statemachine_enter_delayed(1000, STATE_TIMER_READY);
      break;
    case STATE_TIMER_READY:
      relay_set(false);
      alarm_reset();

      lcd_draw_milliseconds(countdown_base);
      break;
    case STATE_TIMER_COUNTDOWN:
      if (current_state == STATE_TIMER_PAUSED)
      {
        alarm_pause(false);
      }
      else
      {
        statemachine_enter_delayed(countdown_base, STATE_TIMER_READY);
      }
      
      relay_set(true);
      break;
    case STATE_TIMER_PAUSED:
      alarm_pause(true);
      relay_set(false);
      break;
    case STATE_TIMER_TIME_MODE_MENU:
    {
      f_mode = !f_mode;
      
      if (f_mode)
      {
        f_mode_index = 0;
        f_mode_base = countdown_base;
      }

      lcd_draw_text(f_mode ? "f t" : "l t");
      statemachine_enter_delayed(500, STATE_TIMER_READY);
      break;
    }
    case STATE_FOCUS:
      relay_set(true);
      lcd_draw_text("foc");
      break;

    case STATE_STRIP_MENU:
      lcd_draw_text("tst");
      break;

    case STATE_STRIP_BASE:
      relay_set(false);
      strip_index = 0;
      lcd_draw_milliseconds(strip_base);
      break;
    case STATE_STRIP_COUNT:
      lcd_draw_number(strip_count);
      break;
    case STATE_STRIP_INCREMENT:
    {
      lcd_draw_text(exposure_factors_txt[strip_increment / 3]);
      break;
    }      
    case STATE_STRIP_MODE:
      lcd_draw_text(strip_mode == STRIP_INCREMENTAL ? "inc" : "abs");
      break;
    case STATE_STRIP_COUNTDOWN:
      statemachine_enter_delayed(compute_next_strip_time(), strip_index == strip_count - 1 ? STATE_STRIP_BASE : STATE_STRIP_WAITING);
      relay_set(true);
      break;
    case STATE_STRIP_WAITING:
      relay_set(false);
      strip_index++;
      lcd_draw_milliseconds(compute_next_strip_time());
      break;

    case STATE_SET_MENU:
      lcd_draw_text("set");
      break;
    case STATE_SET_BRIGHTNESS_MENU:
      lcd_draw_text("brt");
      break;
  }

  current_state_delayed = current_state = state;
}

void statemachine_enter_onbutton(Button button, State state)
{
  if ((input_get_changes() & button) && (input_get() & button))
  {
    statemachine_enter(state);
  }
}

void statemachine_process(void)
{
  uint8_t changed_buttons = input_get_changes();
  uint8_t button_states = input_get();

  if (current_state_delayed != current_state)
  {
    /* handle state changes done within alarm handler */
    
    statemachine_enter(current_state_delayed);
  }

  switch (current_state)
  {
    case STATE_START:
      statemachine_enter(STATE_SPLASH);
      break;
    case STATE_SPLASH:
      /* splash animation */
      break;

    case STATE_TIMER_READY:
    {
      /* start countdown */

      if ((changed_buttons & BUTTON_TRIGGER) && (button_states & BUTTON_TRIGGER))
      {
        statemachine_enter(STATE_TIMER_COUNTDOWN);
      }

      /* increase value */

      if (button_states == BUTTON_PLUS)
      {
        if (changed_buttons == BUTTON_PLUS)
        {
          f_mode_index++;
          countdown_base = round_milliseconds(f_mode ? compensate_exposure(f_mode_base, f_mode_index) : countdown_base + 100);
          lcd_draw_milliseconds(countdown_base);

        }
        else if (!f_mode && input_changed_time() > 1000 && input_changed_time() % RAPID_INCREASE_PERIOD == 0)
        {
          countdown_base = round_milliseconds(countdown_base + (input_changed_time() > 2000 ? 1000 : 100));
          lcd_draw_milliseconds(countdown_base);
        }
      }

      /* decrease value */

      if (button_states == BUTTON_MINUS)
      {
        if (changed_buttons == BUTTON_MINUS)
        {
          f_mode_index--;
          countdown_base = round_milliseconds(f_mode ? compensate_exposure(f_mode_base, f_mode_index) : countdown_base - 100);
          lcd_draw_milliseconds(countdown_base);
        }
        else if (!f_mode && input_changed_time() > 1000 && input_changed_time() % RAPID_INCREASE_PERIOD == 0)
        {
          countdown_base = round_milliseconds(countdown_base - (input_changed_time() > 2000 ? 1000 : 100));
          lcd_draw_milliseconds(countdown_base);
        }
      }

      /* start focus mode */

      if (button_states == BUTTON_CANCEL && input_changed_time() > 1000)
      {
        statemachine_enter(STATE_FOCUS);
      }        

      /* enter test strip menu */

      if (button_states == BUTTON_OK && input_changed_time() > 1000)
      {
        statemachine_enter(STATE_STRIP_MENU);
      }

      if (changed_buttons == BUTTON_OK && button_states == 0)
      {
        statemachine_enter(STATE_TIMER_TIME_MODE_MENU);
      }
      break;
    }
    
    case STATE_TIMER_TIME_MODE_MENU:
      break;
      
    case STATE_TIMER_COUNTDOWN:
    {
      lcd_draw_milliseconds(alarm_get_remaining());

      #if 0
      if ((changed_buttons & BUTTON_TRIGGER) && (button_states & BUTTON_TRIGGER))
      {
        statemachine_enter(STATE_TIMER_PAUSED);
      }
      #endif
      if ((changed_buttons & BUTTON_CANCEL) && (button_states & BUTTON_CANCEL))
      {
        statemachine_enter(STATE_TIMER_READY);
      }
      break;
    }
    #if 0
    case STATE_TIMER_PAUSED:
      if ((changed_buttons & BUTTON_TRIGGER) && (button_states & BUTTON_TRIGGER))
      {
        statemachine_enter(STATE_TIMER_COUNTDOWN);
      }

      if ((changed_buttons & BUTTON_CANCEL) && (button_states & BUTTON_CANCEL))
      {
        statemachine_enter(STATE_TIMER_READY);
      }

      break;
    #endif
    case STATE_FOCUS:
      statemachine_enter_onbutton(BUTTON_CANCEL, STATE_TIMER_READY);
      break;

    case STATE_STRIP_MENU:
      /* go back to main menu */

      statemachine_enter_onbutton(BUTTON_CANCEL, STATE_TIMER_READY);
      
      /* advance to main strip menu */

      statemachine_enter_onbutton(BUTTON_OK, STATE_STRIP_BASE);

      /* advance to set menu */
      
      statemachine_enter_onbutton(BUTTON_PLUS, STATE_SET_MENU);
      statemachine_enter_onbutton(BUTTON_MINUS, STATE_SET_MENU);

      break;

    case STATE_STRIP_BASE:
      /* increase value */

      if (button_states == BUTTON_PLUS)
      {
        if (changed_buttons == BUTTON_PLUS || (input_changed_time() > 1000 && input_changed_time() % RAPID_INCREASE_PERIOD == 0))
        {
          strip_base = round_milliseconds(strip_base + (input_changed_time() > 2000 ? 1000 : 100));
          lcd_draw_milliseconds(strip_base);
        }
      }

      /* decrease value */

      if (button_states == BUTTON_MINUS)
      {
        if (changed_buttons == BUTTON_MINUS || (input_changed_time() > 1000 && input_changed_time() % RAPID_INCREASE_PERIOD == 0))
        {
          strip_base = round_milliseconds(strip_base - (input_changed_time() > 2000 ? 1000 : 100));
          lcd_draw_milliseconds(strip_base);
        }
      }

      /* go back to main menu */

      statemachine_enter_onbutton(BUTTON_CANCEL, STATE_TIMER_READY);

      /* advance to strip count menu */

      statemachine_enter_onbutton(BUTTON_OK, STATE_STRIP_COUNT);

      /* start countdown for current step */

      statemachine_enter_onbutton(BUTTON_TRIGGER, STATE_STRIP_COUNTDOWN);

      break;

    case STATE_STRIP_COUNT:
      /* increase value */

      if ((button_states & BUTTON_PLUS) && (changed_buttons & BUTTON_PLUS))
      {
        strip_count++;
      }

      /* decrease value */

      if ((button_states & BUTTON_MINUS) && (changed_buttons & BUTTON_MINUS))
      {
        if (strip_count > 0) strip_count--;
      }

      lcd_draw_number(strip_count);

      /* go back to main menu */

      statemachine_enter_onbutton(BUTTON_CANCEL, STATE_TIMER_READY);

      /* advance to strip increment menu */

      statemachine_enter_onbutton(BUTTON_OK, STATE_STRIP_INCREMENT);
      
      break;

    case STATE_STRIP_INCREMENT:
      /* increase value */

      if ((button_states & BUTTON_PLUS) && (changed_buttons & BUTTON_PLUS))
      {
        if (strip_increment == 1)
        {
          strip_increment = 3;
        }
        else if (strip_increment < 12)
        {
          strip_increment += 3;
        }
      }

      /* decrease value */

      if ((button_states & BUTTON_MINUS) && (changed_buttons & BUTTON_MINUS))
      {
        if (strip_increment == 3)
        {
          strip_increment = 1;
        }
        else if (strip_increment > 3)
        {
          strip_increment -= 3;
        }        
      }

      lcd_draw_text(exposure_factors_txt[strip_increment / 3]);

      /* go back to main menu */

      statemachine_enter_onbutton(BUTTON_CANCEL, STATE_TIMER_READY);

      /* go to strip mode */

      statemachine_enter_onbutton(BUTTON_OK, STATE_STRIP_MODE);

      break;

    case STATE_STRIP_MODE:
      if (((changed_buttons & BUTTON_PLUS) && (button_states & BUTTON_PLUS)) ||
        ((changed_buttons & BUTTON_MINUS) && (button_states & BUTTON_MINUS)))
      {
        strip_mode = strip_mode == STRIP_INCREMENTAL ? STRIP_ABSOLUTE : STRIP_INCREMENTAL;
        lcd_draw_text(strip_mode == STRIP_INCREMENTAL ? "inc" : "abs");
      }

      /* go back to main menu */

      statemachine_enter_onbutton(BUTTON_CANCEL, STATE_TIMER_READY);

      /* rotate back to base entry */

      statemachine_enter_onbutton(BUTTON_OK, STATE_STRIP_BASE);

      break;

    case STATE_STRIP_COUNTDOWN:
      lcd_draw_milliseconds(alarm_get_remaining());
      break;
    case STATE_STRIP_WAITING:
      /* continue to next step */

      statemachine_enter_onbutton(BUTTON_TRIGGER, STATE_STRIP_COUNTDOWN);

      statemachine_enter_onbutton(BUTTON_CANCEL, STATE_STRIP_BASE);
      break;

    case STATE_SET_MENU:      
      statemachine_enter_onbutton(BUTTON_OK, STATE_SET_BRIGHTNESS_MENU);
      statemachine_enter_onbutton(BUTTON_CANCEL, STATE_TIMER_READY);
      break;

    case STATE_SET_BRIGHTNESS_MENU:
      if ((button_states & BUTTON_PLUS) && (changed_buttons & BUTTON_PLUS))
      {
        lcd_set_brightness(lcd_get_brightness() - 1);
      }
      else if ((button_states & BUTTON_MINUS) && (changed_buttons & BUTTON_MINUS))
      {
        lcd_set_brightness(lcd_get_brightness() + 1);
      }

      statemachine_enter_onbutton(BUTTON_OK, STATE_TIMER_READY);
      statemachine_enter_onbutton(BUTTON_CANCEL, STATE_SET_MENU);
      
      break;
  }
}