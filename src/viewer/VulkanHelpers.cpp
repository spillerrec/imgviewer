// SPDX-License-Identifier: GPL-3.0-or-later

#include "VulkanHelpers.hpp"


#include "vulkan/Buffer.h"
#include "vulkan/DeviceMemory.h"
#include "vulkan/PhysicalDevice.h"
#include "vulkan/CommandBuffer.h"

#include <cstring>
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

#include <QFile>
#include <QList>
#include <QVulkanWindow>

bool SetMaxMSE( QVulkanWindow* w, int mse_target ) {
	const QList<int> counts = w->supportedSampleCounts();
	for (int s = 16; s >= 4; s /= 2) {
		if (counts.contains(s)) {
			w->setSampleCount(s);
			return true;
		}
	}
	return false;
}




std::unique_ptr<vulkan::Texture> createTextureImage(const QImage &image, vulkan::PhysicalDevice physicalDevice, vulkan::Device& device, vulkan::Queue graphicsQueue, VkCommandPool commandPool) {
	using namespace vulkan;
	ZoneScoped;

	// Handle empty/null images by creating a 1x1 black placeholder
	QImage source = image;
	if (image.isNull() || image.size().isEmpty()) {
		qWarning("TriangleRenderer::createTextureImage: empty or null image, using black placeholder");
		source = QImage(1, 1, QImage::Format_RGBA8888);
		source.fill(Qt::black);
	}

	TracyCFrameMarkStart("convertToFormat"); 
	QImage converted = source.convertToFormat(QImage::Format_RGBA8888);
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

std::unique_ptr<vulkan::ShaderModule> createShader(
	QVulkanWindow *window,
	vulkan::Device device,
	const QString &name)
{
	QFile file(name);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning("Failed to read shader %s", qPrintable(name));
		return nullptr;
	}
	QByteArray blob = file.readAll();
	file.close();

	qDebug("Shader size %s: %lld", qPrintable(name), blob.size());

	std::vector<char> code(blob.begin(), blob.end());
	try {
		return std::make_unique<vulkan::ShaderModule>(device, code);
	} catch (const std::runtime_error& e) {
		qWarning("Failed to create shader module: %s", qPrintable(QString::fromStdString(e.what())));
		return nullptr;
	}
}

void createBufferWithMemory(
	VkDevice device,
	QVulkanDeviceFunctions* devFuncs,
	const VkBufferCreateInfo& bufInfo,
	uint32_t hostVisibleMemoryTypeIndex,
	VkBuffer* outBuffer,
	VkDeviceMemory* outMemory)
{
	VkResult err = devFuncs->vkCreateBuffer(device, &bufInfo, nullptr, outBuffer);
	if (err != VK_SUCCESS)
		qFatal("Failed to create buffer: %d", err);

	VkMemoryRequirements memReq;
	devFuncs->vkGetBufferMemoryRequirements(device, *outBuffer, &memReq);

	VkMemoryAllocateInfo memAllocInfo = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		memReq.size,
		hostVisibleMemoryTypeIndex
	};

	err = devFuncs->vkAllocateMemory(device, &memAllocInfo, nullptr, outMemory);
	if (err != VK_SUCCESS)
		qFatal("Failed to allocate memory: %d", err);

	err = devFuncs->vkBindBufferMemory(device, *outBuffer, *outMemory, 0);
	if (err != VK_SUCCESS)
		qFatal("Failed to bind buffer memory: %d", err);
}

void createPipelineCache(
	VkDevice device,
	QVulkanDeviceFunctions* devFuncs,
	VkPipelineCache* outPipelineCache)
{
	VkPipelineCacheCreateInfo pipelineCacheInfo{};
	pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

	VkResult err = devFuncs->vkCreatePipelineCache(device, &pipelineCacheInfo, nullptr, outPipelineCache);
	if (err != VK_SUCCESS)
		qFatal("Failed to create pipeline cache: %d", err);
}

void createPipelineLayout(
	VkDevice device,
	QVulkanDeviceFunctions* devFuncs,
	uint32_t setLayoutCount,
	const VkDescriptorSetLayout* pSetLayouts,
	VkPipelineLayout* outPipelineLayout)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = setLayoutCount;
	pipelineLayoutInfo.pSetLayouts = pSetLayouts;

	VkResult err = devFuncs->vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, outPipelineLayout);
	if (err != VK_SUCCESS)
		qFatal("Failed to create pipeline layout: %d", err);
}

