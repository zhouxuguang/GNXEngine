#include "AssimpAssetImporter.h"
#include "ModelAssetPackager.h"
#include "TextureImporter.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include <array>
#include <filesystem>
#include <set>

NS_ASSETPROCESS_BEGIN

namespace
{
void CollectMaterialTextures(const aiScene* scene, const std::filesystem::path& sourceDirectory,
                             std::set<std::filesystem::path>& textures)
{
    constexpr std::array<aiTextureType, 7> types = {
        aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR, aiTextureType_NORMALS,
        aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS,
        aiTextureType_EMISSIVE, aiTextureType_AMBIENT_OCCLUSION};

    for (unsigned int materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
    {
        const aiMaterial* material = scene->mMaterials[materialIndex];
        for (const aiTextureType type : types)
        {
            for (unsigned int textureIndex = 0;
                 textureIndex < material->GetTextureCount(type); ++textureIndex)
            {
                aiString value;
                if (material->GetTexture(type, textureIndex, &value) != AI_SUCCESS)
                    continue;
                const std::string relative = value.C_Str();
                if (relative.empty() || relative.front() == '*')
                    continue;
                const auto path = (sourceDirectory / relative).lexically_normal();
                if (std::filesystem::is_regular_file(path))
                    textures.insert(path);
            }
        }
    }
}
}

AssimpAssetImporter::AssimpAssetImporter() = default;
AssimpAssetImporter::~AssimpAssetImporter() = default;

bool AssimpAssetImporter::ImportFromFile(const std::string& fileName, const std::string& saveDir)
{
    return ImportFromFile(fileName, saveDir,
                          std::filesystem::path(saveDir).parent_path().string(), nullptr);
}

bool AssimpAssetImporter::ImportFromFile(const std::string& fileName, const std::string& saveDir,
                                         const std::string& projectRoot,
                                         std::vector<std::string>* generatedAssets)
{
    namespace fs = std::filesystem;
    const fs::path source = fs::absolute(fileName).lexically_normal();
    const fs::path destination = fs::absolute(saveDir).lexically_normal();
    const fs::path project = fs::absolute(projectRoot).lexically_normal();
    if (!fs::is_regular_file(source) || projectRoot.empty())
        return false;

    fs::create_directories(destination);
    const fs::path importedSource = destination / source.filename();
    if (source != importedSource)
        fs::copy_file(source, importedSource, fs::copy_options::overwrite_existing);

    const std::vector<uint8_t> sourceData = baselib::FileUtil::ReadBinaryFile(source.string());
    if (sourceData.empty())
        return false;
    const std::string guid = baselib::GUIDToString(
        CreateGUIDFromBinaryData(sourceData.data(), sourceData.size()));
    const fs::path meshDirectory = project / ".gnx" / "Cache" / "Mesh";
    fs::create_directories(meshDirectory);
    const fs::path meshAsset = meshDirectory / (guid + ".meshasset");
    if (!ModelAssetPackager::PackMeshFromFile(source.string(), meshAsset.string(),
                                              source.stem().string()))
        return false;

    if (generatedAssets)
    {
        generatedAssets->push_back(importedSource.string());
        generatedAssets->push_back(meshAsset.string());
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(source.string(), aiProcess_ValidateDataStructure);
    if (!scene)
        return true;

    std::set<fs::path> textures;
    CollectMaterialTextures(scene, source.parent_path(), textures);
    for (const fs::path& texture : textures)
    {
        TextureImporter textureImporter;
        if (textureImporter.Import(texture.string(), destination.string(), project.string()) &&
            generatedAssets)
        {
            const fs::path importedTexture = destination / texture.filename();
            const uint64_t hash = textureImporter.GetSourceFileHash();
            generatedAssets->push_back(importedTexture.string());
            generatedAssets->push_back(importedTexture.string() + ".meta");
            generatedAssets->push_back(
                (project / ".gnx" / "Cache" /
                 (std::to_string(hash) + ".texture"))
                    .string());
            generatedAssets->push_back(
                TextureImporter::GetThumbnailFilePath(hash, project.string()));
        }
    }
    return true;
}

NS_ASSETPROCESS_END
