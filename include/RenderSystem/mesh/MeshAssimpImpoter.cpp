//
//  MeshAssimpImpoter.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#include "MeshAssimpImpoter.h"
#include "Material.h"
#include "ImageCodec/ImageDecoder.h"
#include "RenderSystem/ImageTextureUtil.h"
#include "MathUtil/Quaternion.h"
#include "BuildSetting.h"

NS_RENDERSYSTEM_BEGIN

static Texture2DPtr TextureFromFile(const char *filename)
{
    if (filename == nullptr)
    {
        return nullptr;
    }
    
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    if (!imagecodec::ImageDecoder::DecodeFile(filename, image.get()))
    {
        return nullptr;
    }
    
    TextureDescriptor textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    textureDescriptor.mipmaped = true;
    
    Texture2DPtr texture = getRenderDevice()->createTextureWithDescriptor(textureDescriptor);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->replaceRegion(rect, image->GetPixels());
    return texture;
}

static Texture2DPtr CreateDiffuseTexture(float r, float g, float b)
{
    uint8_t *pData = (uint8_t*)malloc(4);
    pData[0] = r * 255;
    pData[1] = g * 255;
    pData[2] = b * 255;
    pData[3] = 255;
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(FORMAT_RGBA8, 1, 1, pData, free);
    
    TextureDescriptor textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    //textureDescriptor.mipmaped = true;
    
    Texture2DPtr texture = getRenderDevice()->createTextureWithDescriptor(textureDescriptor);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->replaceRegion(rect, image->GetPixels());
    return texture;
}

static Texture2DPtr CreateMetalRoughTexture()
{
    uint8_t *pData = (uint8_t*)malloc(4);
    pData[0] = 0;
    pData[1] = 255;
    pData[2] = 255;
    pData[3] = 255;
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(FORMAT_RGBA8, 1, 1, pData, free);
    
    TextureDescriptor textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    //textureDescriptor.mipmaped = true;
    
    Texture2DPtr texture = getRenderDevice()->createTextureWithDescriptor(textureDescriptor);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->replaceRegion(rect, image->GetPixels());
    return texture;
}

static Texture2DPtr CreateNormalTexture()
{
    uint8_t *pData = (uint8_t*)malloc(4);
    pData[0] = 0;
    pData[1] = 0;
    pData[2] = 255;
    pData[3] = 0;
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(FORMAT_RGBA8, 1, 1, pData, free);
    
    TextureDescriptor textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    //textureDescriptor.mipmaped = true;
    
    Texture2DPtr texture = getRenderDevice()->createTextureWithDescriptor(textureDescriptor);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->replaceRegion(rect, image->GetPixels());
    return texture;
}

static Texture2DPtr CreateEmmisveTexture()
{
    uint8_t *pData = (uint8_t*)malloc(4);
    pData[0] = 0;
    pData[1] = 0;
    pData[2] = 0;
    pData[3] = 0;
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(FORMAT_RGBA8, 1, 1, pData, free);
    
    TextureDescriptor textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    //textureDescriptor.mipmaped = true;
    
    Texture2DPtr texture = getRenderDevice()->createTextureWithDescriptor(textureDescriptor);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->replaceRegion(rect, image->GetPixels());
    return texture;
}

static Texture2DPtr CreateAOTexture()
{
    uint8_t *pData = (uint8_t*)malloc(4);
    pData[0] = 255;
    pData[1] = 0;
    pData[2] = 0;
    pData[3] = 0;
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(FORMAT_RGBA8, 1, 1, pData, free);
    
    TextureDescriptor textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    //textureDescriptor.mipmaped = true;
    
    Texture2DPtr texture = getRenderDevice()->createTextureWithDescriptor(textureDescriptor);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->replaceRegion(rect, image->GetPixels());
    return texture;
}

static Texture2DPtr TextureFromMemory(const uint8_t* pImageData, uint32_t dataSize)
{
    if (pImageData == nullptr || 0 == dataSize)
    {
        return nullptr;
    }
    
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    if (!imagecodec::ImageDecoder::DecodeMemory(pImageData, dataSize, image.get()))
    {
        return nullptr;
    }
    
    TextureDescriptor textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    
    Texture2DPtr texture = getRenderDevice()->createTextureWithDescriptor(textureDescriptor);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->replaceRegion(rect, image->GetPixels());
    return texture;
}

MeshAssimpImpoter::MeshAssimpImpoter()
{
}

