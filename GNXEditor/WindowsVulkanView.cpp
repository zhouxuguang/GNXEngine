#include "WindowsVulkanView.h"
#include "RenderCore/RenderDevice.h"

//static  LRESULT CALLBACK  wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	if (WM_CREATE == message)
//	{
//		CREATESTRUCT* pSTRUCT = (CREATESTRUCT*)lParam;
//        WindowsVulkanView* pApp = (WindowsVulkanView*)pSTRUCT->lpCreateParams;
//#ifndef GWL_USERDATA
//#define GWL_USERDATA (-21)
//#endif
//		//SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)pApp);
//		//return  pApp->eventProc(hWnd, WM_CREATE, wParam, lParam);
//	}
//	else
//	{
//		//CELLWinApp* pApp = (CELLWinApp*)GetWindowLongPtr(hWnd, GWL_USERDATA);
//		/*if (pApp)
//		{
//			return  pApp->eventProc(hWnd, message, wParam, lParam);
//		}
//		else
//		{
//			return DefWindowProc(hWnd, message, wParam, lParam);
//		}*/
//	}
//
//    return 0;
//}

void OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) {
	int nHeight = HIWORD(lParam);
	int nWidth = LOWORD(lParam);

	MoveWindow(hWnd, 0, 0, nWidth, nHeight, TRUE);
}

static  LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifndef GWL_USERDATA
#define GWL_USERDATA (-21)
#endif

	switch (message)	//根据不同的消息类型进行不同的处理
	{
    case   WM_CREATE:
    {
		CREATESTRUCT* pSTRUCT = (CREATESTRUCT*)lParam;
		WindowsVulkanView* pView = (WindowsVulkanView*)pSTRUCT->lpCreateParams;
		SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)pView);
    }
        break;
	case WM_PAINT:	//若是客户区重绘消息
		//Window_Paint();	//调用窗口绘制函数
		ValidateRect(hWnd, NULL);	//更新客户区的显示，使无效区域变有效
		break;

	case WM_KEYDOWN:	//若是键盘按下消息
		if (wParam == VK_ESCAPE)	//若是ESC键
			DestroyWindow(hWnd);	//摧毁窗口并发送一条WM_DESTROY消息
		break;

	case WM_DESTROY:	//若是窗口摧毁消息
		//Window_CleanUp();	//先调用资源清理函数清理掉预先的资源
		PostQuitMessage(0);	//向系统表明有个线程有终止请求，用来响应WM_DESTROY消息
		break;

	case WM_SIZE:
    {
		WindowsVulkanView* pView = (WindowsVulkanView*)GetWindowLongPtr(hWnd, GWL_USERDATA);
        if (pView && !pView->hasRenderDevice())
        {
            pView->createRenderDevice();
			int nHeight = HIWORD(lParam);
			int nWidth = LOWORD(lParam);
            pView->resize(nWidth, nHeight);
        }
    }
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);	//调用默认窗口过程为应用程序没有处理的窗口消息提供默认的处理
	}
	return 0;
}

WindowsVulkanView::WindowsVulkanView()
{
}

WindowsVulkanView::~WindowsVulkanView()
{
}

/// 创建窗口函数
bool WindowsVulkanView::createWindow(int width, int height)
{
    /// 1.注册窗口类
    /// 2.创建窗口
    /// 3.更新显示

    WNDCLASSEXA wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(nullptr);
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = "GNXEngine";
    wcex.hIconSm = 0;
    RegisterClassExA(&wcex);
    /// 创建窗口
    mWindow = CreateWindowA(
        "GNXEngine"
        , "GNXEngine"
        , WS_OVERLAPPEDWINDOW
        , CW_USEDEFAULT
        , 0
        , CW_USEDEFAULT
        , 0
        , nullptr
        , nullptr
        , GetModuleHandle(nullptr)
        , this);
    if (mWindow == 0)
    {
        return  false;
    }

    ShowWindow(mWindow, SW_SHOW);
    UpdateWindow(mWindow);

    return true;
}

bool WindowsVulkanView::createRenderDevice()
{
	mRenderDevicePtr = RenderCore::createRenderDevice(RenderCore::RenderDeviceType::VULKAN, mWindow);

    return true;
}

void WindowsVulkanView::resize(int width, int height)
{
    mRenderDevicePtr->resize(width, height);
}
