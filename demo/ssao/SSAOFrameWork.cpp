//
//  SSAOFrameWork.cpp
//  ssao
//
//  SSAO (Screen Space Ambient Occlusion) Demo
//

#include "SSAOFrameWork.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/ImageTextureUtil.h"
#include "Runtime/RenderSystem/include/Transform.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"

RenderSystem::MeshPtr CreatePlaneMesh(float xsize, float zsize, int xdivs, int zdivs, float smax, float tmax)
{
    int nPoints = (xdivs + 1) * (zdivs + 1);
    std::vector<float> p(4 * nPoints);
    std::vector<float> n(4 * nPoints);
    std::vector<float> tex(2 * nPoints);
    std::vector<float> tang(4 * nPoints);
    std::vector<uint32_t> el(6 * xdivs * zdivs);

    float x2 = xsize / 2.0f;
    float z2 = zsize / 2.0f;
    float iFactor = (float)zsize / zdivs;
    float jFactor = (float)xsize / xdivs;
    float texi = smax / xdivs;
    float texj = tmax / zdivs;
    float x, z;
    int vidx = 0, tidx = 0;
    for( int i = 0; i <= zdivs; i++ ) 
    {
        z = iFactor * i - z2;
        for( int j = 0; j <= xdivs; j++ ) 
        {
            x = jFactor * j - x2;
            p[vidx] = x;
            p[vidx+1] = 0.0f;
            p[vidx+2] = z;
            p[vidx+3] = 1.0;
            n[vidx] = 0.0f;
            n[vidx+1] = 1.0f;
            n[vidx+2] = 0.0f;
            n[vidx+2] = 0.0f;

            tex[tidx] = j * texi;
            tex[tidx+1] = (zdivs - i) * texj;

            vidx += 4;
            tidx += 2;
        }
    }

    for (int i = 0; i < nPoints; i++) 
    {
        tang[i * 4 + 0] = 1.0f;
        tang[i * 4 + 1] = 0.0f;
        tang[i * 4 + 2] = 0.0f;
        tang[i * 4 + 3] = 1.0f;
    }

    uint32_t rowStart, nextRowStart;
    int idx = 0;
    for( int i = 0; i < zdivs; i++ ) 
    {
        rowStart = (uint32_t)( i * (xdivs+1) );
        nextRowStart = (uint32_t)( (i+1) * (xdivs+1));
        for( int j = 0; j < xdivs; j++ )
        {
            el[idx] = rowStart + j;
            el[idx+1] = nextRowStart + j;
            el[idx+2] = nextRowStart + j + 1;
            el[idx+3] = rowStart + j;
            el[idx+4] = nextRowStart + j + 1;
            el[idx+5] = rowStart + j + 1;
            idx += 6;
        }
    }

    RenderSystem::MeshPtr mesh = std::make_shared<RenderSystem::Mesh>();

    RenderSystem::VertexData& vertexData = mesh->GetVertexData();
    uint32_t vertexCount = nPoints;
    vertexData.Resize(vertexCount, 56);

    RenderSystem::ChannelInfo* channels = vertexData.GetChannels();
    channels[RenderSystem::kShaderChannelPosition].offset = 0;
    channels[RenderSystem::kShaderChannelPosition].format = VertexFormatFloat4;
    channels[RenderSystem::kShaderChannelPosition].stride = sizeof(Vector4f);
    channels[RenderSystem::kShaderChannelTangent].offset = p.size() * 4;
    channels[RenderSystem::kShaderChannelTangent].format = VertexFormatFloat4;
    channels[RenderSystem::kShaderChannelTangent].stride = 16;
    channels[RenderSystem::kShaderChannelNormal].offset = p.size() * 4 + n.size() * 4;
    channels[RenderSystem::kShaderChannelNormal].format = VertexFormatFloat4;
    channels[RenderSystem::kShaderChannelNormal].stride = 16;
    channels[RenderSystem::kShaderChannelTexCoord0].offset = p.size() * 4 + n.size() * 4 + tang.size() * 4;
    channels[RenderSystem::kShaderChannelTexCoord0].format = VertexFormatFloat2;
    channels[RenderSystem::kShaderChannelTexCoord0].stride = 8;

    mesh->SetPositions((const Vector4f*)(p.data()), vertexCount);
    mesh->SetNormals((const Vector4f*)(n.data()), vertexCount);
    mesh->SetTangents((const Vector4f*)(tang.data()), vertexCount);
    mesh->SetUv(0, (const Vector2f*)(tex.data()), vertexCount);
    mesh->SetIndices(el.data(), el.size());

    RenderSystem::SubMeshInfo subMeshInfo;
    subMeshInfo.firstIndex = 0;
    subMeshInfo.indexCount = el.size();
    subMeshInfo.vertexCount = vertexCount;
    subMeshInfo.topology = PrimitiveMode_TRIANGLES;
    mesh->AddSubMeshInfo(subMeshInfo);

    mesh->SetUpBuffer();
    
    return mesh;
}

