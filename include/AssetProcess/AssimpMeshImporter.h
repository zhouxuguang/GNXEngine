#ifndef GNX_ENGINE_ASSIMP_MESH_IMPORTER
#define GNX_ENGINE_ASSIMP_MESH_IMPORTER

#include "AssetProcessDefine.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/cimport.h>
#include "RenderSystem/mesh/Mesh.h"

USING_NS_RENDERSYSTEM

NS_ASSETPROCESS_BEGIN

// Assimp的mesh导入

class AssimpMeshImporter
{
public:
	AssimpMeshImporter(const aiScene* scene);
	~AssimpMeshImporter();

	void LoadMesh();

private:
	const aiScene* mScene = nullptr;

	void getVertexCountAndLayout(aiNode* node, const aiScene* scene);

	void processMeshVertex(const aiScene* scene);

	void setupLayout(Mesh* mesh);

	void processIndice();

	uint32_t mVertexCount = 0;
	uint32_t mVertexSize = 0;
	std::vector<uint32_t> mSubVertexCounts;    //每个submesh的顶点个数
	std::vector<aiMesh*> mSubMeshs;

	//顶点信息
	std::vector<Vector4f> mPosition;
	std::vector<Vector4f> mNormal;
	std::vector<uint32_t> mColor;
	std::vector<Vector2f> mTexCoord0;
	std::vector<Vector2f> mTexCoord1;
	std::vector<Vector4f> mTangent;

	//索引信息和SubMeshInfo
	std::vector<uint32_t> mIndices;
	std::vector<SubMeshInfo> mSubMeshInfos;
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_ASSIMP_MESH_IMPORTER
