//
//  RCTexture.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#ifndef GNX_ENGINE_RCTEXTURE_INCLUDE_FHJDSVJ
#define GNX_ENGINE_RCTEXTURE_INCLUDE_FHJDSVJ

#include "RenderDefine.h"
#include "TextureFormat.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief 资源访问类型（RHI 抽象）
 */
enum class ResourceAccess : uint32_t
{
    None = 0,
    
    // 缓冲区访问
    VertexBuffer = 1 << 0,
    IndexBuffer = 1 << 1,
    UniformBuffer = 1 << 2,
    StorageBufferRead = 1 << 3,
    StorageBufferWrite = 1 << 4,
    IndirectBuffer = 1 << 5,
    TransferSrc = 1 << 6,
    TransferDst = 1 << 7,
    
    // 纹理访问
    ShaderResource = 1 << 8,
    ColorAttachment = 1 << 9,
    DepthStencilAttachment = 1 << 10,
    DepthStencilReadOnly = 1 << 11,
    
    // 计算着色器
    ComputeShaderResource = 1 << 12,
    
    All = 0xFFFFFFFF
};

inline ResourceAccess operator|(ResourceAccess a, ResourceAccess b)
{
    return static_cast<ResourceAccess>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ResourceAccess operator&(ResourceAccess a, ResourceAccess b)
{
    return static_cast<ResourceAccess>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/**
 * @brief 资源管线阶段（RHI 抽象）
 */
enum class ResourcePipelineStage : uint32_t
{
    TopOfPipe = 0,
    DrawIndirect = 1,
    VertexInput = 2,
    VertexShader = 3,
    TessellationControlShader = 4,
    TessellationEvaluationShader = 5,
    GeometryShader = 6,
    FragmentShader = 7,
    EarlyFragmentTests = 8,
    LateFragmentTests = 9,
    ColorAttachmentOutput = 10,
    ComputeShader = 11,
    Transfer = 12,
    BottomOfPipe = 13,
    Host = 14,
    AllGraphics = 15,
    AllCommands = 16
};

/**
 * @brief 资源布局（RHI 抽象）
 */
enum class ResourceLayout : uint32_t
{
    Undefined = 0,
    General = 1,
    ColorAttachmentOptimal = 2,
    DepthStencilAttachmentOptimal = 3,
    DepthStencilReadOnlyOptimal = 4,
    ShaderReadOnlyOptimal = 5,
    TransferSrcOptimal = 6,
    TransferDstOptimal = 7,
    Preinitialized = 8,
    PresentSrc = 9
};

/**
 * @brief 资源状态（RHI 抽象）
 */
struct ResourceState
{
    ResourceAccess access = ResourceAccess::None;
    ResourcePipelineStage stage = ResourcePipelineStage::TopOfPipe;
    ResourceLayout layout = ResourceLayout::Undefined;
    bool initialized = false;
};

/**
 * @brief RHI纹理的基类
 * 
 */
class RCTexture
{
public:
    RCTexture(TextureType textureType)
    {
        mTextureType = textureType;
    }
    
    virtual ~RCTexture(){}
    
    /**
     纹理是否有效

     @return ture or false
     */
    virtual bool IsValid() const = 0;
    
    virtual uint32_t GetWidth() const = 0;
    
    virtual uint32_t GetHeight() const = 0;
    
    virtual uint32_t GetDepth() const = 0;
    
    virtual uint32_t GetMipLevels() const = 0;
    
    virtual uint32_t GetLayerCount() const = 0;

    virtual void SetName(const char* name) = 0;
    
    /**
     * @brief 获取纹理的当前状态
     */
    virtual ResourceState GetState() const = 0;
    
    /**
     * @brief 设置纹理的当前状态
     */
    virtual void SetState(const ResourceState& state) = 0;
    
    /**
     * @brief 在读取前插入资源屏障
     * @param commandBuffer 命令缓冲区（平台特定的句柄）
     * @param access 访问类型
     * @param stage 管线阶段
     * @param layout 布局
     */
    virtual void PreReadBarrier(void* commandBuffer, ResourceAccess access, 
                             ResourcePipelineStage stage, ResourceLayout layout) = 0;
    
    /**
     * @brief 在写入前插入资源屏障
     * @param commandBuffer 命令缓冲区（平台特定的句柄）
     * @param access 访问类型
     * @param stage 管线阶段
     * @param layout 布局
     */
    virtual void PreWriteBarrier(void* commandBuffer, ResourceAccess access,
                             ResourcePipelineStage stage, ResourceLayout layout) = 0;
    
    TextureType GetTextureType() const
    {
        return mTextureType;
    }
    
    TextureFormat GetTextureFormat() const
    {
        return mFormat;
    }
    
    void SetFormat(TextureFormat format)
    {
        if (!mInited)
        {
            mFormat = format;
            mInited = true;
        }
    }
    
private:
    TextureType mTextureType = TextureType_Unkown;
    TextureFormat mFormat = kTexFormatInvalid;
    bool mInited = false;
};

typedef std::shared_ptr<RCTexture> RCTexturePtr;

/**
 * @brief RHI 2D纹理
 * 
 */
class RCTexture2D : virtual public RCTexture
{
public:
    RCTexture2D() : RCTexture(TextureType_2D){}
    virtual ~RCTexture2D(){}

    /**
       更新纹理数据
     
     @param rect 更新纹理区域
     @param level 纹理mipmap等级
     @param pixelBytes 纹理数据
     @param bytesPerRow 每行的字节数
     */
    virtual void ReplaceRegion(const Rect2D& rect, 
                        uint32_t level, 
                        const uint8_t* pixelBytes, 
                        uint32_t bytesPerRow) = 0;
};

typedef std::shared_ptr<RCTexture2D> RCTexture2DPtr;

/**
 * @brief RHI 3D纹理
 * 
 */
class RCTexture3D : virtual public RCTexture
{
public:
    RCTexture3D() : RCTexture(TextureType_3D){}
    virtual ~RCTexture3D(){}
    
    /**
       更新纹理数据
     
     @param rect 更新纹理区域
     @param level 纹理mipmap等级
     @param slice 切片索引
     @param pixelBytes 纹理数据
     @param bytesPerRow 每行的字节数
     @param bytesPerImage 每个切片的字节数
     */
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage) = 0;
};

using RCTexture3DPtr = std::shared_ptr<RCTexture3D>;

/**
 * @brief RHI cube纹理
 * 
 */
class RCTextureCube : virtual public RCTexture
{
public:
    RCTextureCube() : RCTexture(TextureType_CUBE){}
    virtual ~RCTextureCube(){}
    
    /**
       更新纹理数据
     
     @param rect 更新纹理区域
     @param level 纹理mipmap等级
     @param slice 切片索引
     @param pixelBytes 纹理数据
     @param bytesPerRow 每行的字节数
     @param bytesPerImage 每个切片的字节数
     */
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage) = 0;
};

using RCTextureCubePtr = std::shared_ptr<RCTextureCube>;

/**
 * @brief RHI 2d array纹理
 *
 */
class RCTexture2DArray : virtual public RCTexture
{
public:
    RCTexture2DArray() : RCTexture(TextureType_2D_ARRAY){}
    virtual ~RCTexture2DArray(){}
    
    /**
       更新纹理数据
     
     @param rect 更新纹理区域
     @param level 纹理mipmap等级
     @param slice 切片索引
     @param pixelBytes 纹理数据
     @param bytesPerRow 每行的字节数
     @param bytesPerImage 每个切片的字节数
     */
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage) = 0;
};

using RCTexture2DArrayPtr = std::shared_ptr<RCTexture2DArray>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RCTEXTURE_INCLUDE_FHJDSVJ */
