#ifndef __STATEMACHINE_H__
#define __STATEMACHINE_H__

typedef enum
{
  STATE_START,
  STATE_SPLASH,

  STATE_TIMER_READY,
  STATE_TIMER_COUNTDOWN,
  STATE_TIMER_PAUSED,
  STATE_TIMER_TIME_MODE_MENU,

  STATE_FOCUS,

  STATE_STRIP_MENU,
  STATE_STRIP_BASE,
  STATE_STRIP_COUNT,
  STATE_STRIP_INCREMENT,
  STATE_STRIP_MODE,
  STATE_STRIP_COUNTDOWN,
  STATE_STRIP_WAITING,

  /* TODO:
   * - settings (brightness, delay)
   * - mem save/load
   */
} State;

void statemachine_process(void);

#endif /* __STATEMACHINE_H__ */