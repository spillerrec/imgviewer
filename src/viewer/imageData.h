// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QImage>
#include <QVulkanFunctions>

namespace vulkan{
	class Texture;
	class PhysicalDevice;
	class Device;
	class Queue;
}

class imageData {
	private:
		QImage data;
		
	public:
		imageData( QImage image_data ) : data( image_data ) { }
		static imageData black_image();
		
		QImage frame() const { return data; }
		QSize size() const { return data.size(); }
		
		std::unique_ptr<vulkan::Texture> createTextureImage(
			vulkan::PhysicalDevice physicalDevice, 
			vulkan::Device& device, 
			vulkan::Queue graphicsQueue, 
			VkCommandPool commandPool) const;


};