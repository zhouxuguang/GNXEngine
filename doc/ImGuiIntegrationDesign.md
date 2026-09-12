# Dear ImGui 集成设计方案

> 适用范围：GNXEngine 全平台（Metal / Vulkan 双后端）
>
> 平台矩阵：
>
> | 平台 | 窗口系统 | 渲染后端 |
> |---|---|---|
> | Windows / Linux | GLFW | Vulkan |
> | macOS | GLFW | Metal |
> | iOS | SDL2（SDL_Metal_CreateView → CAMetalLayer） | Metal |
> | Android | SDL2（ANativeWindow） | Vulkan |

## 1. 目标与总体思路

在 GNXEngine 中集成 Dear ImGui，作为引擎调试 HUD / 工具面板的统一 UI 层（长期看也可以作为编辑器的一部分补充现有 Qt 编辑器）。

ImGui 后端分两层：

- **平台后端**（窗口事件 / 输入）
- **渲染后端**（绘制 UI 三角形）

总体思路：**官方 GLFW/SDL2 平台后端不采用**，**Metal/Vulkan 官方渲染后端也不采用**。理由与方案如下。

### 1.1 渲染后端：自写一套对接引擎 RHI（推荐方案）

| 对比项 | 官方 Vulkan/Metal 渲染后端 | 自写 RHI 渲染后端（本方案） |
|---|---|---|
| 代码量 | 少 | 约 400 行，一次性 |
| 双后端 | 两套初始化：Vulkan 要自管 descriptor pool、render pass 兼容、swapchain resize；Metal 要 encoder/纹理管理 | **一套代码**，后端差异由 RHI 吸收 |
| 引擎侵入 | 需要给 `MTLRenderEncoder.h:178` / `VKRenderEncoder.h:102` / `VKRenderDevice.h:115` 加原生句柄 getter | **零侵入** |
| 管线一致性 | 绕过引擎管线，与 FrameGraph / PostProcessing 时序易冲突 | 走引擎 encoder，天然在同一命令流 |

引擎 RHI 已具备 ImGui 全部所需能力（调研确认）：

| ImGui 需求 | 引擎现有支持 |
|---|---|
| Alpha 混合 | `ColorAttachmentDesc` 默认即标准 alpha blend；`RenderDescriptor.h:287` 另有 `GetPreMultilyAlphaBlendDes()`（匹配 ImGui 预乘字体图集） |
| 顶点格式 pos(float2) + uv(float2) + col(u8x4 归一化) | `VertexFormatFloat2` + `VertexFormatUChar4Norm`（`RenderDefine.h:372/394`） |
| 每帧动态顶点/索引缓冲 | `RCBuffer(StorageModeShared)` + `Map()/Unmap()`（范例：`demo/meshlet/MeshletFrameWork.cpp:196`） |
| 字体图集上传 / 重建 | `RCTexture2D::ReplaceRegion`（`RCTexture.h:94`；范例：`RenderSystem/source/ImageTextureUtil.cpp:172`） |
| 裁剪 | `RenderEncoder::SetScissorRect`（`RenderEncoder.h:302`） |

> 注：RHI 层没有公开 `SetViewport`（viewport 在 encoder 创建时按 renderRegion 自动设置）。ImGui 不需要它——用正交投影把屏幕坐标映射到 clip space，全屏 viewport 即可，裁剪交给 `SetScissorRect`。

### 1.2 平台后端：不用官方 glfw/sdl2 后端，桥接引擎统一事件体系

官方做法是 `imgui_impl_glfw`（桌面）+ `imgui_impl_sdl2`（移动）两套。但引擎已有别人没有的优势：

**GLFW 与 SDL2 的输入在引擎里已经是同一套 Event 体系**（`MouseMovedEvent` / `MouseButtonPressedEvent` / `KeyPressedEvent`…）：

