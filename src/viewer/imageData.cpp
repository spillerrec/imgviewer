// SPDX-License-Identifier: GPL-3.0-or-later
#include "imageData.h"

#include "vulkan/Buffer.h"
#include "vulkan/DeviceMemory.h"
#include "vulkan/PhysicalDevice.h"
#include "vulkan/CommandBuffer.h"

#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>


imageData imageData::black_image() {
	QImage source(1, 1, QImage::Format_RGBA8888);
	source.fill(Qt::black);
	return {source};
}

std::unique_ptr<vulkan::Texture> imageData::createTextureImage(vulkan::PhysicalDevice physicalDevice, vulkan::Device& device, vulkan::Queue graphicsQueue, VkCommandPool commandPool) const {
	using namespace vulkan;
	ZoneScoped;

	// Handle empty/null images by creating a 1x1 black placeholder
	if (data.isNull() || data.size().isEmpty())
		return imageData::black_image().createTextureImage(physicalDevice, device, graphicsQueue, commandPool);

	TracyCFrameMarkStart("convertToFormat"); 
	QImage converted = data.convertToFormat(QImage::Format_RGBA8888);
	TracyCFrameMarkEnd("convertToFormat");
	
	TracyCFrameMarkStart("imageData init"); 
	auto imageData = std::make_unique<uint8_t[]>(converted.size().width() * converted.size().height() * 4);
	auto lineWidth = converted.size().width()*4;
	for (int y = 0; y < converted.size().height(); ++y)
    	memcpy(&imageData[y * lineWidth], reinterpret_cast<uint8_t*>(converted.scanLine(y)), lineWidth);
	TracyCFrameMarkEnd("imageData init");
	
	VkDeviceSize imageSize = lineWidth * converted.size().height();
	vulkan::Buffer stagingBuffer( device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT );
	
	auto memRequirements = stagingBuffer.memRequirements();
	
	DeviceMemory stagingBufferMemory( device, memRequirements.size, physicalDevice.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) );
	
	stagingBuffer.bindMemory(stagingBufferMemory);

	TracyCFrameMarkStart("map-staging"); 
	void* data;
	stagingBufferMemory.Map(0, imageSize, 0, &data);
	memcpy(data, imageData.get(), static_cast<size_t>(imageSize));
	stagingBufferMemory.Unmap();
	TracyCFrameMarkEnd("map-staging");
	
	// create image
	auto texture = std::make_unique<Texture>( device, physicalDevice, converted.size().width(), converted.size().height(), VK_FORMAT_R8G8B8A8_UNORM );
	
	TracyCFrameMarkStart("gpu-work"); 
	auto commandBuffer = std::make_unique<CommandBuffer>( device, commandPool );
	commandBuffer->Begin();
	
	texture->transitionImageLayout( *commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
	commandBuffer->CopyBufferToImage( stagingBuffer, *texture, converted.size().width(), converted.size().height() );
	texture->transitionImageLayout( *commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	
	commandBuffer->End();
	graphicsQueue.SubmitAndWait( std::move(commandBuffer) );
	TracyCFrameMarkEnd("gpu-work");
	
	return texture;
}
