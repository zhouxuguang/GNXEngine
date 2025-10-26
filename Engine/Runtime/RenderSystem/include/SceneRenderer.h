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

class SceneRenderer
{
public:
    SceneRenderer();
    
    ~SceneRenderer();
    
    //void BeginRender();
    
    //void EndRenderer();
    
    // 渲染场景部分
    void Render(float deltaTime);
    
    // 渲染天空盒部分
    void RenderSkyBox(float deltaTime);
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SCENE_RENDERER_INCLUDE_HJDSJFHSH */