- GLFW 回调：`Engine/Runtime/GNXEngine/source/GLFWRenderWindow.cpp:210-282`
- SDL 轮询已把触摸统一映射为鼠标事件：`Engine/Runtime/GNXEngine/source/SDLRenderWindow.cpp:266-308`
  - `SDL_FINGERDOWN` → `MouseButtonPressedEvent(Button0)`
  - `SDL_FINGERMOTION` → `MouseMovedEvent(x, y)`（tfinger 归一化坐标已换算为窗口坐标）

因此 ImGui 输入桥接只需写**一份**（`ImGuiInputBridge`），挂在引擎事件分发处，即可全平台复用；触摸天然就是"鼠标"（ImGui `io.ConfigInputTouch` 默认开启）。

不用官方 `imgui_impl_sdl2` 的三个具体原因：

1. **Hint 冲突**：`SDLRenderWindow.cpp:97-98` 设置了 `SDL_HINT_TOUCH_MOUSE_EVENTS = "0"`（防止触摸合成鼠标事件污染引擎输入），而 ImGui SDL2 后端依赖触摸→鼠标合成。
2. **事件所有权**：`ImGui_ImplSDL2_ProcessEvent` 要插进 `SDL_PollEvent` 循环，与 `HandleSDLEvents` 的翻译层形成两套并存。
3. **多后端多维护**：官方 glfw/sdl2 后端各自处理生命周期、剪贴板、屏幕键盘，行为不一致。

## 2. 总体架构

```
                    ┌─ 桌面: GLFW 回调 ──┐
   窗口层           │                    ├─→ 统一 Event 体系 ─→ ImGuiInputBridge ─→ ImGuiIO
                    └─ 移动: SDL2 轮询 ──┘   (触摸已映射为鼠标)      （一份代码，全平台）

   渲染层           ImGuiRenderer（自写 RHI 渲染后端，一份代码）
                    ├─ Metal  (macOS/iOS)     ← msl_macos / msl_ios
                    └─ Vulkan (Win/Linux/Android) ← spirv
                    插入点: RenderPresentPass 后处理之后（全平台同一处）
```

## 3. 实施步骤

### 3.1 引入源码（ThirdParty 组织方式）

```
ThirdParty/imgui/imgui/            # Dear ImGui 核心源码（不含 backends）
ThirdParty/imgui/CMakeLists.txt    # STATIC 库：imgui.cpp, imgui_draw.cpp, imgui_tables.cpp, imgui_widgets.cpp, imgui_demo.cpp
```

- 在 `ThirdParty/CMakeLists.txt` 中 `add_subdirectory(imgui imgui)`
- 不引入 `imgui_impl_glfw.cpp` / `imgui_impl_sdl2.cpp` / `imgui_impl_vulkan.cpp` / `imgui_impl_metal.cpp`（全部由本方案替代）
- imgui 核心库不链接 SDL/GLFW 头，依赖干净

### 3.2 ImGui Shader

新建 `Engine/Shader/built-in/ImGui.shader`，走引擎标准 shader_compile 管线：

```hlsl
cbuffer ImGuiCB : register(b0)
{
    float4x4 proj;                 // 正交投影：屏幕坐标 -> clip space
};

struct VS_IN  { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 col : COLOR0; };
struct VS_OUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 col : COLOR0; };

Texture2D fontTex;
SamplerState fontTexSam;

[shader("vertex")]
VS_OUT VS(VS_IN i)
{
    VS_OUT o;
    o.pos = mul(float4(i.pos, 0.0, 1.0), proj);
    o.uv  = i.uv;
    o.col = i.col;
    return o;
}

[shader("pixel")]
float4 PS(VS_OUT i) : SV_Target0
{
    return i.col * fontTex.Sample(fontTexSam, i.uv);
}
```

编译（与现有 built-in shader 相同流程）：

```bash
shader_compile Engine/Shader/built-in/ImGui.shader -a -f spirv     -o data_asset/Shader/ImGui.spirv.gnxasset
shader_compile Engine/Shader/built-in/ImGui.shader -a -f msl_macos -o data_asset/Shader/ImGui.msl_macos.gnxasset
shader_compile Engine/Shader/built-in/ImGui.shader -a -f msl_ios    -o data_asset/Shader/ImGui.msl_ios.gnxasset
```

