//
//  ClusterSelection.h
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#ifndef ClusterSelection_hpp
#define ClusterSelection_hpp

#include "Runtime/RenderCore/include/RenderDevice.h"

//init cluster selection
void InitClusterSelectionPass(RenderCore::RenderDevicePtr renderDevice);

//cluster selection pass
void ExecuteClusterSelectionPass(RenderCore::RenderDevicePtr renderDevice);

#endif /* ClusterSelection_hpp */
