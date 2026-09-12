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
    // 1.92 动态字体：声明后端支持 ImDrawData::Textures 纹理协议（按需创建/增量更新/销毁）。
    // 开启后字形在绘制时按需光栅化，无需再预先声明字形范围（AddGlyphText 已移除）。
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
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

    // ImGui 上下文仍存活时，释放它通过纹理协议请求创建的纹理
    if (ImGui::GetCurrentContext())
    {
        DestroyAllTextures();
    }
    mTextures.clear();

    mPosBuffer.reset();
    mUVBuffer.reset();
    mColorBuffer.reset();
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

    // ---- 5) 字体加载 ----
    // 1.92 动态字体：这里只加载字体源，字体纹理由 ImGui 在运行时通过纹理协议请求创建。
    // 先取设备 2D 纹理尺寸上限，用于约束动态图集的增长范围。
    mMaxTextureSize = mDevice->GetFeatures().limits.maxTextureSize2D;
    return LoadFonts();
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

namespace
{
// 纹理销毁延迟帧数：ImGui 要求 Status==WantDestroy 且 UnusedFrames 达到阈值后才可释放。
// 引擎侧 Vulkan 有自己的延迟回收队列（VulkanGarbageCollector，默认延迟 2 帧），
// Metal 侧无延迟队列（由 ARC 与命令缓冲对资源的持有保证安全）。
// 取 2 与前者对齐，为"ImGui 认为可销毁"到"引擎真正释放"之间留出安全边界。
constexpr int kTextureDestroyDelayFrames = 2;
}

// ---------------------------------------------------------------------------
// 字体（1.92 动态字体图集）
// ---------------------------------------------------------------------------
// 与 1.91 静态图集的差异：
//   1) 不再需要声明字形范围（GetGlyphRanges* / ImFontGlyphRangesBuilder 均已废弃），
//      字形在绘制时按需光栅化、图集随用字增长，生僻字无需任何注册即可显示；
//   2) 不再由后端一次性构建纹理，改由 ImGui 通过 ImDrawData::Textures 请求创建/更新
//      （见 UpdateTexture），本函数只负责加载字体源，不接触纹理。
// 注意：动态字体免除的是「字形范围声明」，CJK 字体文件仍必须显式加载。
bool ImGuiRenderer::LoadFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    // 基准字号（逻辑尺寸，未乘 DPI）。不要在这里乘 dpiScale：
    // 1.92 的后端会依据 io.DisplayFramebufferScale 自动设置字形光栅化密度
    // （g.FontRasterizerDensity = DisplayFramebufferScale），Retina 下自动清晰。
    style.FontSizeBase = mFontSize;
    // 保持 1.0：本引擎用 DisplayFramebufferScale 表达 DPI，
    // 若再设置 FontScaleDpi 会与自动密度叠加、导致字号被二次放大。
    style.FontScaleMain = 1.0f;
    style.FontScaleDpi = 1.0f;

    // 图集初始尺寸。1.92 只光栅化「实际用到的字形」，因此图集远小于 1.91 的
    // 固定范围图集（后者需覆盖拉丁 + 约 2500 常用汉字，约 1024/2048 见方）。
    // 实测（大气 demo 全量面板、Retina 2x）：无论起始尺寸多少，图集最终稳定在
    // 512x256（RGBA32 = 0.5MB），且扩容只发生一次、在启动首帧的 1ms 内完成。
    // 这里取 512x512（1MB，约 2 倍余量）：比原先 1024x1024（4MB）省 4 倍显存，
    // 同时避免首帧扩容。若 UI 大量增加用字，ImGui 会自动扩容，无需改这里。
    io.Fonts->TexMinWidth = 512;
    io.Fonts->TexMinHeight = 512;

    // 图集上限与设备能力对齐（ImGui 默认 8192）
    if (mMaxTextureSize > 0)
    {
        const int maxSize = (int)std::min<uint32_t>(8192u, mMaxTextureSize);
        io.Fonts->TexMaxWidth = maxSize;
        io.Fonts->TexMaxHeight = maxSize;
    }

    ImFontConfig config;
    config.PixelSnapH = true;
    // OversampleH/V 保持默认「自动(0)」：自 1.91.8 起官方建议不要手动指定

    // 中文字形：优先使用显式指定的字体，其次自动探测系统 CJK 字体。
    // ImGui 内置字体（ProggyClean/ProggyForever）只含拉丁字形，必须加载 CJK 字体文件。
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
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), mFontSize, &config);
        if (font)
        {
            mHasCjkFont = true;
            LOG_INFO("ImGuiRenderer: 字体已加载 %s (%.1fpx, 字形按需光栅化)", fontPath.c_str(), mFontSize);
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

    return true;
}

