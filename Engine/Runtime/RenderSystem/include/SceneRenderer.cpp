//
//  SceneRenderer.cpp
//  rendersystem
//
//  Created by zhouxuguang on 2024/6/4.
//

#include "SceneRenderer.h"

NS_RENDERSYSTEM_BEGIN

SceneRenderer::SceneRenderer()
{
    mTransientResources = new TransientResources(RenderCore::GetRenderDevice());
}

SceneRenderer::~SceneRenderer()
{
    if (mTransientResources)
    {
        delete mTransientResources;
        mTransientResources = nullptr;
    }
}

NS_RENDERSYSTEM_END
