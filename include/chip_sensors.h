#pragma once

#include <stdbool.h>

void chip_sensors_init(void);
bool chip_sensors_read_temp_c(float *out_c);
