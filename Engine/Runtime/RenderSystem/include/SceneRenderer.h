//
//  SceneRenderer.h
//  rendersystem
//
//  Created by zhouxuguang on 2024/6/4.
//

#ifndef GNX_ENGINE_SCENE_RENDERER_INCLUDE_HJDSJFHSH
#define GNX_ENGINE_SCENE_RENDERER_INCLUDE_HJDSJFHSH

#include "RSDefine.h"

NS_RENDERSYSTEM_BEGIN

class SceneManager;

/**
 场景渲染基类
 */
class SceneRenderer
{
public:
    SceneRenderer();
    
    virtual ~SceneRenderer();
    
    // 渲染场景部分
    virtual void Render(SceneManager *sceneManager, float deltaTime) = 0;
};

using SceneRendererPtr = std::shared_ptr<SceneRenderer>;
using SceneRendererUniPtr = std::unique_ptr<SceneRenderer>;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SCENE_RENDERER_INCLUDE_HJDSJFHSH */
