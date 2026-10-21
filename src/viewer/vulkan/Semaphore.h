// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "util.h"
#include "Device.h"

#include <QVulkanFunctions>

namespace vulkan
{

class Semaphore {
private:
	VulkanHandle<VkSemaphore, Device, &QVulkanDeviceFunctions::vkDestroySemaphore> semaphore;
	
public:
	Semaphore(Device device) : semaphore(device) {
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphore.create( &QVulkanDeviceFunctions::vkCreateSemaphore, semaphoreInfo );
	}
	
	
	operator VkSemaphore() { return semaphore; }
};

}