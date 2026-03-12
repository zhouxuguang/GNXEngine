//
//  LumenFrameWork.cpp
//  lumen
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "LumenFrameWork.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/MathUtil/include/MathUtil.h"

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
        Vector3f(0.0, 0.0, 0.0), Quaternionf(1.0, 0.0, 0.0, 0.0), Vector3f(10.0, 0.1, 10.0));

	RenderSystem::SceneNode* node2 = sceneManager->GetRootNode()->CreateChildSceneNode("BOX2",
		Vector3f(0.0, 500.0, 0.0), Quaternionf(1.0, 0.0, 0.0, 0.0), Vector3f(10.0, 0.1, 10.0));

	RenderSystem::MeshPtr mesh = std::make_shared<RenderSystem::Mesh>();

	std::string strDataFile = GetProjectAssetDir() + "Lumen/PositionOnly.data";
	std::vector<uint8_t> positionData = baselib::FileUtil::ReadBinaryFile(strDataFile);

	std::string strAttrDataFile = GetProjectAssetDir() + "Lumen/TangentAndNormal.data";
	std::vector<uint8_t> attrData = baselib::FileUtil::ReadBinaryFile(strAttrDataFile);

    std::string indexFile = GetProjectAssetDir() + "Lumen/IndexBuffer.data";
    std::vector<uint8_t> indexData = baselib::FileUtil::ReadBinaryFile(indexFile);
    Vector3f* posPtr = (Vector3f*)positionData.data();

    RenderSystem::VertexData& vertexData = mesh->GetVertexData();
	uint32_t vertexCount = positionData.size() / 12;
	vertexData.Resize(vertexCount, 48);

    RenderSystem::ChannelInfo* channels = vertexData.GetChannels();
	channels[RenderSystem::kShaderChannelPosition].offset = 0;
	channels[RenderSystem::kShaderChannelPosition].format = VertexFormatFloat4;
    channels[RenderSystem::kShaderChannelPosition].stride = sizeof(Vector4f);

    std::vector<Vector4f> position(vertexCount);
    for (uint32_t i = 0; i < vertexCount; i++)
    {
        position[i].x = posPtr[i].x;
        position[i].y = posPtr[i].y;
        position[i].z = posPtr[i].z;
        position[i].w = 1;
    }

	std::vector<float> attrDataFlt(attrData.size());
	for (int i = 0; i < attrData.size(); i++)
	{
		int8_t value = attrData[i];

		attrDataFlt[i] = Clamp(value, (int8_t)-127, (int8_t)127) / 127.0f;
	}

	std::vector<simd_float4> normalData(vertexCount);
	std::vector<simd_float4> tangentData(vertexCount);
	for (uint32_t i = 0; i < vertexCount; i++)
	{
		memcpy(tangentData.data() + i, attrDataFlt.data() + i * 8, 16);
		memcpy(normalData.data() + i, attrDataFlt.data() + 4 + (i * 8), 16);
	}

    // tangentData 和 normalData需要做特殊处理，适配我自己引擎的坐标系
    for (uint32_t i = 0; i < vertexCount; i++)
    {
        //std::swap(tangentData[i].y, tangentData[i].z);
        std::swap(normalData[i].y, normalData[i].z);
    }

	channels[RenderSystem::kShaderChannelTangent].offset = position.size() * 16;
	channels[RenderSystem::kShaderChannelTangent].format = VertexFormatFloat4;
	channels[RenderSystem::kShaderChannelTangent].stride = 16;
	channels[RenderSystem::kShaderChannelNormal].offset = position.size() * 16 + tangentData.size() * 16;
	channels[RenderSystem::kShaderChannelNormal].format = VertexFormatFloat4;
	channels[RenderSystem::kShaderChannelNormal].stride = 16;

    mesh->SetPositions(position.data(), vertexCount);
    mesh->SetNormals(normalData.data(), vertexCount);
    mesh->SetTangents(tangentData.data(), vertexCount);

    uint32_t* indices = new uint32_t[indexData.size() / 2];
	for (uint32_t i = 0; i < indexData.size() / 2; i++)
	{
        indices[i] = ((uint16_t*)indexData.data())[i];
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

	cameraPtr->LookAt(mathutil::Vector3f(1059.769897f, 336.560120f, -833.207886f), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
	cameraPtr->SetLens(60, float(width) / height, 10.0f, 10000.f);
    
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