MeshAssimpImpoter::~MeshAssimpImpoter()
{
}

struct NodeHierarchy
{
    std::string name;          //当前节点的名字
    std::string parentName;   //父节点的名字
    aiMatrix4x4 globalMatrix;  //全局的变换矩阵
    aiMatrix4x4 localMatrix;  //骨骼的局部变换矩阵
};

static std::vector<std::string> LoadJointNames(const aiMesh* mesh, const std::vector<NodeHierarchy>& nodes)
{
    unsigned int boneCount = (unsigned int)mesh->mNumBones;
    std::vector<std::string> result(boneCount, "Not Set");

    for (unsigned int i = 0; i < boneCount; ++i)
    {
        aiBone* node = mesh->mBones[i];
        result[i] = node->mName.C_Str();
    }

    return result;
    
//    std::vector<std::string> result;
//    for (auto &iter : nodes)
//    {
//        result.push_back(iter.name);
//    }
//    return result;
}

int GetBoneIndex(const std::vector<std::string>& boneNames, const std::string& name)
{
    for (uint32_t i = 0; i < boneNames.size(); i ++)
    {
        if (name == boneNames[i])
        {
            return i;
        }
    }
    
    return -1;
}

inline Matrix4x4f assimpToMyMatrix(const aiMatrix4x4& mat)
{
    Matrix4x4f m;
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            m[y][x] = mat[y][x];
        }
    }
    return m;
}

AnimationPose LoadBindPose(const aiMesh* mesh, const std::vector<NodeHierarchy>& nodes)
{
    unsigned int boneCount = mesh->mNumBones;
    AnimationPose result(boneCount);

    for (unsigned int i = 0; i < boneCount; ++i)
    {
        aiBone* bone = mesh->mBones[i];
        std::string nodeName = bone->mName.C_Str();

        // 找到父节点的索引
        std::string parentName = "";
        for (auto &iter : nodes)
        {
            if (iter.name == nodeName)
            {
                parentName = iter.parentName;
                break;
            }
        }
        
        int parentIndex = -1;
        for (unsigned int j = 0; j < boneCount; ++j)
        {
            aiBone* bone = mesh->mBones[j];
            if (std::string(bone->mName.C_Str()) == parentName)
            {
                parentIndex = j;
                break;
            }
        }
        
        //设置父节点的索引
        result.SetParent(i, parentIndex);
        
        // 找到当前节点对应的变换矩阵
        aiMatrix4x4 localMatrix;
        aiMatrix4x4 globalMatrix;
        for (auto &iter : nodes)
        {
            if (iter.name == nodeName)
            {
                localMatrix = iter.localMatrix;
                globalMatrix = iter.globalMatrix;
                break;
            }
        }
        
        aiMatrix4x4 bindPoseMat = bone->mOffsetMatrix.Inverse();
//        aiVector3D scale;
//        aiVector3D rotationAxis;
//        float rotationAngle = 0;
//        aiVector3D position;
//        bindPoseMat.Decompose(scale, rotationAxis, rotationAngle, position);
//        rotationAxis = rotationAxis.NormalizeSafe();
        
        Matrix4x4f local = assimpToMyMatrix(bindPoseMat);
        
        //加载每个骨骼的局部变换
        Transform transform;
        transform.TransformFromMat4(local);
//        transform.position = Vector3f(position.x, position.y, position.z);
//        transform.scale = Vector3f(scale.x, scale.y, scale.z);
//        transform.rotation.FromAngleAxis(rotationAngle * 57.29577951308232, Vector3f(rotationAxis.x, rotationAxis.y, rotationAxis.z));
        
        result.SetLocalTransform(i, transform);
    }

    return result;
}

void ReadNodeHierarchy(const aiNode* pNode, const aiMatrix4x4& parentTransform, std::vector<NodeHierarchy>& nodes)
{
    std::string nodeName(pNode->mName.data);
    std::string parentName = "";
    if (pNode->mParent)
    {
        parentName = pNode->mParent->mName.C_Str();
    }

    aiMatrix4x4 nodeTransformation = (pNode->mTransformation);
    aiMatrix4x4 globalTransformation = nodeTransformation * parentTransform;

    NodeHierarchy node;
    node.name = nodeName;
    node.parentName = parentName;
    node.globalMatrix = globalTransformation;
    node.localMatrix = nodeTransformation;
    nodes.push_back(node);

    for (uint32_t i = 0 ; i < pNode->mNumChildren ; i++)
    {
        ReadNodeHierarchy(pNode->mChildren[i], globalTransformation, nodes);
    }
}

