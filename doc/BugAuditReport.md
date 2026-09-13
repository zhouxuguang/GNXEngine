# GNXEngine 代码缺陷审计报告（第 1 轮）

> **审计范围**：仓库自有代码（`Engine/**`、`demo/**`、`tool/**`、`unittest/**`），**不含** `ThirdParty/**`。
> **审计方式**：多路并行静态审计（按模块分工）+ 逐条人工复核（读源码确证，剔除误报）+ 编写复现测试。
> **审计日期**：2026-09-13
> **本轮结论**：确证缺陷 **12 条**，全部已修复；其中 **3 条**有确定性复现测试（修复前崩溃/失败 → 修复后通过），
> 其余 9 条为静态确证 + 运行/回归验证。另有 **8 条**已确证但未纳入本轮修复（需 Vulkan/移动端环境或影响面需单独评估），
> 见 §5「待处理清单」。

---

## 1. 缺陷总表

| 编号 | 位置 | 缺陷 | 严重度 | 缺陷类型 | 修复前证据 | 修复后验证 |
|---|---|---|---|---|---|---|
| BUG-01 | `ImageCodec/source/ImageDecoderHDR.cpp:54` | 格式探测忽略 `size` → 短缓冲区**越界读** | 高 | 活代码 | 测试**进程崩溃**（exit=138） | 测试通过 |
| BUG-02 | `MathUtil/source/MathUtil.cpp:161,179` | 查表索引可达 360 → 读 `SinTable[361]` **越界**，`FastCos` **返回 NaN** | 高 | 活代码 | 断言失败：`nanf is within 0.001 of 1.0` | 测试通过 |
| BUG-03 | `RenderSystem/source/Camera.cpp:137` | `SetNearClipDistance` 不重算投影矩阵 | 中 | 活代码 | 断言失败：`REQUIRE_FALSE(MatricesEqual(before, after))` | 测试通过 |
| BUG-04 | `RenderCore/source/metal/MTLVertexBuffer.mm:27` | `GetBufferLength()` 缺 `return`，**恒返回 0** | 中 | 活代码（Metal） | 静态确证 | 编译 + 运行回归 |
| BUG-05 | `RenderCore/source/metal/MTLCommandBuffer.mm:69` | stencil 纹理被赋值成 depthStencil，**参数被丢弃** | 中 | 活代码（Metal） | 静态确证 | 编译 + 运行回归 |
| BUG-06 | `GNXEngine/source/GLFWRenderWindow.cpp:172` | `Shutdown()` 不置空窗口指针 → **析构二次销毁** | 低 | 活代码（桌面） | 静态确证 | 运行回归（terrain / atmosphere） |
| BUG-07 | `GNXEngine/source/SDLRenderWindow.cpp:33` | 字母键判断用 `'A'..'Z'`，而 SDL 给 `'a'..'z'` → **字母键全部失效** | 高 | 活代码（移动端） | 静态确证 | 需 Android/iOS 构建（见 §4.7） |
| BUG-08 | `RenderSystem/source/DeferredSceneRenderer.cpp:678` | Skybox Pass 用**未赋值**的资源成员 `Get(0)` 取资源 | 低（潜伏） | 活代码 | 静态确证；实测因资源 0 巧合未触发断言 | terrain demo 运行正常，无效访问已移除 |
| BUG-09 | `ImageCodec/source/ImageDecoderPng.cpp:17` | PNG 读回调**无边界检查**、不报 EOF → 截断 PNG 越界读 | 高 | **潜伏**（`USE_PNG_LIB` 未启用） | 静态确证 | 见 §4.9 |
| BUG-10 | `ImageCodec/source/ImageDecoderPng.cpp:256` | `IsFormat` 忽略 `size`，固定读 8 字节 | 中 | **潜伏** | 静态确证 | 见 §4.9 |
| BUG-11 | `ImageCodec/source/ImageEncoderPng.cpp:21` | `formatHasAlpha()` 恒返回 false | 中 | **潜伏** | 静态确证 | 见 §4.9 |
| BUG-12 | `ImageCodec/source/ImageEncoderPng.cpp:229` | `transform_scanline_5551()` 只读不写 → 输出未初始化数据 | 中 | **潜伏** | 静态确证 | 见 §4.9 |
| BUG-13 | `BaseLib/CMakeLists.txt:21` | zlib 仅 Windows 链接 → **单元测试目标无法链接**（`test_baselib` / `test_mathutil`） | 中 | 构建 | `Undefined symbols: _compress, _inflate...` | 全量构建通过 + 测试可运行 |
| BUG-14 | `unittest/test_baselib.cpp:699` | 断言 `GetAllocationSize(ptr) == 257` 与 API 语义（可用大小 ≥ 请求值）不符 → **恒失败** | 低 | 测试 | `test cases: 71 \| 70 passed \| 1 failed` | `All tests passed` |

