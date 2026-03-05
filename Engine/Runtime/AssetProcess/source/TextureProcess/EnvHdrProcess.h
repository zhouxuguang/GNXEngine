//
//  EnvHdrProcess.h
//  GNXEngine
//
//  Created by zhouxuguang on 2026/2/12.
//

#ifndef GNX_ENGINE_ENV_HDR_PROCESS_H
#define GNX_ENGINE_ENV_HDR_PROCESS_H

#include "Runtime/AssetProcess/include/AssetProcessDefine.h"
#include "Runtime/MathUtil/include/Vector2.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/ImageCodec/include/VImage.h"

NS_ASSETPROCESS_BEGIN

ASSET_PROCESS_API imagecodec::VImagePtr ConvertEquirectangularMapToVerticalCross(const imagecodec::VImage* envImage);

ASSET_PROCESS_API std::vector<imagecodec::VImagePtr > ConvertVerticalCrossToCubeMapFaces(const imagecodec::VImage* envImage);

NS_ASSETPROCESS_END

#endif