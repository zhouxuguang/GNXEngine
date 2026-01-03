#include <Windows.h>
#include "Runtime/RenderCore/include/RenderDevice.h"

#include "testSkybox.h"
#include "TestPost/TestPost.hpp"
#include "TestTransform.hpp"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/SceneNode.h"
#include "Runtime/RenderSystem/include/ArcballManipulate.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/RenderSystem/include/SkyBoxNode.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "TestComputeShader.hpp"
#include "Runtime/BaseLib/include/DateTime.h"
#include "Runtime/GNXEngine/include/FrameGraph/TransientResources.h"

class WindowsVulkanView
{
public:
	WindowsVulkanView();
	~WindowsVulkanView();

	bool createWindow(int width, int height);

	bool createRenderDevice();

	void resize(int width, int height);

	void Render();

	void* GetWindow() const
	{
		return mWindow;
	}

	bool hasRenderDevice() const
	{
		return mRenderDevice != nullptr;
	}

private:
	HWND mWindow = nullptr;
	RenderCore::RenderDevicePtr mRenderDevice = nullptr;
	uint32_t mWidth = 0;
	uint32_t mHeight = 0;

	//test
    RCTexturePtr renderTexture = nullptr;
	RCTexturePtr depthStencilTexture = nullptr;
    SceneManager* sceneManager = nullptr;
	RCTexturePtr computeTexture = nullptr;
    ComputePipelinePtr computePipeline = nullptr;
    
    uint64_t lastTime = 0;

	GNXEngine::TransientResources* mTransientResources;

};