> 「潜伏」= 该代码路径当前构建**未启用**（`USE_PNG_LIB` 全仓未定义，macOS 走 `ImageDecoderPngApple.mm` / `ImageEncoderApple.mm`），
> 但仍属真实缺陷（一旦启用该宏或在其它平台启用即生效），故一并修复。

---

## 2. 复现与回归测试

### 2.1 如何运行

```bash
# 配置（Catch2 由 unittest/CMakeLists.txt 的 FetchContent 提供）
cmake -B build -DENABLE_TESTING=ON

# 构建并运行本轮缺陷的复现 + 回归测试
cmake --build build --config Debug --target test_bugfixes --parallel 8
./build/Debug/test_bugfixes

# 也可只跑某一条
./build/Debug/test_bugfixes "BUG-01*"
```

测试文件：`unittest/test_bugfixes.cpp`，目标：`test_bugfixes`（`unittest/CMakeLists.txt`）。

### 2.2 关键技巧：用「保护页」确定性复现越界读

越界读通常表现为"偶尔崩溃"，依赖内存布局，难以稳定复现。本测试用 `mmap` + `mprotect` 构造：

```
[ 第 1 页 RW，数据 n 字节放在页尾 ] [ 第 2 页 PROT_NONE ]
                                     ↑ 任何超出 n 字节的读取 → 立即崩溃
```

这样无需 ASAN 也能把「越界读」变成 100% 确定性的崩溃，且不依赖堆布局。

### 2.3 实测记录（修复前 → 修复后）

| 用例 | 修复前 | 修复后 |
|---|---|---|
| `BUG-01`（2 字节缓冲，内容为 HDR 魔数前缀 `"#?"`） | **进程崩溃** `exit=138` | All tests passed |
| `BUG-02`（`FastSin/FastCos` 19 个边界角度） | `1 assertion failed: nanf is within 0.001 of 1.0`（`FastCos(-1e-7f)` 返回 NaN） | All tests passed |
| `BUG-03` / `BUG-03b`（Camera 近平面 setter） | `1 assertion failed: REQUIRE_FALSE(MatricesEqual(before, after))` | All tests passed |
| 全套（`ctest --test-dir build -C Debug`） | `test_baselib` 1 failed（BUG-14）、`test_mathutil` / `test_bugfixes` 无法链接或未运行 | **100% tests passed, 0 tests failed out of 3**<br>`test_baselib` All tests passed (3923 assertions in 71 test cases)<br>`test_mathutil` All tests passed (472 assertions in 149 test cases)<br>`test_bugfixes` All tests passed (78 assertions in 4 test cases) |

---

## 3. 缺陷详情（本次修复）

### BUG-01 图像格式探测越界读（`ImageDecoderHDR::IsFormat`）

**原因**
`hdr_test_core()` 按签名逐字节比对，签名 `"#?RADIANCE\n"` 长 11 字节；而 `IsFormat(buffer, size)` **完全忽略 `size`**：

