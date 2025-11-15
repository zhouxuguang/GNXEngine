#include "SDL2RenderWindow.h"
#include "SDL_syswm.h"

#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

NAMESPACE_GNXENGINE_BEGIN

extern void* GetSDL2PlatformWindow(const SDL_SysWMinfo& wmInfo);

SDL2RenderWindow::SDL2RenderWindow(const WindowProps& props)
{
    SDL_Init(SDL_INIT_VIDEO);

    mWindow = SDL_CreateWindow(props.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, props.width, props.height, 0);

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(mWindow, &wmInfo);
	
	// 在这里选择底层的渲染器类型，创建它
#if OS_WINDOWS
    HWND nativeWnd = wmInfo.info.win.window;
	mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::VULKAN, nativeWnd);
#elif OS_MACOS
    void * nativeWnd = GetSDL2PlatformWindow(wmInfo);
	mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::METAL, nativeWnd);
#endif

	mData.width = props.width;
	mData.height = props.height;

	mRenderDevice->Resize(props.width, props.height);
	SetVSync(false);
	Init();

	RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();

	//初始化相机
	RenderSystem::CameraPtr cameraPtr = sceneManager->createCamera("MainCamera");
	cameraPtr->LookAt(mathutil::Vector3f(0, 0, 5), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
	cameraPtr->SetLens(60, float(mData.width) / mData.height, 0.1f, 100.f);
}

SDL2RenderWindow::~SDL2RenderWindow()
{

}

bool SDL2RenderWindow::ShouldClose() const
{
    return false;
}

void SDL2RenderWindow::SetVSync(bool enabled)
{

}

void SDL2RenderWindow::SetEventCallback(const EventCallbackFunc& callback)
{

}

void SDL2RenderWindow::Resize(uint32_t width, uint32_t height)
{

}

void SDL2RenderWindow::Shutdown()
{

}

void SDL2RenderWindow::Init()
{

}

NAMESPACE_GNXENGINE_END
