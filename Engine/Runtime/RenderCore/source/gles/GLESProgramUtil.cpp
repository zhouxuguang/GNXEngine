//
//  GLESProgramUtil.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/26.
//

#include "GLESProgramUtil.h"

NAMESPACE_RENDERCORE_BEGIN

void GetProgramAttributeInfo(GLint programID, std::vector<GLVertexDescriptor>& vecAttrDesc)
{
    GLint nAttrCount = 0;
    glGetProgramiv(programID, GL_ACTIVE_ATTRIBUTES, &nAttrCount);
    vecAttrDesc.resize(nAttrCount);
    
    for (int i = 0; i < nAttrCount; i ++)
    {
        GLsizei bufSize = 20;
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        GLchar name[20] = {0};
        glGetActiveAttrib(programID, i, bufSize, &length, &size, &type, name);
        int nAttrLoc = glGetAttribLocation(programID, name);
        
        GLboolean normalized = GL_FALSE;
        
        if (GL_FLOAT_VEC3 == type)
        {
            type = GL_FLOAT;
            size *= 3;
        }
        
        else if (GL_FLOAT_VEC4 == type)
        {
            type = GL_FLOAT;
            size *= 4;
        }
        
        else if (GL_FLOAT_VEC2 == type)
        {
            type = GL_FLOAT;
            size *= 2;
        }
        
        GLVertexDescriptor vertexDes(nAttrLoc, size, type, 0, normalized);
        vecAttrDesc[nAttrLoc] = vertexDes;
    }
}

void GetProgramUniformInfo(GLint programID)
{
    GLint nUniformCount = 0;
    glGetProgramiv(programID, GL_ACTIVE_UNIFORMS, &nUniformCount);
    
    std::vector<TextureUniform> textureUniformArray;
    
    for (int i = 0; i < nUniformCount; i ++)
    {
        const GLsizei bufSize = 128;
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        GLchar name[bufSize] = {0};
        glGetActiveUniform(programID, i, bufSize, &length, &size, &type, name);
        int nUniformLoc = glGetUniformLocation(programID, name);
        std::string strName = name;
        
        if (GL_SAMPLER_2D == type || GL_SAMPLER_CUBE == type)
        {
            textureUniformArray.emplace_back(strName, nUniformLoc);
        }
        
        //GL_SAMPLER_2D    sampler2D
        //GL_SAMPLER_3D    sampler3D
        //GL_SAMPLER_CUBE    samplerCube
    }
    
    //给采样器分配槽位
    for (size_t i = 0; i < textureUniformArray.size(); i ++)
    {
        glUniform1i(textureUniformArray[i].location, (int)i);
    }
}

void GetProgramUniformBlockInfo(GLint programID, std::vector<GLUBODescriptor>& vecUBODesc)
{
    GLint nUBOCount = 0;
    glGetProgramiv(programID, GL_ACTIVE_UNIFORM_BLOCKS, &nUBOCount);
    
    //名字最大属性
    GLint maxNameLength = 0;
    glGetProgramiv(programID, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &maxNameLength);
    
    vecUBODesc.resize(nUBOCount);
    
    for (int i = 0; i < nUBOCount; i ++)
    {
        
        GLsizei bufSize = 256;
        GLsizei length = 0;
        GLchar name[256] = {0};
        glGetActiveUniformBlockName(programID, i, bufSize, &length, name);
        GLuint uboIndex = glGetUniformBlockIndex(programID, name);
        
        GLint dataSize = 0;
        glGetActiveUniformBlockiv(programID, uboIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &dataSize);
        
        GLint binding = 0;
        glGetActiveUniformBlockiv(programID, uboIndex, GL_UNIFORM_BLOCK_BINDING, &binding);
        
        //glUniformBlockBinding(programID, uboIndex, uint32_t(vecUBODesc.size() + i));
        glUniformBlockBinding(programID, uboIndex, i);
        
        GLUBODescriptor uboDesc;
        uboDesc.index = uboIndex;
        uboDesc.dataSize = dataSize;
        vecUBODesc[uboIndex] = uboDesc;
        //printf("%s", name);
    }
}

NAMESPACE_RENDERCORE_END
