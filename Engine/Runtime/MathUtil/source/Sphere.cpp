//
//  Sphere.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#include "Sphere.h"

NS_MATHUTIL_BEGIN

template <typename T>
bool Sphere<T>::IsInside(const Sphere<T>& inSphere) const
{
    T sqrDist = (GetCenter() - inSphere.GetCenter()).Length();
    if (GetRadius() * GetRadius() > sqrDist + inSphere.GetRadius() * inSphere.GetRadius())
    {
        return true;
    }

    return false;
}

template class Sphere<float>;
template class Sphere<double>;

NS_MATHUTIL_END
