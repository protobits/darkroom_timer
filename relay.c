#include "fw_hal.h"
#include "gpio.h"
#include "relay.h"

void relay_set(bool activate)
{
  GPIO_RELAY = activate;
}


void relay_set_handler(void* arg)
{
  bool* activate = arg;
  relay_set(*activate);
}
