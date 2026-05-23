#ifndef VIRTUAL_TEXTURE_COMMON_HLSL
#define VIRTUAL_TEXTURE_COMMON_HLSL

const uint VALID_BIT = 1u << 31;
const uint MIP_MASK  = 0x1Fu;
const uint PAGE_MASK = 0xFFu;

uint PackPageData(in uint mip, in uint page_x, in uint page_y) 
{
    return VALID_BIT |
          (mip & MIP_MASK) |
          ((page_x & PAGE_MASK) << 5) |
          ((page_y & PAGE_MASK) << 13);
}

float ComputeMipLevel(float u, float v, float textureWidth, float textureHeight)
{
    float du_dx = ddx(u * textureWidth);
    float dv_dx = ddx(v * textureHeight);
    float du_dy = ddy(u * textureWidth);
    float dv_dy = ddy(v * textureHeight);

    float lengthX = sqrt(du_dx * du_dx + dv_dx * dv_dx);
    float lengthY = sqrt(du_dy * du_dy + dv_dy * dv_dy);
    float rho = max(lengthX, lengthY);

    return log2(rho);
}

#endif