```cpp
// 修复前 Engine/Runtime/ImageCodec/source/ImageDecoderHDR.cpp:54
bool ImageDecoderHDR::IsFormat(const void* buffer, size_t size)
{
    return hdr_test((const uint8_t*)buffer);   // size 未参与判断
}
```

`hdr_test_core` 只有在**首字节就不匹配**时才提前返回，因此**缓冲区内容与签名前缀一致且长度不足**时（例如被截断的 `.hdr` 文件前 2 字节 `"#?"`）会继续读取缓冲区之外的内存。

**修改方案**（最小改动：加长度校验）

```cpp
const size_t kMinHeaderSize = 11;              // "#?RADIANCE\n"
if (nullptr == buffer || size < kMinHeaderSize)
{
    return false;
}
```

**复现与验证**
`unittest/test_bugfixes.cpp` 的 `BUG-01` 用例：保护页缓冲 2 字节、内容填 `'#'`/`'?'`，调用
`ImageDecoder::DecodeMemory(buf, 2, &img)`。
修复前崩溃（`exit=138`，保护页命中）；修复后返回 `false`，测试通过。

> 注：若缓冲内容首字节不匹配（如全 `0xCD`），函数会立即返回而**不会**越界 —— 这也是本缺陷容易被漏测的原因。

---

### BUG-02 `FastSin/FastCos` 查表越界并返回 NaN

**原因**
角度归一化后，`fValue` 可能因浮点舍入**恰好等于 360.0**（例如极小负数 `-1e-7f`：`fmod(-1e-7,360) = -1e-7`，
`< 0` 触发 `+= 360.0`，在 `float` 精度下 `360 + (-1e-7) == 360.0f`）。于是：

```cpp
// 修复前 Engine/Runtime/MathUtil/source/MathUtil.cpp:161
int nValueInt = (int)fValue;                  // = 360
double thetaFrac = fValue - nValueInt;        // = 0
return CosTable[nValueInt] + thetaFrac*(CosTable[nValueInt+1] - CosTable[nValueInt]);
//                          ^ 读取 CosTable[361]，而表长为 361（合法下标 0..360）→ 越界读
```

越界读到的内存若为 NaN/Inf，即使 `thetaFrac == 0`，`0 * NaN` 仍得到 **NaN** —— 所以这不只是 UB，而是**可观测的错误输出**（`FastCos(-1e-7f) == NaN`）。

**修改方案**（1 处赋值前钳制，两个函数各 3 行）

```cpp
if (nValueInt > 359) { nValueInt = 359; }   // thetaFrac 变为 1，插值结果仍精确等于 Table[360]
```

**复现与验证**
`BUG-02` 用例遍历 19 个边界角度（含 `±360`、`359.9999`、`-1e-7`、`12345.678` 等），与 `std::sin/cos` 比对：
修复前 `FastCos(-1e-7f)` 返回 NaN（断言失败）；修复后全部通过。

---

### BUG-03 `Camera::SetNearClipDistance` 不重算投影矩阵

**原因**
投影矩阵由 `SetLens()` 计算（`zNear` 参与），而 setter 只改字段：

```cpp
// 修复前 Engine/Runtime/RenderSystem/source/Camera.cpp:137
void Camera::SetNearClipDistance(float nearClipDistance) { mNearZ = nearClipDistance; }
```

后果：`GetNearZ()` 与 `GetProjectionMatrix()` 不一致，深度、视锥、剔除等继续使用过期近平面
（本引擎 `BuildSetting::mUseReverseZ == true`，投影为无限远 Reverse-Z，同样依赖 zNear）。

**修改方案**（复用已有 `SetLens`，不新增重复代码）

```cpp
mNearZ = nearClipDistance;
if (mWidth > 0 && mHeight > 0)      // 未 SetLens 过时跳过，避免 aspect 除零
{
    SetLens(mFov, mWidth, mHeight, mNearZ, mFarZ);
}
```

