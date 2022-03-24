#ifndef __INPUT_H__
#define __INPUT_H__

#include "alarm.h"

typedef enum
{
  BUTTON_TRIGGER = (1 << 0),
  BUTTON_CANCEL = (1 << 1),
  BUTTON_OK = (1 << 2),
  BUTTON_PLUS = (1 << 3),
  BUTTON_MINUS = (1 << 4)
} Button;

void input_sample(void);
void input_update(void);

__XDATA extern uint8_t button_states;
__XDATA extern uint8_t changed_buttons;
__XDATA extern uint32_t last_changed;

inline uint8_t input_get(void)
{
    return button_states;
}

inline uint8_t input_get_changes(void)
{
    return changed_buttons;
}

uint32_t input_changed_time(void);
#endif /* __INPUT_H__ */