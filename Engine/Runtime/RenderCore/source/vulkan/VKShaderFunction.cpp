//
//  VKShaderFunction.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKShaderFunction.h"
#include "spirv_reflection.h"
#include "VulkanBufferUtil.h"
#include "VulkanDescriptorUtil.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <set>

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

std::shared_ptr<ShaderFunction> VKShaderFunction::InitWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage)
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

static DescriptorType GetDescriptorType(VkDescriptorType bindingType)
{
    DescriptorType retType = DESCRIPTOR_TYPE_MAX;
    switch (bindingType)
    {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            retType = DESCRIPTOR_TYPE_SAMPLER;
            break;
            
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            retType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
            
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            retType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
            
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            retType = DESCRIPTOR_TYPE_STORAGE_IMAGE;
            break;
            
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            retType = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
            
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            retType = DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
            
        default:
            break;
    }
    
    return retType;
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
        
        // 收集不同的binding的类型
        std::set<VkDescriptorType> bindingTypes;
        
        for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding)
        {
            const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
            VkDescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];
            layout_binding.binding = refl_binding.binding;
            layout_binding.descriptorType = (VkDescriptorType)refl_binding.descriptor_type;
            bindingTypes.insert(layout_binding.descriptorType);
            layout_binding.descriptorCount = 1;
            for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim)
            {
                layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
            }
            layout_binding.stageFlags = (VkShaderStageFlagBits)shaderModule.shader_stage;

            // 
            layout_binding.stageFlags = VK_SHADER_STAGE_ALL;
            
            //printf("binding %d = %s\n", refl_binding.binding, refl_binding.name);
        }
        
        // 判断每个set上的绑定是否是同一类型，这里每个set上有不同类型的desType，这里以后可能需要重构
        //assert(bindingTypes.size() == 1);
        layout.descriptorType = GetDescriptorType(*bindingTypes.begin());
        
        layout.set_number = refl_set.set;
        layout.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        
        // PUSH Descriptor必须使用VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR这个flag
        layout.create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        //layout.create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
        layout.create_info.pNext = nullptr;
        layout.create_info.bindingCount = refl_set.binding_count;
        layout.create_info.pBindings = layout.bindings.data();
    }
    
    return std::move(set_layouts);
}

static VertexInputLayout GetVertexInfo(const SpvReflectShaderModule& shaderModule)
{
    // 获得输入的数据
    uint32_t count = 0;
    SpvReflectResult result = spvReflectEnumerateInputVariables(&shaderModule, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectInterfaceVariable*> inputVars(count);
    result = spvReflectEnumerateInputVariables(&shaderModule, &count, inputVars.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.reserve(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i)
    {
        const SpvReflectInterfaceVariable& refl_var = *(inputVars[i]);
        // ignore built-in variables
        if (refl_var.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) 
        {
            continue;
        }
        VkVertexInputAttributeDescription attr_desc{};
        attr_desc.location = refl_var.location;
        attr_desc.binding = attr_desc.location;
        attr_desc.format = (VkFormat)(refl_var.format);
        attr_desc.offset = 0;  // final offset computed below after sorting.
        attributeDescriptions.push_back(attr_desc);
    }
    
    // 根据位置进行属性排序
    std::sort(std::begin(attributeDescriptions), std::end(attributeDescriptions),
              [](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b)
    {
        return a.location < b.location;
    });
    
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    
    // 计算最终每个顶点属性的索引以及顶点的跨距大小
    for (auto& attribute : attributeDescriptions)
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = attribute.binding;
        bindingDescription.stride = VulkanBufferUtil::GetFormatSize(attribute.format);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        bindingDescriptions.push_back(bindingDescription);
    }
    
    VertexInputLayout vertexInputLayout;
    vertexInputLayout.attributeDescriptions = std::move(attributeDescriptions);
    vertexInputLayout.inputBindings = std::move(bindingDescriptions);
    
    return vertexInputLayout;
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
        mShaderFunction = nullptr;
        return nullptr;
    }
    mShaderFunction = computeShaderModule;
    mShaderStage = shaderStage;
    
    switch (shaderStage)
    {
        case ShaderStage_Vertex:
            mVKShaderStage = VK_SHADER_STAGE_VERTEX_BIT;
            break;
            
        case ShaderStage_Fragment:
            mVKShaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
            
        case ShaderStage_Compute:
            mVKShaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
            break;
            
        default:
            break;
    }
    
    SpvReflectShaderModule shaderModule = {};
    SpvReflectResult result = spvReflectCreateShaderModule(shaderSource.size(), shaderSource.data(), &shaderModule);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    
    mDescriptorSets = GetDescriptorInfo(shaderModule);
    mEntryName = shaderModule.entry_point_name;
    
    assert(shaderModule.entry_point_count);
    
    // 获得计算着色器的局部工作组线程的大小
    if (shaderModule.shader_stage == SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT)
    {
        const SpvReflectEntryPoint* entryPoint = spvReflectGetEntryPoint(&shaderModule, shaderModule.entry_point_name);
        mWorkGroupSize.x = entryPoint->local_size.x;
        mWorkGroupSize.y = entryPoint->local_size.y;
        mWorkGroupSize.z = entryPoint->local_size.z;
    }
    
    if (shaderModule.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
    {
        mVertexInputLayout = GetVertexInfo(shaderModule);
    }
    
    spvReflectDestroyShaderModule(&shaderModule);
    
    return shared_from_this();
}

ShaderStage VKShaderFunction::GetShaderStage() const
{
    return mShaderStage;
}

static VkShaderModule CreateShaderModule(VkDevice device, const ShaderCode& shaderCode)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = (const uint32_t*)shaderCode.data();
	VkShaderModule shaderModule = VK_NULL_HANDLE;
	VkResult res = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    assert(res == VK_SUCCESS);

    return shaderModule;
}

