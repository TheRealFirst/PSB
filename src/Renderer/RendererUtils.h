#pragma once
#include "webgpu/webgpu.h"

void setDefault(WGPULimits& limits);
void setDefault(WGPUBindGroupLayoutEntry& bindingLayout);
void setDefault(WGPUStencilFaceState& stencilFaceState);
void setDefault(WGPUDepthStencilState& depthStencilState);

uint32_t ceilToNextMultiple(uint32_t value, uint32_t step);