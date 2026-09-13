# 提交 18084f92 后续审计报告：每个 demo 都应自建相机（当前实现状态）

## 摘要

提交 `18084f92`（"engine：相机不要在 appframework 中初始化"）把创建 `MainCamera` 的代码从 `GLFWRenderWindow` 删除，方向正确——窗口层不该持有场景状态。审计与补齐后，**全部 10 个 demo 现在都在自己的 demo 文件里创建并配置 `MainCamera`**：统一的写法是"先 `GetCamera("MainCamera")`，没有才 `CreateCamera`，`LookAt` 只在首次创建时执行一次，`SetLens` 每次 `Resize` 都执行"。同时修掉了 4 类会破坏"demo 正常运行 / 鼠标键盘交互"的问题：编辑器视口失去相机（本次提交引入的回归）、6 个 demo 重复转发输入事件（滚轮缩放翻倍且绕过 ImGui 输入捕获）、ssao/lumen/terrain 在 `Resize` 里重复创建灯光与场景、ssr/nanite/meshlet 在 `Resize` 里重复摆位导致视角被重置。meshshader 原本完全没有相机且从不使用 `SceneManager`，现已补上相机并真正接入渲染（原来算完 view/projection 又被 `MakeIdentity()` 覆盖，是死代码）。所有改动已通过 Xcode Debug 增量编译验证。

## 背景

`18084f92` 的实质改动有三处：`AppFrameWork::OnEvent` 删掉一行事件日志；`GLFWRenderWindow` 删掉两个构造函数里各自的 `CreateCamera("MainCamera") + LookAt((0,0,5)→原点) + SetLens(60, w, h, 0.1, 1000)`，并在 GLFW 按键/鼠标/滚轮/光标回调里补写 `InputState`；`SceneManager::Render` 把延迟渲染分支的 `return` 提前。

三个决定所有结论的引擎事实是：`SceneManager::CreateCamera` **不做重名校验**（只 `mCameras.push_back`）；`GetCamera(name)` 返回列表中**第一个**同名相机；`CreateLight`、`SceneNode::CreateChildSceneNode` 同样不去重。另外 `SceneManager::Update` 取不到 `MainCamera` 时会回退到 `mCameras[0]`，`DeferredSceneRenderer` 对空相机做了判空保护——不会崩，但没有相机 UBO 数据，画面等于没有相机。

`RunLoop()` 的顺序是 `Initlize()` → `Resize(w, h)` → 主循环，桌面、Android（`SDL_main`）、iOS 都走 `AppFrameWork::RunLoop`，所以"在 demo 的 `Resize()` 里建相机"这条路径在所有平台都会被走到一次。

## 逐 demo 现状

| demo | 相机创建位置 | 创建前 GetCamera | LookAt 只执行一次 | 键鼠交互 | 判定 |
| --- | --- | --- | --- | --- | --- |
| pbr | `Resize()`，首次进 `CreateScene()` 摆位 | 是 | 是 | 轨道相机（左/右拖旋转、中键平移、滚轮缩放、WASD/QE、Shift 加速） | 完成 |
| ssao | `Resize()` | 是 | 是 | 同上 | 完成 |
| ssr | `Resize()` + `mSceneCreated` | 是 | 是 | 同上 | 完成 |
| terrain | `Resize()` | 是 | 是 | 同上 | 完成 |
| meshlet | `Resize()` | 是 | 是 | 同上 | 完成 |
| lumen | `Resize()` | 是 | 是 | 同上 | 完成 |
| nanite | `Resize()` | 是 | 是 | 轨道相机 + `Input::IsKeyPressed` 轮询 W/S 直推位置 + ↑/↓ 切 mip | 完成（两种移动方式会互相覆盖，见观察 6） |
| atmosphere | `CreateScene()`（`mSceneCreated` 守卫的 `Resize` 调用） | 是 | 是 | ImGui 面板 + 方向键移太阳 / WASD 改视线，`e.handled` 正确拦截 UI 捕获 | 完成 |
| virtualtexture | `SetupScene()`（`Initlize()` 内只走一次） | 是 | 是 | ImGui 面板 + 轨道相机 | 完成 |
| meshshader | `Resize()`（本次新增） | 是 | 是 | 轨道相机；`RenderFrame()` 现在用相机矩阵 + `SceneManager::Update()` | 完成（画面尺寸变化见下） |

## 修复清单

**1）编辑器视口失去相机（本次提交引入的回归）。** `Engine/Editor/GNXEditor/Viewport/EditorRenderHost.cpp` 只做 `scene->Update(dt); scene->Render(nullptr);`，整个 `Engine/Editor` 下不存在任何 `CreateCamera`，被删的第二段代码恰好就在 `GLFWRenderWindow(props, externalWindowHandle)` 这个 Qt 专用分支里。已增加 `EnsureSceneCamera()`：`Attach()` 成功后按被删除的默认视角创建一次 `MainCamera`，窗口尺寸变化只更新投影。

**2）6 个 demo 把输入事件转发了两次。** `AppFrameWork::OnEvent` 内部已经 `SceneManager::OnEvent(e)`，ssao/pbr/ssr/terrain/lumen/nanite 又各自再调一次。逐事件看：鼠标移动（第二次 dx=0）、按键按下/抬起（幂等）无害，但**滚轮缩放会被应用两次（速度翻倍）**，更严重的是"ImGui 捕获输入就 return"的保护被绕过——将来这些 demo 打开 ImGui 时，拖面板会同时拖相机。已统一为只调基类，atmosphere 原本就正确（`e.handled` 判断），保持不变。