bool MeshAssimpImpoter::ImportFromFile(const std::string &fileName, Mesh* mesh, SkinnedMesh* skinnedMesh)
{
    if (fileName.empty() || nullptr == mesh)
    {
        return false;
    }
    
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(fileName.c_str(),
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
                                             aiProcess_GenBoundingBoxes  |
                                             aiProcess_FixInfacingNormals |
                                             aiProcess_JoinIdenticalVertices  //相同顶点只索引一次
                                             //aiProcess_FlipWindingOrder
                                             );
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        return false;
    }
    
    //设置当前模型文件的路径
    mDirectory = fileName.substr(0, fileName.find_last_of('/') + 1);
    
    getVertexCountAndLayout(scene->mRootNode, scene);
    mMaterials = processMeshVertex(scene);
    
    if (scene->HasAnimations())
    {
        // 恢复bone的层次信息
        std::vector<NodeHierarchy> nodes;
        ReadNodeHierarchy(scene->mRootNode, aiMatrix4x4(), nodes);
        
        std::vector<std::string> boneNames = LoadJointNames(scene->mMeshes[0], nodes);
        LoadAnimationClips(scene, boneNames);
        mBoneNames = std::move(boneNames);
        
        mBindPose = LoadBindPose(scene->mMeshes[0], nodes);
        
        setupSkinnedMeshLayout(skinnedMesh);
        processIndice();
        
        skinnedMesh->GetVertexData().Resize(mVertexCount, mVertexSize);
        skinnedMesh->SetPositions(mPosition.data(), mPosition.size());
        skinnedMesh->SetNormals(mNormal.data(), mNormal.size());
        skinnedMesh->SetColors(mColor.data(), mColor.size());
        skinnedMesh->SetUv(0, mTexCoord0.data(), mTexCoord0.size());
        skinnedMesh->SetUv(1, mTexCoord1.data(), mTexCoord1.size());
        skinnedMesh->SetTangents(mTangent.data(), mTangent.size());
        skinnedMesh->SetBoneIndexs(mBoneIndex.data(), mBoneIndex.size());
        skinnedMesh->SetBoneWeights(mBoneWeight.data(), mBoneWeight.size());
        skinnedMesh->SetIndices(mIndices.data(), mIndices.size());
        
        VertexData& skinnedVertexData = skinnedMesh->GetSkinnedVertexData();
        skinnedVertexData.Resize(mVertexCount, mVertexSize);
        memcpy(skinnedVertexData.GetDataPtr(), skinnedMesh->GetVertexData().GetDataPtr(), skinnedVertexData.GetDataSize());
        
        //构建gpu资源
        skinnedMesh->SetUpBuffer();
        
        for (auto &iter : mSubMeshInfos)
        {
            skinnedMesh->AddSubMeshInfo(iter);
        }
    }
    
    else
    {
        setupLayout(mesh);
        processIndice();
        
        mesh->GetVertexData().Resize(mVertexCount, mVertexSize);
        mesh->SetPositions(mPosition.data(), mPosition.size());
        mesh->SetNormals(mNormal.data(), mNormal.size());
        mesh->SetColors(mColor.data(), mColor.size());
        mesh->SetUv(0, mTexCoord0.data(), mTexCoord0.size());
        mesh->SetUv(1, mTexCoord1.data(), mTexCoord1.size());
        mesh->SetTangents(mTangent.data(), mTangent.size());
        mesh->SetIndices(mIndices.data(), mIndices.size());
        
        //构建gpu资源
        mesh->SetUpBuffer();
        
        for (auto &iter : mSubMeshInfos)
        {
            mesh->AddSubMeshInfo(iter);
        }
    }
    
    return true;
}

