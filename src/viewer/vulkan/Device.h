// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QVulkanFunctions>
#include <QVulkanInstance>

namespace vulkan
{

class Device{
	private:
		VkDevice device = nullptr;
		QVulkanDeviceFunctions* functions_ptr = nullptr;
		
	public:
		Device(QVulkanInstance* instance, VkDevice device = nullptr) : device(device) {
			functions_ptr = instance->deviceFunctions(device);
		}
		
		using ParentType = VkDevice;
		
		operator VkDevice() { return device; } 
		
		void WaitIdle() { functions()->vkDeviceWaitIdle(device); }
		
		QVulkanDeviceFunctions* functions() { return functions_ptr; }
};

}