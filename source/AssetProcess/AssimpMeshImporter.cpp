#include "AssimpMeshImporter.h"
#include "MeshMessageUtil.h"

NS_ASSETPROCESS_BEGIN

AssimpMeshImporter::AssimpMeshImporter(const aiScene* scene, const std::string& saveDir) : mScene(scene), mSaveDir(saveDir)
{
}

AssimpMeshImporter::~AssimpMeshImporter()
{
}

void AssimpMeshImporter::LoadMesh()
{
	getVertexCountAndLayout(mScene->mRootNode, mScene);
	processMeshVertex(mScene);

	MeshPtr mesh = std::make_shared<Mesh>();
	setupLayout(mesh.get());
	processIndice();

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

	ByteVectorPtr encodedBuffer = MeshMessageUtil::EncodeMeshMessage(mesh.get());

	MeshPtr meshDecode = std::make_shared<Mesh>();
	MeshMessageUtil::DecodeMeshMessage(encodedBuffer->data(), encodedBuffer->size(), meshDecode.get());
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
			Vector4f vector;

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

void AssimpMeshImporter::setupLayout(Mesh* mesh)
{
	VertexData& vertexData = mesh->GetVertexData();
	ChannelInfo* channels = vertexData.GetChannels();
	uint32_t offset = 0;
	if (!mPosition.empty())
	{
		channels[kShaderChannelPosition].offset = offset;
		channels[kShaderChannelPosition].format = VertexFormatFloat4;
		channels[kShaderChannelPosition].stride = sizeof(Vector4f);
		mVertexSize += 16;
		offset += mPosition.size() * sizeof(Vector4f);
	}
	if (!mNormal.empty())
	{
		channels[kShaderChannelNormal].offset = offset;
		channels[kShaderChannelNormal].format = VertexFormatFloat4;
		channels[kShaderChannelNormal].stride = sizeof(Vector4f);
		mVertexSize += 16;
		offset += mNormal.size() * sizeof(Vector4f);
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