SSAOFrameWork::SSAOFrameWork(const GNXEngine::WindowProps& props) 
    : GNXEngine::AppFrameWork(props)
{
}

void SSAOFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
}

void SSAOFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    
    // Update camera
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
    RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
    if (!cameraPtr)
    {
        cameraPtr = sceneManager->CreateCamera("MainCamera");
    }
    cameraPtr->LookAt(
        mathutil::Vector3f(2.1f, 1.5f, 2.1f),
        mathutil::Vector3f(0.0f, 1.0f, 0.0f),
        mathutil::Vector3f(0.0f, 1.0f, 0.0f)
    );
    
    cameraPtr->SetLens(50, width, height, 0.3f, 100.0f);
    
    // Create a simple light
    RenderSystem::PointLight* pointLight = (RenderSystem::PointLight*)sceneManager->CreateLight(
        "mainLight", RenderSystem::Light::PointLight);
    pointLight->setPosition(mathutil::Vector3f(4.91507435f, 4.70739937, 0.672433853));
    pointLight->setColor(mathutil::Vector3f(1.0f, 1.0f, 1.0f));
    pointLight->setStrength(mathutil::Vector3f(1.0f, 1.0f, 1.0f));
    pointLight->setFalloffStart(100.0);
    pointLight->setFalloffEnd(300.0);
    
    std::string strMarryFile = GetProjectAssetDir() + "ssao/Marry.obj";
    //sceneManager->GetRootNode()->CreateRendererNode("marry", strMarryFile);
    
    RenderCore::RCTexturePtr woodImage = RenderSystem::ImageTextureUtil::TextureFromFile(
                        (GetProjectAssetDir() + "ssao/hardwood2_diffuse.jpg").c_str());
    
    RenderCore::RCTexturePtr brickImage = RenderSystem::ImageTextureUtil::TextureFromFile(
                        (GetProjectAssetDir() + "ssao/brick1.jpg").c_str());
    
    RenderCore::RCTexturePtr normalImage = RenderSystem::ImageTextureUtil::CreateNormalTexture();
    RenderCore::RCTexturePtr metalRoughImage = RenderSystem::ImageTextureUtil::CreateMetalRoughTexture();
    RenderCore::RCTexturePtr ambientImage = RenderSystem::ImageTextureUtil::CreateAOTexture();
    
    RenderSystem::MeshPtr mesh = CreatePlaneMesh(10, 10, 1, 1, 1, 1);
    {
        RenderSystem::MeshRenderer* meshRender1 = new(std::nothrow) RenderSystem::MeshRenderer();
        meshRender1->SetSharedMesh(mesh);
        
        RenderSystem::MaterialPtr material1 = std::make_shared<RenderSystem::Material>();
        material1->SetTexture("diffuseTexture", woodImage);
        material1->SetTexture("normalTexture", normalImage);
        material1->SetTexture("roughnessTexture", metalRoughImage);
        material1->SetTexture("ambientTexture", ambientImage);
        meshRender1->AddMaterial(material1);
        
        RenderSystem::SceneNode* floorNode = sceneManager->GetRootNode()->CreateChildSceneNode("floorNode");
        floorNode->AddComponent(meshRender1);
    }
    
    {
        RenderSystem::MeshRenderer* meshRender1 = new(std::nothrow) RenderSystem::MeshRenderer();
        meshRender1->SetSharedMesh(mesh);
        
        RenderSystem::MaterialPtr material1 = std::make_shared<RenderSystem::Material>();
        material1->SetTexture("diffuseTexture", brickImage);
        material1->SetTexture("normalTexture", normalImage);
        material1->SetTexture("roughnessTexture", metalRoughImage);
        material1->SetTexture("ambientTexture", ambientImage);
        meshRender1->AddMaterial(material1);
        
        //model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -2));
//        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
        
        Matrix4x4f translateMatrix = Matrix4x4f::CreateTranslate(0, 0, -2);
        Matrix4x4f rotateMatrix = Matrix4x4f::CreateRotation(1, 0, 0, 90.0f);
        Matrix4x4f modelMatrix = rotateMatrix * translateMatrix;
        RenderSystem::Transform transform;
        transform.TransformFromMat4(modelMatrix);
        
        RenderSystem::SceneNode* wallNode = sceneManager->GetRootNode()->CreateChildSceneNode("wallNode1",
                                            transform.position, transform.rotation, transform.scale);
        wallNode->AddComponent(meshRender1);
    }
}

void SSAOFrameWork::RenderFrame()
{
    using namespace RenderCore;
    using namespace RenderSystem;
    
    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;
    
    SceneManager* sceneManager = SceneManager::GetInstance();
    sceneManager->Update(deltaTime);
    
    sceneManager->Render(nullptr);
}

void SSAOFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
}

bool SSAOFrameWork::OnKeyUp(GNXEngine::KeyReleasedEvent& e)
{
    return true;
}
