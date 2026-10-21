// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Device.h"
#include "util.h"

namespace vulkan
{

class Sampler {
private:
	VulkanHandle<VkSampler, Device, &QVulkanDeviceFunctions::vkDestroySampler> sampler;
	
public:
	Sampler(Device device, float maxSamplerAnisotropy) : sampler(device) {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		samplerInfo.anisotropyEnable = VK_TRUE;

		samplerInfo.maxAnisotropy = maxSamplerAnisotropy;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		samplerInfo.unnormalizedCoordinates = VK_FALSE; //NOTE: True for real coordinates

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		sampler.create( &QVulkanDeviceFunctions::vkCreateSampler, samplerInfo );
	}
	
	
	operator VkSampler() { return sampler; }
};

}