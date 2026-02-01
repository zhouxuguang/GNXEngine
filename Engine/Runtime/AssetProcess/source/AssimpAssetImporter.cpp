#include "AssimpAssetImporter.h"
#include "AssimpMeshImporter.h"
#include "TextureImporter.h"
#include "Runtime/BaseLib/include/BaseLib.h"

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
				// 高度为0说明是压缩的嵌入式纹理（如DDS）
				if (texture->mHeight == 0)
				{
					// mHeight = 0 means compressed embedded texture (e.g. DDS)
					// texture->mWidth contains the size in bytes
					// texture->pcData contains the compressed data
					baselib::NXGUID guid = CreateGUIDFromBinaryData((uint8_t*)texture->pcData, texture->mWidth);
					std::string guidStr = baselib::GUIDToString(guid);
					std::string outputFileName = guidStr + ".ktx";

					// 使用ImageImporter从内存导入嵌入式纹理
                    TextureImporter::ImportFromMemory((uint8_t*)texture->pcData, texture->mWidth, saveDir, outputFileName);
				}
				else
				{
					// 未压缩的嵌入式纹理（ARGB8888等）
					// texture->mHeight > 0 表示未压缩的像素数据
					// texture->mWidth 表示宽度
					// texture->mHeight 表示高度
					// texture->pcData 包含像素数据（每个像素是aiTexel结构，包含r,g,b,a）
					uint32_t pixelCount = texture->mWidth * texture->mHeight;
					std::vector<uint8_t> pixelData;
					pixelData.reserve(pixelCount * 4); // RGBA每个像素4字节

					for (uint32_t j = 0; j < pixelCount; j++)
					{
						const aiTexel& texel = texture->pcData[j];
						pixelData.push_back(texel.r);
						pixelData.push_back(texel.g);
						pixelData.push_back(texel.b);
						pixelData.push_back(texel.a);
					}

					// 使用LoadFromRawPixels方法直接处理原始像素数据
					baselib::NXGUID guid = CreateGUIDFromBinaryData(pixelData.data(), pixelData.size());
					std::string guidStr = baselib::GUIDToString(guid);
					std::string outputFileName = guidStr + ".ktx";

                    TextureImporter::ImportFromRawPixels(pixelData.data(), texture->mWidth, texture->mHeight,
						imagecodec::FORMAT_RGBA8, saveDir, outputFileName);
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
            TextureImporter imageImporter;
			imageImporter.Import(parentDir + std::string("/") + std::string(diffuseMap.C_Str()), saveDir);
		}

		aiString baseColorMap;
		material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &baseColorMap);
		
		aiString normalMap;
		material->Get(AI_MATKEY_TEXTURE_NORMALS(0), normalMap);
		{
            TextureImporter imageImporter;
            imageImporter.Import(parentDir + std::string("/") + std::string(normalMap.C_Str()), saveDir);
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
