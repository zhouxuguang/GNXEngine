//
//  GLESProgramUtil.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/26.
//

#ifndef GNX_ENGINE_GLES_PROGRAM_UTIL_INCLUDE_H
#define GNX_ENGINE_GLES_PROGRAM_UTIL_INCLUDE_H

#include "GLRenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

struct GLUBODescriptor
{
    GLuint index;
    GLint dataSize;
};

struct TextureUniform
{
    TextureUniform(const std::string &name, int location)
    {
        this->name = name;
        this->location = location;
    }
    std::string name;
    int location;
};

struct GLVertexDescriptor
{
    GLint index;
    GLint    size = 0;                    //纬度大小，例如vec2，就是2
    GLenum   type = 0;                    //数据类型，例如GL_FLOAT
    GLint    stride = 0;                  //跨距大小，例如0或者sizeof(Vertex)
    GLboolean normalized = GL_FALSE;        //是否需要规范化
    GLint offset = 0;                      //是VBO的话就是VBO偏移的大小，非VBO就是数据的指针地址
    GLVertexDescriptor(){}
    GLVertexDescriptor(GLint index, GLint size, GLenum type, GLint stride, GLboolean normalized)
    {
        this->index = index;
        this->size = size;
        this->type = type;
        this->stride = stride;
        this->normalized = normalized;
    }
};

void GetProgramAttributeInfo(GLint programID, std::vector<GLVertexDescriptor>& vecAttrDesc);

void GetProgramUniformInfo(GLint programID);

void GetProgramUniformBlockInfo(GLint programID, std::vector<GLUBODescriptor>& vecUBODesc);

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GLES_PROGRAM_UTIL_INCLUDE_H */
