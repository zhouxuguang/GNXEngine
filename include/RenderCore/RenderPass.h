//
//  RenderPass.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/4/30.
//

#ifndef GNXENGINE_RENDERPASS_INCLUDE_HHFEGGH_
#define GNXENGINE_RENDERPASS_INCLUDE_HHFEGGH_

#include "RenderDefine.h"
#include "RenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

//渲染附件基类
struct RenderPassAttachment
{
    RenderTexturePtr texture;
};

typedef std::shared_ptr<RenderPassAttachment> RenderPassAttachmentPtr;

//清除颜色定义
struct ClearColor
{
    float red;
    float green;
    float blue;
    float alpha;
};

inline ClearColor MakeClearColor(float red, float green, float blue, float alpha)
{
    ClearColor clearColor;
    clearColor.red = red;
    clearColor.green = green;
    clearColor.blue = blue;
    clearColor.alpha = alpha;
    return clearColor;
}

//颜色附件定义
struct RenderPassColorAttachment : public RenderPassAttachment
{
    ClearColor clearColor;
};

typedef std::shared_ptr<RenderPassColorAttachment> RenderPassColorAttachmentPtr;

//深度附件定义
struct RenderPassDepthAttachment : public RenderPassAttachment
{
    float clearDepth;
};

typedef std::shared_ptr<RenderPassDepthAttachment> RenderPassDepthAttachmentPtr;

//模板附件定义
struct RenderPassStencilAttachment : RenderPassAttachment
{
    uint32_t clearStencil;
};

typedef std::shared_ptr<RenderPassStencilAttachment> RenderPassStencilAttachmentPtr;

//不同的attachmemt必须要一样大小
class RenderPass
{
public:
    RenderPass();
    
    ~RenderPass();
    
    std::vector<RenderPassColorAttachmentPtr> colorAttachments; //color attach 0, 1 2, 3
    RenderPassDepthAttachmentPtr depthAttachment = nullptr;
    RenderPassStencilAttachmentPtr stencilAttachment = nullptr;
    
    Rect2D renderRegion;  //渲染的区域
};

typedef std::shared_ptr<RenderPass> RenderPassPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNXENGINE_RENDERPASS_INCLUDE_HHFEGGH_ */
