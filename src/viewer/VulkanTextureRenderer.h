// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRIANGLERENDERER_H
#define TRIANGLERENDERER_H

#include <QVulkanWindow>

#include <QVulkanFunctions>
#include "vulkan/Buffer.h"
#include "vulkan/CommandBuffer.h"
#include "vulkan/DescriptorPool.h"
#include "vulkan/DeviceMemory.h"
#include "vulkan/ImageView.h"
#include "vulkan/Pipeline.h"
#include "vulkan/Sampler.h"

class VulkanTextureRenderer : public QVulkanWindowRenderer
{
public:
	VulkanTextureRenderer(QVulkanWindow *w, const QImage &image = QImage(), bool msaa = false);

	void setImage(const QImage &image, bool isBt2020);
	void setDrawArea(const QRect &area);

	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;

	void startNextFrame() override;

protected:
	QImage m_image;
	bool m_isBt2020 = false;
	bool m_initialized = false;

	std::unique_ptr<vulkan::Texture> m_texture;
	std::unique_ptr<vulkan::ImageView> m_textureImageView;
	std::unique_ptr<vulkan::Sampler> m_textureSampler;

	QVulkanWindow *m_window;
	QVulkanDeviceFunctions *m_devFuncs;

	std::unique_ptr<vulkan::Buffer> m_buf;
	std::unique_ptr<vulkan::DeviceMemory> m_bufMem;
	VkDescriptorBufferInfo m_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

	std::unique_ptr<vulkan::DescriptorPool> m_descPool;
	VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_descSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

	VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

	std::unique_ptr<vulkan::Pipeline> m_pipeline;

	QMatrix4x4 m_proj;
	float m_rotation = 0.0f;

	// Target rectangle in window/swapchain pixel coordinates (simulates painter.drawImage(area, image))
	QRect m_drawArea;

private:
	QMatrix4x4 calculateProjection(const QSize &sz);
};

#endif // TRIANGLERENDERER_H