**复现与验证**
`BUG-03`：`SetLens(...)` 记录投影 → `SetNearClipDistance(1.0f)` → 断言投影已变化、再改回 `0.1f` 后与初始值一致；
修复前第一条断言失败，修复后通过。`BUG-03b` 额外验证"未 SetLens 时改动近平面"不会产生 NaN/Inf。

> 说明：`SetFarClipDistance` 在本引擎 Reverse-Z（无限远平面）下**本就不影响投影**，故未改动其语义，
> 但 `GetFarZ()` 仍返回用户设置值（供剔除使用）。这一点已在代码注释中说明。

---

### BUG-04 `MTLVertexBuffer::GetBufferLength()` 恒返回 0（Metal）

**原因**：漏写 `return`，计算结果被丢弃。

```cpp
// 修复前 source/metal/MTLVertexBuffer.mm:27
if (mBuffer) { mBuffer->getBufferLength(); }   // 结果未返回
return 0;
```

**修改方案**：`return static_cast<uint32_t>(mBuffer->getBufferLength());`

**验证**：编译通过 + Metal 路径运行回归（atmosphere / terrain 均 0 崩溃）。
该函数当前无调用点，属"接口返回值错误"类潜伏缺陷；已确认同类模式（`::GetBufferLength`）在 Metal 后端仅此一处。

---

### BUG-05 `MTLCommandBuffer` 丢弃 stencil 纹理参数（Metal）

**原因**：构造时把传入的 `stencilTexture` 写成了 `depthStencilTexture`。

```cpp
// 修复前 source/metal/MTLCommandBuffer.mm:69
mDepthTexture = depthTexture;
mStencilTexture = depthStencilTexture;   // ← 应为 stencilTexture
mDepthStencilTexture = depthStencilTexture;
```

**修改方案**：`mStencilTexture = stencilTexture;`

**验证**：编译通过 + 运行回归（0 崩溃）。影响面：当调用方传入独立 stencil 纹理时（如
`MTLRenderDevice` 创建命令缓冲的某些路径），模板测试会拿到错误的纹理对象。

---

### BUG-06 `GLFWRenderWindow::Shutdown()` 非幂等

**原因**：`Shutdown()` 销毁窗口后未置空指针，而析构函数也会调用它：

```cpp
// 修复前
GLFWRenderWindow::~GLFWRenderWindow() { Shutdown(); }
void GLFWRenderWindow::Shutdown() { glfwDestroyWindow(mWindow); glfwTerminate(); }  // mWindow 未置空
```

**修改方案**（幂等化，2 行）

```cpp
if (mWindow != nullptr) { glfwDestroyWindow(mWindow); mWindow = nullptr; }
glfwTerminate();
```

**验证**：运行回归（terrain / atmosphere 正常退出，0 崩溃）。
**可达性说明（如实记录）**：`RenderWindow` 基类当前**未暴露** `Shutdown()`，外部只能经析构路径调用一次，
因此该缺陷在本版本**不可从外部触发**；属防御性修复，为后续显式关闭窗口的用法消除隐患。
（这也是它无法写成自动化用例的原因：派生类 `GLFWRenderWindow` 未导出符号，测试无法直接调用。）

---

### BUG-07 SDL 字母键映射全部失效（移动端）

**原因**：SDL 对字母键给出的 `SDLK_a` 是**未加 Shift 的小写 ASCII（97）**，而代码只判断大写区间：

```cpp
// 修复前 source/SDLRenderWindow.cpp:33
if (sdlKey >= 'A' && sdlKey <= 'Z') return static_cast<KeyCode>(sdlKey);  // 恒假
...
default: return static_cast<KeyCode>(0);                                  // 一律返回 0
```

后果：Android/iOS 上所有字母键的 `KeyCode` 都是 0，`Input::IsKeyPressed(KeyCode::W)` 等全部失效。

**修改方案**（同时接受两种写法）

