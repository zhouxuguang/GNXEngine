//
//  ImGuiRenderer.h
//  GNXEngine
//
//  基于自研 RHI 的 Dear ImGui 渲染后端 + 输入桥接。
//
//  设计要点（详见 doc/ImGuiIntegrationDesign.md）：
//    1) 不使用 ImGui 官方的 glfw/sdl2/vulkan/metal backends，渲染走引擎 RHI，
//       一套实现同时支持 Metal 与 Vulkan 后端。
//    2) 顶点属性按引擎约定分成 3 个独立顶点缓冲（每属性独占一个 buffer）：
//         buffer0 = POSITION (float2) / buffer1 = TEXCOORD0 (float2) / buffer2 = COLOR0 (RGBA8_UNORM)
//    3) 引擎的 IndexBuffer 为 StorageModePrivate 不支持逐帧更新，因此这里把
//       ImDrawList 的索引数据在 CPU 侧展开为顶点序列，使用非索引绘制。
//    4) 输入侧复用引擎统一的 Event 体系（GLFW/SDL2 已统一），
//       当 UI 捕获鼠标/键盘时 OnEvent 返回 true，调用方据此停止向 3D 场景分发。
//

#ifndef GNX_ENGINE_IMGUI_RENDERER_INCLUDE_HJFHJSD
#define GNX_ENGINE_IMGUI_RENDERER_INCLUDE_HJFHJSD

#include "../RSDefine.h"
#include "Runtime/GNXEngine/include/Events/Event.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/RenderCore/include/UniformBuffer.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/RCBuffer.h"
#include "Runtime/RenderCore/include/RenderEncoder.h"

#include <memory>
#include <vector>

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API ImGuiRenderer
{
public:
    ImGuiRenderer();
    ~ImGuiRenderer();

    // 创建 ImGui 上下文与 GPU 资源（管线 / 字体纹理 / 动态顶点缓冲）
    bool Initialize(RenderDevice* device, bool installIniFile = false);

    void Shutdown();

    bool IsInitialized() const { return mInitialized; }

    // 每帧开始：更新显示尺寸、帧间隔与 DPI 缩放，并驱动 ImGui::NewFrame()
    void NewFrame(float deltaTime, uint32_t width, uint32_t height);

    // 绘制当前帧的 ImGui 内容（必须在 ImGui::Render() 之后调用）
    void Render(RenderEncoderPtr renderEncoder);

    // 输入事件桥接。返回 true 表示事件已被 UI 消费，调用方应停止向 3D 场景分发。
    bool OnEvent(GNXEngine::Event& e);

    // 供业务侧主动查询 UI 是否正在捕获输入
    bool WantsCaptureMouse() const;
    bool WantsCaptureKeyboard() const;

    // 高分屏缩放：ImGui 逻辑坐标 = 像素坐标 / dpiScale
    void SetDPIScale(float scale) { mDPIScale = scale > 0.0f ? scale : 1.0f; }
    float GetDPIScale() const { return mDPIScale; }

    // 字体图集失效（如动态加载新字体后），下次 NewFrame 时重建纹理
    void InvalidateFontAtlas() { mFontTextureDirty = true; }

private:
    bool CreateDeviceObjects();
    bool CreateFontTexture();
    bool EnsureVertexCapacity(uint32_t vertexCount);
    void ExpandDrawData();
    void SetupStyle();

    struct DrawBatch
    {
        uint32_t firstVertex = 0;
        uint32_t vertexCount = 0;
        RCTexturePtr texture;
        int clipX = 0;
        int clipY = 0;
        int clipW = 0;
        int clipH = 0;
    };

    RenderDevice* mDevice = nullptr;

    GraphicsPipelinePtr mPipeline;
    TextureSamplerPtr   mSampler;
    UniformBufferPtr    mProjUBO;
    RCTexture2DPtr      mFontTexture;

    // 顶点属性分 3 个 buffer（与引擎 Metal/Vulkan 顶点布局约定一致）
    RCBufferPtr mPosBuffer;
    RCBufferPtr mUVBuffer;
    RCBufferPtr mColorBuffer;
    uint32_t    mVertexCapacity = 0;

    // 每帧展开后的顶点数据
    std::vector<float>     mPositions;   // 2 float / 顶点
    std::vector<float>     mUVs;         // 2 float / 顶点
    std::vector<uint32_t>  mColors;      // 打包 RGBA8 / 顶点
    std::vector<DrawBatch> mBatches;

    float    mDPIScale = 1.0f;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    bool     mInitialized = false;
    bool     mFontTextureDirty = true;
    bool     mIniFileEnabled = false;
};

typedef std::shared_ptr<ImGuiRenderer> ImGuiRendererPtr;

NS_RENDERSYSTEM_END

#endif // GNX_ENGINE_IMGUI_RENDERER_INCLUDE_HJFHJSD
