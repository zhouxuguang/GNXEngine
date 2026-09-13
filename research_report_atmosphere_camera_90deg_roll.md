# 大气散射 Demo「鼠标/WASD 交互后画面滚转 90°」根因分析

## 摘要

大气散射 demo 一交互就滚转 90° 的问题，直接诱因确实是 commit `18084f92`（"相机不要在 appframework 中初始化"）。该提交移除了 `GLFWRenderWindow` 构造时由引擎预创建的 `MainCamera`，使 demo 的相机从"列表里第二个、被控制器忽略的相机"变成了"唯一且被自动挂载的轨道相机控制器接管的相机"。真正的根因是一个长期潜伏的设计冲突：`SceneManager::CreateCamera()` 会无条件挂上一个 `EditorCameraController`，而该控制器用世界 **Y-up** 重建视图矩阵；大气散射场景的"上"方向是 **+Z**（Earth center = (0,0,-6360)），两者相差约 90°，于是任何一个触发 `ApplyTransform()` 的输入都会让画面整体滚转约 90°。

修复方式：让 demo 完全接管相机（新增 `SceneManager::DestroyCameraController()` 并在 `CreateScene()` 中调用），并在 demo 内补齐鼠标交互（拖拽旋转视线、滚轮缩放距离）。已通过 `cmake --build . --target atmosphere` 编译验证（BUILD SUCCEEDED）。

## 背景

`18084f92` 之前的相机归属是"双相机"结构：

1. `GLFWRenderWindow` 构造函数（普通窗口与外部窗口两个分支）都会 `SceneManager::CreateCamera("MainCamera")` 并 `LookAt((0,0,5),(0,0,0),(0,1,0))` + `SetLens(60,...)`，得到相机 A；
2. demo 的 `CreateScene()` 又调用一次 `CreateCamera("MainCamera")`，得到相机 B；
3. `SceneManager::GetCamera(name)` 返回列表中**第一个**同名相机，所以渲染/`UpdateCameraInfo` 用的是 A；
4. 而 `CreateController()` 在创建 B 时 `delete` 了旧的控制器并把 `mActiveController` 换成绑定到 B 的 `EditorCameraController`；
5. demo 的 `UpdateCamera()` 内部同样用 `GetCamera("MainCamera")` → 摆位的是 A。

结果：demo 摆位 A（画面正确），控制器却只操作没人看的 B。相机基向量的冲突被"重复相机"这个意外掩盖了 —— 交互要么无效，要么作用在一个不参与渲染的相机上，所以看不到滚转。

`18084f92` 删掉了第 1 步，相机列表里只剩 demo 的相机 A，控制器也绑定到 A。此时 demo 的键盘处理与引擎控制器开始同时抢同一台相机，问题暴露。

## 根因分析

### 1. 场景是 Z-up，控制器是 Y-up

`AtmosphereComponent::Initialize()` 里 `mEarthCenter = (0,0,-bottom_radius)`，太阳方向为 `(cos a·sin z, sin a·sin z, cos z)`，都以上 `+Z` 为极轴。相机的摆放同样如此（`AtmosphereFrameWork::UpdateCamera`）：

```
uz = ( sin z·cos a,  sin z·sin a,  cos z)
uy = (-cos z·cos a, -cos z·sin a,  sin z)
position = uz · distance,  target = position - uz,  up = uy
```

代入 demo 默认值 `view_zenith = 1.47`、`view_azimuth = -0.1`：

```
uz ≈ ( 0.9900, -0.0993,  0.1006)
uy ≈ (-0.1001,  0.0100,  0.9949)   ← 相机真正的“上”几乎就是 +Z
```

`uy` 与 `uz` 正交，构造上是严格的侧向基向量。

### 2. 控制器把 up 换成了 (0,1,0)

`EditorCameraController::SyncFromCamera()` 先从相机反解轨道参数：

```
mYaw   = atan2(offset.x, offset.z)   // offset = position - target = uz
mPitch = asin(offset.y)
```

`ApplyTransform()` 再用 `up = (0,1,0)` 重建：

```
position = focus + (cos p·sin y, sin p, cos p·cos y) · distance
camera->LookAt(position, focus, (0,1,0))
```

视线方向 `-uz` 不变，但相机的 up 从 `uy` 变成了世界 Y。屏幕上的"上"是视图矩阵 `CreateLookAt` 通过 `s = f × up`、`u = s × f` 正交化得到的，所以 up 变了多少，画面在屏幕空间就滚转多少。

按默认参数计算屏幕 up：

```
demo up（投影后）： uy ≈ (-0.1001, 0.0100, 0.9949)
控制器 up（投影后）： ≈ ( 0.0983, 0.9901, 0.0100)
两者夹角 ≈ arccos(0.010) ≈ 89.4°
```

即约 90° 滚转，与"场景旋转了 90 度（地平线变竖）"的观感完全一致。

### 3. 为什么只在"交互之后"才出现