运行时 `LoadShaderAsset("ImGui")` 自动按当前后端选择格式。

### 3.3 渲染后端 `ImGuiRenderer`

```
Engine/Runtime/RenderSystem/include/UI/ImGuiRenderer.h
Engine/Runtime/RenderSystem/include/UI/ImGuiRenderer.cpp
```

接口设计：

```cpp
class ImGuiRenderer
{
public:
    bool Initialize(RenderDevice* device, bool installIniFile = false);  // 创建管线 + 加载字体
    void NewFrame(float deltaTime, uint32_t width, uint32_t height);     // 驱动 ImGui::NewFrame()
    void Render(RenderEncoderPtr encoder);                              // 纹理协议 + 每帧绘制
    void Shutdown();
    bool OnEvent(GNXEngine::Event& e);                                  // 输入桥接

private:
    GraphicsPipelinePtr mPipeline;               // alpha blend, CullNone, 无深度测试
    RCBufferPtr mPosBuffer, mUVBuffer, mColorBuffer;  // 3 个独立顶点缓冲，按需扩容、每帧 Map/Unmap
    std::unordered_map<ImTextureData*, RCTexture2DPtr> mTextures;  // ImGui 按需创建的字集纹理由后端持有
    UniformBufferPtr mProjUBO;
};
```

`Render()` 内部流程（标准 ImGui 模板）：

```
ImDrawData* drawData = ImGui::GetDrawData();
每帧 Map 顶点/索引缓冲 -> memcpy -> Unmap       // StorageModeShared
encoder->SetGraphicsPipeline(mPipeline);
encoder->SetFragmentUniformBuffer("ImGuiCB", mProjUBO);
for (each drawList):
    encoder->SetVertexBuffer(...);
    encoder->SetFragmentTextureAndSampler("fontTex", texture, sampler);
    for (each cmd):
        encoder->SetScissorRect(clipRect 转换到像素坐标);
        encoder->DrawIndexedPrimitives(...);
```

关键实现细节：

- **顶点/索引缓冲重建策略**：预分配按需增长（官方后端做法），容量不足时销毁重建
- **in-flight 帧数**：参考 `RenderSystem/source/terrain/QuadTreeTerrain.cpp:467` 的 N 份缓冲模式；Vulkan 多帧并行提交时会踩同一缓冲
- **非字体纹理**（ImGui 图像按钮）：用 `ImTextureID` 存 `RCTexture2D*`，渲染时查表
- **Y 翻转**：Metal 与 Vulkan clip space Y 方向相反。走 RHI 统一入口，初始化时按 `device->GetDeviceType()` 分支一次（Vulkan：`proj[1][1] *= -1`）

**动态纹理协议（ImGui 1.92，`ImGuiBackendFlags_RendererHasTextures`）**：

1.92 起字体纹理由 ImGui 在运行时按需请求创建/更新/销毁，后端不再一次性构建图集。
`Initialize()` 声明能力标志，`Render()` 在绘制前逐帧跟进：

```cpp
io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;   // Initialize()
...
UpdateTextures(drawData);   // Render() 开头（早退判断之后、展开顶点之前）
ExpandDrawData();
```

`UpdateTexture()` 按 `ImTextureData::Status` 分派：

| 状态 | 处理 |
|---|---|
| `WantCreate` | 按 `Width/Height` 创建 `kTexFormatRGBA8` 纹理，全量上传 `GetPixels()`（行距 `GetPitch()`），`SetTexID()` 写入引擎纹理指针 |
| `WantUpdates` | 遍历 `Updates[]`（`ImTextureRect{x,y,w,h}`）逐块 `ReplaceRegion` 子矩形上传，数据取 `GetPixelsAt(x,y)` |
| `WantDestroy` | `UnusedFrames >= 2` 时释放引擎纹理，`SetTexID(ImTextureID_Invalid)` 并置 `Destroyed` |
| `OK` | 跳过（绝大多数帧仅 1 项且为此状态，开销可忽略） |

