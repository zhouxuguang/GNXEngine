//
//  ImGuiRenderer.cpp
//  GNXEngine
//
//  基于自研 RHI 的 Dear ImGui 渲染后端 + 输入桥接实现。
//  详见 doc/ImGuiIntegrationDesign.md
//

#include "ImGuiRenderer.h"

#include "../ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/GNXEngine/include/Events/MouseEvent.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/BaseLib/include/LogService.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cstring>
#include <fstream>

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

namespace
{
// 探测系统中可用的中文字体（按优先级返回第一个存在的文件）。
// ImGui 内置字体（ProggyClean）只包含拉丁字形，中文必须加载系统 CJK 字体。
const char* FindSystemCjkFont()
{
#if defined(__APPLE__)
    static const char* kCandidates[] = {
        "/System/Library/Fonts/PingFang.ttc",           // macOS 10.11+ 默认中文字体
        "/System/Library/Fonts/STHeiti Medium.ttc",
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
        "/Library/Fonts/Arial Unicode.ttf",
    };
#elif defined(_WIN32)
    static const char* kCandidates[] = {
        "C:/Windows/Fonts/msyh.ttc",                    // 微软雅黑
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/simhei.ttf",                  // 黑体
        "C:/Windows/Fonts/simsun.ttc",                  // 宋体
    };
#else
    static const char* kCandidates[] = {
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "/usr/share/fonts/truetype/arphic/uming.ttc",
    };
#endif

    for (const char* path : kCandidates)
    {
        std::ifstream file(path, std::ios::binary);
        if (file.good())
        {
            return path;
        }
    }
    return nullptr;
}

// ImGui 逻辑坐标 -> 引擎 Event 的键值映射（引擎 KeyCode 与 GLFW keycode 一致）
ImGuiKey ToImGuiKey(GNXEngine::KeyCode key)
{
    using namespace GNXEngine;
    if (key >= A && key <= Z)      return (ImGuiKey)(ImGuiKey_A + (key - A));
    if (key >= D0 && key <= D9)    return (ImGuiKey)(ImGuiKey_0 + (key - D0));
    if (key >= F1 && key <= F12)   return (ImGuiKey)(ImGuiKey_F1 + (key - F1));

    switch (key)
    {
    case Space:      return ImGuiKey_Space;
    case Apostrophe: return ImGuiKey_Apostrophe;
    case Comma:      return ImGuiKey_Comma;
    case Minus:      return ImGuiKey_Minus;
    case Period:     return ImGuiKey_Period;
    case Slash:      return ImGuiKey_Slash;
    case Semicolon:  return ImGuiKey_Semicolon;
    case Equal:      return ImGuiKey_Equal;
    case LeftBracket:  return ImGuiKey_LeftBracket;
    case Backslash:    return ImGuiKey_Backslash;
    case RightBracket: return ImGuiKey_RightBracket;
    case GraveAccent:  return ImGuiKey_GraveAccent;

    case Escape:     return ImGuiKey_Escape;
    case Enter:      return ImGuiKey_Enter;
    case Tab:        return ImGuiKey_Tab;
    case Backspace:  return ImGuiKey_Backspace;
    case Insert:     return ImGuiKey_Insert;
    case Delete:     return ImGuiKey_Delete;
    case Right:      return ImGuiKey_RightArrow;
    case Left:       return ImGuiKey_LeftArrow;
    case Down:       return ImGuiKey_DownArrow;
    case Up:         return ImGuiKey_UpArrow;
    case PageUp:     return ImGuiKey_PageUp;
    case PageDown:   return ImGuiKey_PageDown;
    case Home:       return ImGuiKey_Home;
    case End:        return ImGuiKey_End;
    case CapsLock:   return ImGuiKey_CapsLock;
    case ScrollLock: return ImGuiKey_ScrollLock;
    case PrintScreen:return ImGuiKey_PrintScreen;
    case Pause:      return ImGuiKey_Pause;

    case KP0: return ImGuiKey_Keypad0;
    case KP1: return ImGuiKey_Keypad1;
    case KP2: return ImGuiKey_Keypad2;
    case KP3: return ImGuiKey_Keypad3;
    case KP4: return ImGuiKey_Keypad4;
    case KP5: return ImGuiKey_Keypad5;
    case KP6: return ImGuiKey_Keypad6;
    case KP7: return ImGuiKey_Keypad7;
    case KP8: return ImGuiKey_Keypad8;
    case KP9: return ImGuiKey_Keypad9;
    case KPDecimal:  return ImGuiKey_KeypadDecimal;
    case KPDivide:   return ImGuiKey_KeypadDivide;
    case KPMultiply: return ImGuiKey_KeypadMultiply;
    case KPSubtract: return ImGuiKey_KeypadSubtract;
    case KPAdd:      return ImGuiKey_KeypadAdd;
    case KPEnter:    return ImGuiKey_KeypadEnter;
    case KPEqual:    return ImGuiKey_KeypadEqual;

    case LeftShift:    return ImGuiKey_LeftShift;
    case LeftControl:  return ImGuiKey_LeftCtrl;
    case LeftAlt:      return ImGuiKey_LeftAlt;
    case LeftSuper:    return ImGuiKey_LeftSuper;
    case RightShift:   return ImGuiKey_RightShift;
    case RightControl: return ImGuiKey_RightCtrl;
    case RightAlt:     return ImGuiKey_RightAlt;
    case RightSuper:   return ImGuiKey_RightSuper;
    case Menu:         return ImGuiKey_Menu;
    default:           return ImGuiKey_None;
    }
}
}

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------
ImGuiRenderer::ImGuiRenderer()
{
}

