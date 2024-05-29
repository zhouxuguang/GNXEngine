//
//  testShader.cpp
//  testNX
//
//  Created by zhouxuguang on 2021/5/9.
//

#if 1

#include "testShader.h"
#include "RenderCore/RenderDevice.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "ResourceUtil.h"
#include <Foundation/Foundation.h>

static const char * vertexShader = R"(
#version 300 es
#ifdef GL_ARB_separate_shader_objects
#extension GL_ARB_separate_shader_objects : enable
#endif

layout(std140) uniform main0_uniforms{
    mat4 u_MVPMatrix;
} ubo;

layout(location = 0) in vec3 a_position;

void main()
{
    gl_Position = ubo.u_MVPMatrix * vec4(a_position, 1.0);
}
)";

//layout (binding=0) uniform sampler2D Tex1;

static const char * fragmentShader = R"(
#version 300 es
precision lowp float;



layout(std140) uniform main0_uniforms{
   vec4 u_color;
} ubo;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = ubo.u_color;
}
)";

static const char * glslVertexTest = R"(
#version 450
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D frontTexture;
uniform sampler2D backTexture;

void main()
{
    if(gl_FrontFacing)
        FragColor = texture(frontTexture, TexCoords);
    else
        FragColor = texture(backTexture, TexCoords);
}
)";

typedef std::shared_ptr<std::vector<uint32_t>> ShaderCodePtr;

//加载shader文件到二进制代码 pszShaderName 是shader的名字
static ShaderCodePtr loadShaderAsset(const char* pszShaderName)
{
    if (nullptr == pszShaderName)
    {
        return nullptr;
    }
    
    FILE* fpShader = fopen(pszShaderName, "rb");
    
    if (nullptr == fpShader)
    {
        return nullptr;
    }

    fseek(fpShader, 0, SEEK_END);
    long len = ftell(fpShader);
    if (len % 4 != 0)
    {
        fclose(fpShader);
        return nullptr;
    }
    fseek(fpShader, 0, SEEK_SET);

    ShaderCodePtr shaderCodePtr = std::make_shared<std::vector<uint32_t>>();
    shaderCodePtr->resize(len / 4);

    size_t nReadBytes = fread(shaderCodePtr->data(), 1, len, fpShader);
    if (nReadBytes != len)
    {
        shaderCodePtr->clear();
        fclose(fpShader);
        return nullptr;
    }

    fclose(fpShader);
    return shaderCodePtr;
}

//void testShader(RenderDevicePtr renderDevice)
//{
//    ShaderFunctionPtr shaderFunction1 = renderDevice->createShaderFunction(vertexShader, ShaderStage_Vertex);
//    if (shaderFunction1) {
//        printf("");
//    }
//    
//    ShaderFunctionPtr shaderFunction2 = renderDevice->createShaderFunction(fragmentShader, ShaderStage_Fragment);
//    if (shaderFunction2) {
//        printf("");
//    }
//    
//    std::string path = getShaderPath();
//    ShaderCodePtr codePtr = loadShaderAsset(path.c_str());
//    std::string essl30Source = shader_compiler::compileToESSL30(*codePtr, ShaderStage_Fragment);
//    if (essl30Source.empty()) {
//        printf("");
//    }
//    
//    ShaderFunctionPtr shaderFunction3 = renderDevice->createShaderFunction(essl30Source.c_str(), ShaderStage_Fragment);
//    if (shaderFunction3) {
//        printf("");
//    }
//    
//    NSString *shadePath = [[NSBundle mainBundle] pathForResource:@"Skybox" ofType:@"hlsl"];
//    ShaderCodePtr hlshCodePtr = shader_compiler::compileHLSLToSPIRV(shadePath.UTF8String, ShaderStage_Vertex);
//    std::string essl30Source1 = shader_compiler::compileToESSL30(*hlshCodePtr, ShaderStage_Vertex);
//    
//    ShaderCodePtr hlshCodePtr1 = shader_compiler::compileHLSLToSPIRV(shadePath.UTF8String, ShaderStage_Fragment);
//    std::string essl30Source2 = shader_compiler::compileToESSL30(*hlshCodePtr1, ShaderStage_Fragment);
//    printf("");
//    
//}

#endif
