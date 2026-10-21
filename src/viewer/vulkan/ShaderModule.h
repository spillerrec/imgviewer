// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "util.h"
#include "Device.h"

#include <fstream>
#include <vector>
#include <QVulkanFunctions>

namespace vulkan
{

class ShaderModule {
private:
	VulkanHandle<VkShaderModule, Device, &QVulkanDeviceFunctions::vkDestroyShaderModule> shader;
	
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t) file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}
	
public:
	ShaderModule(Device device, const std::vector<char>& code) : shader(device) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		
		shader.create( &QVulkanDeviceFunctions::vkCreateShaderModule, createInfo );
	}
	
	ShaderModule(Device device, const std::string& filename) : ShaderModule(device, readFile(filename)) { }
	
	operator VkShaderModule() { return shader; }
};

}