ImGuiRenderer::~ImGuiRenderer()
{
    Shutdown();
}

bool ImGuiRenderer::Initialize(RenderDevice* device, bool installIniFile)
{
    if (mInitialized)
    {
        return true;
    }
    if (!device)
    {
        LOG_ERROR("ImGuiRenderer::Initialize: device is null");
        return false;
    }

    mDevice = device;
    mIniFileEnabled = installIniFile;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "GNXEngine_PlatformBridge";
    io.BackendRendererName = "GNXEngine_RHI";
    // 顶点由 CPU 侧展开，不使用 VtxOffset，因此不设置 RendererHasVtxOffset 标志
    io.IniFilename = mIniFileEnabled ? "imgui.ini" : nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SetupStyle();

    if (!CreateDeviceObjects())
    {
        LOG_ERROR("ImGuiRenderer::Initialize: CreateDeviceObjects failed");
        ImGui::DestroyContext();
        return false;
    }

    mInitialized = true;
    LOG_INFO("ImGuiRenderer: initialized (backend=RHI)");
    return true;
}

void ImGuiRenderer::Shutdown()
{
    if (!mInitialized && !mPipeline)
    {
        return;
    }



    mBatches.clear();
    mPositions.clear();
    mUVs.clear();
    mColors.clear();

    mPosBuffer.reset();
    mUVBuffer.reset();
    mColorBuffer.reset();
    mFontTexture.reset();
    mProjUBO.reset();
    mSampler.reset();
    mPipeline.reset();
    mVertexCapacity = 0;
    mDevice = nullptr;

    if (ImGui::GetCurrentContext())
    {
        ImGui::DestroyContext();
    }

    mInitialized = false;
}

void ImGuiRenderer::SetupStyle()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
}

