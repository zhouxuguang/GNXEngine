//
//  SceneManager.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/13.
//

#ifndef GNXENGINE_INCLUDE_SCENEMANGER_INCVSDJVKJND
#define GNXENGINE_INCLUDE_SCENEMANGER_INCVSDJVKJND

#include "RSDefine.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "SceneNode.h"
#include "Camera.h"
#include "Light.h"
#include "ArcballManipulate.h"
#include "mesh/MeshDrawUtil.h"
#include "PostProcess/PostProcessing.h"
#include "SceneRenderer.h"
#include <memory>

NS_RENDERSYSTEM_BEGIN

class SkyBoxNode;

// 渲染路径枚举
enum class RenderPath
{
    Forward,        // 前向渲染
    Deferred,       // 延迟渲染（默认）
    Hybrid          // 混合渲染（部分前向，部分延迟）
};

// 场景管理器，先用这种简单的管理
class RENDERSYSTEM_API SceneManager
{
public:
    static SceneManager* GetInstance();

    SceneNode* GetRootNode() const;

    PostProcessing *GetPostProcessing() const
    {
        return mPostProcessing;
    }

    Light *CreateLight(const std::string &name, Light::LightType type);

    virtual Light *GetLight(const std::string &name) const;

    virtual bool HasLight(const std::string &name) const;

    // 移除灯光（不删除）
    void RemoveLight(Light* light);

    // 销毁灯光（删除）
    void DestroyLight(Light* light);

    // 清空所有灯光
    void ClearLights();

    CameraPtr CreateCamera(const std::string &name);

    CameraPtr GetCamera(const std::string &name) const;

    bool HasCamera(const std::string &name) const;

    void AddCamera(CameraPtr camera)
    {
        mCameras.push_back(camera);
    }
    
    SkyBoxNode* GetSkyBox() const
    {
        return mSkyBoxNode;
    }
    
    RenderInfo GetRenderInfo() const
    {
        RenderInfo renderInfo;
        renderInfo.cameraUBO = mCameraUBO;
        renderInfo.lightUBO = mLightUBO;
        
        return renderInfo;
    }
    
    /**
     * 设置渲染路径
     * @param path 渲染路径类型
     */
    void SetRenderPath(RenderPath path);
    
    /**
     * 获取当前渲染路径
     */
    RenderPath GetRenderPath() const { return mRenderPath; }
    
    //渲染（使用选择的渲染路径）
    void Render(RenderEncoderPtr renderEncoder);

    //更新函数, deltaTime 秒
    void Update(float deltaTime);

    // 清空场景（删除所有节点和灯光）
    void ClearScene();

    // 重置场景到初始状态
    void ResetScene();

private:
    // 递归渲染节点，支持父子关系变换计算
    void RenderNodeRecursive(SceneNode* node, const RenderInfo& renderInfo);

    // 递归更新节点
    void UpdateNodeRecursive(SceneNode* node, float deltaTime);

private:
    SceneNode *mRootSceneNode = nullptr;       //根节点
    std::vector<Light*> mLights;              //灯光的列表
    std::vector<CameraPtr> mCameras;            //相机的列表
    
    SkyBoxNode* mSkyBoxNode = nullptr;   //天空盒的特殊节点
    PostProcessing *mPostProcessing = nullptr;
    
    ArcballManipulate* mCameraMani = nullptr;
    
    UniformBufferPtr mCameraUBO = nullptr;
    UniformBufferPtr mLightUBO = nullptr;
    
    RenderPath mRenderPath = RenderPath::Deferred;  // 默认使用延迟渲染
    SceneRendererUniPtr mSceneRenderer = nullptr;
    
    SceneManager();
    
    ~SceneManager();
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_INCLUDE_SCENEMANGER_INCVSDJVKJND */
