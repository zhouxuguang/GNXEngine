#pragma once

#include "Runtime/RenderCore/include/RenderDevice.h"

void InitVisualizationPass(RenderCore::RenderDevicePtr renderDevice);

void ExecuteVisualizationPass(RenderCore::CommandBufferPtr commandBuffer, RenderCore::RCTexture2DPtr visBuffer64, RenderCore::RCTexture2DPtr visualizationBuffer);