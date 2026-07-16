//
//  Culling.hlsl
//  GNXEngine
//
//  通用剔除工具函数：AABB/包围球与视锥体、包围球之间的相交测试。
//  视锥体平面格式为 float4(nx, ny, nz, d)，平面方程 dot(n, P) + d = 0。
//  正半空间（dot + d >= 0）为视锥体内部。
//

#ifndef GNXENGINE_CULLING_HLSL
#define GNXENGINE_CULLING_HLSL

bool AABBInFrustum(float3 aabbMin, float3 aabbMax, float4 planes[6])
{
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        // p-vertex: 选沿平面法线方向最远的AABB角点（最可能在视锥体内）
        float3 p = float3(
            planes[i].x > 0 ? aabbMax.x : aabbMin.x,
            planes[i].y > 0 ? aabbMax.y : aabbMin.y,
            planes[i].z > 0 ? aabbMax.z : aabbMin.z
        );
        if (dot(planes[i].xyz, p) + planes[i].w < 0)
            return false;
    }
    return true;
}

//=============================================================================
// 包围球 vs 视锥体
//=============================================================================

/**
 * 判断包围球是否在视锥体内（与 C++ Frustum::IsSphereInFrustum 完全一致）。
 *
 * 对每个视锥体平面：计算球心到平面的有符号距离（dot(n, center) + d），
 * 如果 dist + radius < 0，说明整个球（包括最远点）都在平面背面 → 完全不可见，直接剔除。
 * 所有 6 个平面都通过则球在视锥体内（或与视锥体相交），返回 true。
 *
 * 注意：planes[i] 必须已归一化（|n|=1），否则 dist 不是真实几何距离。
 * C++ 端 Frustum::createPlane() 末尾已对 mPlanes 调用 NormalizePlane()，
 * GPU 端传过来的 frustumPlanes 也由同样的流程保证归一化。
 *
 * @param center    球心 (world space)
 * @param radius    球半径
 * @param planes    视锥体 6 个平面 float4(nx, ny, nz, d)，已归一化
 * @return          true = 可见（内部或与视锥体相交），false = 完全在视锥体外
 */
bool SphereInFrustum(float3 center, float radius, float4 planes[6])
{
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        // 带符号距离 + 半径 < 0 → 整个球在平面背面
        if (dot(planes[i].xyz, center) + planes[i].w + radius < 0)
            return false;
    }
    return true;
}

#endif // GNXENGINE_CULLING_HLSL