const uint32_t MAX_FRAMES_IN_FLIGHT = 3;

VKGraphicsShader::VKGraphicsShader(VulkanContextPtr context, const ShaderCode& vertexShader, const ShaderCode& fragmentShader)
{
    mContext = context;
    mDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	SpvReflectShaderModule vertexShaderModule = {};
	SpvReflectResult result = spvReflectCreateShaderModule(vertexShader.size(), vertexShader.data(), &vertexShaderModule);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	CollectResource(vertexShaderModule, ShaderStage_Vertex);

    // 收集顶点布局数据
    mVertexInputLayout = GetVertexInfo(vertexShaderModule);

	SpvReflectShaderModule fragShaderModule = {};
	result = spvReflectCreateShaderModule(fragmentShader.size(), fragmentShader.data(), &fragShaderModule);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);
	CollectResource(fragShaderModule, ShaderStage_Fragment);

	mVertexShader = CreateShaderModule(mContext->device, vertexShader);
    if (vertexShaderModule.entry_point_name)
    {
        mVertexEntryName = std::string(vertexShaderModule.entry_point_name);
    }

    spvReflectDestroyShaderModule(&vertexShaderModule);
    
	mFragShader = CreateShaderModule(mContext->device, fragmentShader);
	if (fragShaderModule.entry_point_name)
	{
		mFragmentEntryName = std::string(fragShaderModule.entry_point_name);
	}
    spvReflectDestroyShaderModule(&fragShaderModule);

    GenerateVulkanDescriptorSetLayout();
    GenerateDescriptorSets();
}

VKGraphicsShader::~VKGraphicsShader()
{
    vkDestroyShaderModule(mContext->device, mVertexShader, nullptr);
    mVertexShader = VK_NULL_HANDLE;
    vkDestroyShaderModule(mContext->device, mFragShader, nullptr);
    mFragShader = VK_NULL_HANDLE;

	// destroy layout
	for (auto& descriptorSetLayout : mDescriptorSetLayouts) 
    {
		vkDestroyDescriptorSetLayout(mContext->device, descriptorSetLayout, nullptr);
	}
    mDescriptorSetLayouts.clear();

	for (auto& iter : mDescriptorSets)
	{
        vkFreeDescriptorSets(mContext->device, mContext->graphicsDescriptorPool, iter.size(), iter.data());
        iter.clear();
	}
    mDescriptorSets.clear();
}