`SyncFromCamera()` 只记录状态，不调用 `ApplyTransform()`。第一帧 `SceneManager::Update()` 只做同步，相机保持 demo 摆好的姿态。真正调用 `ApplyTransform()` 的入口有四个，全部由输入触发：

| 入口 | 触发条件 |
| --- | --- |
| `OnMouseMoved` → `ApplyTransform()` | 左键或右键拖拽中 |
| `OnMouseScrolled` → `ApplyTransform()` | 滚轮 |
| `Update()` → `ApplyTransform()` | 按住 W/A/S/D/Q/E 任一键 |

因此表现就是"初始画面正常，一拖鼠标/一按 WASD 就整体跳转 90°"，与用户描述逐字吻合。

### 4. 键盘为什么双重驱动

除了控制器，`AtmosphereFrameWork::OnKeyPressed` 自己也在处理 W/S（改 `mViewZenith`）与 A/D（改 `mViewAzimuth`）并调用 `UpdateCamera()`（用正确的 Z-up）。`AppFrameWork::OnEvent` 又必然把事件转给 `SceneManager::OnEvent` → `mActiveController->OnEvent`。两个处理器在同一帧内先后写同一台相机：demo 写对，随后控制器在 `sceneManager->Update()` 里用 Y-up 覆盖，于是正确结果被冲掉。

## 结论：是不是 18084f92 导致的

是，但它的角色是"触发器/暴露者"而非原始缺陷：

* 在 `18084f92` 之前，引擎预创建的 `MainCamera` 让渲染用的相机天然落在 `GetCamera()` 返回的第一个相机上，而控制器绑定在与渲染无关的第二个同名相机上，Y-up/Z-up 冲突被掩盖，用户看不到滚转（代价是鼠标交互本身也没作用在画面相机上）。
* `18084f92` 移除引擎侧相机后，demo 的相机成为唯一相机、同时成为控制器的目标，冲突立即显现。工作区里 `CreateCamera` → `GetCamera` 去重的改动不改变这一点。

若只回滚该提交而不处理设计冲突，问题会退化为"交互无效 + 重复相机"的旧状态，而不是真正修好。

## 修复

1. `SceneManager` 新增 `DestroyCameraController()`（解绑并销毁 `mActiveController`；`Update()` 与 `OnEvent()` 都已有空指针判断，无需额外改动）。这给"自行管理相机"的 demo 一个正式的退出通道 —— 引擎默认的轨道相机对 Y-up 场景仍然是最合适的选择，其它 demo（terrain/ssao/lumen/nanite/meshlet/ssr 等）都依赖它，不应改动其默认行为。
2. `AtmosphereFrameWork::CreateScene()` 在拿到相机后调用 `sceneManager->DestroyCameraController();`，demo 从此独占相机。
3. 补齐鼠标交互（此前鼠标只是被引擎控制器"顺带"响应）：左/右键拖拽 = 旋转视线（`mViewAzimuth -= dx·0.001`、`mViewZenith -= dy·0.001`，灵敏度与 `EditorCameraController::mRotateSpeed` 一致，天顶角夹取到 `[0, π/2]`）；滚轮 = 缩放距离（`mViewDistance *= 1 - Δ·0.0005`，夹取到 `[1, 200]`，与 ImGui 滑块范围一致）。
4. 健壮性处理：拖拽首帧只对齐鼠标位置不做旋转，避免按下瞬间跳变；当鼠标移动被 ImGui 面板捕获（事件被 `handled` 而不派发给场景）时仍同步一次光标位置并复位对齐标志，避免拖拽经过面板后累积出一次大跳变。

## 验证

* `cmake --build . --target atmosphere -j 8` → **BUILD SUCCEEDED**（含 `SceneManager`、`GNXEngine` 重编译与链接），`read_lints` 无新增诊断。
* 交互手感（拖拽方向、缩放步长）无法在无人值守环境下实测，方向与灵敏度按 `EditorCameraController` 的既有约定选取；若希望上下拖拽方向相反，只需翻转 `mViewZenith` 那一行的符号。

## 涉及文件

* `Engine/Runtime/RenderSystem/include/SceneManager.h`：新增 `DestroyCameraController()` 声明与说明。
* `Engine/Runtime/RenderSystem/source/SceneManager.cpp`：实现 `DestroyCameraController()`。
* `demo/atmosphere/AtmosphereFrameWork.h`：鼠标事件回调声明、鼠标状态成员、`MouseEvent.h` 头文件。
* `demo/atmosphere/AtmosphereFrameWork.cpp`：解绑控制器、鼠标事件分发与处理、面板提示文案。

## 局限

* 未在真实窗口中进行交互实测（仅编译验证 + 逐帧推演），拖拽方向的"手感"可能需要一次手动确认。
* 其它 demo 若也存在"自管相机且基向量非 Y-up"的情况，需要同样调用 `DestroyCameraController()`；本次仅审计并修改了大气散射 demo。
