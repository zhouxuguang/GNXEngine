//
//  SkyBoxNode.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/25.
//

#include "SkyBoxNode.h"
#include "SceneManager.h"

NS_RENDERSYSTEM_BEGIN

void SkyBoxNode::AttachSkyBoxObject(SkyBox *obj)
{
    //SceneNode::attachObject(obj);
    mSkyBox = obj;
}

void SkyBoxNode::Render(RenderEncoderPtr renderEncoder)
{
    if (mSkyBox)
    {
        mSkyBox->Render(renderEncoder, SceneManager::GetInstance()->GetRenderInfo().cameraUBO);
    }
}

NS_RENDERSYSTEM_END
