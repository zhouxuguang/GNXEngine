#pragma once

#include "Runtime/RenderCore/include/RenderDevice.h"

void InitRasterClearPass(RenderCore::RenderDevicePtr renderDevice);

void ExecuteRasterClearPass(RenderCore::CommandBufferPtr commandBuffer,
                            RenderCore::RCBufferPtr queueState,
                            RenderCore::RCBufferPtr workArgs0,
                            RenderCore::RCBufferPtr workArgs1,
                            RenderCore::RCTexture2DPtr visBuffer64);
