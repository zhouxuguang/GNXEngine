#ifndef GNX_ENGINE_MESHMESSAGE_UTIL_INCLUDE_SDGMJFKJ
#define GNX_ENGINE_MESHMESSAGE_UTIL_INCLUDE_SDGMJFKJ

#include "AssetProcessDefine.h"
#include "RenderSystem/mesh/Mesh.h"

NS_ASSETPROCESS_BEGIN

USING_NS_RENDERSYSTEM

class MeshMessageUtil
{
public:
	MeshMessageUtil();
	~MeshMessageUtil();

	static bool DecodeMeshMessage(const uint8_t* pData, uint32_t dataSize, Mesh* mesh);

	static ByteVectorPtr EncodeMeshMessage(const Mesh* mesh);

private:
	//
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_MESHMESSAGE_UTIL_INCLUDE_SDGMJFKJ