void VKGraphicsShader::BindUniformBuffer(VkCommandBuffer commandBuffer, const std::string& resourceName, const ShaderBufferDesc& bufferDesc, VkPipelineLayout layout)
{
	auto bindData = mReflectionDatas.find(resourceName);
	if (bindData == mReflectionDatas.end())
	{
		printf("Fail to find shader resource %s", resourceName.c_str());
		return;
	}
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = bufferDesc.buffer;
	bufferInfo.offset = bufferDesc.offset;
	bufferInfo.range = bufferDesc.range;

	uint32_t nextFrameIndex = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    bool enablePushDesDescriptor = mContext->vulkanExtension.enablePushDesDescriptor;

	VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetBufferWriteDescriptorSet(
        !enablePushDesDescriptor ? mDescriptorSets[nextFrameIndex][bindData->second.set] : VK_NULL_HANDLE,
		bindData->second.descriptorType,
		bindData->second.binding, &bufferInfo,
		bindData->second.descriptorCount);

    if (enablePushDesDescriptor)
    {
        vkCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, bindData->second.set, 1, &writeDescriptorSet);
    }
    else
    {
        vkUpdateDescriptorSets(mContext->device, 1, &writeDescriptorSet, 0, nullptr);
    }
}

void VKGraphicsShader::BindTexture(VkCommandBuffer commandBuffer, const std::string& resourceName, const ShaderImageDesc& imageDesc, VkPipelineLayout layout)
{
	auto bindData = mReflectionDatas.find(resourceName);
	if (bindData == mReflectionDatas.end())
	{
		LOG_INFO("Fail to find shader resource %s", resourceName.c_str());
		return;
	}
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageView = imageDesc.image;
	imageInfo.imageLayout = imageDesc.imageLayout;
	imageInfo.sampler = imageDesc.sampler;

	uint32_t nextFrameIndex = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    bool enablePushDesDescriptor = mContext->vulkanExtension.enablePushDesDescriptor;

	VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetImageWriteDescriptorSet(
        !enablePushDesDescriptor ? mDescriptorSets[nextFrameIndex][bindData->second.set] : VK_NULL_HANDLE,
		bindData->second.descriptorType,
		bindData->second.binding, &imageInfo,
		bindData->second.descriptorCount);

	if (enablePushDesDescriptor)
	{
		vkCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, bindData->second.set, 1, &writeDescriptorSet);
	}
	else
	{
		vkUpdateDescriptorSets(mContext->device, 1, &writeDescriptorSet, 0, nullptr);
	}
}

void VKGraphicsShader::BindSampler(VkCommandBuffer commandBuffer, const std::string& resourceName, VkSampler sampler, VkPipelineLayout layout)
{
	auto bindData = mReflectionDatas.find(resourceName);
	if (bindData == mReflectionDatas.end())
	{
        LOG_INFO("Fail to find shader resource %s", resourceName.c_str());
		return;
	}
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.sampler = sampler;

	uint32_t nextFrameIndex = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    bool enablePushDesDescriptor = mContext->vulkanExtension.enablePushDesDescriptor;

	VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetImageWriteDescriptorSet(
        !enablePushDesDescriptor ? mDescriptorSets[nextFrameIndex][bindData->second.set] : VK_NULL_HANDLE,
		bindData->second.descriptorType,
		bindData->second.binding, &imageInfo,
		bindData->second.descriptorCount);

	if (enablePushDesDescriptor)
	{
		vkCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, bindData->second.set, 1, &writeDescriptorSet);
	}
	else
	{
		vkUpdateDescriptorSets(mContext->device, 1, &writeDescriptorSet, 0, nullptr);
	}
}

