#include "RenderSystem/mesh/MeshImporter.h"
#include "RenderSystem/mesh/Mesh.h"
#include "RenderSystem/RenderEngine.h"
#include "pb.h"

using namespace RenderSystem;

int main(int argc, char* argv[])
{
	MeshImporter* meshImporter = CreateMeshImporter();
	RenderSystem::MeshPtr mesh = std::make_shared<Mesh>();

	std::string pathSplit = std::string(1, PATHSPLIT);
	std::string filePath = "model" + pathSplit + "obj" + pathSplit + "box.obj";
	std::string modelPath = getMediaDir() + filePath;

	meshImporter->ImportFromFile(modelPath, mesh.get(), nullptr);
	return 0;
}