// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Device.h"
#include "util.h"

namespace vulkan
{

class Pipeline {
private:
	VulkanHandle<VkPipeline, Device, &QVulkanDeviceFunctions::vkDestroyPipeline> pipeline;

public:
	Pipeline(Device device, VkPipelineCache cache, const VkGraphicsPipelineCreateInfo& createInfo)
		: pipeline(device)
	{
		VkPipeline tmp = VK_NULL_HANDLE;
		VkResult result = pipeline.functions()->vkCreateGraphicsPipelines(
			pipeline.getParent(), cache, 1, &createInfo, nullptr, &tmp);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline");
		pipeline.assign(tmp);
	}

	operator VkPipeline() { return pipeline; }
};

}
