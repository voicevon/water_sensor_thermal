#include "data_cache.h"

static SensorDataCache s_sensor_cache[SENSOR_COUNT] = {};

void data_cache_update_sensor(int idx, const SensorDataCache& data) {
    if (idx < 0 || idx >= SENSOR_COUNT) return;
    s_sensor_cache[idx] = data;
}

const SensorDataCache& data_cache_get_sensor(int idx) {
    if (idx < 0 || idx >= SENSOR_COUNT) idx = 0;
    return s_sensor_cache[idx];
}
