// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "util.h"
#include "Device.h"

#include <QVulkanFunctions>

namespace vulkan
{

class DescriptorSetLayout {
private:
	VulkanHandle<VkDescriptorSetLayout, Device, &QVulkanDeviceFunctions::vkDestroyDescriptorSetLayout> layout;
	
public:
	DescriptorSetLayout(Device device) : layout(device) {
		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 0;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 1> bindings = {samplerLayoutBinding};
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		layout.create( &QVulkanDeviceFunctions::vkCreateDescriptorSetLayout, layoutInfo );
	}
	
	
	operator VkDescriptorSetLayout() { return layout; }
};

}