//
//  LumenFrameWork.cpp
//  lumen
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "LumenFrameWork.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"

LumenFrameWork::LumenFrameWork(const GNXEngine::WindowProps& props) : GNXEngine::AppFrameWork(props)
{
//    mWidth = props.width;
//    mHeight = props.height;
}

void LumenFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
    
    //mRenderDevice = RenderCore::GetRenderDevice();
}

static void LoadGeometryData(RenderSystem::SceneManager* sceneManager)
{
	RenderSystem::SceneNode* node1 = sceneManager->GetRootNode()->CreateChildSceneNode("BOX1", 
        Vector3f(0.0, 0.0, 0.0), Quaternionf(1.0, 0.0, 0.0, 0.0), Vector3f(10.0, 10.0, 0.1));

	RenderSystem::SceneNode* node2 = sceneManager->GetRootNode()->CreateChildSceneNode("BOX2",
		Vector3f(0.0, 0.0, 500.0), Quaternionf(1.0, 0.0, 0.0, 0.0), Vector3f(10.0, 10.0, 0.1));

	RenderSystem::MeshPtr mesh = std::make_shared<RenderSystem::Mesh>();

	std::string strDataFile = GetProjectAssetDir() + "Lumen/PositionOnly.data";
	std::vector<uint8_t> positionData = baselib::FileUtil::ReadBinaryFile(strDataFile);
    std::string indexFile = GetProjectAssetDir() + "Lumen/IndexBuffer.data";
    std::vector<uint8_t> indexData = baselib::FileUtil::ReadBinaryFile(indexFile);
    Vector3f* posPtr = (Vector3f*)positionData.data();

    RenderSystem::VertexData& vertexData = mesh->GetVertexData();

    RenderSystem::ChannelInfo* channels = vertexData.GetChannels();
	channels[RenderSystem::kShaderChannelPosition].offset = 0;
	channels[RenderSystem::kShaderChannelPosition].format = VertexFormatFloat4;
    channels[RenderSystem::kShaderChannelPosition].stride = sizeof(Vector4f);

    vertexData.Resize(positionData.size() / 12, 12);

    Vector4f* position = new Vector4f[positionData.size() / 12];
    for (uint32_t i = 0; i < positionData.size() / 12; i++)
    {
        position[i].x = posPtr[i].x;
        position[i].y = posPtr[i].y;
        position[i].z = posPtr[i].z;
        position[i].w = 1;
    }

    mesh->SetPositions(position, positionData.size() / 12);
    delete []position;

    uint32_t* indices = new uint32_t[indexData.size() / 2];
	for (uint32_t i = 0; i < indexData.size() / 2; i++)
	{
        indices[i] = indexData[i];
	}

    mesh->SetIndices(indices, indexData.size() / 2);
    delete []indices;

    RenderSystem::SubMeshInfo subMeshInfo;
    subMeshInfo.firstIndex = 0;
    subMeshInfo.indexCount = indexData.size() / 2;
    subMeshInfo.vertexCount = positionData.size() / 12;
    subMeshInfo.topology = PrimitiveMode_TRIANGLES;
    mesh->AddSubMeshInfo(subMeshInfo);

    mesh->SetUpBuffer();

    RenderSystem::MeshRenderer* meshRender = new(std::nothrow) RenderSystem::MeshRenderer();
	meshRender->SetSharedMesh(mesh);
	
    node1->AddComponent(meshRender);
    node2->AddComponent(meshRender);
}

void LumenFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    
    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();
    LoadGeometryData(sceneManager);

	RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
	if (!cameraPtr)
	{
		cameraPtr = sceneManager->CreateCamera("MainCamera");
	}

	cameraPtr->LookAt(mathutil::Vector3f(1059.769897f, -833.207886f, 336.560120f), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
	cameraPtr->SetLens(90, float(width) / height, 10.0f, 10000.f);
    
}

void LumenFrameWork::RenderFrame()
{
    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;
    //LOG_INFO("deltaTime = %f\n", deltaTime);

    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();
    sceneManager->Update(deltaTime);

    sceneManager->Render(nullptr);
}

void LumenFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
}

bool LumenFrameWork::OnKeyUp(GNXEngine::KeyReleasedEvent& e)
{
    return true;
}