void MeshAssimpImpoter::LoadAnimationClips(const aiScene *scene, const std::vector<std::string>& boneNames)
{
    if (scene->mNumAnimations <= 0)
    {
        return;
    }
    
    for (int nAniIndex = 0; nAniIndex < scene->mNumAnimations; nAniIndex ++)
    {
        aiAnimation* anim = scene->mAnimations[nAniIndex];
        
        AnimationClipPtr animationClip = std::make_shared<AnimationClip>();
        std::string name = anim->mName.C_Str();
        animationClip->SetName(name);
        
        // 这个才是真实的动画时长（秒为单位），最终动画内部的时间除以anim->mTicksPerSecond得到真实的秒数，目前这个变量没什么用
        float duration = anim->mDuration / anim->mTicksPerSecond;

        // 每个channel代表一个骨骼
        for (int i = 0; i < anim->mNumChannels; i++)
        {
            aiNodeAnim* channel = anim->mChannels[i];
            
            int boneIndex = GetBoneIndex(boneNames, channel->mNodeName.C_Str());
            if (-1 == boneIndex)
            {
                continue;
            }
            assert(boneIndex != -1);
            
            TransformTrack &transformTrack = (*animationClip)[boneIndex];
            
            VectorTrackPtr positionTrack = transformTrack.GetPositionTrack();
            QuaternionTrackPtr quaternionTrack = transformTrack.GetRotationTrack();
            VectorTrackPtr scaleTrack = transformTrack.GetScaleTrack();
            
            positionTrack->Resize(channel->mNumPositionKeys);
            quaternionTrack->Resize(channel->mNumRotationKeys);
            scaleTrack->Resize(channel->mNumScalingKeys);
            
            for (int j = 0; j < channel->mNumPositionKeys; j++)
            {
                AnimationFrame<3>& frame = (*positionTrack)[j];
                frame.mValue[0] = channel->mPositionKeys[j].mValue.x;
                frame.mValue[1] = channel->mPositionKeys[j].mValue.y;
                frame.mValue[2] = channel->mPositionKeys[j].mValue.z;
                frame.mTime = channel->mPositionKeys[j].mTime / anim->mTicksPerSecond;
            }
            
            
            for (int j = 0; j < channel->mNumRotationKeys; j++)
            {
                AnimationFrame<4>& frame = (*quaternionTrack)[j];
                frame.mValue[0] = channel->mRotationKeys[j].mValue.w;
                frame.mValue[1] = channel->mRotationKeys[j].mValue.x;
                frame.mValue[2] = channel->mRotationKeys[j].mValue.y;
                frame.mValue[3] = channel->mRotationKeys[j].mValue.z;
                frame.mTime = channel->mRotationKeys[j].mTime / anim->mTicksPerSecond;
            }
            
            for (int j = 0; j < channel->mNumScalingKeys; j++)
            {
                AnimationFrame<3>& frame = (*scaleTrack)[j];
                frame.mValue[0] = channel->mScalingKeys[j].mValue.x;
                frame.mValue[1] = channel->mScalingKeys[j].mValue.y;
                frame.mValue[2] = channel->mScalingKeys[j].mValue.z;
                frame.mTime = channel->mScalingKeys[j].mTime / anim->mTicksPerSecond;
            }
            
            positionTrack->UpdateIndexLookupTable();
            quaternionTrack->UpdateIndexLookupTable();
            scaleTrack->UpdateIndexLookupTable();
        }
        
        animationClip->RecalculateDuration();
        mAnimationClips.push_back(animationClip);
    }
}

void MeshAssimpImpoter::getVertexCountAndLayout(aiNode *node, const aiScene *scene)
{
    // 处理节点所有的网格（如果有的话）
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        mVertexCount += mesh->mNumVertices;
        mSubVertexCounts.push_back(mesh->mNumVertices);
        mSubMeshs.push_back(mesh);
    }
    
    // 接下来对它的子节点重复这一过程
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        getVertexCountAndLayout(node->mChildren[i], scene);
    }
}

