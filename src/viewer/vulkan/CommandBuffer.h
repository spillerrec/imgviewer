// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Device.h"
#include "PhysicalDevice.h"
#include "DeviceMemory.h"
#include "util.h"

#include <QVulkanFunctions>

namespace vulkan
{
	

class CommandPool {
private:
	VulkanHandle<VkCommandPool, Device, &QVulkanDeviceFunctions::vkDestroyCommandPool> cmdPool;
	
public:
	CommandPool(Device device, uint32_t queueFamilyIndex) : cmdPool(device) {
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndex;

		cmdPool.create( &QVulkanDeviceFunctions::vkCreateCommandPool, poolInfo );
	}
	
	operator VkCommandPool() { return cmdPool; }
};

class CommandBuffer {
private:
	VkCommandBuffer cmdBuf = nullptr;
	Device device = nullptr;
	VkCommandPool cmdPool = nullptr; ///< For deletion
	
public:
	CommandBuffer( Device device, VkCommandPool pool ) : device(device) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdPool = allocInfo.commandPool = pool;
		allocInfo.commandBufferCount = 1;

		device.functions()->vkAllocateCommandBuffers(device, &allocInfo, &cmdBuf);
	}
	CommandBuffer(const CommandBuffer&) = delete;
	CommandBuffer(CommandBuffer&&) = default; //TODO: This appears to cause errors
	
	~CommandBuffer() {
		//TODO: Verify state
		if (cmdBuf)
			device.functions()->vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuf);
	}
	
	operator VkCommandBuffer() { return cmdBuf; }
	VkCommandBuffer& get() { return cmdBuf; }
	
	void Begin() {		
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if( device.functions()->vkBeginCommandBuffer(cmdBuf, &beginInfo) != VK_SUCCESS )
			throw std::runtime_error("failed to begin recording command buffer!");
	}
	
	void End() {
		if( device.functions()->vkEndCommandBuffer( cmdBuf ) != VK_SUCCESS )
			throw std::runtime_error("failed to record command buffer!");
	}
	
	void CopyBuffer(VkBuffer source, VkBuffer destination, VkDeviceSize byteCount) {
		VkBufferCopy region = { 0, 0, byteCount };
		device.functions()->vkCmdCopyBuffer( cmdBuf, source, destination, 1, &region );
	}
	
	
	void CopyBufferToImage(VkBuffer source, VkImage destination, uint32_t width, uint32_t height) {
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = {0, 0, 0};
		region.imageExtent = {
			width,
			height,
			1
		};
		
		device.functions()->vkCmdCopyBufferToImage(
			cmdBuf,
			source,
			destination,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);
	}
	
	
};


class Queue {
private:
	Device device;
	VkQueue queue;
	
public:
	Queue( Device device, VkQueue queue ) : device(device), queue(queue) { }
	Queue( Device device, uint32_t queueFamilyIndex, uint32_t queueIndex = 0 ) : device(device) {
		device.functions()->vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, &queue);
	}
	VkQueue& get() { return queue; }
	
	void SubmitAndWait( std::unique_ptr<CommandBuffer> cmdBuf ) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuf->get();

		device.functions()->vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		device.functions()->vkQueueWaitIdle(queue);
	}
};


class Texture {
private:
	VulkanHandle<VkImage, Device, &QVulkanDeviceFunctions::vkDestroyImage> textureImage;
	//TODO: Make these configurable?
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkFormat format = VK_FORMAT_R16G16B16A16_UNORM;
	std::unique_ptr<DeviceMemory> textureImageMemory;
	
public:
	Texture( Device device, PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, VkFormat imageFormat ) : textureImage(device), format(imageFormat) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width; // static_cast<uint32_t>(texture.width)
		imageInfo.extent.height = height; // static_cast<uint32_t>(texture.height)
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = layout;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0; // Optional
		
		textureImage.create( &QVulkanDeviceFunctions::vkCreateImage, imageInfo );

		
		VkMemoryRequirements memRequirements;
		device.functions()->vkGetImageMemoryRequirements(device, textureImage, &memRequirements);

		auto memoryTypeIndex = physicalDevice.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		textureImageMemory = std::make_unique<DeviceMemory>(device, memRequirements.size, memoryTypeIndex);

		device.functions()->vkBindImageMemory(device, textureImage, *textureImageMemory, 0);
	}

	auto getFormat() const { return format; }
	
	
	void transitionImageLayout(CommandBuffer& commandBuffer, VkImageLayout newLayout) {
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = layout;
		barrier.newLayout = newLayout;
		
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		
		barrier.image = textureImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		
		
		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else {
			throw std::invalid_argument("unsupported layout transition!");
		}
		
		textureImage.functions()->vkCmdPipelineBarrier(
			commandBuffer.get(),
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		
		layout = newLayout;
	}
	
	//TODO vkFreeMemory(device, textureImageMemory, nullptr);
	
	operator VkImage() { return textureImage; }
	
};

}