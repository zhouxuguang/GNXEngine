#pragma once

#include "Runtime/RenderCore/include/RenderDevice.h"

void InitHWRasterizePass(RenderCore::RenderDevicePtr renderDevice);

void ExecuteHWRasterizePass(RenderCore::CommandBufferPtr commandBuffer, 
                            RenderCore::RCTexture2DPtr visBuffer64,
                            RenderCore::ComputeBufferPtr clusterPageData,
                            uint32_t width, uint32_t height);