void MeshAssimpImpoter::ProcessMatTexture(MaterialPtr mat, aiMaterial *const material, const std::vector<Texture2DPtr>& fileTextures)
{
    aiString s;
    if (AI_SUCCESS == material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), s))
    {
        if (s.data[0] == '*')
        {
            std::string textureName = std::string(s.data);
            std::string subName = textureName.substr(1, textureName.length() - 1);
            int textureId = std::stoi(subName);
            if (textureId < fileTextures.size())
            {
                if (fileTextures[textureId]) {
                    mat->SetTexture("diffuseTexture", fileTextures[textureId]);
                }
            }
        }
    }
    
    //AI_MATKEY_COLOR_AMBIENT
    aiColor3D ambientColor;
    material->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor);
    mat->SetColor("ambientColor", Vector4f(ambientColor.r, ambientColor.g, ambientColor.b, 1.0));
    
    aiColor3D diffuseColor;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
    mat->SetColor("diffuseColor", Vector4f(diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0));
    
    //AI_MATKEY_COLOR_SPECULAR
    aiColor3D specularColor;
    material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
    mat->SetColor("specularColor", Vector4f(specularColor.r, specularColor.g, specularColor.b, 1.0));
    
    //AI_MATKEY_SHININESS
    float shininess;
    aiReturn ret = material->Get(AI_MATKEY_SHININESS, shininess);
    mat->SetValue("shininess", shininess);
    
    //获得相关的纹理贴图
    aiString diffuseMap;
    material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseMap);
    Texture2DPtr diffuseTexture = TextureFromFile((mDirectory + "Woman.png").c_str());
    //Texture2DPtr diffuseTexture = TextureFromFile((mDirectory + diffuseMap.C_Str()).c_str());
    if (diffuseTexture) {
        mat->SetTexture("diffuseTexture", diffuseTexture);
    }
    
    printf("diffuse texname = %s\n", diffuseMap.C_Str());
    
    aiString normalMap;
    material->Get(AI_MATKEY_TEXTURE_NORMALS(0), normalMap);
    Texture2DPtr normalTexture = TextureFromFile((mDirectory + normalMap.C_Str()).c_str());
    if (normalTexture) {
        mat->SetTexture("normalTexture", normalTexture);
    }
    
    aiString specularMap;
    material->Get(AI_MATKEY_TEXTURE_SPECULAR(0), specularMap);
    Texture2DPtr specularTexture = TextureFromFile((mDirectory + specularMap.C_Str()).c_str());
    if (specularTexture) {
        mat->SetTexture("specularTexture", specularTexture);
    }
    
    aiString baseColorMap;
    material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &baseColorMap);
    Texture2DPtr baseColorTexture = TextureFromFile((mDirectory + baseColorMap.C_Str()).c_str());
    if (baseColorTexture)
    {
        mat->SetTexture("diffuseTexture", baseColorTexture);
    }
    
    //加载metallic贴图
    aiString metallicMap;
    material->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &metallicMap);
    Texture2DPtr metallicTexture = TextureFromFile((mDirectory + metallicMap.C_Str()).c_str());
    if (metallicTexture)
    {
        mat->SetTexture("metallicTexture", metallicTexture);
    }
    
    //加载roughness贴图
    aiString roughnessMap;
    material->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughnessMap);
    Texture2DPtr roughnessTexture = TextureFromFile((mDirectory + roughnessMap.C_Str()).c_str());
    if (roughnessTexture)
    {
        mat->SetTexture("roughnessTexture", roughnessTexture);
    }
    assert(roughnessMap == metallicMap);
    
    if (roughnessMap == metallicMap)
    {
        printf("");
    }
    
    //加载自发光材质贴图
    aiString emissiveMap;
    material->GetTexture(aiTextureType_EMISSIVE, 0, &emissiveMap);
    Texture2DPtr emissiveTexture = TextureFromFile((mDirectory + emissiveMap.C_Str()).c_str());
    if (emissiveTexture)
    {
        mat->SetTexture("emissiveTexture", emissiveTexture);
    }
    
    // 加载AO贴图
    aiString aoMap;
    material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &aoMap);
    Texture2DPtr ambientTexture = TextureFromFile((mDirectory + aoMap.C_Str()).c_str());
    if (ambientTexture)
    {
        mat->SetTexture("ambientTexture", ambientTexture);
    }
    
    //针对没有pbr纹理的模型，创建默认的纹理
    if (baseColorTexture == nullptr && diffuseTexture == nullptr)
    {
        diffuseTexture = CreateDiffuseTexture(diffuseColor.r, diffuseColor.g, diffuseColor.b);
        mat->SetTexture("diffuseTexture", diffuseTexture);
    }
    
    if (roughnessTexture == nullptr)
    {
        roughnessTexture = CreateMetalRoughTexture();
        mat->SetTexture("roughnessTexture", roughnessTexture);
    }
    
    if (!normalTexture)
    {
        normalTexture = CreateNormalTexture();
        mat->SetTexture("normalTexture", normalTexture);
    }
    
    if (!emissiveTexture)
    {
        emissiveTexture = CreateEmmisveTexture();
        mat->SetTexture("emissiveTexture", emissiveTexture);
    }
    
    if (!ambientTexture)
    {
        ambientTexture = CreateAOTexture();
        mat->SetTexture("ambientTexture", ambientTexture);
    }
}

