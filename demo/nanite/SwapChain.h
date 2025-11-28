#pragma once

#include "Runtime/RenderCore/include/RenderDevice.h"

void InitSwapChainPass(RenderCore::RenderDevicePtr renderDevice);

void ExecuteSwapChainPass(RenderCore::CommandBufferPtr commandBuffer,
                          RenderCore::RenderEncoderPtr renderEncoder,
                          RenderCore::RCTexture2DPtr visBuffer);