// ---------------------------------------------------------------------------
// GPU 资源
// ---------------------------------------------------------------------------
bool ImGuiRenderer::CreateDeviceObjects()
{
    // ---- 1) 渲染管线 ----
    GraphicsShaderInfo shaderInfo = CreateGraphicsShaderInfo("ImGui");
    if (!shaderInfo.graphicsShader)
    {
        LOG_ERROR("ImGuiRenderer: failed to load ImGui shader asset");
        return false;
    }

    GraphicsPipelineDesc desc = shaderInfo.graphicsPipelineDesc;

    // 顶点属性：着色器反射得到的是 float4 颜色，这里显式覆盖为 UChar4Norm
    // 以匹配 ImGui 的 ImDrawVert（pos/uv float2 + col RGBA8）
    desc.vertexDescriptor.attributes.clear();
    desc.vertexDescriptor.layouts.clear();
    {
        VertextAttributesDesc attr;
        VertexBufferLayoutDesc layout;

        attr.index = 0; attr.format = VertexFormatFloat2;     attr.offset = 0;
        desc.vertexDescriptor.attributes.push_back(attr);
        layout.stride = 8; layout.stepRate = 0; layout.stepFunction = VertexStepFunctionPerVertex;
        desc.vertexDescriptor.layouts.push_back(layout);

        attr.index = 1; attr.format = VertexFormatFloat2;     attr.offset = 0;
        desc.vertexDescriptor.attributes.push_back(attr);
        layout.stride = 8; layout.stepRate = 0; layout.stepFunction = VertexStepFunctionPerVertex;
        desc.vertexDescriptor.layouts.push_back(layout);

        attr.index = 2; attr.format = VertexFormatUChar4Norm; attr.offset = 0;
        desc.vertexDescriptor.attributes.push_back(attr);
        layout.stride = 4; layout.stepRate = 0; layout.stepFunction = VertexStepFunctionPerVertex;
        desc.vertexDescriptor.layouts.push_back(layout);
    }

    // 颜色混合：RGB 用 SrcAlpha/OneMinusSrcAlpha，Alpha 用 One/OneMinusSrcAlpha
    // （与 ImGui 官方后端一致）
    desc.renderTargetCount = 1;
    desc.colorAttachmentDescriptors[0] = ColorAttachmentDesc::GetCommonBlendDes();
    desc.colorAttachmentDescriptors[0].sourceAlphaBlendFactor = BlendFactorOne;
    desc.cullMode = CullModeNone;

    mPipeline = mDevice->CreateGraphicsPipeline(desc);
    if (!mPipeline)
    {
        LOG_ERROR("ImGuiRenderer: CreateGraphicsPipeline failed");
        return false;
    }
    mPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);

    // ---- 2) 采样器 ----
    // 默认 SamplerDesc 即 线性过滤 + CLAMP_TO_EDGE，正合 UI 字体纹理需要
    SamplerDesc samplerDesc;
    mSampler = mDevice->CreateSamplerWithDescriptor(samplerDesc);

    // ---- 3) 投影矩阵 UBO（64B）----
    mProjUBO = mDevice->CreateUniformBufferWithSize(sizeof(Matrix4x4f));

    // ---- 4) 动态顶点缓冲 ----
    if (!EnsureVertexCapacity(4096))
    {
        return false;
    }

    // ---- 5) 字体图集纹理 ----
    return CreateFontTexture();
}

bool ImGuiRenderer::EnsureVertexCapacity(uint32_t vertexCount)
{
    if (vertexCount <= mVertexCapacity && mPosBuffer && mUVBuffer && mColorBuffer)
    {
        return true;
    }

    uint32_t capacity = std::max(vertexCount, mVertexCapacity ? mVertexCapacity * 2 : 4096u);
    capacity = std::max(capacity, 4096u);

    const uint32_t posBytes   = capacity * sizeof(float) * 2;
    const uint32_t uvBytes    = capacity * sizeof(float) * 2;
    const uint32_t colorBytes = capacity * sizeof(uint32_t);

    mPosBuffer   = mDevice->CreateBuffer(RCBufferDesc(posBytes, RCBufferUsage::VertexBuffer, StorageModeShared));
    mUVBuffer    = mDevice->CreateBuffer(RCBufferDesc(uvBytes, RCBufferUsage::VertexBuffer, StorageModeShared));
    mColorBuffer = mDevice->CreateBuffer(RCBufferDesc(colorBytes, RCBufferUsage::VertexBuffer, StorageModeShared));

    if (!mPosBuffer || !mUVBuffer || !mColorBuffer)
    {
        LOG_ERROR("ImGuiRenderer: create vertex buffers failed");
        return false;
    }

    mPosBuffer->SetName("ImGui_Positions");
    mUVBuffer->SetName("ImGui_UVs");
    mColorBuffer->SetName("ImGui_Colors");

    mVertexCapacity = capacity;
    mPositions.reserve(capacity * 2);
    mUVs.reserve(capacity * 2);
    mColors.reserve(capacity);
    return true;
}

