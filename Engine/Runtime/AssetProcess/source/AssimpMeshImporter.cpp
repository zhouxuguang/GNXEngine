#include "AssimpMeshImporter.h"
#include "MeshMessageUtil.h"
#include "Runtime/BaseLib/include/FileUtil.h"
#include <meshoptimizer.h>

NS_ASSETPROCESS_BEGIN

AssimpMeshImporter::AssimpMeshImporter(const aiScene* scene, const std::string& saveDir) : mScene(scene), mSaveDir(saveDir)
{
}

AssimpMeshImporter::~AssimpMeshImporter()
{
}

void AssimpMeshImporter::LoadMesh(const baselib::NXGUID& guid)
{
	getVertexCountAndLayout(mScene->mRootNode, mScene);
	mOriginalVertexCount = mVertexCount;
	processMeshVertex(mScene);
	processIndice();
	// 顶点坐标去重（各属性数组同步收窄）——必须在 setupLayout 之前，
	// 否则 channel offset 会按去重前的数组大小计算而错位。
	DeduplicateVertices();

	MeshPtr mesh = std::make_shared<Mesh>();
	setupLayout(mesh.get());

	mesh->GetVertexData().Resize(mVertexCount, mVertexSize);
	mesh->SetPositions(mPosition.data(), mPosition.size());
	mesh->SetNormals(mNormal.data(), mNormal.size());
	mesh->SetColors(mColor.data(), mColor.size());
	mesh->SetUv(0, mTexCoord0.data(), mTexCoord0.size());
	mesh->SetUv(1, mTexCoord1.data(), mTexCoord1.size());
	mesh->SetTangents(mTangent.data(), mTangent.size());
	mesh->SetIndices(mIndices.data(), mIndices.size());

	for (auto& iter : mSubMeshInfos)
	{
		mesh->AddSubMeshInfo(iter);
	}

    ByteVectorPtr encodedBuffer = AssetManager::MeshMessageUtil::EncodeMeshMessage(mesh.get());

	std::string guidStr = baselib::GUIDToString(guid);
	fs::path path = (fs::path(mSaveDir) / guidStr).lexically_normal();

	baselib::FileUtil::WriteBinaryFile(path.string(), encodedBuffer->data(), encodedBuffer->size());

	MeshPtr meshDecode = std::make_shared<Mesh>();
    AssetManager::MeshMessageUtil::DecodeMeshMessage(encodedBuffer->data(), encodedBuffer->size(), meshDecode.get());
}

bool AssimpMeshImporter::EncodeMeshToMemory(std::vector<uint8_t>& outData)
{
	// 清空上一次解析的状态（复用对象时安全）
	mVertexCount = 0;
	mOriginalVertexCount = 0;
	mVertexSize = 0;
	mSubVertexCounts.clear();
	mSubMeshs.clear();
	mPosition.clear();
	mNormal.clear();
	mColor.clear();
	mTexCoord0.clear();
	mTexCoord1.clear();
	mTangent.clear();
	mIndices.clear();
	mSubMeshInfos.clear();

	if (!mScene || !mScene->mRootNode || !mScene->HasMeshes())
	{
		return false;
	}

	getVertexCountAndLayout(mScene->mRootNode, mScene);
	mOriginalVertexCount = mVertexCount;
	processMeshVertex(mScene);

	if (mVertexCount == 0)
	{
		return false;
	}

	processIndice();
	// 顶点坐标去重（各属性数组同步收窄）——必须在 setupLayout 之前，
	// 否则 channel offset 会按去重前的数组大小计算而错位。
	DeduplicateVertices();

	MeshPtr mesh = std::make_shared<Mesh>();
	setupLayout(mesh.get());

	mesh->GetVertexData().Resize(mVertexCount, mVertexSize);
	mesh->SetPositions(mPosition.data(), mPosition.size());
	mesh->SetNormals(mNormal.data(), mNormal.size());
	mesh->SetColors(mColor.data(), mColor.size());
	mesh->SetUv(0, mTexCoord0.data(), mTexCoord0.size());
	mesh->SetUv(1, mTexCoord1.data(), mTexCoord1.size());
	mesh->SetTangents(mTangent.data(), mTangent.size());
	mesh->SetIndices(mIndices.data(), mIndices.size());

	for (auto& iter : mSubMeshInfos)
	{
		mesh->AddSubMeshInfo(iter);
	}

	ByteVectorPtr encodedBuffer = AssetManager::MeshMessageUtil::EncodeMeshMessage(mesh.get());
	if (!encodedBuffer || encodedBuffer->empty())
	{
		return false;
	}

	outData.assign(encodedBuffer->begin(), encodedBuffer->end());
	return true;
}

