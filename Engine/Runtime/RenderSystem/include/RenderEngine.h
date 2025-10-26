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

std::string getBuiltInShaderDir();

std::string getMediaDir();

std::string getAssetsDir();

std::string GetProjectAssetDir();

bool EnsurePathExists(const fs::path& path);

#endif /* RenderEngine_hpp */
