// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vector>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <set>
#include <string>
#include <stdexcept>

namespace vulkan
{
	

class PhysicalDevice{ //This refers to the actual psycial GPU ?
	private:
		VkPhysicalDevice device = nullptr;
		QVulkanFunctions* functions_ptr = nullptr;
		
	public:
		PhysicalDevice(QVulkanInstance* instance, VkPhysicalDevice device = nullptr) : device(device), functions_ptr(instance->functions()) { }
		
		auto functions(){ return functions_ptr; }
		
		bool SupportsExtensions(const std::vector<const char*>& extensions);
		
		std::vector<VkExtensionProperties> DeviceExtensionProperties(){
			uint32_t extensionCount;
			functions()->vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			functions()->vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
			return availableExtensions;
		};
		
		std::vector<VkQueueFamilyProperties> QueueFamilyProperties(){
			uint32_t queueFamilyCount = 0;
			functions()->vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			functions()->vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
			return queueFamilies;
		}
		
		/*
		Device CreateDevice(VkDeviceCreateInfo& info){
			return Device(device, info);
		}
		*/
		
		/*
		//Checks if a queue family supports this surface
		bool SupportsSurface(uint32_t queueFamilyIndex, VkSurfaceKHR surface){
			VkBool32 presentSupport = false;
			functions()->vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, surface, &presentSupport);
			return presentSupport;
		}
		*/
		
		operator VkPhysicalDevice() { return device; } 
		
		VkPhysicalDeviceMemoryProperties GetMemoryProperties() {
			VkPhysicalDeviceMemoryProperties memProperties;
			functions()->vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
			return memProperties;
		}
		
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
			VkPhysicalDeviceMemoryProperties memProperties = GetMemoryProperties();
			
			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if (typeFilter & (1 << i))
					if (properties == memProperties.memoryTypes[i].propertyFlags)
						return i;
			}

			throw std::runtime_error("failed to find suitable memory type!");
		}
		
};


inline bool PhysicalDevice::SupportsExtensions(const std::vector<const char*>& extensions) {
	std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

	for (const auto& extension : DeviceExtensionProperties()) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}



}