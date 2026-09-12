//
//  ImGui.shader
//  GNXEngine
//
//  Dear ImGui 的 UI 渲染着色器（供自研 RHI 渲染后端使用）
//
//  顶点属性按引擎约定分到 3 个独立顶点缓冲（每个属性独占一个 buffer）：
//    buffer 0 -> POSITION   float2  (R32G32_FLOAT)
//    buffer 1 -> TEXCOORD0  float2  (R32G32_FLOAT)
//    buffer 2 -> COLOR0     float4  (R8G8B8A8_UNORM，着色器侧自动归一化到 [0,1])
//
//  参考: doc/ImGuiIntegrationDesign.md
//

// UI 投影矩阵：屏幕像素坐标 -> 裁剪空间（正交投影，由 CPU 侧按屏幕尺寸与后端 Y 方向构建）
cbuffer ImGuiCB : register(b0)
{
    float4x4 proj;
};

struct VS_IN
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};

// 字体图集（含 UI 用到的其它纹理）
Texture2D    fontTex;
SamplerState fontTexSam;

[shader("vertex")]
VS_OUT VS(VS_IN input)
{
    VS_OUT output;
    output.pos = mul(float4(input.pos, 0.0, 1.0), proj);
    output.uv  = input.uv;
    output.col = input.col;
    return output;
}

[shader("pixel")]
float4 PS(VS_OUT input) : SV_Target0
{
    return input.col * fontTex.Sample(fontTexSam, input.uv);
}
