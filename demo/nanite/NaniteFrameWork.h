//
//  NaniteFrameWork.h
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#ifndef NaniteFrameWork_hpp
#define NaniteFrameWork_hpp

#include "Runtime/GNXEngine/include/AppFrameWork.h"

class NaniteFrameWork : public GNXEngine::AppFrameWork
{
public:
    NaniteFrameWork(const GNXEngine::WindowProps& props);
    
    virtual void Initlize();
    
    virtual void RenderFrame();
};

#endif /* NaniteFrameWork_hpp */
