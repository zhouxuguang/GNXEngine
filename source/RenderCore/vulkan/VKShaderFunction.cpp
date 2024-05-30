//
//  VKShaderFunction.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKShaderFunction.h"
#include "spirv_reflection.h"

NAMESPACE_RENDERCORE_BEGIN

VKShaderFunction::VKShaderFunction(VulkanContextPtr context)
{
    mContext = context;
}

VKShaderFunction::~VKShaderFunction()
{
    if (mShaderFunction != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(mContext->device, mShaderFunction, nullptr);
    }
}

std::shared_ptr<ShaderFunction> VKShaderFunction::initWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderSource.size();
    shaderModuleCreateInfo.pCode = (const uint32_t*)shaderSource.data();
    VkShaderModule computeShaderModule = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(mContext->device, &shaderModuleCreateInfo, nullptr, &computeShaderModule);
    if (res != VK_SUCCESS)
    {
        mShaderFunction = VK_NULL_HANDLE;
        return nullptr;
    }
    mShaderFunction = computeShaderModule;
    
    return shared_from_this();
}

static DescriptorSetLayoutDataVec GetDescriptorInfo(const SpvReflectShaderModule& shaderModule)
{
    uint32_t count = 0;
    SpvReflectResult result = spvReflectEnumerateDescriptorSets(&shaderModule, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&shaderModule, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    
    DescriptorSetLayoutDataVec set_layouts(sets.size(), DescriptorSetLayoutData{});
    for (size_t i_set = 0; i_set < sets.size(); ++i_set)
    {
        const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);
        DescriptorSetLayoutData& layout = set_layouts[i_set];
        layout.bindings.resize(refl_set.binding_count);
        for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding) 
        {
            const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
            VkDescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];
            layout_binding.binding = refl_binding.binding;
            layout_binding.descriptorType = (VkDescriptorType)refl_binding.descriptor_type;
            layout_binding.descriptorCount = 1;
            for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim)
            {
                layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
            }
            layout_binding.stageFlags = (VkShaderStageFlagBits)shaderModule.shader_stage;
        }
        layout.set_number = refl_set.set;
        layout.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        
        // PUSH Descriptor必须使用VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR这个flag
        layout.create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        layout.create_info.pNext = nullptr;
        layout.create_info.bindingCount = refl_set.binding_count;
        layout.create_info.pBindings = layout.bindings.data();
    }
    
    return std::move(set_layouts);
}

std::shared_ptr<VKShaderFunction> VKShaderFunction::initWithShaderSourceInner(const ShaderCode& shaderSource, ShaderStage shaderStage)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderSource.size();
    shaderModuleCreateInfo.pCode = (const uint32_t*)shaderSource.data();
    VkShaderModule computeShaderModule = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(mContext->device, &shaderModuleCreateInfo, nullptr, &computeShaderModule);
    if (res != VK_SUCCESS)
    {
        mShaderFunction = VK_NULL_HANDLE;
        return nullptr;
    }
    mShaderFunction = computeShaderModule;
    
    SpvReflectShaderModule shaderModule = {};
    SpvReflectResult result = spvReflectCreateShaderModule(shaderSource.size(), shaderSource.data(), &shaderModule);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    
    mDescriptorSets = GetDescriptorInfo(shaderModule);
    mEntryName = shaderModule.entry_point_name;
    
    spvReflectDestroyShaderModule(&shaderModule);
    
    return shared_from_this();
}

ShaderStage VKShaderFunction::getShaderStage() const
{
    return mShaderStage;
}

NAMESPACE_RENDERCORE_END
