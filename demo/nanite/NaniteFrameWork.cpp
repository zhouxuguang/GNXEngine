//
//  NaniteFrameWork.cpp
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "NaniteFrameWork.h"
#include "ClusterSelection.h"

NaniteFrameWork::NaniteFrameWork(const GNXEngine::WindowProps& props) : GNXEngine::AppFrameWork(props)
{
    //
}

void NaniteFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
}

void NaniteFrameWork::RenderFrame()
{
    ExecuteClusterSelectionPass(RenderCore::GetRenderDevice());
}
