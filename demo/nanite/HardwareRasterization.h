#pragma once

#include "Runtime/RenderCore/include/RenderDevice.h"

void InitHWRasterizePass(RenderCore::RenderDevicePtr renderDevice, RenderCore::ComputeBufferPtr cluterPageData, RenderCore::RCTexture2DPtr visBuffer64);

void ExecuteHWRasterizePass(RenderCore::CommandBufferPtr commandBuffer, RenderCore::RCTexture2DPtr visBuffer64, uint32_t width, uint32_t height);