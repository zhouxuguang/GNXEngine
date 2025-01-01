#include "AssimpAssetImporter.h"
#include "AssimpMeshImporter.h"

NS_ASSETPROCESS_BEGIN

AssimpAssetImporter::AssimpAssetImporter()
{
}

AssimpAssetImporter::~AssimpAssetImporter()
{
}

bool AssimpAssetImporter::ImportFromFile(const std::string& fileName, const std::string& saveDir)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName.c_str(),
		aiProcess_SplitLargeMeshes |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |     //三角化
		aiProcess_SortByPType |        //
		//aiProcess_FlipUVs |         //翻转UV坐标
		aiProcess_GenNormals |
		//aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace |   //计算切线空间
		aiProcess_OptimizeMeshes |     //网格优化
		aiProcess_RemoveRedundantMaterials | //移除多余的材质
		//aiProcess_PreTransformVertices | //预变换顶点坐标
		aiProcess_OptimizeGraph | //配合 aiProcess_OptimizeMeshes 使用
		aiProcess_GenBoundingBoxes |
		aiProcess_FixInfacingNormals |
		aiProcess_JoinIdenticalVertices  //相同顶点只索引一次
		//aiProcess_FlipWindingOrder
	);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE 
		|| !scene->mRootNode || !scene->HasMeshes()) // if is Not Zero
	{
		return false;
	}

	// 导入mesh
	AssimpMeshImporter assimpMeshImport(scene, saveDir);
	assimpMeshImport.LoadMesh();

    return true;
}

NS_ASSETPROCESS_END