bool ImGuiRenderer::CreateFontTexture()
{
    ImGuiIO& io = ImGui::GetIO();

    // 重建字体图集（支持运行时切换字体 / DPI 变化）
    io.Fonts->Clear();

    // 高分屏：按 dpiScale 放大字形光栅化尺寸，再用 FontGlobalScale 缩回逻辑尺寸，保证文字清晰
    const float scale = std::max(mDPIScale, 1.0f);
    const float pixelSize = mFontSize * scale;
    io.FontGlobalScale = 1.0f / scale;

    ImFontConfig config;
    config.SizePixels = pixelSize;
    config.OversampleH = 2;
    config.OversampleV = 1;
    config.PixelSnapH = true;

    // 中文字形：优先使用显式指定的字体，其次自动探测系统 CJK 字体。
    // ImGui 内置字体（ProggyClean）只含拉丁字形，直接使用会导致中文显示为乱码。
    mHasCjkFont = false;
    std::string fontPath = mCjkFontPath;
    if (fontPath.empty())
    {
        if (const char* systemFont = FindSystemCjkFont())
        {
            fontPath = systemFont;
        }
    }

    if (!fontPath.empty())
    {
        // 字形范围：拉丁/标点 + 常用汉字(约2500) + 调用方注册的文本。
        // 常用汉字范围之外的字（如“曝”）若未注册会显示为 '?'，可通过 AddGlyphText 补齐。
        // 注意：glyphRanges 需存活到字体图集构建完成（本函数内 GetTexDataAsRGBA32 即构建）。
        ImVector<ImWchar> glyphRanges;
        {
            ImFontGlyphRangesBuilder builder;
            builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
            builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            if (!mExtraGlyphText.empty())
            {
                builder.AddText(mExtraGlyphText.c_str());
            }
            builder.BuildRanges(&glyphRanges);
        }

        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), pixelSize, &config, glyphRanges.Data);
        if (font)
        {
            mHasCjkFont = true;
            LOG_INFO("ImGuiRenderer: 字体已加载 %s (%.1fpx, 字形数=%d)",
                     fontPath.c_str(), pixelSize, (int)io.Fonts->Fonts[0]->Glyphs.Size);
        }
        else
        {
            LOG_WARN("ImGuiRenderer: 字体加载失败，回退到内置字体: %s", fontPath.c_str());
        }
    }

    if (!mHasCjkFont)
    {
        io.Fonts->AddFontDefault(&config);
        LOG_WARN("ImGuiRenderer: 未找到中文系统字体，中文将无法正常显示（可调用 SetCjkFontPath 指定）");
    }

    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    if (!pixels || width <= 0 || height <= 0)
    {
        LOG_ERROR("ImGuiRenderer: font atlas is empty");
        return false;
    }

    RCTexture2DPtr texture = mDevice->CreateTexture2D(
        kTexFormatRGBA8,
        TextureUsage::TextureUsageShaderRead,
        (uint32_t)width, (uint32_t)height, 1);
    if (!texture)
    {
        LOG_ERROR("ImGuiRenderer: create font texture failed");
        return false;
    }

    Rect2D rect(0, 0, width, height);
    texture->ReplaceRegion(rect, 0, pixels, (uint32_t)width * 4);

    mFontTexture = texture;
    // 本方案下 ImTextureID 直接存放引擎纹理对象指针，绘制时还原
    io.Fonts->SetTexID((ImTextureID)(uintptr_t)mFontTexture.get());

    io.FontGlobalScale = 1.0f / std::max(mDPIScale, 1.0f);
    mFontTextureDirty = false;
    return true;
}

// ---------------------------------------------------------------------------
// 每帧
// ---------------------------------------------------------------------------
void ImGuiRenderer::NewFrame(float deltaTime, uint32_t width, uint32_t height)
{
    if (!mInitialized)
    {
        return;
    }

    // 防御：ImGui 上下文可能因外部误用而失效，避免直接触发断言
    if (!ImGui::GetCurrentContext())
    {
        LOG_ERROR("ImGuiRenderer::NewFrame: ImGui context is null (this=%p)", (void*)this);
        return;
    }

    mWidth = width;
    mHeight = height;

    if (mFontTextureDirty)
    {
        CreateFontTexture();
    }

    ImGuiIO& io = ImGui::GetIO();

    // ImGui 工作在「逻辑坐标」空间：像素尺寸 / dpiScale
    const float scale = std::max(mDPIScale, 1.0f);
    io.DisplaySize = ImVec2((float)width / scale, (float)height / scale);
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    io.DeltaTime = deltaTime > 0.0f ? deltaTime : (1.0f / 60.0f);

    if (!io.Fonts->IsBuilt())
    {
        io.Fonts->Build();
    }

    ImGui::NewFrame();
}

