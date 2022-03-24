#ifndef __ALARM_H__
#define __ALARM_H__

#include <stdbool.h>

typedef void (*alarm_handler_t)(void* arg);
extern alarm_handler_t alarm_handler;
extern void* alarm_handler_arg;

extern uint32_t system_time_ms;
extern uint32_t alarm_t;

void alarm_init(void);

void alarm_isr(void);

/* Run specified alarm handler after t milliseconds */

void alarm_set(uint32_t t, alarm_handler_t alarm, void* arg);
void alarm_pause(bool pause);
void alarm_reset(void);

uint32_t alarm_get_remaining(void);

inline int alarm_enabled(void) { return (int)alarm_handler; }

#endif /* __ALARM_H__ */