// ---------------------------------------------------------------------------
// 动态纹理协议（ImGuiBackendFlags_RendererHasTextures）
// ---------------------------------------------------------------------------
void ImGuiRenderer::UpdateTextures(const ImDrawData* drawData)
{
    if (!drawData || drawData->Textures == nullptr)
    {
        return;
    }

    // 绝大多数帧该列表仅有 1 项且状态为 OK，开销可忽略
    for (ImTextureData* tex : *drawData->Textures)
    {
        if (tex->Status != ImTextureStatus_OK)
        {
            UpdateTexture(tex);
        }
    }
}

void ImGuiRenderer::UpdateTexture(ImTextureData* tex)
{
    if (tex->Status == ImTextureStatus_WantCreate)
    {
        // 本后端只接受 RGBA32（ImGui 默认格式，对应引擎的 kTexFormatRGBA8）
        const bool sizeOk = (mMaxTextureSize == 0) ||
                            ((uint32_t)tex->Width <= mMaxTextureSize &&
                             (uint32_t)tex->Height <= mMaxTextureSize);
        if (tex->Format != ImTextureFormat_RGBA32 || !sizeOk)
        {
            if (!mTextureErrorLogged)
            {
                mTextureErrorLogged = true;
                LOG_ERROR("ImGuiRenderer: 字体图集纹理不受支持 (format=%d, %dx%d, 设备上限=%u)",
                          (int)tex->Format, tex->Width, tex->Height, mMaxTextureSize);
            }
            tex->SetTexID(ImTextureID_Invalid);
            tex->SetStatus(ImTextureStatus_Destroyed);
            return;
        }

        RCTexture2DPtr texture = mDevice->CreateTexture2D(
            kTexFormatRGBA8,
            TextureUsage::TextureUsageShaderRead,
            (uint32_t)tex->Width, (uint32_t)tex->Height, 1);
        if (!texture)
        {
            if (!mTextureErrorLogged)
            {
                mTextureErrorLogged = true;
                LOG_ERROR("ImGuiRenderer: 创建字体图集纹理失败 (%dx%d)", tex->Width, tex->Height);
            }
            tex->SetTexID(ImTextureID_Invalid);
            tex->SetStatus(ImTextureStatus_Destroyed);
            return;
        }
        texture->SetName("ImGui_FontAtlas");

        // 全量上传：行距使用 ImGui 给出的 pitch（= Width * BytesPerPixel）
        Rect2D rect(0, 0, tex->Width, tex->Height);
        texture->ReplaceRegion(rect, 0, (const uint8_t*)tex->GetPixels(), (uint32_t)tex->GetPitch());

        // ImTextureID 约定存放引擎纹理基类指针（RCTexture*），绘制时在 ExpandDrawData 还原。
        // 注意：RCTexture2D 虚继承自 RCTexture，虚基类地址与派生类地址不同，
        // 必须用隐式向上转型取得 RCTexture*，不能直接存 RCTexture2D* 后 reinterpret_cast。
        RCTexturePtr baseTexture = texture;
        tex->SetTexID((ImTextureID)(uintptr_t)baseTexture.get());
        tex->BackendUserData = baseTexture.get();
        mTextures[tex] = texture;
        tex->SetStatus(ImTextureStatus_OK);

        LOG_INFO("ImGuiRenderer: 字体图集纹理已创建 %dx%d", tex->Width, tex->Height);
    }
    else if (tex->Status == ImTextureStatus_WantUpdates)
    {
        auto it = mTextures.find(tex);
        if (it == mTextures.end() || !it->second)
        {
            LOG_WARN("ImGuiRenderer: 收到未知纹理的增量更新请求 (UniqueID=%d)", tex->UniqueID);
            tex->SetStatus(ImTextureStatus_OK);
            return;
        }

        // ImGui 保证只会写入「从未被使用过」的区域，因此无需读回原内容做混合
        for (const ImTextureRect& r : tex->Updates)
        {
            Rect2D rect((int)r.x, (int)r.y, (int)r.w, (int)r.h);
            it->second->ReplaceRegion(rect, 0, (const uint8_t*)tex->GetPixelsAt(r.x, r.y),
                                      (uint32_t)tex->GetPitch());
        }
        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames >= kTextureDestroyDelayFrames)
    {
        DestroyTexture(tex);
    }
}

