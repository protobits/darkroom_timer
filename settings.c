#include <fw_hal.h>
#include "settings.h"

#define BASE_ADDRESS 0x0000

__XDATA settings_t settings = { 0 };

void load_settings(void)
{
  IAP_SetWaitTime();
  IAP_SetEnabled(HAL_State_ON);

  for (int i = 0; i < sizeof(settings); i++)
  {
    IAP_CmdRead(BASE_ADDRESS + i);
    ((uint8_t*)&settings)[i] = IAP_ReadData();
  }

}

void save_settings(void)
{
  IAP_CmdErase(BASE_ADDRESS);

  for (int i = 0; i < sizeof(settings); i++)
  {
    IAP_WriteData(((uint8_t*)&settings)[i]);
    IAP_CmdWrite(BASE_ADDRESS + i);
  }
}