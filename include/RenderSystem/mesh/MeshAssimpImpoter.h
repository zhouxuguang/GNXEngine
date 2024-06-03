//
//  MeshAssimpImpoter.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#ifndef FNXENGINE_ASSIMP_INCLUDE_MESH_IMPORTER_INCLUDE_H
#define FNXENGINE_ASSIMP_INCLUDE_MESH_IMPORTER_INCLUDE_H

#include "MeshImporter.h"
#include "RenderCore/RenderDefine.h"
#include "MathUtil/Vector4.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include "Material.h"
#include "../animation/AnimationClip.h"
#include "../skinnedMesh/SkinnedMesh.h"

NS_RENDERSYSTEM_BEGIN

// Assimp 模型导入器
class MeshAssimpImpoter : public MeshImporter
{
public:
    MeshAssimpImpoter();
    
    ~MeshAssimpImpoter();
    
    virtual bool ImportFromFile(const std::string &fileName, Mesh* mesh, SkinnedMesh* skinnedMesh);
    
    std::vector<std::shared_ptr<Material>> GetMaterials() const
    {
        return mMaterials;
    }
    
private:
    uint32_t mVertexCount = 0;
    uint32_t mVertexSize = 0;
    std::vector<uint32_t> mSubVertexCounts;    //各个子mesh的顶点个数
    std::vector<Vector4f> mPosition;
    std::vector<Vector4f> mNormal;
    std::vector<uint32_t> mColor;
    std::vector<Vector2f> mTexCoord0;
    std::vector<Vector2f> mTexCoord1;
    std::vector<Vector4f> mTangent;
    
    std::vector<BoneIndexInfo> mBoneIndex;
    std::vector<Vector4f> mBoneWeight;
    
    std::vector<uint32_t> mIndices;
    std::vector<SubMeshInfo> mSubMeshInfos;
    
    std::vector<aiMesh*> mSubMeshs;
    
    std::string mDirectory;
    
    std::vector<std::shared_ptr<Material>> mMaterials;
    
    void getVertexCountAndLayout(aiNode *node, const aiScene *scene);
    
    std::vector<std::shared_ptr<Material>> processMeshVertex(const aiScene *scene);
    
    void setupLayout(Mesh* mesh);
    
    void setupSkinnedMeshLayout(SkinnedMesh* skinnedMesh);
    
    void processIndice();
    
    // 处理材质相关
    void ProcessMatTexture(MaterialPtr mat, aiMaterial *const material, const std::vector<Texture2DPtr>& fileTextures);
    
    // 加载动画片段
    std::vector<AnimationClipPtr> mAnimationClips;
    void LoadAnimationClips(const aiScene *scene, const std::vector<std::string>& boneNames);
public:
    std::vector<AnimationClipPtr>& GetAnimationClips()
    {
        return mAnimationClips;
    }
    
    AnimationPose mBindPose;
    std::vector<std::string> mBoneNames;
public:
    AnimationPose& GetBindPose()
    {
        return mBindPose;
    }
public:
    std::vector<std::string>& GetBoneNames()
    {
        return mBoneNames;
    }
};

NS_RENDERSYSTEM_END

#endif /* FNXENGINE_ASSIMP_INCLUDE_MESH_IMPORTER_INCLUDE_H */
