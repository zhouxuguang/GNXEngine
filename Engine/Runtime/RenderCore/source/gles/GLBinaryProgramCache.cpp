//
//  GLBinaryProgramCache.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/3.
//

#include "GLBinaryProgramCache.h"

NAMESPACE_RENDERCORE_BEGIN

GLBinaryProgramCache::GLBinaryProgramCache()
{
    GLint numBinaryFormat = 0;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numBinaryFormat);
    if (numBinaryFormat >= 1)
    {
        mSupportBinaryProgram = true;
        std::vector<GLint> binaryFormats;
        binaryFormats.resize(numBinaryFormat);
        glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, binaryFormats.data());
        mBinaryFormat = binaryFormats[0];
    }
    
}

GLBinaryProgramCache::~GLBinaryProgramCache()
{
    //
}

bool GLBinaryProgramCache::ProgramBinary(GLuint program, const std::string& vert, const std::string& frag, const std::string& geom,
                   const std::string& tessContol, const std::string& tessEva)
{
    return true;
}

void GLBinaryProgramCache::GetProgramBinary(GLuint program)
{
    if (!mSupportBinaryProgram)
    {
        return;
    }
    GLint binary_size = 0;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH_OES, &binary_size);

    // Allocate some memory to store the program binary
    uint8_t * program_binary = new uint8_t[binary_size];// Now retrieve the binary from the program object
    GLenum binary_format = GL_NONE;
    GLsizei bufLength = 0;
    glGetProgramBinaryOES(program, binary_size, &bufLength, &binary_format, program_binary);
    
    //写入磁盘
//    FILE * fp = fopen(shaderFile.c_str(), "wb");
//    fwrite(&binary_format, 1, 4, fp);
//    fwrite(program_binary, 1, bufLength, fp);
//    fclose(fp);
    delete []program_binary;
}

NAMESPACE_RENDERCORE_END
