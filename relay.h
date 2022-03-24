#ifndef __RELAY_H__
#define __RELAY_H__

#include <stdbool.h>

void relay_set(bool activate);
void relay_set_handler(void* arg);

#endif /* __RELAY_H__ */