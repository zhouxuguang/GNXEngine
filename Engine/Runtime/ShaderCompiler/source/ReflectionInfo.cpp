//
//  ReflectionInfo.cpp
//  shadercompiler
//
//  Created by zhouxuguang on 2023/9/24.
//

#include "ReflectionInfo.h"

NAMESPACE_SHADERCOMPILER_BEGIN

RenderCore::VertexFormat getVertexFormat(const spirv_cross::SPIRType &type)
{
    switch (type.basetype)
    {
        case spirv_cross::SPIRType::BaseType::Float:
            if (type.vecsize == 1 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatFloat;
            }
            
            else if (type.vecsize == 2 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatFloat2;
            }
            
            else if (type.vecsize == 3 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatFloat3;
            }
            
            else if (type.vecsize == 4 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatFloat4;
            }
            break;
            
        case spirv_cross::SPIRType::BaseType::Half:
            if (type.vecsize == 1 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatHalfFloat;
            }
            
            else if (type.vecsize == 2 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatHalfFloat2;
            }
            
            else if (type.vecsize == 3 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatHalfFloat3;
            }
            
            else if (type.vecsize == 4 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatHalfFloat4;
            }
            
        case spirv_cross::SPIRType::BaseType::UInt :
            if (type.vecsize == 1 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatUInt;
            }
            
            else if (type.vecsize == 2 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatUInt2;
            }
            
            else if (type.vecsize == 3 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatUInt3;
            }
            
            else if (type.vecsize == 4 && type.columns == 1)
            {
                return RenderCore::VertexFormat::VertexFormatUInt4;
            }

        default:
            break;
    }
    
    
    return RenderCore::VertexFormat::VertexFormatInvalid;
}

UniformBuffersLayout GetMetalUniformReflectionInfo(const spirv_cross::CompilerMSL& msl, const spirv_cross::ShaderResources& resources)
{
    UniformBuffersLayout uniformBufferLayouts;
    uniformBufferLayouts.resize(resources.uniform_buffers.size());
    int idx = 0;
    for (const spirv_cross::Resource &resource : resources.uniform_buffers)
    {
        auto &type = msl.get_type(resource.base_type_id);
        unsigned member_count = (unsigned)type.member_types.size();
        
        std::vector<UniformMember> & uniformMembers = uniformBufferLayouts[idx].members;
        
        int totalSize = msl.get_declared_struct_size(type);
        uniformBufferLayouts[idx].dataSize = totalSize;
        uniformBufferLayouts[idx].name = msl.get_name(resource.id);
        
        for (unsigned i = 0; i < member_count; i++)
        {
            UniformMember member;
            auto &member_type = msl.get_type(type.member_types[i]);
            size_t member_size = msl.get_declared_struct_member_size(type, i);
            
            totalSize += member_size;

            // Get member offset within this struct.
            size_t offset = msl.type_struct_member_offset(type, i);

            if (!member_type.array.empty())
            {
                // Get array stride, e.g. float4 foo[]; Will have array stride of 16 bytes.
                size_t array_stride = msl.type_struct_member_array_stride(type, i);
            }

            if (member_type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
                size_t matrix_stride = msl.type_struct_member_matrix_stride(type, i);
            }
            const std::string &name = msl.get_member_name(type.self, i);
            
            member.name = name;
            member.size = member_size;
            member.offset = offset;
            uniformMembers.push_back(member);
        }
        
        idx ++;
    }
    
    return std::move(uniformBufferLayouts);
}

VertexDescriptor GetMetalReflectionInfo(const spirv_cross::CompilerMSL& msl, const spirv_cross::ShaderResources& resources)
{
    RenderCore::VertexDescriptor vertexDescriptor;
    
    for (auto &resource : resources.stage_inputs)
    {
        uint32_t index = msl.get_decoration(resource.id, spv::DecorationLocation);

        const spirv_cross::SPIRType &type = msl.get_type(resource.type_id);
        
        RenderCore::VertextAttributesDescritptor vertextAttributesDescritptor;
        RenderCore::VertexBufferLayoutDescriptor vertexBufferLayoutDescriptor;
        
        vertextAttributesDescritptor.index = index;
        vertextAttributesDescritptor.offset = 0;
        vertextAttributesDescritptor.format = getVertexFormat(type);
        vertexDescriptor.attributes.push_back(vertextAttributesDescritptor);
        
        //metal的特殊处理
        int count = type.vecsize;
        if (count == 3)
        {
            count = 4;
        }
        vertexBufferLayoutDescriptor.stride = count * type.width / 8;
        vertexDescriptor.layouts.push_back(vertexBufferLayoutDescriptor);
        
    }
    
    return vertexDescriptor;
}

NAMESPACE_SHADERCOMPILER_END
