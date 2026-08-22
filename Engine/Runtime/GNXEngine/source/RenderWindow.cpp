#include "RenderWindow.h"

#if GNX_WINDOW_SDL
#include "SDLRenderWindow.h"
#else
#include "GLFWRenderWindow.h"
#endif

NAMESPACE_GNXENGINE_BEGIN

std::shared_ptr<GNXEngine::RenderWindow> RenderWindow::Create(const WindowProps& props)
{
#if GNX_WINDOW_SDL
	return std::make_shared<SDLRenderWindow>(props);
#else
	return std::make_shared<GLFWRenderWindow>(props);
#endif
}

std::shared_ptr<GNXEngine::RenderWindow> RenderWindow::CreateWithExternalWindow(const WindowProps& props, void* externalWindowHandle)
{
#if GNX_WINDOW_SDL
	// SDL 路径暂不支持外部窗口嵌入
	return nullptr;
#else
	return std::make_shared<GLFWRenderWindow>(props, externalWindowHandle);
#endif
}

NAMESPACE_GNXENGINE_END