关键约定与约束：

- **`ImTextureID` 存放 `RCTexture*`（基类指针）**。因 `RCTexture2D : virtual public RCTexture` 为虚继承，
  必须用隐式向上转型取得基类地址；若把 `RCTexture2D*` 直接 `reinterpret_cast` 成 `RCTexture*`，
  渲染侧 `dynamic_pointer_cast<MTLTextureBase>` 会失败并在空指针上崩溃。
- 字体图集纹理的所有权在 `mTextures`（`unordered_map<ImTextureData*, RCTexture2DPtr>`）；
  绘制时把 `ImDrawCmd::GetTexID()` 还原为**不接管所有权**的别名 `shared_ptr` 交给编码器。
- `Shutdown()` 必须在 `ImGui::DestroyContext()` **之前**遍历 `GetPlatformIO().Textures`，
  对 `RefCount == 1` 的纹理调用 `DestroyTexture`。
- ImGui 保证增量更新只写入「从未被使用过」的区域，因此无需读回原内容做混合。
- 绘制遍历使用 `ImDrawData::CmdLists.Size`（`CmdListsCount` 已被官方标记 obsolete，
  且 1.92.9 曾出现其恒为 0 的回归，会导致 UI 完全不渲染）。
- 实现对照：官方 `backends/imgui_impl_metal.mm` 的 `ImGui_ImplMetal_UpdateTexture/DestroyTexture`。

### 3.4 输入桥接 `ImGuiInputBridge`

挂在 `AppFrameWork::OnEvent` 事件分发处：

```cpp
bool ImGuiInputBridge::OnEvent(Event& e)
{
    ImGuiIO& io = ImGui::GetIO();
    EventDispatcher d(e);

    d.Dispatch<MouseMovedEvent>([&](MouseMovedEvent& ev){
        io.AddMousePosEvent(ev.GetX() * mScale, ev.GetY() * mScale);
        return io.WantCaptureMouse;
    });
    d.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& ev){
        io.AddMouseButtonEvent(ToImGuiButton(ev.GetMouseButton()), true);
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);  // 触摸来源
        return io.WantCaptureMouse;
    });
    d.Dispatch<MouseScrolledEvent>([&](MouseScrolledEvent& ev){
        io.AddMouseWheelEvent(ev.GetXOffset(), ev.GetYOffset());
        return io.WantCaptureMouse;
    });
    d.Dispatch<CharTypedEvent>([&](CharTypedEvent& ev){
        io.AddInputCharacter(ev.GetKeyCode());
        return io.WantTextInput;
    });
    // KeyPressed / KeyReleased 同理
    return false;
}
```

返回 `io.WantCaptureMouse` / `io.WantCaptureKeyboard` 使 UI 聚焦时不再向场景转发事件（按需选择是否拦截）。

### 3.5 插入点

`Engine/Runtime/RenderSystem/source/DeferredSceneRenderer.cpp` 的 `RenderPresentPass`（约 :529-559）：

```cpp
mPostProcessing->Process(renderEncoder);   // :554  后处理完成
mImGuiRenderer->Render(renderEncoder);     // ← 插这里，同一个 encoder 只切管线
renderEncoder->EndEncode();                // :556
commandBuffer->PresentFrameBuffer();       // :558
```

复用同一个 encoder，无同步开销，UI 永远画在最终画面上。

### 3.6 Demo 使用示例（atmosphere 调试面板）

```cpp
// 初始化
ImGui::CreateContext();
mImGuiInputBridge.Initialize();            // 挂事件回调
mImGuiRenderer.Initialize(device);

// 每帧
mImGuiRenderer.NewFrame(dt, w, h);
ImGui::NewFrame();
ImGui::Begin("Atmosphere Params");
ImGui::SliderFloat("exposure", &exposure, 0.1f, 20.0f);
ImGui::SliderFloat3("sun dir", &sunDir.x, -1.0f, 1.0f);
ImGui::End();
ImGui::Render();
```

