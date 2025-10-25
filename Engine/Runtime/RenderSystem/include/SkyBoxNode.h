//
//  SkyBoxNode.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/25.
//

#ifndef RENDERSYSTEM_SYKBOX_NODE_INCLUDE
#define RENDERSYSTEM_SYKBOX_NODE_INCLUDE

#include "RSDefine.h"
#include "MathUtil/Vector3.h"
#include "SceneNode.h"
#include "SkyBox.h"

NS_RENDERSYSTEM_BEGIN

class SkyBoxNode : public SceneNode
{
public:
    void AttachSkyBoxObject(SkyBox *obj);
    
    //
    void Render(RenderEncoderPtr renderEncoder);
    
private:
    SkyBox* mSkyBox = nullptr;
};

NS_RENDERSYSTEM_END

#endif /* RENDERSYSTEM_SYKBOX_NODE_INCLUDE */
