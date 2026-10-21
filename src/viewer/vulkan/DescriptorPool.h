// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "util.h"
#include "Device.h"

#include <QVulkanFunctions>

namespace vulkan
{

class DescriptorPool {
private:
	VulkanHandle<VkDescriptorPool, Device, &QVulkanDeviceFunctions::vkDestroyDescriptorPool> pool;
	
public:
	DescriptorPool(Device device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets) : pool(device) {
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxSets;

		pool.create( &QVulkanDeviceFunctions::vkCreateDescriptorPool, poolInfo );
	}
	
	operator VkDescriptorPool() { return pool; }
};


}