void AssimpMeshImporter::getVertexCountAndLayout(aiNode* node, const aiScene* scene)
{
	// 处理节点所有的网格（如果有的话）
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		// the node object only contains indices to index the actual objects in the scene.
		// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		mVertexCount += mesh->mNumVertices;
		mSubVertexCounts.push_back(mesh->mNumVertices);
		mSubMeshs.push_back(mesh);
	}

	// 接下来对它的子节点重复这一过程
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		getVertexCountAndLayout(node->mChildren[i], scene);
	}
}

void AssimpMeshImporter::processMeshVertex(const aiScene* scene)
{
	//提前申请好内存
	mPosition.reserve(mVertexCount);
	mNormal.reserve(mVertexCount);
	mColor.reserve(mVertexCount);
	mTexCoord0.reserve(mVertexCount);
	mTexCoord1.reserve(mVertexCount);
	mTangent.reserve(mVertexCount);

	float modelScale = 1.0;
	aiMetadata* metaData = scene->mMetaData;
	if (metaData)
	{
		//坐标单位的缩放
		float scale = 1.0;
		metaData->Get("UnitScaleFactor", scale);

		float scale2 = 1.0;
		metaData->Get("OriginalUnitScaleFactor", scale2);

		modelScale = 1.0f / scale;
	}

	// 全局的变换矩阵，用于将顶点矫正到正确的位置
	aiMatrix4x4 globalTransform = scene->mRootNode->mTransformation;

	int idx = 0;
	for (auto mesh : mSubMeshs)
	{
		// 遍历submesh的每个顶点
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vector3f vector;

			aiVector3D point = globalTransform * mesh->mVertices[i];
			// 顶点
			vector.x = point.x * modelScale;
			vector.y = point.y * modelScale;
			vector.z = point.z * modelScale;
			mPosition.push_back(vector);

			// 法线
			if (mesh->HasNormals())
			{
				aiVector3D normal = globalTransform * mesh->mNormals[i];
				vector.x = normal.x;
				vector.y = normal.y;
				vector.z = normal.z;
				mNormal.push_back(vector);
			}

			// 纹理坐标
			if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				Vector2f vec;
				// 一个顶点最多包含8个不同的纹理坐标，我们假设只包含一个纹理坐标
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				mTexCoord0.push_back(vec);
			}
			if (mesh->mTextureCoords[1])
			{
				Vector2f vec;
				// 一个顶点最多包含8个不同的纹理坐标，我们假设只包含一个纹理坐标
				vec.x = mesh->mTextureCoords[1][i].x;
				vec.y = mesh->mTextureCoords[1][i].y;
				mTexCoord1.push_back(vec);
			}

			// 切线和副切线
			if (mesh->HasTangentsAndBitangents())
			{
				// 切线
				Vector4f vector;
				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;

				//计算偏手性
				Vector3f b = Vector3f(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
				Vector3f t = Vector3f(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
				Vector3f n = Vector3f(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

				Vector3f tempVec = Vector3f::CrossProduct(n, t);
				vector.w = tempVec.DotProduct(b) < 0.0f ? -1.0f : 1.0f;

				aiVector3D tangent = globalTransform * mesh->mTangents[i];
				vector.x = tangent.x;
				vector.y = tangent.y;
				vector.z = tangent.z;

				mTangent.push_back(vector);
			}

			if (mesh->HasVertexColors(0))
			{
			}
		}

		idx += mesh->mNumVertices;
	}
}

// ---------------------------------------------------------------------------
// 顶点去重：使用 meshoptimizer 按【全部顶点属性】作为键去除完全重复的顶点，
// 并同步重映射各属性数组，保持各属性数组长度一致，避免顶点数据膨胀。
//
// 说明：
//  - 仅当所有“非空”属性数组长度都与顶点总数一致时才去重；
//    若某 submesh 缺某属性导致数组长度不一致，则回退（不去重），保证正确性。
//  - 去重键 = 位置 + 法线 + UV + 切线等【所有非空属性】；
//    只有全部属性二进制一致的顶点才会合并，因此不会破坏硬边 / UV 接缝处的
//    法线或纹理坐标（渲染结果与去重前一致，仅剔除真正重复的顶点）。
//  - 会同步更新 mIndices 与 mSubMeshInfos（顶点范围缩小），并把 mVertexCount
//    更新为去重后的唯一顶点数。
// ---------------------------------------------------------------------------
bool AssimpMeshImporter::DeduplicateVertices()
{
	if (mPosition.empty() || mIndices.empty())
	{
		return false;
	}

	const size_t oldVertexCount = mPosition.size();

	// 各属性数组必须与顶点总数一致（允许整体为空：无该属性）
	auto consistent = [oldVertexCount](const auto& attr) 
	{
		return attr.empty() || attr.size() == oldVertexCount;
	};
	if (!consistent(mNormal) || !consistent(mColor) ||
		!consistent(mTexCoord0) || !consistent(mTexCoord1) ||
		!consistent(mTangent))
	{
		// 属性不齐全/长度不一致，无法安全去重，回退原逻辑
		LOG_WARN("AssimpMeshImporter: attribute arrays inconsistent, skip vertex dedup");
		return false;
	}

	// 把所有非空顶点属性组成 meshoptimizer 多流（每个元素 = 一个顶点的一种属性）。
	// meshopt_generateVertexRemapMulti 以全部流的二进制内容为键去重，
	// 只有属性完全一致的顶点才合并（不会破坏 UV/法线接缝）。
	std::vector<meshopt_Stream> streams;
	streams.reserve(6);

	auto addStream = [&streams](const void* data, size_t elemSize) 
	{
		meshopt_Stream s;
		s.data   = data;
		s.size   = elemSize;
		s.stride = elemSize;   // SoA 布局：每元素连续存放
		streams.push_back(s);
	};

	addStream(mPosition.data(), sizeof(Vector3f));
	if (!mNormal.empty())    addStream(mNormal.data(), sizeof(Vector3f));
	if (!mColor.empty())     addStream(mColor.data(), sizeof(uint32_t));
	if (!mTexCoord0.empty()) addStream(mTexCoord0.data(), sizeof(Vector2f));
	if (!mTexCoord1.empty()) addStream(mTexCoord1.data(), sizeof(Vector2f));
	if (!mTangent.empty())   addStream(mTangent.data(), sizeof(Vector4f));

	// remap[i] == ~0u 表示该旧顶点未被任何索引引用（会从顶点缓冲中剔除）
	std::vector<uint32_t> remap(oldVertexCount);

	const size_t uniqueCount = meshopt_generateVertexRemapMulti(
		remap.data(),
		mIndices.data(), mIndices.size(),
		oldVertexCount,
		streams.data(), streams.size());

	if (uniqueCount == 0 || uniqueCount >= oldVertexCount)
	{
		// 没有可去除的重复顶点（或去重失败），无需改写
		return false;
	}

	// 按 remap 重映射各属性数组（SoA）：dst 大小 = uniqueCount
	auto remapAttr = [&](auto& dst) 
	{
		if (dst.empty())
		{
			return;
		}
		using AttrType = typename std::remove_reference<decltype(dst)>::type::value_type;
		std::vector<AttrType> tmp(uniqueCount);
		meshopt_remapVertexBuffer(tmp.data(), dst.data(), oldVertexCount,
								  sizeof(AttrType), remap.data());
		dst.swap(tmp);
	};

	remapAttr(mPosition);
	remapAttr(mNormal);
	remapAttr(mColor);
	remapAttr(mTexCoord0);
	remapAttr(mTexCoord1);
	remapAttr(mTangent);

	// 重映射索引
	std::vector<uint32_t> remappedIndices(mIndices.size());
	meshopt_remapIndexBuffer(remappedIndices.data(), mIndices.data(),
							 mIndices.size(), remap.data());
	mIndices.swap(remappedIndices);

	// 更新顶点计数（子网格顶点范围也一并收窄；firstIndex/indexCount 不受影响）
	mVertexCount = (uint32_t)uniqueCount;
	for (auto& sub : mSubMeshInfos)
	{
		if (sub.vertexCount > mVertexCount)
		{
			sub.vertexCount = mVertexCount;
		}
	}

	LOG_INFO("AssimpMeshImporter: vertex dedup %u -> %u (saved %.1f%%)",
			 (uint32_t)oldVertexCount, mVertexCount,
			 100.0 * (1.0 - (double)uniqueCount / (double)oldVertexCount));
	return true;
}

void AssimpMeshImporter::setupLayout(Mesh* mesh)
{
	VertexData& vertexData = mesh->GetVertexData();
	ChannelInfo* channels = vertexData.GetChannels();
	uint32_t offset = 0;
	if (!mPosition.empty())
	{
		channels[kShaderChannelPosition].offset = offset;
		channels[kShaderChannelPosition].format = VertexFormatFloat3;
		channels[kShaderChannelPosition].stride = sizeof(Vector3f);
		mVertexSize += 12;
		offset += mPosition.size() * sizeof(Vector3f);
	}
	if (!mNormal.empty())
	{
		channels[kShaderChannelNormal].offset = offset;
		channels[kShaderChannelNormal].format = VertexFormatFloat3;
		channels[kShaderChannelNormal].stride = sizeof(Vector3f);
		mVertexSize += 12;
		offset += mNormal.size() * sizeof(Vector3f);
	}
	if (!mColor.empty())
	{
		channels[kShaderChannelColor].offset = offset;
		channels[kShaderChannelColor].format = VertexFormatUInt;
		channels[kShaderChannelColor].stride = sizeof(uint32_t);
		mVertexSize += 4;
		offset += mColor.size() * sizeof(uint32_t);
	}
	if (!mTexCoord0.empty())
	{
		channels[kShaderChannelTexCoord0].offset = offset;
		channels[kShaderChannelTexCoord0].format = VertexFormatFloat2;
		channels[kShaderChannelTexCoord0].stride = sizeof(Vector2f);
		mVertexSize += 8;
		offset += mTexCoord0.size() * sizeof(Vector2f);
	}
	if (!mTexCoord1.empty())
	{
		channels[kShaderChannelTexCoord1].offset = offset;
		channels[kShaderChannelTexCoord1].format = VertexFormatFloat2;
		channels[kShaderChannelTexCoord1].stride = sizeof(Vector2f);
		mVertexSize += 8;
		offset += mTexCoord1.size() * sizeof(Vector2f);
	}
	if (!mTangent.empty())
	{
		channels[kShaderChannelTangent].offset = offset;
		channels[kShaderChannelTangent].format = VertexFormatFloat4;
		channels[kShaderChannelTangent].stride = sizeof(Vector4f);
		mVertexSize += 16;
		offset += mTangent.size() * sizeof(Vector4f);
	}
}

void AssimpMeshImporter::processIndice()
{
	uint32_t currentIndexCount = 0;
	uint32_t currentVertexCount = 0;
	for (auto mesh : mSubMeshs)
	{
		uint16_t indexCount = 0;
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			const aiFace& face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				mIndices.push_back(face.mIndices[j] + currentVertexCount);
			}

			indexCount += face.mNumIndices;
		}

		currentVertexCount += mesh->mNumVertices;

		SubMeshInfo subInfo;
		subInfo.firstIndex = currentIndexCount;
		subInfo.indexCount = indexCount;
		subInfo.vertexCount = mesh->mNumVertices;
		subInfo.topology = PrimitiveMode_TRIANGLES;
		mSubMeshInfos.push_back(subInfo); 

		currentIndexCount += indexCount;
	}
}

NS_ASSETPROCESS_END
