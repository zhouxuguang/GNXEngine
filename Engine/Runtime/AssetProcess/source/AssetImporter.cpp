#include "AssetImporter.h"
#include "AssimpAssetImporter.h"
#include "TextureImporter.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

NS_ASSETPROCESS_BEGIN

static std::string LowerExtension(const fs::path& filePath)
{
	std::string extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(),
		[](unsigned char value) { return static_cast<char>(std::tolower(value)); });
	return extension;
}

AssetImporter::AssetImporter()
{
}

AssetImporter::~AssetImporter()
{
}

bool AssetImporter::ImportFromFile(const std::string& fileName, const std::string& saveDir)
{
	return ImportFromFileDetailed(fileName, saveDir,
		fs::path(saveDir).parent_path().string()).success;
}

AssetImportOutcome AssetImporter::ImportFromFileDetailed(
	const std::string& fileName, const std::string& saveDir,
	const std::string& projectRoot)
{
	AssetImportOutcome outcome;
	outcome.sourceFile = fileName;
	if (fileName.empty() || saveDir.empty() || projectRoot.empty())
	{
		outcome.errorCode = "invalid_arguments";
		outcome.errorMessage = "Source, destination and project root are required";
		return outcome;
	}

	try
	{
		const fs::path filePath(fileName);
		if (!fs::is_regular_file(filePath))
		{
			outcome.errorCode = "source_not_found";
			outcome.errorMessage = "Source file does not exist";
			return outcome;
		}

		const std::string extension = LowerExtension(filePath);
		if (extension == ".bmp" || extension == ".tga" ||
			extension == ".jpeg" || extension == ".jpg" ||
			extension == ".png" || extension == ".hdr" ||
			extension == ".webp" || extension == ".exr")
		{
			TextureImporter importer;
			outcome.success = importer.Import(fileName, saveDir, projectRoot);
			if (outcome.success)
			{
				const fs::path imported = fs::path(saveDir) / filePath.filename();
				const uint64_t hash = importer.GetSourceFileHash();
				outcome.generatedAssets.push_back(imported.string());
				outcome.generatedAssets.push_back(imported.string() + ".meta");
				outcome.generatedAssets.push_back(
					(fs::path(projectRoot) / ".gnx" / "Cache" /
					 (std::to_string(hash) + ".texture")).string());
				outcome.generatedAssets.push_back(
					TextureImporter::GetThumbnailFilePath(hash, projectRoot));
			}
		}
		else if (extension == ".obj" || extension == ".fbx" ||
			extension == ".gltf" || extension == ".glb" || extension == ".3ds")
		{
			AssimpAssetImporter importer;
			outcome.success = importer.ImportFromFile(fileName, saveDir, projectRoot,
				&outcome.generatedAssets);
		}
		else
		{
			outcome.errorCode = "unsupported_format";
			outcome.errorMessage = "Unsupported asset format: " + extension;
			return outcome;
		}

		if (!outcome.success)
		{
			outcome.errorCode = "import_failed";
			outcome.errorMessage = "Asset importer returned failure";
		}
	}
	catch (const std::exception& exception)
	{
		outcome.errorCode = "exception";
		outcome.errorMessage = exception.what();
	}
	catch (...)
	{
		outcome.errorCode = "unknown_exception";
		outcome.errorMessage = "Unknown importer exception";
	}
	return outcome;
}

baselib::NXGUID CreateGUIDFromBinaryData(const uint8_t* data, size_t size)
{
	baselib::SHA256 sha;
	sha.update(data, size);
	std::array<uint8_t, 32> digest = sha.digest();

	return baselib::CreateGUIDFromBytes(digest.data());
}

NS_ASSETPROCESS_END
