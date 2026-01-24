#include "RenderWindow.h"
#include "DefaultRenderWindow.h"

NAMESPACE_GNXENGINE_BEGIN

std::shared_ptr<GNXEngine::RenderWindow> RenderWindow::Create(const WindowProps& props)
{
	return std::make_shared<DefaultRenderWindow>(props);
}

std::shared_ptr<GNXEngine::RenderWindow> RenderWindow::CreateWithExternalWindow(const WindowProps& props, void* externalWindowHandle)
{
	return std::make_shared<DefaultRenderWindow>(props, externalWindowHandle);
}

NAMESPACE_GNXENGINE_END

