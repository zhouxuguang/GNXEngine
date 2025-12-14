#pragma once

#include "Runtime/RenderCore/include/RenderDevice.h"

void InitRasterClearPass(RenderCore::RenderDevicePtr renderDevice);

void ExecuteRasterClearPass(RenderCore::CommandBufferPtr commandBuffer,
                            RenderCore::ComputeBufferPtr queueState,
                            RenderCore::ComputeBufferPtr workArgs0,
                            RenderCore::ComputeBufferPtr workArgs1,
                            RenderCore::RCTexture2DPtr visBuffer64);
