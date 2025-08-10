#include "AssimpAssetImporter.h"
#include "AssimpMeshImporter.h"
#include "ImageImporter.h"
#include "BaseLib/BaseLib.h"

NS_ASSETPROCESS_BEGIN

AssimpAssetImporter::AssimpAssetImporter()
{
}

AssimpAssetImporter::~AssimpAssetImporter()
{
}

bool AssimpAssetImporter::ImportFromFile(const std::string& fileName, const std::string& saveDir)
{
	std::string parentDir = baselib::FileUtil::GetParentPath(fileName);
	std::vector<uint8_t> data = baselib::FileUtil::ReadBinaryFile(fileName);
	if (data.empty())
	{
		return false;
	}

	std::string ktxfile = parentDir + ".ktx";

	// 计算GUID
	baselib::NXGUID guid = CreateGUIDFromBinaryData(data.data(), data.size());
	std::string guidStr = baselib::GUIDToString(guid);

	size_t n = guidStr.length();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName.c_str(),
		aiProcess_SplitLargeMeshes |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |     //三角化
		aiProcess_SortByPType |        //
		//aiProcess_FlipUVs |         //翻转UV坐标
		aiProcess_GenNormals |
		//aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace |   //计算切线空间
		aiProcess_OptimizeMeshes |     //网格优化
		aiProcess_RemoveRedundantMaterials | //移除多余的材质
		//aiProcess_PreTransformVertices | //预变换顶点坐标
		aiProcess_OptimizeGraph | //配合 aiProcess_OptimizeMeshes 使用
		aiProcess_GenBoundingBoxes |
		aiProcess_FixInfacingNormals |
		aiProcess_JoinIdenticalVertices  //相同顶点只索引一次
		//aiProcess_FlipWindingOrder
	);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE 
		|| !scene->mRootNode || !scene->HasMeshes()) // if is Not Zero
	{
		return false;
	}

	if (scene->HasMeshes())
	{
		// 导入mesh
		AssimpMeshImporter assimpMeshImport(scene, saveDir);
		assimpMeshImport.LoadMesh(guid);
	}

	if (scene->HasTextures())
	{
		for (int i = 0; i < scene->mNumTextures; i++)
		{
			const aiTexture* texture = scene->mTextures[i];
			if (texture != nullptr)
			{
				// 高度为0说明是嵌入的纹理
				if (texture->mHeight == 0)
				{
					// mHeight = 0 means embedded textures inside
					// here we use stb to save texture images
					//Texture2DPtr texturePtr = TextureFromMemory((unsigned char*)texture->pcData, texture->mWidth);
					//fileTextures.push_back(texturePtr);
				}
			}
		}
	}

	// 处理材质
	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* material = scene->mMaterials[i];

		//获得相关的纹理贴图

		aiString diffuseMap;
		material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseMap);
		printf("diffuse texname = %s\n", diffuseMap.C_Str());
		{
			ImageImporter imageImporter((parentDir + std::string("/") + std::string(diffuseMap.C_Str())).c_str(), saveDir);
			imageImporter.Load();
		}

		aiString baseColorMap;
		material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &baseColorMap);
		
		aiString normalMap;
		material->Get(AI_MATKEY_TEXTURE_NORMALS(0), normalMap);
		{
			ImageImporter imageImporter((parentDir + std::string("/") + std::string(normalMap.C_Str())).c_str(), saveDir);
			imageImporter.Load();
		}
		
		
		//加载metallic贴图
		aiString metallicMap;
		material->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &metallicMap);
		
		//加载roughness贴图
		aiString roughnessMap;
		material->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughnessMap);
		assert(roughnessMap == metallicMap);
		
		if (roughnessMap == metallicMap)
		{
			printf("");
		}
		
		//加载自发光材质贴图
		aiString emissiveMap;
		material->GetTexture(aiTextureType_EMISSIVE, 0, &emissiveMap);
		
		// 加载AO贴图
		aiString aoMap;
		material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &aoMap);
		printf("aoMap texname = %s\n", aoMap.C_Str());
	}
	

    return true;
}

NS_ASSETPROCESS_END