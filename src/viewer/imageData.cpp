// SPDX-License-Identifier: GPL-3.0-or-later
#include "imageData.h"

#include "vulkan/Buffer.h"
#include "vulkan/DeviceMemory.h"
#include "vulkan/PhysicalDevice.h"
#include "vulkan/CommandBuffer.h"

#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>


imageData::imageData( int w, int h, std::unique_ptr<uint8_t[]> pixels_arg )
	: pixels(std::move(pixels_arg)), width(w), height(h) {}

imageData::imageData( QImage image_data ) {
	if (image_data.isNull() || image_data.size().isEmpty()) {
		width = height = 0;
		return;
	}

	ZoneScoped;
	QImage converted = image_data.convertToFormat(QImage::Format_RGBA8888);
	width = converted.width();
	height = converted.height();
	pixels = std::make_unique<uint8_t[]>(width * height * 4);
	int byteWidth = width * 4;
	for (int y = 0; y < height; ++y)
		memcpy(&pixels[y * byteWidth], reinterpret_cast<const uint8_t*>(converted.scanLine(y)), byteWidth);
}

imageData imageData::black_image() {
	auto img = QImage(1, 1, QImage::Format_RGBA8888);
	img.fill(0);
	return imageData(img);
}

std::unique_ptr<vulkan::Texture> imageData::createTextureImage(vulkan::PhysicalDevice physicalDevice, vulkan::Device& device, vulkan::Queue graphicsQueue, VkCommandPool commandPool) const {
	using namespace vulkan;
	ZoneScoped;

	// Handle empty/null images by creating a 1x1 black placeholder
	if (!pixels || width == 0 || height == 0)
		return imageData::black_image().createTextureImage(physicalDevice, device, graphicsQueue, commandPool);

	VkDeviceSize imageSize = width * 4 * height;
	vulkan::Buffer stagingBuffer( device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT );
	
	auto memRequirements = stagingBuffer.memRequirements();
	
	DeviceMemory stagingBufferMemory( device, memRequirements.size, physicalDevice.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) );
	
	stagingBuffer.bindMemory(stagingBufferMemory);

	TracyCFrameMarkStart("map-staging"); 
	void* data;
	stagingBufferMemory.Map(0, imageSize, 0, &data);
	memcpy(data, pixels.get(), static_cast<size_t>(imageSize));
	stagingBufferMemory.Unmap();
	TracyCFrameMarkEnd("map-staging");
	
	// create image
	auto texture = std::make_unique<Texture>( device, physicalDevice, width, height, VK_FORMAT_R8G8B8A8_UNORM );
	
	TracyCFrameMarkStart("gpu-work"); 
	auto commandBuffer = std::make_unique<CommandBuffer>( device, commandPool );
	commandBuffer->Begin();
	
	texture->transitionImageLayout( *commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
	commandBuffer->CopyBufferToImage( stagingBuffer, *texture, width, height );
	texture->transitionImageLayout( *commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	
	commandBuffer->End();
	graphicsQueue.SubmitAndWait( std::move(commandBuffer) );
	TracyCFrameMarkEnd("gpu-work");
	
	return texture;
}
