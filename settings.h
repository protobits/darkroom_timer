#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <fw_hal.h>

typedef struct {
  uint8_t brightness;
} settings_t;

extern __XDATA settings_t settings;

void load_settings(void);
void save_settings(void);
__XDATA settings_t* get_settings(void);

#endif /* __SETTINGS_H__ */