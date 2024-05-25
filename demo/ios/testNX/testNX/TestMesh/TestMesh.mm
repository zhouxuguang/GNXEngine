//
//  TestMesh.cpp
//  testNX
//
//  Created by zhouxuguang on 2022/5/28.
//

#include "TestMesh.hpp"
#include "RenderSystem/mesh/MeshAssimpImpoter.h"
#include <Foundation/Foundation.h>
#include "RenderSystem/ShaderAssetLoader.h"

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
//#include <glm/ext/constants.hpp> // glm::pi

static const char* str_mesh_vert = R"(
#version 300 es
#ifdef GL_EXT_separate_shader_objects
#extension GL_EXT_separate_shader_objects : enable
#endif

//layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec3 aNormal;
//layout (location = 2) in vec2 aTexCoords;

in vec3 aPos;
in vec3 aNormal;
in vec2 aTexCoords;

out vec2 TexCoords;

layout(std140) uniform vert_uniform{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

out highp vec4 gl_Position;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(aPos, 1.0);
}
)";

static const char* str_mesh_frag = R"(
#version 300 es
precision lowp float;

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = texture(texture_diffuse1, TexCoords);
    //FragColor = vec4(1.0, 1.0, 0.0, 1.0);
}
)";

static RenderSystem::Mesh* mesh = nullptr;
static rendercore::GraphicsPipelinePtr mPipeline = nullptr;
static rendercore::UniformBufferPtr uniformBuffer = nullptr;

struct MVP
{
    mathutil::Matrix4x4f model;
    mathutil::Matrix4x4f view;
    mathutil::Matrix4x4f projection;
};