```cpp
if (sdlKey >= 'a' && sdlKey <= 'z') return static_cast<KeyCode>(sdlKey - 'a' + 'A');
if (sdlKey >= 'A' && sdlKey <= 'Z') return static_cast<KeyCode>(sdlKey);
```

**验证方式（平台限定，本机不可自动化）**
`SDLRenderWindow.cpp` 整体由 `#if GNX_WINDOW_SDL` 守卫，macOS 构建不编译该文件（SDL2 仅移动端引入），
故无法在主机侧构造单测。移动端验证步骤：

1. 构建 Android/iOS demo（`demo/*/android` 或 Xcode iOS scheme）；
2. 在 `OnUpdate` 中打印 `Input::IsKeyPressed(KeyCode::W)`；
3. 修复前：按 W 无任何输出（恒 false）；修复后：按下为 true、松开为 false。

---

### BUG-08 Skybox Pass 使用未赋值资源成员取资源

**原因**：`SkyboxPassData::inputColor` **从未赋值**（第 678 行原本被注释掉），但 exec 里仍用它取资源：

```cpp
// 修复前 source/DeferredSceneRenderer.cpp:678 与 :690
//data.inputColor = builder.Read(colorTexture, ...);      // 未赋值
...
FrameGraphTexture& inputColor = resources.Get<FrameGraphTexture>(data.inputColor);  // data.inputColor == 0
```

`FrameGraphPassResources::Get` 带断言（`FrameGraph.h:265`）：

```cpp
assert(mPassNode.reads(id) || mPassNode.creates(id) || mPassNode.writes(id));
```

**实测结论（如实记录）**：terrain demo 在**缺陷存在**时运行 35 秒**未触发断言**。原因是资源索引 0 恰好
属于本 pass 自己声明的资源（`colorTexture`/`depthTexture`），断言侥幸通过，`Get(0)` 取到的也正是同一张纹理，
而该引用在函数体内**从未被使用**。因此这是**潜伏脆弱性**：一旦资源注册顺序变化，即会断言失败（Debug）
或绑定到无关纹理（Release）。

**修改方案**：删除从未赋值的成员与对应的无效 `Get`，并加注释说明"颜色附件以 Write + LOAD/STORE 复用，
无需再 Read 同一资源"，防止后人再次误加。

**验证**：修复后 terrain demo（使用 SkyBox 的场景）运行 75 秒，**0 崩溃 / 0 断言**，天空盒路径正常执行。

---

### BUG-13 单元测试目标无法链接（zlib 只在 Windows 链接）

**原因**
`BaseLib::DataCompress` 在所有平台都调用 zlib 的 `compress` / `inflate` / `compressBound`，但 `BaseLib` 仅在 Windows 下链接 zlib：

```cmake
# 修复前 Engine/Runtime/BaseLib/CMakeLists.txt:21
if(WIN32)
    target_link_libraries(BaseLib PUBLIC zlib)
elseif(LINUX)
    target_link_libraries(BaseLib PRIVATE ${UUID_LIBRARIES})
endif()
```

macOS/Linux 上之所以一直没暴露，是因为**引擎共享库**从其它依赖（assimp / ktx 等）间接拿到了 libz；
而「直接链接 `BaseLib` 的目标」——即 README 中推荐的 `cmake -B build -DENABLE_TESTING=ON` 单元测试流程——
会立即失败：

```
Undefined symbols for architecture x86_64:
  "_compress", "_compressBound", "_inflate", "_inflateEnd", "_inflateInit2_"
      referenced from: baselib::DataCompress(...) in DataCompress.o
```

> 该缺陷属于"文档承诺的流程实际不可用"：`unittest/CMakeLists.txt` 与 README 都描述了启用测试的构建方式，
> 但该方式在 macOS/Linux 上从未成功过（`test_baselib` / `test_mathutil` 均无法链接）。

**修改方案**：非 Windows 平台链接系统 zlib（`ThirdParty/zlib` 只在 `if(WIN32)` 下创建 `zlib` 目标，故不能直接写 `zlib`）

