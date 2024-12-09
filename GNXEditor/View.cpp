#include "View.h"
#include "WindowsVulkanView.h"

void* test()
{
	WindowsVulkanView* winView = new WindowsVulkanView();
	if (!winView->createWindow(2560, 1440))
	{
		return nullptr;
	}
	return winView->GetWindow();;
}