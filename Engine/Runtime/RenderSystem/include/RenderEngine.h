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

// 编译后的 shader 资产目录（data_asset/Shader/）
// 全平台（PC / iOS / Android）运行时统一从此目录加载预编译产物
RENDERSYSTEM_API std::string getCompiledShaderDir();

RENDERSYSTEM_API std::string getMediaDir();

RENDERSYSTEM_API std::string getAssetsDir();

RENDERSYSTEM_API std::string GetProjectAssetDir();

RENDERSYSTEM_API bool EnsurePathExists(const fs::path& path);

#endif /* RenderEngine_hpp */
