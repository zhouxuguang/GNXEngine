//
//  RSDefine.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/29.
//

#ifndef GNXENGINE_INCLUDE_ASSET_PROCESS_INCLSFJDN
#define GNXENGINE_INCLUDE_ASSET_PROCESS_INCLSFJDN

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "MathUtil/Vector2.h"
#include "MathUtil/Vector4.h"
#include "BaseLib/BaseLib.h"

#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

#ifdef __cplusplus
#define NS_ASSETPROCESS_BEGIN                     namespace AssetProcess {
#define NS_ASSETPROCESS_END                       }
#define USING_NS_ASSETPROCESS                     using namespace AssetProcess;
#else
#define NS_ASSETPROCESS_BEGIN
#define NS_ASSETPROCESS_END
#define USING_NS_ASSETPROCESS
#endif

USING_NS_MATHUTIL


#endif /* GNXENGINE_INCLUDE_ASSET_PROCESS_INCLSFJDN */
