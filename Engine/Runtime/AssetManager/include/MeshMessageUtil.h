#ifndef GNX_ENGINE_MESHMESSAGE_UTIL_INCLUDE_SDGMJFKJ
#define GNX_ENGINE_MESHMESSAGE_UTIL_INCLUDE_SDGMJFKJ

#include "AssetDefine.h"
#include "Runtime/RenderSystem/include/mesh/Mesh.h"

NS_ASSETMANAGER_BEGIN

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

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_MESHMESSAGE_UTIL_INCLUDE_SDGMJFKJ
