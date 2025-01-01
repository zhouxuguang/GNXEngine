#include "RenderSystem/RenderEngine.h"
#include "AssetProcess/AssetImporter.h"
#include "pb.h"
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	fs::path currentPath = getMediaDir();

	fs::path filePath = currentPath/fs::path("model/obj/box.obj");
	std::string modelPath = filePath.string();

	AssetProcess::AssetImporter assetImporter;
	assetImporter.ImportFromFile(modelPath, modelPath);

	return 0;
}