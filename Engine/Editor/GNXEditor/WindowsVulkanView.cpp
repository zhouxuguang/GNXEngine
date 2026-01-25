#include "WindowsVulkanView.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderSystem/include/FrameGraph/FrameGraph.h"
#include "Runtime/RenderSystem/include/FrameGraph/FrameGraphBlackboard.h"



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

class MyTask : public baselib::TaskRunner
{
public:
	MyTask();
	~MyTask();

private:
	virtual void Run()
	{
		printf("fuck\n");
		//::Sleep(300);
	}

};

MyTask::MyTask()
{
}

MyTask::~MyTask()
{
}

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

    case WM_PAINT:
    {
		WindowsVulkanView* pView = (WindowsVulkanView*)GetWindowLongPtr(hWnd, GWL_USERDATA);
		if (pView && pView->hasRenderDevice())
		{
            pView->Render();
		}
        break;
    }

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
	mRenderDevice = RenderCore::CreateRenderDevice(RenderCore::RenderDeviceType::VULKAN, mWindow);

	{
		baselib::ThreadPool threadPool(32);

		threadPool.Start();
		for (int i = 0; i < 100000; i++)
		{
			threadPool.Execute(std::make_shared<MyTask>());
		}

	}

	mTransientResources = new RenderSystem::TransientResources(mRenderDevice);

    return true;
}