void ImGuiRenderer::DestroyTexture(ImTextureData* tex)
{
    auto it = mTextures.find(tex);
    if (it != mTextures.end())
    {
        // 引擎侧纹理对象的释放本身是安全的：Vulkan 走延迟回收队列，Metal 由 ARC 处理
        it->second.reset();
        mTextures.erase(it);
    }

    tex->SetTexID(ImTextureID_Invalid);
    tex->BackendUserData = nullptr;
    tex->SetStatus(ImTextureStatus_Destroyed);
}

void ImGuiRenderer::DestroyAllTextures()
{
    // 关闭时销毁所有纹理：RefCount == 1 表示该纹理仅由本上下文引用（未被共享）
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
    {
        if (tex->RefCount == 1)
        {
            DestroyTexture(tex);
        }
    }
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

    ImGuiIO& io = ImGui::GetIO();

    // ImGui 工作在「逻辑坐标」空间：像素尺寸 / dpiScale
    const float scale = std::max(mDPIScale, 1.0f);
    io.DisplaySize = ImVec2((float)width / scale, (float)height / scale);
    io.DeltaTime = deltaTime > 0.0f ? deltaTime : (1.0f / 60.0f);

    // 1.92 动态字体：框架会依据 DisplayFramebufferScale 自动设置当前字形的光栅化密度
    // （见 imgui.cpp 中 g.FontRasterizerDensity = DisplayFramebufferScale），Retina 自动清晰。
    // 注意：不要再手工 FontGlobalScale 缩放、也不要设置 style.FontScaleDpi，
    // 否则会与自动密度叠加导致字号二次放大。
    io.DisplayFramebufferScale = ImVec2(scale, scale);

    // 基准字号（逻辑尺寸）：动态字体允许运行期修改，立即生效
    ImGuiStyle& style = ImGui::GetStyle();
    if (style.FontSizeBase != mFontSize)
    {
        style.FontSizeBase = mFontSize;
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
    // 1.92：CmdListsCount 已被标记 obsolete（且 1.92.9 曾出现其恒为 0 的回归，
    // 会导致 UI 完全不渲染），统一改用 CmdLists.Size。
    if (!drawData || drawData->CmdLists.Size == 0)
    {
        return;
    }

    uint32_t totalVertices = 0;
    for (int i = 0; i < drawData->CmdLists.Size; ++i)
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

    for (int listIdx = 0; listIdx < drawData->CmdLists.Size; ++listIdx)
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
            // ImTextureID 约定存放引擎纹理「基类」指针（RCTexture*）：
            //   - 字体图集纹理由本后端在 UpdateTexture() 中 SetTexID 写入，所有权在 mTextures；
            //   - 其它纹理来自调用方（如 ImGui::Image），调用方须传 RCTexture* 且不转移所有权。
            //     （RCTexture2D 虚继承自 RCTexture，派生类指针不可直接转成基类指针使用。）
            const ImTextureID texId = cmd.GetTexID();
            if (texId == ImTextureID_Invalid)
            {
                continue;
            }
            batch.texture = RCTexturePtr(reinterpret_cast<RCTexture*>((uintptr_t)texId),
                                         [](RCTexture*) { /* 所有权归调用方，这里不释放 */ });

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
    if (!drawData || drawData->CmdLists.Size == 0)
    {
        return;
    }

    // 先跟进 ImGui 的纹理请求（新建 / 增量更新 / 销毁）再绘制：
    // 新增字形会在本帧请求增量上传，必须在上传顶点数据之前完成。
    UpdateTextures(drawData);

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
