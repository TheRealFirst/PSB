#pragma once
#include "webgpu/webgpu.h"

void setDefault(WGPULimits& limits);
void setDefault(WGPUBindGroupLayoutEntry& bindingLayout);

uint32_t ceilToNextMultiple(uint32_t value, uint32_t step);