//
//  SceneManager.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/13.
//

#ifndef GNXENGINE_INCLUDE_SCENEMANGER_INCVSDJVKJND
#define GNXENGINE_INCLUDE_SCENEMANGER_INCVSDJVKJND

#include "RSDefine.h"
#include "MathUtil/Vector3.h"
#include "SceneNode.h"
#include "Camera.h"
#include "Light.h"
#include "ArcballManipulate.h"
#include "mesh/MeshDrawUtil.h"

NS_RENDERSYSTEM_BEGIN

class SkyBoxNode;

// 场景管理器，先用这种简单的管理
class SceneManager
{
public:
    static SceneManager* GetInstance();
    
    SceneNode* getRootNode() const;
    
    Light * createLight(const std::string &name, Light::LightType type);
    
    virtual Light * getLight(const std::string &name) const;
     
    virtual bool hasLight(const std::string &name) const;
    
    CameraPtr createCamera(const std::string &name);
    
    CameraPtr getCamera(const std::string &name) const;
    
    bool hasCamera(const std::string &name) const;
    
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
    
    //渲染
    void Render(RenderEncoderPtr renderEncoder);
    
    //更新函数, deltaTime 秒
    void Update(float deltaTime);
    
private:
    SceneNode *mRootSceneNode = nullptr;       //根节点
    std::vector<Light*> mLights;              //灯光的列表
    std::vector<CameraPtr> mCameras;            //相机的列表
    
    SkyBoxNode* mSkyBoxNode = nullptr;   //天空盒的特殊节点
    
    ArcballManipulate* mCameraMani = nullptr;
    
    UniformBufferPtr mCameraUBO = nullptr;
    UniformBufferPtr mLightUBO = nullptr;
    
    SceneManager();
    
    ~SceneManager();
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_INCLUDE_SCENEMANGER_INCVSDJVKJND */
