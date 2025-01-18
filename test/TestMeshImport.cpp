#include "RenderSystem/RenderEngine.h"
#include "AssetProcess/AssetImporter.h"
#include "BaseLib/ThreadPool.h"

int main(int argc, char* argv[])
{
	fs::path currentPath = getMediaDir();

	fs::path filePath = (currentPath/fs::path("backpack/backpack.obj")).lexically_normal();
	std::string modelPath = filePath.string();

	AssetProcess::AssetImporter assetImporter;
	assetImporter.ImportFromFile(modelPath, filePath.parent_path().string());

	// 这个是ktx2格式的轻量级的库
	// https://github.com/DeanoC/tiny_ktx/tree/master

	return 0;
}