## 4. 现有事件体系需要补齐的缺口

| ImGui 需要 | 引擎现状 | 动作 |
|---|---|---|
| 鼠标位置/按钮/拖动 | ✅ 已有（触摸已覆盖） | 无 |
| 滚轮 | GLFW 有 `MouseScrolledEvent`；SDL 端未处理 `SDL_MOUSEWHEEL` | 可顺手在 SDL 轮询中补齐（移动端无滚轮，非必需） |
| **字符输入**（InputText 必需） | ❌ 无 `CharTypedEvent` | GLFW 加 `glfwSetCharCallback`；SDL 加 `SDL_TEXTINPUT` 处理；新增统一 `CharTypedEvent` |
| 修饰键状态 | `KeyPressedEvent(key, 0)` modifiers 传 0 | SDL 的 `key.keysym.mod` / GLFW 的 `mods` 透传，ImGui 快捷键（Ctrl+C/V）需要 |
| 屏幕键盘（移动端 InputText） | 无 | 聚焦时 `SDL_StartTextInput()` / 失焦 `SDL_StopTextInput()`（监听 `io.WantTextInput` 变化触发） |

## 5. 移动端专属注意事项

### 5.1 坐标空间一致性（HiDPI）

现状问题：`SDL_FINGERMOTION` 用 `SDL_GetWindowSize`（**逻辑点**）换算（`SDLRenderWindow.cpp:299-308`），而 `SDL_WINDOWEVENT_RESIZED` 用 `SDL_GetWindowSizeInPixels`（**像素**）更新 `mData.width/height`。ImGui 要求 `io.DisplaySize` 与鼠标坐标**同一空间**。

建议统一用**像素空间**：

- `io.DisplaySize` = framebuffer 像素尺寸
- 触摸坐标：`tfinger.x * pxW`（用 `SDL_GetWindowSizeInPixels`，不是 `SDL_GetWindowSize`）
- `io.FontGlobalScale` 设为 dpiScale，或字体图集直接按 2x 构建（`ImFontConfig::RasterizerFlags`）

### 5.2 生命周期（Android surface 重建）

`SDL_WINDOWEVENT_RESTORED` 时引擎重建 VkSurfaceKHR + swapchain（`SDLRenderWindow.cpp:344-368`）。ImGui 资源（字体纹理、管线、顶点缓冲）都在 **VkDevice 上，不随 surface 销毁**，因此**不需要重建 ImGui 资源**——这是走 RHI 后端的又一个好处（官方 Vulkan 后端在此场景要手动处理 descriptor/swapchain 关联）。

只需保证 `mAppActive == false`（窗口 MINIMIZED / APP_WILLENTERBACKGROUND）期间跳过 `NewFrame/Render`。

## 6. 已知风险点

1. **UBO 自动 patch 成 push constant**：`Engine/Runtime/ShaderCompiler/source/ShaderCompiler.cpp:558` 会把 ≤256B 的 UBO patch 成 push constant。ImGui 投影矩阵 UBO 为 64B，会命中此路径——Metal 侧 MSL 转换正常，Vulkan 侧需确认 push constant 在管线创建时正确传递。
   - 备选方案：把投影矩阵放在顶点缓冲开头（官方 Metal 后端 `setVertexBytes` 等价做法），彻底绕开。
   - 参考：sky pass 的 `AtmosphereViewCB` 144B 未触发，此 UBO 会触发。
2. **Y 翻转**：见 3.3。
3. **预乘 alpha**：字体图集是预乘格式，若使用 `GetPreMultilyAlphaBlendDes()`（src=One, dst=OneMinusSrcAlpha），shader 中不再额外乘 alpha；若用标准 blend（src=SrcAlpha, dst=OneMinusSrcAlpha），`ImFontConfig` 需设置 `ImGuiBackendFlags_RendererHasTextures` 相关逻辑对齐（详见实现时验证）。

## 7. 改动文件清单（预估）