```cmake
if(WIN32)
    target_link_libraries(BaseLib PUBLIC zlib)   # 仓库内置
else()
    target_link_libraries(BaseLib PUBLIC z)      # 系统 zlib（-lz）
endif()
```

**验证**：修复前 `** BUILD FAILED **`（两个测试目标链接失败）；修复后全量构建 `** BUILD SUCCEEDED **`，
三个测试目标均可运行。

---

### BUG-14 `test_baselib` 断言与 `GetAllocationSize` 语义不符（恒失败）

**原因**
`GetAllocationSize` 的各个后端实现返回的都是分配器的「可用大小」：

| 后端 | 实现 |
|---|---|
| macOS / BaseLib | `malloc_size(ptr)` |
| Linux / BaseLib | `malloc_usable_size(ptr)` |
| Windows / BaseLib | `_aligned_msize(ptr, 16, 0)` |
| TBB / mimalloc / TLSF | `scalable_msize` / `mi_malloc_size` / `tlsf_block_size` |

可用大小总是**向上取整且 ≥ 请求值**，因此 `AlignedMalloc(257, 16)` 后断言 `== 257` 必然失败
（`test_baselib.cpp:699`）。同一文件的另一个用例（第 687 行）用的就是正确的 `>= size`。

**修改方案**：断言改为 `REQUIRE(GetAllocationSize(ptr) >= 257);` 并补注释说明语义
（这是**测试缺陷**而非产品缺陷 —— 实现与 API 语义一致）。

**验证**：修复前 `test cases: 71 | 70 passed | 1 failed`（`assertions: 3913 | 3912 passed | 1 failed`）；
修复后 `test_baselib` 全绿，`ctest` 无失败项。

---

## 4. 潜伏缺陷修复（`USE_PNG_LIB` 未启用）

下述 4 条位于 `#ifdef USE_PNG_LIB` 代码块内（全仓未定义该宏，macOS 实际走 Apple 的 ImageIO 实现），
**当前不参与编译**，但仍已修复。

### 4.9.1 BUG-09 PNG 读回调无边界检查（`pngtest_read_data`）

**原因**：回调直接 `memcpy(data, io_ptr, length)` 并 `png_ptr->io_ptr += length`，既不校验剩余长度，
也不向 libpng 报告 EOF —— 截断/损坏的 PNG 会让 libpng 持续请求，读取越过缓冲区末尾（堆越界读）。

**修改方案**：引入带末尾边界的读取游标结构，按剩余长度拷贝、缺失部分补 0（不依赖 longjmp）：

```cpp
struct PngMemoryReader { const uint8_t* cur; const uint8_t* end; };
// 回调内：
const size_t remain = (size_t)(reader->end - reader->cur);
const size_t copy = (length < remain) ? length : remain;
if (copy > 0) { memcpy(data, reader->cur, copy); reader->cur += copy; }
if (copy < length) { memset(data + copy, 0, length - copy); }   // 缺失补 0，绝不越界
```

### 4.9.2 BUG-10 `ImageDecoderPNG::IsFormat` 忽略 `size`

**修改方案**：`if (NULL == buffer || size < 8) { return false; }`（PNG 签名固定 8 字节）。

### 4.9.3 BUG-11 `formatHasAlpha()` 恒返回 false

**原因**：函数体只有 `return false;`，导致带 alpha 的输入被当作无 alpha 处理：
`EncodeWithLibPNG(..., hasAlpha, ...)` 的参数与扫描线转换选择 `choose_tranform_proc(format, hasAlpha)` 都会出错。

**修改方案**：按格式真实返回（`RGBA8 / SRGB8_ALPHA8 / RGBA4444 / RGB5A1 / GRAY8_ALPHA8 / RGBA32Float` 为 true）。

### 4.9.4 BUG-12 `transform_scanline_5551()` 空实现

