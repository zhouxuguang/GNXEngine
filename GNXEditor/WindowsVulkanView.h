#include <Windows.h>
#include "RenderCore/RenderDevice.h"

class WindowsVulkanView
{
public:
	WindowsVulkanView();
	~WindowsVulkanView();

	bool createWindow(int width, int height);

	bool createRenderDevice();

	void resize(int width, int height);

	void* GetWindow() const
	{
		return mWindow;
	}

	bool hasRenderDevice() const
	{
		return mRenderDevicePtr != nullptr;
	}

private:
	HWND mWindow = nullptr;
	RenderCore::RenderDevicePtr mRenderDevicePtr = nullptr;

};


