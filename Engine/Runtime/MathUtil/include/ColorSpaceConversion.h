//
//  ColorSpaceConversion.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/10/21.
//

#ifndef GNX_ENGINE_COLORSPACECONVERSION_INCLUDE_H
#define GNX_ENGINE_COLORSPACECONVERSION_INCLUDE_H

#include "Math3DCommon.h"

NS_MATHUTIL_BEGIN

MATH3D_API float GammaToLinearSpace(float value);

MATH3D_API float LinearToGammaSpace(float value);

MATH3D_API float GammaToLinearSpaceXenon(float val);

MATH3D_API float LinearToGammaSpaceXenon(float val);

NS_MATHUTIL_END

#endif /* GNX_ENGINE_COLORSPACECONVERSION_INCLUDE_H */