| 文件 | 类型 | 内容 |
|---|---|---|
| `ThirdParty/imgui/imgui/*` | 新增 | ImGui 核心源码（子模块或直接拷贝） |
| `ThirdParty/imgui/CMakeLists.txt` | 新增 | 库定义（**统一 STATIC**，唯一副本由引擎库承载，见第 8 节） |
| `ThirdParty/CMakeLists.txt` | 修改 | `add_subdirectory(imgui)` |
| `Engine/Shader/built-in/ImGui.shader` | 新增 | UI shader（pos+uv+color） |
| `Engine/Runtime/RenderSystem/include/UI/ImGuiRenderer.h/.cpp` | 新增 | RHI 渲染后端 |
| `Engine/Runtime/RenderSystem/source/DeferredSceneRenderer.cpp` | 修改 | RenderPresentPass 插入 UI pass |
| `Engine/Runtime/RenderSystem/include/.../DeferredSceneRenderer.h` | 修改 | 持有 ImGuiRenderer |
| `Engine/Runtime/GNXEngine/include/Events/KeyEvent.h`（或新文件） | 修改/新增 | `CharTypedEvent` |
| `Engine/Runtime/GNXEngine/source/GLFWRenderWindow.cpp` | 修改 | `glfwSetCharCallback` + 修饰键透传 |
| `Engine/Runtime/GNXEngine/source/SDLRenderWindow.cpp` | 修改 | `SDL_TEXTINPUT` + 触摸坐标改像素空间（可选 `SDL_MOUSEWHEEL`） |
| `Engine/Runtime/GNXEngine/source/AppFrameWork.cpp` | 修改 | `ImGuiInputBridge` 挂载 + NewFrame 时机 |
| demo（如 atmosphere） | 修改 | 调试面板示例 |

---

## 8. 实施记录（已完成）

### 8.1 实际落地方式

| 文件 | 内容 |
|---|---|
| `ThirdParty/imgui/imgui/` | 仅 4 个 .cpp（imgui / imgui_draw / imgui_tables / imgui_widgets）+ 头文件，**不含 backends/docs/examples/misc** |
| `ThirdParty/imgui/CMakeLists.txt` | **统一 STATIC**（`libimgui.a`），另提供 `imgui_headers` 接口目标，详见 8.2 |
| `Engine/Shader/built-in/ImGui.shader` | 顶点属性 3 个独立 buffer（POSITION/TEXCOORD0/COLOR0），输出线性色的 `col * tex` |
| `Engine/Runtime/RenderSystem/include/UI/ImGuiRenderer.{h,cpp}` | RHI 渲染后端 + 输入桥接（约 500 行） |
| `Engine/Runtime/RenderSystem/include/SceneManager.h` + `.cpp` | 持有 ImGui 层，`GetImGuiRenderer()` 按需创建 |
| `Engine/Runtime/RenderSystem/source/DeferredSceneRenderer.cpp` | `RenderPresentPass` 中后处理之后绘制 UI |
| `Engine/Runtime/GNXEngine/include/AppFrameWork.h` + `.cpp` | `SetImGuiEnabled()` / `GetImGui()` / `UpdateImGuiFrame()` / 事件优先拦截 |
| `Engine/Runtime/GNXEngine/include/RenderWindow.h` | 新增 `GetDPIScale()`（GLFW/SDL 各自实现） |
| `demo/atmosphere/AtmosphereFrameWork.cpp` | ImGui 调参面板（曝光/太阳/相机/LUT 重数/场景几何） |

### 8.2 关键坑：ImGui 全局上下文重复（静态库下的「唯一副本」约束）

**现象**：`ImGui::GetIO()` 断言 `No current context`，即使 `CreateContext()` 已成功调用。

**根因**：`GImGui` 是**每个镜像（dylib / exe）各自一份**的静态变量。若 imgui 静态库被
「引擎动态库」与「可执行文件」分别链接，两者各持一份上下文：`CreateContext()` 设置的是
引擎 dylib 里那份，而 demo 可执行文件中的 `ImGui::GetIO()` 读的是自己那份（仍为 null）。

