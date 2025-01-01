#include "RenderSystem/mesh/MeshImporter.h"
#include "RenderSystem/mesh/Mesh.h"
#include "RenderSystem/RenderEngine.h"
#include "AssetProcess/AssetImporter.h"
#include "pb.h"
#include <filesystem>

namespace fs = std::filesystem;

using namespace RenderSystem;

int main(int argc, char* argv[])
{
	MeshImporter* meshImporter = CreateMeshImporter();
	RenderSystem::MeshPtr mesh = std::make_shared<Mesh>();

	// 
	fs::path currentPath = getMediaDir();

	fs::path filePath = currentPath/fs::path("model/obj/box.obj");
	std::string modelPath = filePath.string();

	bool isExist = fs::exists(filePath);

	meshImporter->ImportFromFile(modelPath, mesh.get(), nullptr);


	AssetProcess::AssetImporter assetImporter;
	assetImporter.ImportFromFile(modelPath, modelPath);

	return 0;
}