#include "RenderWindow.h"
#include "DefaultRenderWindow.h"
#include "SDL2RenderWindow.h"

NAMESPACE_GNXENGINE_BEGIN

std::shared_ptr<GNXEngine::RenderWindow> RenderWindow::Create(const WindowProps& props)
{
	return std::make_shared<SDL2RenderWindow>(props);
	//return std::make_shared<DefaultRenderWindow>(props);
}

NAMESPACE_GNXENGINE_END
