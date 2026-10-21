// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <QVulkanFunctions>

#include "vulkan/ShaderModule.h"

class QVulkanWindow;
class QImage;
namespace vulkan{
	class Texture;
	class PhysicalDevice;
	class Device;
	class Queue;
}

bool SetMaxMSE( QVulkanWindow* w, int mse_target = 16 );

std::unique_ptr<vulkan::Texture> createTextureImage(
	const QImage &image, 
	vulkan::PhysicalDevice physicalDevice, 
	vulkan::Device& device, 
	vulkan::Queue graphicsQueue, 
	VkCommandPool commandPool);

std::unique_ptr<vulkan::ShaderModule> createShader(
	QVulkanWindow *window,
	vulkan::Device device,
	const QString &name);

// Creates a buffer, allocates host-visible memory from the given memory type index,
// and binds it. On failure, calls qFatal().
void createBufferWithMemory(
	VkDevice device,
	QVulkanDeviceFunctions* devFuncs,
	const VkBufferCreateInfo& bufInfo,
	uint32_t hostVisibleMemoryTypeIndex,
	VkBuffer* outBuffer,
	VkDeviceMemory* outMemory);

// Creates a pipeline cache. On failure, calls qFatal().
void createPipelineCache(
	VkDevice device,
	QVulkanDeviceFunctions* devFuncs,
	VkPipelineCache* outPipelineCache);

// Creates a pipeline layout from the given descriptor set layouts.
// On failure, calls qFatal().
void createPipelineLayout(
	VkDevice device,
	QVulkanDeviceFunctions* devFuncs,
	uint32_t setLayoutCount,
	const VkDescriptorSetLayout* pSetLayouts,
	VkPipelineLayout* outPipelineLayout);