void init(rendercore::RenderDevicePtr renderDevice)
{
    ShaderAssetString shaderAssetString = LoadShaderAsset("ModelShader");
    const char* vertexShader = nullptr;
    const char* fragmentShader = nullptr;
    if (renderDevice->getRenderDeviceType() == rendercore::RenderDeviceType::GLES)
    {
        vertexShader = shaderAssetString.gles30Shader.vertexShaderStr.c_str();
        fragmentShader = shaderAssetString.gles30Shader.fragmentShaderStr.c_str();
    }
    else if (renderDevice->getRenderDeviceType() == rendercore::RenderDeviceType::METAL)
    {
        vertexShader = shaderAssetString.metalShader.vertexShaderStr.c_str();
        fragmentShader = shaderAssetString.metalShader.fragmentShaderStr.c_str();
    }
    
//    FILE* fp1 = fopen("/Users/zhouxuguang/work/vert.glsl", "wb");
//    fwrite(vertexShader, 1, strlen(vertexShader), fp1);
//    fclose(fp1);
//
//    FILE* fp2 = fopen("/Users/zhouxuguang/work/frag.glsl", "wb");
//    fwrite(fragmentShader, 1, strlen(fragmentShader), fp2);
//    fclose(fp2);
    
    rendercore::ShaderFunctionPtr vertShader = renderDevice->createShaderFunction(vertexShader, rendercore::ShaderStage_Vertex);
    rendercore::ShaderFunctionPtr fragShader = renderDevice->createShaderFunction(fragmentShader, rendercore::ShaderStage_Fragment);
    
    rendercore::GraphicsPipelineDescriptor graphicsPipelineDescriptor;
    rendercore::VertextAttributesDescritptor vertextAttributesDescritptor;
    rendercore::VertexBufferLayoutDescriptor vertexBufferLayoutDescriptor;
    
    //positon
    vertextAttributesDescritptor.index = 0;
    vertextAttributesDescritptor.offset = 0;
    vertextAttributesDescritptor.format = rendercore::VertexFormatFloat3;
    graphicsPipelineDescriptor.vertexDescriptor.attributes.push_back(vertextAttributesDescritptor);
    vertexBufferLayoutDescriptor.stride = sizeof(Vertex);
    graphicsPipelineDescriptor.vertexDescriptor.layouts.push_back(vertexBufferLayoutDescriptor);
    
    //normal
    vertextAttributesDescritptor.index = 1;
    vertextAttributesDescritptor.offset = offsetof(Vertex, Normal);
    vertextAttributesDescritptor.format = rendercore::VertexFormatFloat3;
    graphicsPipelineDescriptor.vertexDescriptor.attributes.push_back(vertextAttributesDescritptor);
    vertexBufferLayoutDescriptor.stride = sizeof(Vertex);
    graphicsPipelineDescriptor.vertexDescriptor.layouts.push_back(vertexBufferLayoutDescriptor);
    
    //texCoord
    vertextAttributesDescritptor.index = 2;
    vertextAttributesDescritptor.offset = offsetof(Vertex, TexCoords);
    vertextAttributesDescritptor.format = rendercore::VertexFormatFloat2;
    graphicsPipelineDescriptor.vertexDescriptor.attributes.push_back(vertextAttributesDescritptor);
    vertexBufferLayoutDescriptor.stride = sizeof(Vertex);
    graphicsPipelineDescriptor.vertexDescriptor.layouts.push_back(vertexBufferLayoutDescriptor);
    
    //tangent
    vertextAttributesDescritptor.index = 3;
    vertextAttributesDescritptor.offset = offsetof(Vertex, Tangent);
    vertextAttributesDescritptor.format = rendercore::VertexFormatFloat3;
    graphicsPipelineDescriptor.vertexDescriptor.attributes.push_back(vertextAttributesDescritptor);
    vertexBufferLayoutDescriptor.stride = sizeof(Vertex);
    graphicsPipelineDescriptor.vertexDescriptor.layouts.push_back(vertexBufferLayoutDescriptor);
    
    //bitangent
    vertextAttributesDescritptor.index = 4;
    vertextAttributesDescritptor.offset = offsetof(Vertex, Bitangent);
    vertextAttributesDescritptor.format = rendercore::VertexFormatFloat3;
    graphicsPipelineDescriptor.vertexDescriptor.attributes.push_back(vertextAttributesDescritptor);
    vertexBufferLayoutDescriptor.stride = sizeof(Vertex);
    graphicsPipelineDescriptor.vertexDescriptor.layouts.push_back(vertexBufferLayoutDescriptor);
    
    graphicsPipelineDescriptor.depthStencilDescriptor.depthCompareFunction = rendercore::CompareFunctionLess;
    graphicsPipelineDescriptor.depthStencilDescriptor.depthWriteEnabled = true;
    
    mPipeline = renderDevice->createGraphicsPipeline(graphicsPipelineDescriptor);
    mPipeline->attachVertexShader(vertShader);
    mPipeline->attachFragmentShader(fragShader);
    
    uniformBuffer = renderDevice->createUniformBufferWithSize(sizeof(MVP));
    MVP mvp;
    
    glm::mat4 Projection = glm::perspective(60.0f * float(M_PI) / 180.0f, 0.75f, 0.1f, 100.f);
    glm::mat4 View = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    mvp.projection = mathutil::Matrix4x4f::CreatePerspective(60, 0.75, 0.1f, 100.f);
    mvp.view = mathutil::Matrix4x4f::CreateLookAt(mathutil::Vector3f(0, 0, 5), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
    mvp.model = mathutil::Matrix4x4f::IDENTITY;
    mvp.projection = mvp.projection.Transpose();
    mvp.view = mvp.view.Transpose();
    
    mvp.model = mathutil::Matrix4x4f::CreateRotation(1, 0, 0, 90);
    mvp.model = mvp.model.Transpose();
    
    mvp.model = Matrix4x4f::IDENTITY;
    
//    Matrix4x4 scale;
//    Matrix4x4::CreateScale(0.01f, 0.01f, 0.01f, scale);
//    mvp.model = scale;
    
//    memcpy(&mvp.projection, &Projection, 64);
//    memcpy(&mvp.view, &View, 64);
    
//    mathutil::Matrix4x4::CreateTranslate(0, 0, 0, mvp.model);
//    mvp.model.Transpose();
    
//    mvp.projection = mathutil::Matrix4x4::IDENTITY;
//    mvp.view = mathutil::Matrix4x4::IDENTITY;
    uniformBuffer->setData(&mvp, 0, sizeof(MVP));
}

#include <omp.h>

void test()
{
    int core = omp_get_num_procs();
    int n = 1 << 4;
    
     #pragma omp parallel for
    for (int i = 0; i < n; i ++)
    {
        sin(i);
        
        printf("thread = %p, num thread = %d\n", pthread_self(), omp_get_num_threads());
    }
}

void initMesh(rendercore::RenderDevicePtr renderDevice)
{
    //NSString *modelPath = [[NSBundle mainBundle] pathForResource:@"DamagedHelmet" ofType:@"glb"];
    //NSString *modelPath = [[NSBundle mainBundle] pathForResource:@"backpack" ofType:@"obj"];
    //NSString *modelPath = [[NSBundle mainBundle] pathForResource:@"nanosuit" ofType:@"obj"];
    NSString *modelPath = [[NSBundle mainBundle] pathForResource:@"box" ofType:@"fbx"];
    mesh = new RenderSystem::Mesh();
    mesh->Import(modelPath.UTF8String);
    
    RenderSystem::MeshAssimpImpoter meshImporter;
    RenderSystem::Mesh *mesh2 = new RenderSystem::Mesh();
    meshImporter.ImportFromFile(modelPath.UTF8String, mesh2);
    
    init(renderDevice);
    test();
}

void testMesh(const rendercore::RenderEncoderPtr &renderEncoder)
{
    mesh->Render(renderEncoder, mPipeline, uniformBuffer);
}
