//
//  GLBinaryProgramCache.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/3.
//

#ifndef GNX_ENGINE_GL_BINARY_PROGRAM_CACHE_INCLUDE_NJSDJ
#define GNX_ENGINE_GL_BINARY_PROGRAM_CACHE_INCLUDE_NJSDJ

#include "GLRenderDefine.h"
#include <unordered_map>
#include <vector>
#include <string>

NAMESPACE_RENDERCORE_BEGIN

typedef std::shared_ptr<std::vector<uint8_t>> ByteVectorPtr;

class GLBinaryProgramCache
{
public:
    GLBinaryProgramCache();
    
    ~GLBinaryProgramCache();
    
    bool ProgramBinary(GLuint program, const std::string& vert, const std::string& frag, const std::string& geom,
                       const std::string& tessContol, const std::string& tessEva);
    
    void GetProgramBinary(GLuint program);
    
private:
    std::unordered_map<uint64_t, ByteVectorPtr> mMemoryCache;
    GLenum mBinaryFormat = GL_NONE;
    bool mSupportBinaryProgram = false;
};

typedef std::shared_ptr<GLBinaryProgramCache> GLBinaryProgramCachePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GL_BINARY_PROGRAM_CACHE_INCLUDE_NJSDJ */