**现行方案（imgui 统一编译为静态库）**：由 GNXEngine 共享库承载**唯一副本**并导出全部 ImGui 符号。

| 角色 | 做法 |
|---|---|
| `ThirdParty/imgui` | `add_library(imgui STATIC ...)`；另提供 `imgui_headers`（INTERFACE，仅头文件） |
| `GNXEngine`（唯一宿主） | **整库加载**并入静态库：APPLE `-Wl,-force_load,<libimgui.a>`、MSVC `/WHOLEARCHIVE:`、其它 `-Wl,--whole-archive` |
| 可执行文件（demo 等） | **不链接 `imgui` 静态库**，只链接 `imgui_headers`，ImGui 符号从引擎库导入 |
| 移动端（iOS/Android） | 整体静态链接为单一二进制，天然只有一份 |

必须**整库加载**而非按需加载：静态库默认只把被引用到的 `.o` 拉进镜像，而 ImGui 的小部件实现
（`imgui_widgets.cpp`：`Text` / `Button` / `SliderFloat` 等）主要由 demo 调用、引擎自身并未引用；
若不整库加载，这些符号不会进入引擎库，可执行文件链接时会报未定义符号。验证方式：

```bash
# 引擎库应导出「仅被 demo 使用」的符号；可执行文件自身不得定义 ImGui 符号（即无第二份）
nm -g build/Debug/libGNXEngined.dylib | grep -E "SliderFloat|ColorEdit3" | head
nm -g build/Debug/atmosphere | grep " T " | grep -c ImGui      # 期望 0
otool -L build/Debug/atmosphere | grep imgui                   # 期望无输出
```

其余 CMake 细节：

1. 工程根 `CMakeLists.txt` 全局设置 `CMAKE_CXX_VISIBILITY_PRESET hidden`，而 imgui 内部没有导出宏；
   若沿用 hidden，并入共享库后不导出任何 ImGui 符号（可执行文件链接报未定义符号），
   因此需为该 target 覆盖 `CXX_VISIBILITY_PRESET default` 并关闭 `VISIBILITY_INLINES_HIDDEN`。
2. 需 `POSITION_INDEPENDENT_CODE ON`（静态库会被并入 dylib / so）。
3. `DEBUG_POSTFIX ""` 统一产物名，避免 Xcode 生成器产物名不一致导致链接失败。
4. Windows 需 `IMGUI_API=__declspec(dllexport)`（PRIVATE，随 DLL 导出）与
   `IMGUI_API=__declspec(dllimport)`（由 `imgui_headers` 以 INTERFACE 传给使用方）；
   macOS / Linux 依靠默认可见性由共享库自动导出，无需额外处理。
5. IDE 分组：`set_property(TARGET imgui PROPERTY FOLDER "ThirdParty")`（与 glfw / ktx /
   meshoptimizer / tbb 等 13 个第三方库一致），并配合
   `source_group(TREE <imgui 目录> PREFIX ThirdParty FILES ...)`，
   使 **目标** 与 **源文件** 都归入 IDE 的 ThirdParty 分组。

### 8.3 事件分发验证（已通过）

在 demo 中注入合成事件得到的实测结果：

| 场景 | 结果 |
|---|---|
| 鼠标位于面板内 → 左键按下 | 事件被 UI 消费（`e.handled=true`），**不再传给 3D 场景** ✓ |
| 鼠标位于面板内 → 左键松开 | 同上 ✓ |
| 鼠标移出面板 → 左键按下 | 事件未被消费，**正常传给 3D 场景** ✓ |

实现要点：`AppFrameWork::OnEvent` 中先 `imgui->OnEvent(e)`，返回 true 时直接 `return`；
`ImGuiRenderer::OnEvent` 内部通过 `EventDispatcher` 消费事件，其返回值会置位 `e.handled`，
因此业务侧（demo）只需判断 `e.handled` 即可跳过自身快捷键处理。

