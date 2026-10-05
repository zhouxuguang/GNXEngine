
#include "GNXEngineCommon.hlsl"

#ifndef GNX_ENGINE_INTERSECTIONTEST_INCLUDE_H
#define GNX_ENGINE_INTERSECTIONTEST_INCLUDE_H

//射线与包围盒相交, x 到包围盒最近的距离， y 穿过包围盒的距离
float2 RayBoxDst(float3 boxMin, float3 boxMax, float3 pos, float3 rayDir)
{
    float3 t0 = (boxMin - pos) / rayDir;
    float3 t1 = (boxMax - pos) / rayDir;
    
    float3 tmin = min(t0, t1);
    float3 tmax = max(t0, t1);
    
    //射线到box两个相交点的距离, dstA最近距离， dstB最远距离
    float dstA = max(max(tmin.x, tmin.y), tmin.z);
    float dstB = min(min(tmax.x, tmax.y), tmax.z);
    
    float dstToBox = max(0, dstA);
    float dstInBox = max(0, dstB - dstToBox);
    
    return float2(dstToBox, dstInBox);
}

//射线与球体相交, x 到球体最近的距离， y 穿过球体的距离
//原理是将射线方程(x = o + dl)带入球面方程求解(|x - c|^2 = r^2)
float2 RaySphereDst(float3 sphereCenter, float sphereRadius, float3 pos, float3 rayDir)
{
    float3 oc = pos - sphereCenter;
    float b = dot(rayDir, oc);
    float c = dot(oc, oc) - sphereRadius * sphereRadius;
    float t = b * b - c;//t > 0有两个交点, = 0 相切， < 0 不相交
    
    float delta = sqrt(max(t, 0));
    float dstToSphere = max(-b - delta, 0);
    float dstInSphere = max(-b + delta - dstToSphere, 0);
    return float2(dstToSphere, dstInSphere);
}

#endif