// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "util.h"
#include "Device.h"

namespace vulkan
{

class Buffer {
private:
	VulkanHandle<VkBuffer, Device, &QVulkanDeviceFunctions::vkDestroyBuffer> buffer;
	
public:
	Buffer(Device device, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE) : buffer(device) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = sharingMode;

		buffer.create( &QVulkanDeviceFunctions::vkCreateBuffer, bufferInfo );
	}
	
	VkMemoryRequirements memRequirements() {
		VkMemoryRequirements memReq;
		buffer.functions()->vkGetBufferMemoryRequirements(buffer.getParent(), buffer, &memReq);
		return memReq;
	}
	
	VkResult bindMemory(VkDeviceMemory memory, VkDeviceSize memoryOffset=0) {
		return buffer.functions()->vkBindBufferMemory(buffer.getParent(), buffer, memory, memoryOffset);
	}
	
	operator VkBuffer() { return buffer; }
};

}