**原因**：函数体只读取源数据、从不写 `dst`，于是 `FORMAT_RGB5A1` 编码输出的扫描线是**未初始化内存**。

**修改方案**：补全 5551 → RGBA8888 展宽（`(v5 << 3) | (v5 >> 2)` 即 `v5 * 255 / 31`）。

**验证**：以上 4 条已完成**语义级编译校验**（libpng 已随仓库内置在 `ImageCodec/source/libpng`，
可直接以 `USE_PNG_LIB` 打开该分支）：

```bash
cd Engine
clang++ -fsyntax-only -std=c++20 -DUSE_PNG_LIB \
  -I Runtime/ImageCodec/source/libpng -I Runtime/ImageCodec/include \
  -I Runtime/BaseLib/include -I Runtime/MathUtil/include \
  -I Runtime/RenderCore/include -I . \
  Runtime/ImageCodec/source/ImageDecoderPng.cpp    # 同上 ImageEncoderPng.cpp
```

`-fsyntax-only` 会完成完整语义分析（类型检查、重载解析、成员可见性），只是不做代码生成 ——
两个文件均**无任何诊断输出**，证明修复在启用该分支后仍可正确编译。

运行时验证方案（启用 `-DUSE_PNG_LIB` 后复用 §2 思路）：`BUG-10` 用保护页 2 字节缓冲调用 `IsFormat`；
`BUG-09` 用截断 PNG 调 `DecodePngData`；`BUG-11/12` 用 `FORMAT_RGB5A1` 图片编码后校验输出像素颜色。

---

## 5. 待处理清单（已确证，本轮未修）

以下条目在审计中已确证或高度可疑，但**修复或验证需要 Vulkan/移动端运行环境，或影响面需单独评估**，
故本轮不擅自改动，列出以便后续排期：

| # | 位置 | 问题 | 为何本轮未修 |
|---|---|---|---|
| 1 | `RenderCore/source/vulkan/VKShaderFunction.cpp:498,530,560` | 描述符写入用 `(mCurrentFrame+1)%N`，绑定用 `mCurrentFrame` → **错位一帧**（首帧绑定未初始化的 set） | 需 Vulkan 环境验证；改动涉及帧同步语义，须配套验证 |
| 2 | `RenderCore/source/vulkan/VKTextureBase.cpp:350,382` | HostImageCopy 路径 3D 纹理 `imageOffset.z` 恒 0；staging 路径 `size` 忽略 `bytesPerImage`/pitch | 同上（Vulkan 子矩形/3D 上传需实机验证） |
| 3 | `RenderCore/source/vulkan/VKBlitEncoder.cpp:199,242` | Blit 未插入 layout transition 即按 `TRANSFER_*_OPTIMAL` 使用 | 需 Vulkan 校验层验证 |
| 4 | `GNXEngine/source/GLFWRenderWindow.cpp:262,276` | 滚轮被量化成 ±120（与 SDL 路径不一致）；鼠标传逻辑坐标而 3D 视口用物理像素（Retina 命中偏移） | 改动会同时影响 ImGui（其依赖逻辑坐标），须统一坐标契约后处理 |
| 5 | `GNXEngine/source/InputState.cpp:113` | `PollFromGLFW` 每帧轮询 0..511 个键码 → GLFW 错误回调刷屏 + 开销 | 需先确定有效 KeyCode 集合，属行为/性能修整 |
| 6 | `RenderSystem/source/PostProcessing.cpp:14` | 未判空 `LoadShaderAsset` 返回值即解引用 | 触发需资产缺失，验证要构造缺资产环境 |
| 7 | `ImageCodec/source/ImageDecoderImpl.cpp:103` / `AssetManager/source/AssetManager.cpp:237` | `ftell` / `tellg` 返回 -1 未校验 → `new[]`/`resize` 巨量分配 | 需构造特殊文件（FIFO/设备文件）复现，验证方案待定 |
| 8 | `AssetProcess/source/AssimpMeshImporter.cpp:418` | `uint16_t indexCount` 累加截断（>65535 索引的子网格） | 需 >21845 面的测试模型，验证成本较高 |

