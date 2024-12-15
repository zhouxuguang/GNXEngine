#include <Windows.h>
#include "RenderCore/RenderDevice.h"

#include "testSkybox.h"
#include "TestPost/TestPost.hpp"
#include "TestTransform.hpp"
#include "RenderSystem/SceneManager.h"
#include "RenderSystem/SceneNode.h"
#include "RenderSystem/ArcballManipulate.h"
#include "MathUtil/Vector3.h"
#include "RenderSystem/SkyBoxNode.h"
#include "ImageCodec/ImageDecoder.h"
#include "RenderSystem/RenderEngine.h"
#include "TestComputeShader.hpp"
#include "BaseLib/DateTime.h"

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
    RenderTexturePtr renderTexture = nullptr;
    RenderTexturePtr depthStencilTexture = nullptr;
    SceneManager* sceneManager = nullptr;
    RenderTexturePtr computeTexture = nullptr;
    ComputePipelinePtr computePipeline = nullptr;
    
    uint64_t lastTime = 0;

};