### 8.4 其他实现细节

- **字体/DPI**（ImGui 1.92 动态字体）：`io.DisplaySize` 使用**逻辑坐标**（帧缓冲像素 / `GetDPIScale()`），
  鼠标坐标同为逻辑坐标（GLFW 光标坐标即逻辑点），scissor 换算回像素；
  同时设 `io.DisplayFramebufferScale = (dpiScale, dpiScale)`，框架会据此自动设置当前字形的
  光栅化密度（`g.FontRasterizerDensity = DisplayFramebufferScale`），Retina 下文字自动清晰且命中正确。
  **不要再手工 `io.FontGlobalScale` 缩放，也不要设置 `style.FontScaleDpi`**，否则会与自动密度
  叠加导致字号被二次放大（`style.FontSizeBase` 只填逻辑字号）。
- **顶点索引**：引擎 `IndexBuffer` 为 `StorageModePrivate` 不支持逐帧更新，故 UI 的索引在 CPU 侧
  展开为顶点序列，使用非索引绘制（`DrawPrimitives`），避免每帧重建索引缓冲。
- **顶点缓冲**：3 个 `RCBuffer(StorageModeShared)` 按需扩容，每帧 `Map/Unmap` 更新。
- **UI 时机**：`RenderPresentPass` 中 `mPostProcessing->Process()` 之后、`EndEncode()` 之前，
  复用同一 encoder，UI 永远绘制在最终画面之上。
- **性能**：实测 UI 每帧开销 **< 0.1ms**（大气 demo 场景渲染 ~95ms/帧，UI 占比可忽略）。

### 8.5 中文显示（乱码问题）

**现象**：UI 中的中文显示为方块/乱码。

**原因**：ImGui 内置字体（ProggyClean / ProggyForever）只包含拉丁字形，必须显式加载 CJK 字体文件。

**解决方案**（`ImGuiRenderer::LoadFonts`，ImGui 1.92 动态字体图集）：

1. **加载中文系统字体**：按平台优先级探测并加载第一个存在的字体文件
   - macOS：`PingFang.ttc` → `STHeiti Medium.ttc` → `Hiragino Sans GB.ttc` → `Arial Unicode.ttf`
   - Windows：`msyh.ttc`(微软雅黑) → `simhei.ttf` → `simsun.ttc`
   - Linux：`NotoSansCJK-Regular.ttc` → `wqy-microhei.ttc` → `uming.ttc`
   - 均找不到时回退内置字体并打印告警；也可用 `SetCjkFontPath()` 显式指定。
2. **不再需要字形范围**：1.92 起字形在绘制时按需光栅化、图集随用字增长，
   `GetGlyphRanges*()` 全部废弃、`ImFontGlyphRangesBuilder` 官方评价"不再真正有用"，
   生僻字（如**“曝”**）**无需任何注册**即可显示。
   > 需区分：动态字体免除的是「字形范围声明」，CJK 字体文件本身仍必须显式加载。
3. **图集尺寸**：初始化时设 `TexMinWidth/TexMinHeight = 1024` 以减少动态扩容
   （扩容 = 重新分配 + 拷贝，短时间内新旧尺寸纹理并存）；
   并把 `TexMaxWidth/TexMaxHeight` 与设备 `maxTextureSize2D` 对齐（ImGui 默认 8192）。

> 历史方案（1.91 静态图集，已废弃）：曾用 `ImFontGlyphRangesBuilder` 组合
> 「默认范围 + 约 2500 常用汉字」，并用 `AddGlyphText()` 注册范围外文本；
> 该方案需人工维护注册列表，且「不采用 `GetGlyphRangesChineseFull()`（Retina 下约 268MB）」
> 的结论同样仅适用于静态图集。升级到 1.92.9b 后两者一并移除。

> 升级的可行性、逐文件改造清单、风险登记册与验证结论见
> **[ImGuiUpgradeTo192Assessment.md](./ImGuiUpgradeTo192Assessment.md)**。
