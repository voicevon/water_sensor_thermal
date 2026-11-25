#pragma once

#include <Arduino.h>
#include "data_cache.h"

void web_config_init();
void web_config_loop();
void web_config_update_sensor(int idx, const SensorDataCache& data);
