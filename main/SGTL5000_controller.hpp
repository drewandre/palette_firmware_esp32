#ifndef __SGTL5000_CONTROLLER_H__
#define __SGTL5000_CONTROLLER_H__

#include <math.h>
#include "esp_log.h"
#include "audiobit.h"
#include "driver/i2s.h"

void init_sgtl5000_controller();

void deinit_sgtl5000_controller();

#endif /* __SGTL5000_CONTROLLER_H__*/