void WindowsVulkanView::resize(int width, int height)
{
    mRenderDevice->Resize(width, height);
    mWidth = width;
    mHeight = height;

    //test
	renderTexture = mRenderDevice->CreateTexture2D(kTexFormatRGBA16Float,
		TextureUsage::TextureUsageRenderTarget, width, height, 1);
	computeTexture = mRenderDevice->CreateTexture2D(kTexFormatRGBA16Float,
		TextureUsage::TextureUsageRenderTarget, width, height, 1);

	depthStencilTexture = mRenderDevice->CreateTexture2D(kTexFormatDepth32FloatStencil8,
		TextureUsage::TextureUsageRenderTarget,
		width, height, 1);
    
    sceneManager = SceneManager::GetInstance();
    SkyBox* skybox = initSky(mRenderDevice);
    sceneManager->GetSkyBox()->AttachSkyBoxObject(skybox);
    initPostResource(mRenderDevice);
    
    //初始化相机
    CameraPtr cameraPtr = sceneManager->CreateCamera("MainCamera");
    cameraPtr->LookAt(mathutil::Vector3f(0, 0, 5), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
    cameraPtr->SetLens(60, float(width) / height, 0.1f, 100.f);

    //初始化灯光信息
    Light * pointLight = sceneManager->CreateLight("mainLight", Light::LightType::PointLight);
    pointLight->setColor(Vector3f(1.0, 1.0, 1.0));
    //pointLight->setPosition(Vector3f(5.0, 8.0, 0.0));
    pointLight->setPosition(Vector3f(-1.0, -1.0, -1.0));
    pointLight->setFalloffStart(5);
    pointLight->setFalloffEnd(300);
    pointLight->setStrength(Vector3f(8.0, 8.0, 8.0));
    
    Quaternionf rotate;
    rotate.FromAngleAxis(90, Vector3f(1.0, 0.0, 0.0));
    //sceneManager->GetRootNode()->CreateRendererNode("hat", "DamagedHelmet/glTF/DamagedHelmet.gltf");


    //gltf/BrainStem/glTF
    //gltf/BrainStem/glTF/BrainStem.gltf
    /*std::string pathSplit = std::string(1, PATHSPLIT);
    sceneManager->GetRootNode()->CreateRendererNode("hat", "gltf" + pathSplit + "BrainStem" + pathSplit + "glTF" + pathSplit + "BrainStem.gltf",
                                                    Vector3f(0, -3.0, -2), Quaternionf(), Vector3f(3, 3, 3));*/
    //sceneManager->GetRootNode()->CreateRendererNode("hat", "skin/Woman.gltf", Vector3f(0, -3.0, -2), Quaternionf(), Vector3f(0.01, 0.01, 0.01));

    //sceneManager->GetRootNode()->CreateRendererNode("Marry", "asset/Marry.obj", Vector3f(0, -2.0, 0));

    //sceneManager->GetRootNode()->CreateRendererNode("Marry", "nanosuit/nanosuit.obj", Vector3f(0, -4.0, 0));
    
    //TestADD();
    //computeTexture = TestImageGray();
    computePipeline = initTestimageGray();
    
    lastTime = GetTickNanoSeconds();
}

void WindowsVulkanView::Render()
{
	uint64_t thisTime = GetTickNanoSeconds();
	float deltaTime = float(thisTime - lastTime) * 0.000000001f;
	printf("deltaTime = %f\n", deltaTime);
	lastTime = thisTime;
	sceneManager->Update(deltaTime);

	// 从Graphics队列创建命令缓冲区
	CommandQueuePtr graphicsQueue = mRenderDevice->GetCommandQueue(QueueType::Graphics, 0);
	CommandBufferPtr commandBuffer = graphicsQueue->CreateCommandBuffer();
    if (!commandBuffer)
    {
        return;
    }

	struct PassData
	{
		RenderSystem::FrameGraphResource colorTarget;
		RenderSystem::FrameGraphResource depthStencilTarget;
	};

	RenderSystem::FrameGraph frameGraph;
	const PassData& basePassData = frameGraph.AddPass<PassData>("BasePass",
		[=](RenderSystem::FrameGraph::Builder& builder, PassData& data)
		{
			RenderSystem::FrameGraphTexture::Desc colorDesc;
			colorDesc.extent.width = mWidth;
			colorDesc.extent.height = mHeight;
			colorDesc.format = kTexFormatRGBA16Float;
			data.colorTarget = builder.Create<RenderSystem::FrameGraphTexture>("ColorTarget0", colorDesc);
			data.colorTarget = builder.Write(data.colorTarget);

			RenderSystem::FrameGraphTexture::Desc depthStencilDesc;
			depthStencilDesc.extent.width = mWidth;
			depthStencilDesc.extent.height = mHeight;
			depthStencilDesc.format = kTexFormatDepth32FloatStencil8;
			data.depthStencilTarget = builder.Create<RenderSystem::FrameGraphTexture>("depthStencilTarget", depthStencilDesc);
			data.depthStencilTarget = builder.Write(data.depthStencilTarget);
		},
		[=](const PassData& data, RenderSystem::FrameGraphPassResources& resources, void*)
		{
			RenderSystem::FrameGraphTexture& colorTexture = resources.Get<RenderSystem::FrameGraphTexture>(data.colorTarget);
			RenderSystem::FrameGraphTexture& depthStencilTexture = resources.Get<RenderSystem::FrameGraphTexture>(data.depthStencilTarget);

			RenderPass renderPass;
			RenderPassColorAttachmentPtr colorAttachmentPtr = std::make_shared<RenderPassColorAttachment>();
			colorAttachmentPtr->clearColor = MakeClearColor(0.0, 0.0, 0.0, 1.0);
			colorAttachmentPtr->texture = colorTexture.texture;
			renderPass.colorAttachments.push_back(colorAttachmentPtr);

			renderPass.depthAttachment = std::make_shared<RenderPassDepthAttachment>();
			renderPass.depthAttachment->texture = depthStencilTexture.texture;
			renderPass.depthAttachment->clearDepth = 1.0;

			renderPass.stencilAttachment = std::make_shared<RenderPassStencilAttachment>();
			renderPass.stencilAttachment->texture = depthStencilTexture.texture;
			renderPass.stencilAttachment->clearStencil = 0x00;

			renderPass.renderRegion = Rect2D(0, 0, mWidth, mHeight);
			RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);

			sceneManager->Render(renderEncoder);

			renderEncoder->EndEncode();
		});

	// 图像灰度化的计算管线
	struct ComputePassData
	{
		RenderSystem::FrameGraphResource inputColor;
		RenderSystem::FrameGraphResource outputColor;
	};

	RenderSystem::FrameGraphBlackboard fgBlackboard;
	fgBlackboard.Add<ComputePassData>() = frameGraph.AddPass<ComputePassData>("GrayCompute",
		[=](RenderSystem::FrameGraph::Builder& builder, ComputePassData& data)
		{
			RenderSystem::FrameGraphTexture::Desc colorDesc;
			colorDesc.extent.width = mWidth;
			colorDesc.extent.height = mHeight;
			colorDesc.format = kTexFormatRGBA16Float;
			data.outputColor = builder.Create<RenderSystem::FrameGraphTexture>("grayColor", colorDesc);
			data.outputColor = builder.Write(data.outputColor);
			builder.EnableAsyncCompute(true);

			data.inputColor = builder.Read(basePassData.colorTarget);
		},
		[=](const ComputePassData& data, RenderSystem::FrameGraphPassResources& resources, void*)
		{
			RenderSystem::FrameGraphTexture& colorTexture = resources.Get<RenderSystem::FrameGraphTexture>(data.inputColor);
			RenderSystem::FrameGraphTexture& grayTexture = resources.Get<RenderSystem::FrameGraphTexture>(data.outputColor);

			ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
			testImageGrayDraw(computeEncoder, computePipeline, colorTexture.texture, grayTexture.texture);
			computeEncoder->EndEncode();
		});


	const ComputePassData& computePassData = fgBlackboard.Get<ComputePassData>();
	frameGraph.AddPass("PresentPass",
		[=](RenderSystem::FrameGraph::Builder& builder, RenderSystem::FrameGraph::NoData& data)
		{
			builder.Read(computePassData.outputColor);

			// present的pass必须设置这个标记，要不然不会执行
			builder.SetSideEffect();
		},
		[=](const RenderSystem::FrameGraph::NoData& data, RenderSystem::FrameGraphPassResources& resources, void*)
		{
			RenderSystem::FrameGraphTexture& colorTexture = resources.Get<RenderSystem::FrameGraphTexture>(computePassData.outputColor);

			RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
			testPost(renderEncoder, colorTexture.texture);
			renderEncoder->EndEncode();
			commandBuffer->PresentFrameBuffer();
		});

	//std::ofstream{ "/Users/zhouxuguang/work/opensource/fg.txt" } << frameGraph;

	frameGraph.Compile();
	frameGraph.Execute(nullptr, mTransientResources);

	mTransientResources->Update(deltaTime);

	return;

	RenderPass renderPass;
	RenderPassColorAttachmentPtr colorAttachmentPtr = std::make_shared<RenderPassColorAttachment>();
	colorAttachmentPtr->clearColor = MakeClearColor(0.0, 0.0, 0.0, 1.0);
	colorAttachmentPtr->texture = renderTexture;
	renderPass.colorAttachments.push_back(colorAttachmentPtr);

	renderPass.depthAttachment = std::make_shared<RenderPassDepthAttachment>();
	renderPass.depthAttachment->texture = depthStencilTexture;
	renderPass.depthAttachment->clearDepth = 1.0;

	renderPass.stencilAttachment = std::make_shared<RenderPassStencilAttachment>();
	renderPass.stencilAttachment->texture = depthStencilTexture;
	renderPass.stencilAttachment->clearStencil = 0x00;

	renderPass.renderRegion = Rect2D(0, 0, mWidth, mHeight);
	RenderEncoderPtr renderEncoder1 = commandBuffer->CreateRenderEncoder(renderPass);

	sceneManager->Render(renderEncoder1);

	renderEncoder1->EndEncode();

	/*ComputeEncoderPtr computeEncoder = commandBuffer->createComputeEncoder();
	testImageGrayDraw(computeEncoder, computePipeline, renderTexture, computeTexture);
	computeEncoder->EndEncode();*/

	RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
	testPost(renderEncoder, renderTexture);
	renderEncoder->EndEncode();
	commandBuffer->PresentFrameBuffer();
}