void VKGraphicsShader::CollectResource(SpvReflectShaderModule shaderModule, ShaderStage shaderStage)
{
    uint32_t count = 0;
    SpvReflectResult result = spvReflectEnumerateDescriptorSets(&shaderModule, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&shaderModule, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<DescriptorSetLayoutData> setLayouts(sets.size(), DescriptorSetLayoutData{});
    
    for (size_t iSet = 0; iSet < sets.size(); ++iSet)
    {
        // 当前的set
        const SpvReflectDescriptorSet& reflDesSet = *(sets[iSet]);
        
        for (uint32_t iBinding = 0; iBinding < reflDesSet.binding_count; ++iBinding)
        {
            const SpvReflectDescriptorBinding& reflBinding = *(reflDesSet.bindings[iBinding]);

			if (mReflectionDatas.find(reflBinding.name) == mReflectionDatas.end())
            {
				std::string resourceName = reflBinding.name;
                uint32_t set = reflBinding.set;
                uint32_t binding = reflBinding.binding;
                uint32_t descriptorCount = 1;
				for (uint32_t iDim = 0; iDim < reflBinding.array.dims_count; ++iDim)
				{
					descriptorCount *= reflBinding.array.dims[iDim];
				}
                VkDescriptorType descriptorType = static_cast<VkDescriptorType>(reflBinding.descriptor_type);
                VkShaderStageFlags shaderStageFlag = VK_SHADER_STAGE_ALL;

				BindMetaData metaData{ set, binding, descriptorCount, descriptorType, shaderStageFlag };
                mReflectionDatas[resourceName] = metaData;
			}
        }
    }
}

void VKGraphicsShader::GenerateVulkanDescriptorSetLayout()
{
	std::vector<VkDescriptorSetLayoutCreateInfo> descriptorSetLayoutCreateInfos;
	std::vector<VkDescriptorSetLayoutBinding>    layoutBindings;

    // 预先分配2个set，引擎约定就是2个set了，set 0是顶点shader，set 1是片元shader
	std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setGroups;
    setGroups[0] = std::vector<VkDescriptorSetLayoutBinding>();
    //setGroups[1] = std::vector<VkDescriptorSetLayoutBinding>();

	for (const auto& [resourceName, metaData] : mReflectionDatas) 
    {
        VkDescriptorSetLayoutBinding binding = {};
		VkShaderStageFlags           stageFlags = metaData.shaderStageFlag;
		uint32_t                       set = metaData.set;

        binding.binding = metaData.binding;
        binding.descriptorCount = metaData.descriptorCount;
        binding.descriptorType = metaData.descriptorType;
        binding.stageFlags = stageFlags;

		layoutBindings.push_back(binding);
        setGroups[set].push_back(binding);
	}

    bool enablePushDesDescriptor = mContext->vulkanExtension.enablePushDesDescriptor;

	for (const auto& [setIndex, bindingVecs] : setGroups) 
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        if (enablePushDesDescriptor)
        {
            descriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        }
        descriptorSetLayoutCreateInfo.bindingCount = (uint32_t)bindingVecs.size();
        descriptorSetLayoutCreateInfo.pBindings = bindingVecs.data();
        VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
        vkCreateDescriptorSetLayout(mContext->device, &descriptorSetLayoutCreateInfo, nullptr, &setLayout);

		mDescriptorSetLayouts.push_back(setLayout);
	}
}

void VKGraphicsShader::GenerateDescriptorSets()
{
    // enablePushDesDescriptor 打开后就不需要创建DescriptorSet
    bool enablePushDesDescriptor = mContext->vulkanExtension.enablePushDesDescriptor;
    if (enablePushDesDescriptor)
    {
        return;
    }

    for (auto &iter : mDescriptorSets)
    {
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mContext->graphicsDescriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)mDescriptorSetLayouts.size();
		allocInfo.pSetLayouts = mDescriptorSetLayouts.data();

        iter.resize(mDescriptorSetLayouts.size());
        VkResult res = vkAllocateDescriptorSets(mContext->device, &allocInfo, iter.data());
		if (res != VK_SUCCESS)
        {
            abort();
		}
    }
}

uint32_t VKGraphicsShader::GetResourceBindIndex(const std::string& resourceName) const
{
    auto bindData = mReflectionDatas.find(resourceName);
    if (bindData == mReflectionDatas.end())
    {
        LOG_INFO("Fail to find shader resource %s", resourceName.c_str());
        return (uint32_t)-1;
    }
    return bindData->second.binding;
}

uint32_t VKGraphicsShader::GetSetOffset(DescriptorType descriptorType) const
{
    // 遍历反射数据找到指定类型的 set
    for (const auto& [name, metaData] : mReflectionDatas)
    {
        DescriptorType type = GetDescriptorType(metaData.descriptorType);
        if (type == descriptorType)
        {
            return metaData.set;
        }
    }
    return 0;
}

NAMESPACE_RENDERCORE_END