void ImGuiRenderer::ExpandDrawData()
{
    mPositions.clear();
    mUVs.clear();
    mColors.clear();
    mBatches.clear();

    const ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData || drawData->CmdListsCount == 0)
    {
        return;
    }

    uint32_t totalVertices = 0;
    for (int i = 0; i < drawData->CmdListsCount; ++i)
    {
        totalVertices += (uint32_t)drawData->CmdLists[i]->VtxBuffer.Size;
    }
    if (totalVertices == 0)
    {
        return;
    }

    EnsureVertexCapacity(totalVertices);

    const float scale = std::max(mDPIScale, 1.0f);
    const float clipW = (float)mWidth;
    const float clipH = (float)mHeight;

    for (int listIdx = 0; listIdx < drawData->CmdListsCount; ++listIdx)
    {
        const ImDrawList* cmdList = drawData->CmdLists[listIdx];
        const ImDrawVert* vtxBuffer = cmdList->VtxBuffer.Data;
        const ImDrawIdx* idxBuffer = cmdList->IdxBuffer.Data;

        for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; ++cmdIdx)
        {
            const ImDrawCmd& cmd = cmdList->CmdBuffer[cmdIdx];

            // 用户回调（如 ImGui::Image 的自定义绘制）本后端不支持，跳过
            if (cmd.UserCallback != nullptr)
            {
                continue;
            }
            if (cmd.ElemCount == 0)
            {
                continue;
            }

            // 裁剪矩形：逻辑坐标 -> 像素坐标，并夹取到视口范围
            const int cx0 = (int)std::max(cmd.ClipRect.x * scale, 0.0f);
            const int cy0 = (int)std::max(cmd.ClipRect.y * scale, 0.0f);
            const int cx1 = (int)std::min(cmd.ClipRect.z * scale, clipW);
            const int cy1 = (int)std::min(cmd.ClipRect.w * scale, clipH);
            if (cx1 <= cx0 || cy1 <= cy0)
            {
                continue;
            }

            DrawBatch batch;
            batch.firstVertex = (uint32_t)(mColors.size());
            batch.vertexCount = (uint32_t)cmd.ElemCount;
            batch.clipX = cx0;
            batch.clipY = cy0;
            batch.clipW = (uint32_t)(cx1 - cx0);
            batch.clipH = (uint32_t)(cy1 - cy0);
            batch.texture = mFontTexture;

            ImTextureID texId = cmd.GetTexID();
            if (texId != 0 && (void*)(uintptr_t)texId != (void*)mFontTexture.get())
            {
                // 外部纹理（如 ImGui::Image 传入的引擎纹理指针）
                batch.texture = RCTexturePtr(reinterpret_cast<RCTexture*>((uintptr_t)texId),
                                             [](RCTexture*) { /* 所有权归调用方，这里不释放 */ });
            }

            for (unsigned int elem = 0; elem < cmd.ElemCount; ++elem)
            {
                const ImDrawIdx index = idxBuffer[cmd.IdxOffset + elem];
                const ImDrawVert& vtx = vtxBuffer[index];

                mPositions.push_back(vtx.pos.x);
                mPositions.push_back(vtx.pos.y);
                mUVs.push_back(vtx.uv.x);
                mUVs.push_back(vtx.uv.y);
                mColors.push_back((uint32_t)vtx.col);
            }

            mBatches.push_back(std::move(batch));
        }
    }
}

void ImGuiRenderer::Render(RenderEncoderPtr renderEncoder)
{
    if (!mInitialized || !renderEncoder)
    {
        return;
    }

    const ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData || drawData->CmdListsCount == 0)
    {
        return;
    }

    ExpandDrawData();
    if (mBatches.empty())
    {
        return;
    }

    // ---- 上传投影矩阵 ----
    // 屏幕像素(逻辑)坐标 -> 裁剪空间；两后端 viewport 均已统一为 NDC +Y 向上
    // （Vulkan 侧使用负高度 viewport），因此不需要按后端分支。
    const float W = drawData->DisplaySize.x > 0.0f ? drawData->DisplaySize.x : (float)mWidth;
    const float H = drawData->DisplaySize.y > 0.0f ? drawData->DisplaySize.y : (float)mHeight;
    // zNear/zFar 取 (1,0) 使顶点 z=0 映射到 NDC z=1，落在 [0,1] 深度范围内
    Matrix4x4f proj = Matrix4x4f::CreateOrthographic(0.0f, W, H, 0.0f, 1.0f, 0.0f);
    mProjUBO->SetData(&proj, 0, sizeof(proj));

    // ---- 上传顶点数据 ----
    void* dst = mPosBuffer->Map();
    if (dst)
    {
        memcpy(dst, mPositions.data(), mPositions.size() * sizeof(float));
        mPosBuffer->Unmap();
    }
    dst = mUVBuffer->Map();
    if (dst)
    {
        memcpy(dst, mUVs.data(), mUVs.size() * sizeof(float));
        mUVBuffer->Unmap();
    }
    dst = mColorBuffer->Map();
    if (dst)
    {
        memcpy(dst, mColors.data(), mColors.size() * sizeof(uint32_t));
        mColorBuffer->Unmap();
    }

    // ---- 绘制 ----
    renderEncoder->SetGraphicsPipeline(mPipeline);
    renderEncoder->SetVertexUniformBuffer("ImGuiCB", mProjUBO);
    renderEncoder->SetVertexBuffer(mPosBuffer, 0, 0);
    renderEncoder->SetVertexBuffer(mUVBuffer, 0, 1);
    renderEncoder->SetVertexBuffer(mColorBuffer, 0, 2);

    for (const DrawBatch& batch : mBatches)
    {
        renderEncoder->SetScissorRect(batch.clipX, batch.clipY, batch.clipW, batch.clipH);
        renderEncoder->SetFragmentTextureAndSampler("fontTex", batch.texture, mSampler);
        renderEncoder->DrawPrimitives(PrimitiveMode_TRIANGLES, (int)batch.firstVertex, (int)batch.vertexCount);
    }

    // 恢复全屏裁剪，避免影响后续 Pass
    renderEncoder->SetScissorRect(0, 0, mWidth, mHeight);
}

