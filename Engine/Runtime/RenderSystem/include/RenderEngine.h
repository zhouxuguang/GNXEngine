//
//  RenderEngine.hpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/29.
//

#ifndef RenderEngine_hpp
#define RenderEngine_hpp

#include <stdio.h>
#include <string>
#include "Runtime/BaseLib/include/BaseLib.h"
#include "RSDefine.h"

RENDERSYSTEM_API std::string getBuiltInShaderDir();

RENDERSYSTEM_API std::string getMediaDir();

RENDERSYSTEM_API std::string getAssetsDir();

RENDERSYSTEM_API std::string GetProjectAssetDir();

RENDERSYSTEM_API bool EnsurePathExists(const fs::path& path);

#endif /* RenderEngine_hpp */
