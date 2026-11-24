#pragma once
#include "light_model.h"
#include <esp_err.h>
esp_err_t dmx_output_init();
void dmx_output_set_state(const studio::LightState &state);
void dmx_output_identify(bool enabled);