std::vector<std::shared_ptr<Material>> MeshAssimpImpoter::processMeshVertex(const aiScene *scene)
{
    //提前申请好内存
    mPosition.reserve(mVertexCount);
    mNormal.reserve(mVertexCount);
    mColor.reserve(mVertexCount);
    mTexCoord0.reserve(mVertexCount);
    mTexCoord1.reserve(mVertexCount);
    mTangent.reserve(mVertexCount);
    mBoneIndex.resize(mVertexCount);
    mBoneWeight.resize(mVertexCount);
    
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
    
    std::vector<Texture2DPtr> fileTextures;
    
    for (int i = 0; i < scene->mNumTextures; i++)
    {
        const aiTexture *texture = scene->mTextures[i];
        if (texture != nullptr)
        {
            // 高度为0说明是嵌入的纹理
            if (texture->mHeight == 0)
            {
                // mHeight = 0 means embedded textures inside
                // here we use stb to save texture images
                Texture2DPtr texturePtr = TextureFromMemory((unsigned char *)texture->pcData, texture->mWidth);
                fileTextures.push_back(texturePtr);
            }
        }
    }
    
    // 全局的变换矩阵，用于将顶点矫正到正确的位置
    aiMatrix4x4 globalTransform = scene->mRootNode->mTransformation;
    
    std::vector<MaterialPtr> mats;
    
    int idx = 0;
    for (auto mesh : mSubMeshs)
    {
        MaterialPtr mat = nullptr;
        bool hasAnimation = scene->HasAnimations();
        if (hasAnimation)
        {
            if (BuildSetting::mCPUSkinning)
            {
                mat = Material::CreateMaterial("SkinnedPBR");
            }
            else
            {
                mat = Material::CreateMaterial("GPUSkin");
            }
            
        }
        else
        {
            mat = Material::CreateMaterial("PBR");
        }
        
        aiMaterial *const material = scene->mMaterials[mesh->mMaterialIndex];
        aiString name;
        material->Get(AI_MATKEY_NAME, name);
        mat->SetName(name.C_Str());
        ProcessMatTexture(mat, material, fileTextures);
        mats.push_back(mat);
        
        // 遍历submesh的每个顶点
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
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
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
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
                //                vector.x = mesh->mTangents[i].x;
                //                vector.y = mesh->mTangents[i].y;
                //                vector.z = mesh->mTangents[i].z;
                
            }
        }
        
        // 处理骨骼的数据
        std::vector<uint> boneCounts;
        boneCounts.resize(mBoneIndex.size(), 0);
        
        for (uint32_t i = 0; i < mesh->mNumBones; i++)
        {
            aiBone *bone = mesh->mBones[i];
            
            for (uint32_t j = 0; j < bone->mNumWeights; j++)
            {
                uint32_t vertexID = bone->mWeights[j].mVertexId + idx;
                float weight = bone->mWeights[j].mWeight;
                boneCounts[vertexID]++;
                switch (boneCounts[vertexID])
                {
                    case 1:
                        mBoneIndex[vertexID].x = i;
                        mBoneWeight[vertexID].x = weight;
                        break;
                    case 2:
                        mBoneIndex[vertexID].y = i;
                        mBoneWeight[vertexID].y = weight;
                        break;
                    case 3:
                        mBoneIndex[vertexID].z = i;
                        mBoneWeight[vertexID].z = weight;
                        break;
                    case 4:
                        mBoneIndex[vertexID].w = i;
                        mBoneWeight[vertexID].w = weight;
                        break;
                    default:
                        //std::cout << "err: unable to allocate bone to vertex" << std::endl;
                        break;

                }
            }
        }
        
        idx += mesh->mNumVertices;
        
        //将权重进行归一化
        for (int i = 0; i < mBoneWeight.size(); i++)
        {
            Vector4f & boneWeights = mBoneWeight[i];
            float totalWeight = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
            if (totalWeight > 0.0f) 
            {
                boneWeights = Vector4f(
                    boneWeights.x / totalWeight,
                    boneWeights.y / totalWeight,
                    boneWeights.z / totalWeight,
                    boneWeights.w / totalWeight
                );
            }
        }
    }
    
    return mats;
}

