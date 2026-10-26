// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QImage>
#include <QVulkanFunctions>
#include <QSize>
#include <memory>

namespace vulkan{
	class Texture;
	class PhysicalDevice;
	class Device;
	class Queue;
}

class imageData {
	private:
		std::unique_ptr<uint8_t[]> pixels;
		int width, height;
		
	public:
		// Construct from raw RGBA8888 pixel data (takes ownership)
		imageData( int w, int h, std::unique_ptr<uint8_t[]> pixels );
		
		// Legacy constructor via QImage
		imageData( QImage image_data );
		static imageData black_image();
		
		QSize size() const { return QSize(width, height); }
		
		std::unique_ptr<vulkan::Texture> createTextureImage(
			vulkan::PhysicalDevice physicalDevice, 
			vulkan::Device& device, 
			vulkan::Queue graphicsQueue, 
			VkCommandPool commandPool) const;


};