---

## 6. 变更清单

| 文件 | 变更 |
|---|---|
| `Engine/Runtime/ImageCodec/source/ImageDecoderHDR.cpp` | BUG-01：`IsFormat` 增加长度校验 |
| `Engine/Runtime/MathUtil/source/MathUtil.cpp` | BUG-02：`FastSin` / `FastCos` 索引钳制 |
| `Engine/Runtime/RenderSystem/source/Camera.cpp` | BUG-03：近平面 setter 重算投影 |
| `Engine/Runtime/RenderCore/source/metal/MTLVertexBuffer.mm` | BUG-04：补 `return` |
| `Engine/Runtime/RenderCore/source/metal/MTLCommandBuffer.mm` | BUG-05：stencil 纹理赋值修正 |
| `Engine/Runtime/GNXEngine/source/GLFWRenderWindow.cpp` | BUG-06：`Shutdown` 幂等化 |
| `Engine/Runtime/GNXEngine/source/SDLRenderWindow.cpp` | BUG-07：字母键大小写兼容 |
| `Engine/Runtime/RenderSystem/source/DeferredSceneRenderer.cpp` | BUG-08：移除未赋值资源成员与其无效访问 |
| `Engine/Runtime/ImageCodec/source/ImageDecoderPng.cpp` | BUG-09/10：读回调边界 + `IsFormat` 长度校验 |
| `Engine/Runtime/ImageCodec/source/ImageEncoderPng.cpp` | BUG-11/12：`formatHasAlpha` + 5551 扫描线转换 |
| `unittest/test_bugfixes.cpp` | 新增：本轮缺陷的复现 / 回归测试 |
| `unittest/CMakeLists.txt` | 新增 `test_bugfixes` 目标与 `add_test` |
| `Engine/Runtime/BaseLib/CMakeLists.txt` | BUG-13：非 Windows 平台链接系统 zlib |
| `unittest/test_baselib.cpp` | BUG-14：修正与 API 语义不符的断言 |

改动规模：生产代码约 **+70 / -25 行**（每条缺陷 1~15 行），无一处改动超过 15 行。

---

## 7. 审计覆盖范围

**已覆盖（本期）**
`Engine/Runtime/BaseLib`、`MathUtil`、`Allocator`、`RenderCore`（Metal + Vulkan 全部 `.mm`/`.cpp`）、
`RenderSystem`（SceneManager / DeferredSceneRenderer / PostProcessing / Passes / FrameGraph / Atmosphere / ImGuiRenderer）、
`GNXEngine`（AppFrameWork / RenderWindow / GLFW / SDL / Events / Input）、
`AssetManager`、`AssetProcess`、`ImageCodec`、`ShaderCompiler`、`tool/*`。

**未覆盖 / 仅抽查**
`ThirdParty/**`（按要求排除）、各 `demo/**` 的业务逻辑细节、`Engine/Editor/**`（默认不参与构建）、
`unittest/**` 与 `test/**` 自身、`FrameGraph` 的深度同步细节、纹理压缩器内部（ispc_texcomp / kernel.ispc）。

**建议的下一轮重点**
1. §5 待处理清单（尤其 Vulkan 描述符帧槽与纹理上传路径，建议开 Vulkan 校验层专项）；
2. 输入/坐标契约统一（DPI 逻辑坐标 vs 物理像素，涉及 3D 拾取与 ImGui 两条链路）；
3. 资产生命周期与缓存失效（AssetManager 引用计数、FrameGraph 瞬态资源池复用分支忽略 `desc.depth` 的疑点）。

---

**报告结束**

> 本报告对应的修复均已提交前验证：`test_bugfixes` 全绿（78 断言 / 4 用例），
> atmosphere / terrain demo 运行 0 崩溃，无回归。