void MeshAssimpImpoter::setupLayout(Mesh* mesh)
{
    VertexData & vertexData = mesh->GetVertexData();
    ChannelInfo* channels = vertexData.GetChannels();
    uint32_t offset = 0;
    if (!mPosition.empty())
    {
        channels[kShaderChannelPosition].offset = offset;
        channels[kShaderChannelPosition].format = VertexFormatFloat3;
        mVertexSize += 16;
        offset += mPosition.size() * sizeof(simd_float3);
    }
    if (!mNormal.empty())
    {
        channels[kShaderChannelNormal].offset = offset;
        channels[kShaderChannelNormal].format = VertexFormatFloat3;
        mVertexSize += 16;
        offset += mNormal.size() * sizeof(simd_float3);
    }
    if (!mColor.empty())
    {
        channels[kShaderChannelColor].offset = offset;
        channels[kShaderChannelColor].format = VertexFormatUInt;
        mVertexSize += 4;
        offset += mColor.size() * sizeof(uint32_t);
    }
    if (!mTexCoord0.empty())
    {
        channels[kShaderChannelTexCoord0].offset = offset;
        channels[kShaderChannelTexCoord0].format = VertexFormatFloat2;
        mVertexSize += 8;
        offset += mTexCoord0.size() * sizeof(simd_float2);
    }
    if (!mTexCoord1.empty())
    {
        channels[kShaderChannelTexCoord1].offset = offset;
        channels[kShaderChannelTexCoord1].format = VertexFormatFloat2;
        mVertexSize += 8;
        offset += mTexCoord1.size() * sizeof(simd_float2);
    }
    if (!mTangent.empty())
    {
        channels[kShaderChannelTangent].offset = offset;
        channels[kShaderChannelTangent].format = VertexFormatFloat4;
        mVertexSize += 16;
        offset += mTangent.size() * sizeof(simd_float4);
    }
    
    channels[kShaderChannelPosition].stride = sizeof(simd_float3);
    channels[kShaderChannelNormal].stride = sizeof(simd_float3);
    channels[kShaderChannelColor].stride = 0;
    channels[kShaderChannelTexCoord0].stride = sizeof(simd_float2);
    channels[kShaderChannelTexCoord1].stride = sizeof(simd_float2);
    channels[kShaderChannelTangent].stride = sizeof(simd_float4);
}

