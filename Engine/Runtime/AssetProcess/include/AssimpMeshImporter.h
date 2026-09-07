#ifndef GNX_ENGINE_ASSIMP_MESH_IMPORTER
#define GNX_ENGINE_ASSIMP_MESH_IMPORTER

#include "AssetProcessDefine.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/cimport.h>
#include "Runtime/RenderSystem/include/mesh/Mesh.h"
#include "Runtime/MathUtil/include/Vector3.h"

USING_NS_RENDERSYSTEM

NS_ASSETPROCESS_BEGIN

// Assimp的mesh导入

class ASSET_PROCESS_API AssimpMeshImporter
{
public:
	AssimpMeshImporter(const aiScene* scene, const std::string& saveDir);
	~AssimpMeshImporter();

	void LoadMesh(const baselib::NXGUID & guid);

	/**
	 * @brief 将场景几何数据解析为 RenderSystem::Mesh，并编码为 MeshMessage pb bytes
	 * 纯 CPU 路径（不写文件、不碰 GPU），供离线 meshasset 打包工具复用
	 * @param outData 输出的 MeshMessage pb 编码数据
	 * @return 成功返回 true
	 */
	bool EncodeMeshToMemory(std::vector<uint8_t>& outData);

	/// 解析出的顶点总数（EncodeMeshToMemory 后有效）
	uint32_t GetVertexCount() const { return mVertexCount; }

	/// 去重前的原始顶点总数（用于统计/对比，EncodeMeshToMemory 后有效）
	uint32_t GetOriginalVertexCount() const { return mOriginalVertexCount; }

private:
	const aiScene* mScene = nullptr;

	void getVertexCountAndLayout(aiNode* node, const aiScene* scene);

	void processMeshVertex(const aiScene* scene);

	void setupLayout(Mesh* mesh);

	void processIndice();

	/**
	 * @brief 使用 meshoptimizer 按顶点坐标去重，并同步重映射全部顶点属性。
	 *
	 * 仅当所有非空属性数组长度都与 mVertexCount 一致（即各 submesh 属性齐全）时执行，
	 * 否则保持原样（不做去重，避免属性错位）。
	 *
	 * 去重键 = 位置坐标；不同 UV/法线的顶点若坐标相同会合并，属性取首次出现的顶点。
	 * 会同步压缩 mPosition/mNormal/mTexCoord0/mTexCoord1/mTangent 与 mIndices，
	 * 并把 mVertexCount 更新为去重后数量。
	 */
	bool DeduplicateVertices();

	uint32_t mVertexCount = 0;
	uint32_t mOriginalVertexCount = 0;
	uint32_t mVertexSize = 0;
	std::vector<uint32_t> mSubVertexCounts;    //每个submesh的顶点个数
	std::vector<aiMesh*> mSubMeshs;

	//顶点信息
	std::vector<Vector3f> mPosition;
	std::vector<Vector3f> mNormal;
	std::vector<uint32_t> mColor;
	std::vector<Vector2f> mTexCoord0;
	std::vector<Vector2f> mTexCoord1;
	std::vector<Vector4f> mTangent;

	//索引信息和SubMeshInfo
	std::vector<uint32_t> mIndices;
	std::vector<SubMeshInfo> mSubMeshInfos;

	std::string mSaveDir;
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_ASSIMP_MESH_IMPORTER