// ---------------------------------------------------------------------------
// 输入桥接
// ---------------------------------------------------------------------------
bool ImGuiRenderer::OnEvent(GNXEngine::Event& e)
{
    if (!mInitialized)
    {
        return false;
    }

    if (!ImGui::GetCurrentContext())
    {
        LOG_ERROR("ImGuiRenderer::OnEvent: ImGui context is null (this=%p)", (void*)this);
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    bool consumed = false;

    GNXEngine::EventDispatcher dispatcher(e);

    dispatcher.Dispatch<GNXEngine::MouseMovedEvent>([&](GNXEngine::MouseMovedEvent& ev)
    {
        // 引擎侧的鼠标坐标为逻辑坐标，与 io.DisplaySize 同一空间，无需换算
        io.AddMousePosEvent(ev.GetX(), ev.GetY());
        consumed = io.WantCaptureMouse;
        return consumed;
    });

    dispatcher.Dispatch<GNXEngine::MouseButtonPressedEvent>([&](GNXEngine::MouseButtonPressedEvent& ev)
    {
        io.AddMouseButtonEvent((int)ev.GetMouseButton(), true);
        consumed = io.WantCaptureMouse;
        return consumed;
    });

    dispatcher.Dispatch<GNXEngine::MouseButtonReleasedEvent>([&](GNXEngine::MouseButtonReleasedEvent& ev)
    {
        io.AddMouseButtonEvent((int)ev.GetMouseButton(), false);
        consumed = io.WantCaptureMouse;
        return consumed;
    });

    dispatcher.Dispatch<GNXEngine::MouseScrolledEvent>([&](GNXEngine::MouseScrolledEvent& ev)
    {
        // 引擎把滚轮标准化为 ±120（一格），ImGui 期望 ±1
        io.AddMouseWheelEvent(ev.GetXOffset() / 120.0f, ev.GetYOffset() / 120.0f);
        consumed = io.WantCaptureMouse;
        return consumed;
    });

    dispatcher.Dispatch<GNXEngine::KeyPressedEvent>([&](GNXEngine::KeyPressedEvent& ev)
    {
        const ImGuiKey key = ToImGuiKey(ev.GetKeyCode());
        if (key != ImGuiKey_None)
        {
            io.AddKeyEvent(key, true);
        }
        // 文本输入模式下吞掉按键，避免触发 3D 场景快捷键
        consumed = io.WantTextInput || io.WantCaptureKeyboard;
        return consumed;
    });

    dispatcher.Dispatch<GNXEngine::KeyReleasedEvent>([&](GNXEngine::KeyReleasedEvent& ev)
    {
        const ImGuiKey key = ToImGuiKey(ev.GetKeyCode());
        if (key != ImGuiKey_None)
        {
            io.AddKeyEvent(key, false);
        }
        consumed = io.WantTextInput || io.WantCaptureKeyboard;
        return consumed;
    });

    dispatcher.Dispatch<GNXEngine::KeyTypedEvent>([&](GNXEngine::KeyTypedEvent& ev)
    {
        const unsigned int c = (unsigned int)ev.GetKeyCode();
        if (c > 0 && c < 0x10000)
        {
            io.AddInputCharacter(c);
        }
        consumed = io.WantTextInput;
        return consumed;
    });

    return consumed;
}

bool ImGuiRenderer::WantsCaptureMouse() const
{
    return mInitialized && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiRenderer::WantsCaptureKeyboard() const
{
    return mInitialized && ImGui::GetIO().WantCaptureKeyboard;
}

NS_RENDERSYSTEM_END