**3）ssao/lumen/terrain 在 `Resize()` 里重复做一次性初始化。** 每次窗口尺寸变化都会新建 `mainLight`、重新加载 dragon.obj/贴图、重建整棵四叉树地形与天空盒。因为 `CreateLight` 不去重，拖一下窗口就会叠加多份灯光（越来越亮）并重复占显存。已按 pbr/ssr/atmosphere 已有的 `mSceneCreated` 写法加守卫，并把这些 demo 的 `LookAt` 也收进"首次创建相机"这一步。

**4）ssr / nanite / meshlet 的 `LookAt` 在 `Resize()` 里无条件执行。** 同样因为 `Resize` 会被反复调用，用户拖一下窗口视角就被拉回初始值。已改为只在首次创建相机时摆位，`SetLens` 保持每次更新。

**5）meshshader 补上相机并真正接入渲染。** 它原本既没有相机，`RenderFrame()` 也不调 `SceneManager::Update/Render`；UBO 里先算了 `CreateLookAt`/`CreatePerspective` 又立刻 `MakeIdentity()` 覆盖，是死代码，而 shader 侧 `MeshShaderDemo.shader` 明确写的是 `mul(mvp, v.position)` 且注释顶点为 object space。现在：`Resize()` 中创建 `MainCamera`（沿用原死代码的参数：eye `(0,0,-5)`、目标原点、FOV 60、near 0.1、far 100），`RenderFrame()` 中先 `SceneManager::Update(deltaTime)` 驱动相机控制器，再把 `camera->GetViewMatrix()/GetProjectionMatrix()`（引擎统一约定）写进 UBO，无相机时回退 identity。副作用是三角形从"裁剪空间直出的接近满屏"变成"object space + 真实相机"的居中约 20% 大小；若想恢复原来的满屏观感，把 `LookAt` 的 `(0,0,-5)` 改成 `(0,0,-1)` 即可（三角形无背面剔除，`GraphicsPipelineDesc::cullMode` 默认 `CullModeNone`，不存在翻转被剔除的风险）。

**6）其余观察（未改动）。** nanite 同时用轨道相机与 `Input::IsKeyPressed` 直推相机位置：`SetPosition` 改了位置，但控制器内部 focus/distance 仍是首帧 `SyncFromCamera` 的值，下一次拖拽会把相机拉回旧轨道，建议二选一。lumen 的 `LoadGeometryData` 里第二个 `MeshRenderer` 加到了 `node1`（`node2` 没有渲染组件），疑为笔误。另外 `Input::IsKeyPressed` 在默认 `Auto` 模式下每次调用都会 `PollFromGLFW` 覆盖事件写入的状态，所以提交中在 GLFW 回调里补写 `InputState` 对桌面路径基本冗余，真正受益的是 `InputMode::Event`（Qt 编辑器）路径。

## 验证

用仓库内已有 Xcode 工程增量编译（`cmake --build build --config Debug --target ...`），涉及改动的目标全部通过：meshshader、meshlet、ssr、nanite、pbr、ssao、terrain、lumen、atmosphere、virtualtexture、GNXEditor 均 `** BUILD SUCCEEDED **`，改动文件无新增诊断信息。图形表现需人工确认：pbr / meshshader / meshlet 的轨道相机与滚轮缩放；拖动窗口后 ssao / terrain 的灯光不再叠加、视角不再被重置；virtualtexture / atmosphere 拖面板时相机不动、滚轮只缩放一次。

## 结论

约定已固化为四条：窗口与 `AppFrameWork` 不创建任何场景对象；每个 demo 在首次初始化时创建并摆好 `MainCamera`（先 `GetCamera` 再 `CreateCamera`）；`Resize()` 只做 `SetLens`（加必要的资源重建），不再做一次性初始化；`OnEvent()` 只调一次基类，由基类统一完成 ImGui 捕获判断与向相机控制器转发。

## 参考

1. [demo/meshshader/MeshShaderFrameWork.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/demo/meshshader/MeshShaderFrameWork.cpp)
2. [Engine/Shader/built-in/MeshShader/MeshShaderDemo.shader](file:///Users/zhouxuguang/work/mycode/GNXEngine/Engine/Shader/built-in/MeshShader/MeshShaderDemo.shader)
3. [demo/ssao/SSAOFrameWork.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/demo/ssao/SSAOFrameWork.cpp)
4. [demo/lumen/LumenFrameWork.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/demo/lumen/LumenFrameWork.cpp)
5. [demo/terrain/TerrainFrameWork.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/demo/terrain/TerrainFrameWork.cpp)
6. [demo/pbr/PBRFrameWork.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/demo/pbr/PBRFrameWork.cpp)
7. [demo/ssr/SSRFrameWork.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/demo/ssr/SSRFrameWork.cpp)
8. [demo/meshlet/MeshletFrameWork.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/demo/meshlet/MeshletFrameWork.cpp)
9. [demo/nanite/NaniteFrameWork.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/demo/nanite/NaniteFrameWork.cpp)
10. [demo/virtualtexture/VTFrameWork.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/demo/virtualtexture/VTFrameWork.cpp)
11. [demo/atmosphere/AtmosphereFrameWork.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/demo/atmosphere/AtmosphereFrameWork.cpp)
12. [Engine/Runtime/RenderSystem/source/SceneManager.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/Engine/Runtime/RenderSystem/source/SceneManager.cpp)
13. [Engine/Runtime/RenderSystem/source/EditorCameraController.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/Engine/Runtime/RenderSystem/source/EditorCameraController.cpp)
14. [Engine/Runtime/GNXEngine/source/GLFWRenderWindow.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/Engine/Runtime/GNXEngine/source/GLFWRenderWindow.cpp)
15. [Engine/Editor/GNXEditor/Viewport/EditorRenderHost.cpp](file:///Users/zhouxuguang/work/mycode/GNXEngine/Engine/Editor/GNXEditor/Viewport/EditorRenderHost.cpp)
