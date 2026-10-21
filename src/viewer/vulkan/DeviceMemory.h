// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Device.h"
#include "util.h"

namespace vulkan
{

class DeviceMemory {
private:
	VulkanHandle<VkDeviceMemory, Device, &QVulkanDeviceFunctions::vkFreeMemory> buffer;
	
public:
	DeviceMemory(Device device, VkDeviceSize size, uint32_t memoryTypeIndex) : buffer(device) {
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = size;
		allocInfo.memoryTypeIndex = memoryTypeIndex;

		buffer.create( &QVulkanDeviceFunctions::vkAllocateMemory, allocInfo );
	}
	
	VkResult Map(VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) {
		return buffer.functions()->vkMapMemory(buffer.getParent(), buffer, offset, size, flags, ppData);
	}
	
	
	void Unmap() {
		buffer.functions()->vkUnmapMemory(buffer.getParent(), buffer);
	}
	
	operator VkDeviceMemory() { return buffer; }
};

}