//
//  Sphere.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#include "Sphere.h"

NS_RENDERSYSTEM_BEGIN
 
//bool Sphere::IsInside (const Sphere& inSphere)const
//{
//    float sqrDist = SqrMagnitude (GetCenter () - inSphere.GetCenter ());
//    if (Sqr (GetRadius ()) > sqrDist + Sqr (inSphere.GetRadius ()))
//        return true;
//    else
//        return false;
//}
//
//inline bool Intersect (const Sphere& inSphere0, const Sphere& inSphere1)
//{
//    float sqrDist = SqrMagnitude (inSphere0.GetCenter () - inSphere1.GetCenter ());
//    if (Sqr (inSphere0.GetRadius () + inSphere1.GetRadius ()) > sqrDist)
//        return true;
//    else
//        return false;
//}
//
//inline float CalculateSqrDistance (const Vector3f& p, const Sphere& s)
//{
//    return std::max (0.0F, SqrMagnitude (p - s.GetCenter ()) - Sqr (s.GetRadius ()));
//}
//
//Sphere MergeSpheres (const Sphere& inSphere0, const Sphere& inSphere1);
//float MergeSpheresRadius (const Sphere& inSphere0, const Sphere& inSphere1);
//
//Sphere MergeSpheres (const Sphere& inSphere0, const Sphere& inSphere1)
//{
//    Vector3 kCDiff = inSphere1.GetCenter() - inSphere0.GetCenter();
//    float fLSqr = SqrMagnitude (kCDiff);
//    float fRDiff = inSphere1.GetRadius() - inSphere0.GetRadius();
//
//    if (fRDiff*fRDiff >= fLSqr)
//       return fRDiff >= 0.0 ? inSphere1 : inSphere0;
//
//    float fLength = sqrt (fLSqr);
//    const float fTolerance = 1.0e-06f;
//    Sphere kSphere;
//
//    if (fLength > fTolerance)
//    {
//       float fCoeff = (fLength + fRDiff) / (2.0 * fLength);
//       kSphere.GetCenter () = inSphere0.GetCenter () + fCoeff * kCDiff;
//    }
//    else
//    {
//       kSphere.GetCenter () = inSphere0.GetCenter ();
//    }
//
//    kSphere.GetRadius () = 0.5F * (fLength + inSphere0.GetRadius () +  inSphere1.GetRadius ());
//
//    return kSphere;
//}
//
//float MergeSpheresRadius (const Sphere& inSphere0, const Sphere& inSphere1)
//{
//    Vector3 kCDiff = inSphere1.GetCenter() - inSphere0.GetCenter();
//    float fLSqr = SqrMagnitude (kCDiff);
//    float fRDiff = inSphere1.GetRadius () - inSphere0.GetRadius();
//
//    if (fRDiff*fRDiff >= fLSqr)
//       return fRDiff >= 0.0 ? inSphere1.GetRadius () : inSphere0.GetRadius ();
//
//    float fLength = sqrt (fLSqr);
//    return 0.5F * (fLength + inSphere0.GetRadius () +  inSphere1.GetRadius ());
//}


NS_RENDERSYSTEM_END