void MeshAssimpImpoter::setupSkinnedMeshLayout(SkinnedMesh* skinnedMesh)
{
    {
        VertexData & vertexData = skinnedMesh->GetVertexData();
        ChannelInfo* channels = vertexData.GetChannels();
        uint32_t offset = 0;
        if (!mPosition.empty())
        {
            channels[kShaderChannelPosition].offset = offset;
            channels[kShaderChannelPosition].format = VertexFormatFloat3;
            mVertexSize += 16;
            offset += mPosition.size() * sizeof(simd_float3);
        }
        if (!mNormal.empty())
        {
            channels[kShaderChannelNormal].offset = offset;
            channels[kShaderChannelNormal].format = VertexFormatFloat3;
            mVertexSize += 16;
            offset += mNormal.size() * sizeof(simd_float3);
        }
        if (!mColor.empty())
        {
            channels[kShaderChannelColor].offset = offset;
            channels[kShaderChannelColor].format = VertexFormatUInt;
            mVertexSize += 4;
            offset += mColor.size() * sizeof(uint32_t);
        }
        if (!mTexCoord0.empty())
        {
            channels[kShaderChannelTexCoord0].offset = offset;
            channels[kShaderChannelTexCoord0].format = VertexFormatFloat2;
            mVertexSize += 8;
            offset += mTexCoord0.size() * sizeof(simd_float2);
        }
        if (!mTexCoord1.empty())
        {
            channels[kShaderChannelTexCoord1].offset = offset;
            channels[kShaderChannelTexCoord1].format = VertexFormatFloat2;
            mVertexSize += 8;
            offset += mTexCoord1.size() * sizeof(simd_float2);
        }
        if (!mTangent.empty())
        {
            channels[kShaderChannelTangent].offset = offset;
            channels[kShaderChannelTangent].format = VertexFormatFloat4;
            mVertexSize += 16;
            offset += mTangent.size() * sizeof(simd_float4);
        }
        if (!mBoneIndex.empty())
        {
            channels[kShaderChannelBoneIndex].offset = offset;
            channels[kShaderChannelBoneIndex].format = VertexFormatInt4;
            mVertexSize += 16;
            offset += mBoneIndex.size() * sizeof(BoneIndexInfo);
        }
        if (!mBoneWeight.empty())
        {
            channels[kShaderChannelWeight].offset = offset;
            channels[kShaderChannelWeight].format = VertexFormatFloat4;
            mVertexSize += 16;
            offset += mBoneWeight.size() * sizeof(simd_float4);
        }
        
        channels[kShaderChannelPosition].stride = sizeof(simd_float3);
        channels[kShaderChannelNormal].stride = sizeof(simd_float3);
        channels[kShaderChannelColor].stride = 0;
        channels[kShaderChannelTexCoord0].stride = sizeof(simd_float2);
        channels[kShaderChannelTexCoord1].stride = sizeof(simd_float2);
        channels[kShaderChannelTangent].stride = sizeof(simd_float4);
        channels[kShaderChannelBoneIndex].stride = sizeof(BoneIndexInfo);
        channels[kShaderChannelWeight].stride = sizeof(simd_float4);
    }
    
    {
        VertexData & vertexData = skinnedMesh->GetSkinnedVertexData();
        ChannelInfo* channels = vertexData.GetChannels();
        uint32_t offset = 0;
        if (!mPosition.empty())
        {
            channels[kShaderChannelPosition].offset = offset;
            channels[kShaderChannelPosition].format = VertexFormatFloat3;
            mVertexSize += 16;
            offset += mPosition.size() * sizeof(simd_float3);
        }
        if (!mNormal.empty())
        {
            channels[kShaderChannelNormal].offset = offset;
            channels[kShaderChannelNormal].format = VertexFormatFloat3;
            mVertexSize += 16;
            offset += mNormal.size() * sizeof(simd_float3);
        }
        if (!mColor.empty())
        {
            channels[kShaderChannelColor].offset = offset;
            channels[kShaderChannelColor].format = VertexFormatUInt;
            mVertexSize += 4;
            offset += mColor.size() * sizeof(uint32_t);
        }
        if (!mTexCoord0.empty())
        {
            channels[kShaderChannelTexCoord0].offset = offset;
            channels[kShaderChannelTexCoord0].format = VertexFormatFloat2;
            mVertexSize += 8;
            offset += mTexCoord0.size() * sizeof(simd_float2);
        }
        if (!mTexCoord1.empty())
        {
            channels[kShaderChannelTexCoord1].offset = offset;
            channels[kShaderChannelTexCoord1].format = VertexFormatFloat2;
            mVertexSize += 8;
            offset += mTexCoord1.size() * sizeof(simd_float2);
        }
        if (!mTangent.empty())
        {
            channels[kShaderChannelTangent].offset = offset;
            channels[kShaderChannelTangent].format = VertexFormatFloat4;
            mVertexSize += 16;
            offset += mTangent.size() * sizeof(simd_float4);
        }
        if (!mBoneIndex.empty())
        {
            channels[kShaderChannelBoneIndex].offset = offset;
            channels[kShaderChannelBoneIndex].format = VertexFormatInt4;
            mVertexSize += 16;
            offset += mBoneIndex.size() * sizeof(BoneIndexInfo);
        }
        if (!mBoneWeight.empty())
        {
            channels[kShaderChannelWeight].offset = offset;
            channels[kShaderChannelWeight].format = VertexFormatFloat4;
            mVertexSize += 16;
            offset += mBoneWeight.size() * sizeof(simd_float4);
        }
        
        channels[kShaderChannelPosition].stride = sizeof(simd_float3);
        channels[kShaderChannelNormal].stride = sizeof(simd_float3);
        channels[kShaderChannelColor].stride = 0;
        channels[kShaderChannelTexCoord0].stride = sizeof(simd_float2);
        channels[kShaderChannelTexCoord1].stride = sizeof(simd_float2);
        channels[kShaderChannelTangent].stride = sizeof(simd_float4);
        channels[kShaderChannelBoneIndex].stride = sizeof(BoneIndexInfo);
        channels[kShaderChannelWeight].stride = sizeof(simd_float4);
    }
}

void MeshAssimpImpoter::processIndice()
{
    uint32_t currentIndexCount = 0;
    uint32_t currentVertexCount = 0;
    for (auto mesh : mSubMeshs)
    {
        // 处理顶点索引
        uint16_t indexCount = 0;
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            const aiFace& face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
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

NS_RENDERSYSTEM_END
