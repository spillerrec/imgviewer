// SPDX-License-Identifier: GPL-3.0-or-later

#include "VulkanTextureRenderer.h"
#include "VulkanHelpers.hpp"
#include "imageCache.h"

#include <QVulkanFunctions>
#include <QFile>

static float vertexData[] = {
-1, +1, 0.0, 1.0, 0.0,
+1, -1, 1.0, 0.0, 0.0,
+1, +1, 1.0, 1.0, 1.0,
-1, -1, 0.0, 0.0, 0.0,
+1, -1, 1.0, 0.0, 0.0,
-1, +1, 0.0, 1.0, 1.0,
};

// Uniform buffer layout: mat4 projection (64 bytes) + float isBt2020 (aligned to 16 bytes)
// Total: 80 bytes
static const int UNIFORM_DATA_SIZE = 80;
struct UniformData {
    float m[16];
    float isBt2020; // passed as float for GLSL compatibility
};

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
	return (v + byteAlign - 1) & ~(byteAlign - 1);
}

VulkanTextureRenderer::VulkanTextureRenderer(QVulkanWindow *w, bool msaa)
	: m_window(w), m_drawArea(0, 0, w->width(), w->height())
{
	if ( msaa )
		SetMaxMSE(w, 16);
}


void VulkanTextureRenderer::setDrawArea(const QRect &area)
{
	if (m_drawArea == area)
		return;

	m_drawArea = area.normalized();
}

void VulkanTextureRenderer::setImage(std::shared_ptr<imageCache> image_cache, int frame_idx)
{
	// Store the original image; createTextureImage will handle empty/null cases
	const imageData* m_image = image_cache ? image_cache->frame(frame_idx) : nullptr;
	m_isBt2020 = image_cache ? image_cache->get_profile().cicp.is_bt2020pq() : false;

	VkDevice dev = m_window->device();

	vulkan::Device v_device(m_window->vulkanInstance(), dev);
	vulkan::PhysicalDevice v_phys_dev(m_window->vulkanInstance(), m_window->physicalDevice());
	vulkan::Queue graphicsQueue(v_device, m_window->graphicsQueue());

	// Recreate texture and image view
	if (m_image)
		m_texture = m_image->createTextureImage(v_phys_dev, v_device, graphicsQueue, m_window->graphicsCommandPool());
	else
		m_texture = imageData::black_image().createTextureImage(v_phys_dev, v_device, graphicsQueue, m_window->graphicsCommandPool());
	m_textureImageView.reset();
	m_textureImageView = std::make_unique<vulkan::ImageView>(v_device, *m_texture, m_texture->getFormat());

	// Update descriptor sets with the new image view
	const int concurrentFrameCount = m_window->concurrentFrameCount();
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = *m_textureImageView;
	imageInfo.sampler = *m_textureSampler;

	for (int i = 0; i < concurrentFrameCount; ++i) {
		VkWriteDescriptorSet descWrite{};
		descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descWrite.dstSet = m_descSet[i];
		descWrite.dstBinding = 1;
		descWrite.descriptorCount = 1;
		descWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descWrite.pImageInfo = &imageInfo;

		m_devFuncs->vkUpdateDescriptorSets(dev, 1, &descWrite, 0, nullptr);
	}
}

void VulkanTextureRenderer::initResources()
{
	VkDevice dev = m_window->device();
	m_devFuncs = m_window->vulkanInstance()->deviceFunctions(dev);

	vulkan::Device v_device(m_window->vulkanInstance(), dev);
	vulkan::PhysicalDevice v_phys_dev(m_window->vulkanInstance(), m_window->physicalDevice());
	vulkan::Queue graphicsQueue(v_device, m_window->graphicsQueue());

	VkPhysicalDeviceProperties properties{};
	v_phys_dev.functions()->vkGetPhysicalDeviceProperties(v_phys_dev, &properties);
	m_textureSampler = std::make_unique<vulkan::Sampler>( v_device, properties.limits.maxSamplerAnisotropy );

	const int concurrentFrameCount = m_window->concurrentFrameCount();
	const VkPhysicalDeviceLimits *pdevLimits = &m_window->physicalDeviceProperties()->limits;
	const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
	qDebug("uniform buffer offset alignment is %u", (uint) uniAlign);

	// Our internal layout is vertex, uniform, uniform, ... with each uniform buffer start offset aligned to uniAlign.
	const VkDeviceSize vertexAllocSize = aligned(sizeof(vertexData), uniAlign);
	const VkDeviceSize uniformAllocSize = aligned(UNIFORM_DATA_SIZE, uniAlign);

	m_buf = std::make_unique<vulkan::Buffer>(v_device,
		vertexAllocSize + concurrentFrameCount * uniformAllocSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	VkMemoryRequirements memReq = m_buf->memRequirements();
	m_bufMem = std::make_unique<vulkan::DeviceMemory>(v_device, memReq.size, m_window->hostVisibleMemoryIndex());
	m_buf->bindMemory(*m_bufMem);

	quint8 *p;
	VkResult err = m_bufMem->Map(0, VK_WHOLE_SIZE, 0, reinterpret_cast<void **>(&p));
	if (err != VK_SUCCESS)
		qFatal("Failed to map memory: %d", err);
	memcpy(p, vertexData, sizeof(vertexData));
	QMatrix4x4 ident;
	memset(m_uniformBufInfo, 0, sizeof(m_uniformBufInfo));
	for (int i = 0; i < concurrentFrameCount; ++i) {
		const VkDeviceSize offset = vertexAllocSize + i * uniformAllocSize;
		memcpy(p + offset, ident.constData(), 16 * sizeof(float));
		m_uniformBufInfo[i].buffer = static_cast<VkBuffer>(*m_buf);
		m_uniformBufInfo[i].offset = offset;
		m_uniformBufInfo[i].range = uniformAllocSize;
	}
	m_bufMem->Unmap();

	VkVertexInputBindingDescription vertexBindingDesc = {
		0, // binding
		5 * sizeof(float),
		VK_VERTEX_INPUT_RATE_VERTEX
	};
	VkVertexInputAttributeDescription vertexAttrDesc[] = {
		{ // position
			0, // location
			0, // binding
			VK_FORMAT_R32G32_SFLOAT,
			0
		},
		{ // color
			1,
			0,
			VK_FORMAT_R32G32B32_SFLOAT,
			2 * sizeof(float)
		}
	};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext = nullptr;
	vertexInputInfo.flags = 0;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
	vertexInputInfo.vertexAttributeDescriptionCount = 2;
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;

	// Set up descriptor set and its layout.
	std::vector<VkDescriptorPoolSize> descPoolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(concurrentFrameCount) },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(concurrentFrameCount) }
	};
	m_descPool = std::make_unique<vulkan::DescriptorPool>(v_device, descPoolSizes, concurrentFrameCount);

	std::vector<VkDescriptorSetLayoutBinding> layoutBinding = {{
		0, // binding
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		1,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	},{
		1, // binding
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}};
	VkDescriptorSetLayoutCreateInfo descLayoutInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		(uint32_t)layoutBinding.size(),
		layoutBinding.data()
	};
	err = m_devFuncs->vkCreateDescriptorSetLayout(dev, &descLayoutInfo, nullptr, &m_descSetLayout);
	if (err != VK_SUCCESS)
		qFatal("Failed to create descriptor set layout: %d", err);

	for (int i = 0; i < concurrentFrameCount; ++i) {
		VkDescriptorSetAllocateInfo descSetAllocInfo = {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			* m_descPool,
			1,
			&m_descSetLayout
		};
		err = m_devFuncs->vkAllocateDescriptorSets(dev, &descSetAllocInfo, &m_descSet[i]);
		if (err != VK_SUCCESS)
			qFatal("Failed to allocate descriptor set: %d", err);

		VkWriteDescriptorSet descWrite{};
		descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descWrite.dstSet = m_descSet[i];
		descWrite.descriptorCount = 1;
		descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descWrite.pBufferInfo = &m_uniformBufInfo[i];

		m_devFuncs->vkUpdateDescriptorSets(dev, 1, &descWrite, 0, nullptr);
	}

	// Create initial empty texture and update descriptor sets with image/sampler binding
	setImage();

	// Pipeline cache
	createPipelineCache(dev, m_devFuncs, &m_pipelineCache);

	// Pipeline layout
	createPipelineLayout(dev, m_devFuncs, 1, &m_descSetLayout, &m_pipelineLayout);

	// Shaders
	auto vertShaderModule = createShader(m_window, v_device, QStringLiteral("shaders/image_scaler.spv"));
	auto fragShaderModule = createShader(m_window, v_device, QStringLiteral("shaders/color_adaption.spv"));
	if (!vertShaderModule || !fragShaderModule)
		qFatal("Failed to load shader files. Make sure you are running from the correct directory (where 'shaders/' exists).");

	// Graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo;
	memset(&pipelineInfo, 0, sizeof(pipelineInfo));
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	VkPipelineShaderStageCreateInfo shaderStages[2] = {
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr,
			0,
			VK_SHADER_STAGE_VERTEX_BIT,
			*vertShaderModule,
			"main",
			nullptr
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr,
			0,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			*fragShaderModule,
			"main",
			nullptr
		}
	};
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;

	VkPipelineInputAssemblyStateCreateInfo ia;
	memset(&ia, 0, sizeof(ia));
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineInfo.pInputAssemblyState = &ia;

	// The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
	// This way the pipeline does not need to be touched when resizing the window.
	VkPipelineViewportStateCreateInfo vp;
	memset(&vp, 0, sizeof(vp));
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.viewportCount = 1;
	vp.scissorCount = 1;
	pipelineInfo.pViewportState = &vp;

	VkPipelineRasterizationStateCreateInfo rs;
	memset(&rs, 0, sizeof(rs));
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_NONE; // we want the back face as well
	rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rs.lineWidth = 1.0f;
	pipelineInfo.pRasterizationState = &rs;

	VkPipelineMultisampleStateCreateInfo ms;
	memset(&ms, 0, sizeof(ms));
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	// Enable multisampling.
	ms.rasterizationSamples = m_window->sampleCountFlagBits();
	pipelineInfo.pMultisampleState = &ms;

	VkPipelineDepthStencilStateCreateInfo ds;
	memset(&ds, 0, sizeof(ds));
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.depthTestEnable = VK_TRUE;
	ds.depthWriteEnable = VK_TRUE;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipelineInfo.pDepthStencilState = &ds;

	VkPipelineColorBlendStateCreateInfo cb;
	memset(&cb, 0, sizeof(cb));
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	// no blend, write out all of rgba
	VkPipelineColorBlendAttachmentState att;
	memset(&att, 0, sizeof(att));
	att.colorWriteMask = 0xF;
	cb.attachmentCount = 1;
	cb.pAttachments = &att;
	pipelineInfo.pColorBlendState = &cb;

	VkDynamicState dynEnable[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dyn;
	memset(&dyn, 0, sizeof(dyn));
	dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dyn.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
	dyn.pDynamicStates = dynEnable;
	pipelineInfo.pDynamicState = &dyn;

	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_window->defaultRenderPass();

	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	m_pipeline = std::make_unique<vulkan::Pipeline>(v_device, m_pipelineCache, pipelineInfo);

	m_initialized = true;
}

void VulkanTextureRenderer::initSwapChainResources()
{
	// Projection matrix
	m_proj = m_window->clipCorrectionMatrix(); // adjust for Vulkan-OpenGL clip space differences
	const QSize sz = m_window->swapChainImageSize();
	m_proj.perspective(45.0f, sz.width() / (float) sz.height(), 0.01f, 100.0f);
	m_proj.translate(0, 0, -4);
}

void VulkanTextureRenderer::releaseSwapChainResources()
{
	qDebug("releaseSwapChainResources");
}

void VulkanTextureRenderer::releaseResources()
{
	qDebug("releaseResources");

	VkDevice dev = m_window->device();

	m_pipeline.reset();

	if (m_pipelineLayout) {
		m_devFuncs->vkDestroyPipelineLayout(dev, m_pipelineLayout, nullptr);
		m_pipelineLayout = VK_NULL_HANDLE;
	}

	if (m_pipelineCache) {
		m_devFuncs->vkDestroyPipelineCache(dev, m_pipelineCache, nullptr);
		m_pipelineCache = VK_NULL_HANDLE;
	}

	if (m_descSetLayout) {
		m_devFuncs->vkDestroyDescriptorSetLayout(dev, m_descSetLayout, nullptr);
		m_descSetLayout = VK_NULL_HANDLE;
	}

	m_descPool.reset();

	m_buf.reset();
	m_bufMem.reset();
	m_texture.reset();
	m_textureImageView.reset();
	m_textureSampler.reset();
	m_initialized = false;
}

QMatrix4x4 VulkanTextureRenderer::calculateProjection(const QSize &sz)
{
	// Build a projection that maps the quad (vertices in [-1,+1] NDC) into m_drawArea.
	// This simulates painter.drawImage(area, image): scales and offsets to fit the target rect.
	const float sw = static_cast<float>(sz.width());
	const float sh = static_cast<float>(sz.height());

	// m_drawArea is in Qt logical pixels; sz is physical. Convert area to physical pixels.
	QRect area = m_drawArea.normalized();
	if (area.isEmpty()) {
		// Fallback: fill entire viewport
		area = QRect(0, 0, sz.width(), sz.height());
	} else {
		const qreal dpr = m_window->devicePixelRatio();
		area = QRect(area.x() * dpr, area.y() * dpr, area.width() * dpr, area.height() * dpr);
	}

	const float dx = static_cast<float>(area.x());
	const float dy = static_cast<float>(area.y());
	const float dw = static_cast<float>(area.width());
	const float dh = static_cast<float>(area.height());

	// Convert Qt QRect to NDC space. Both Qt and Vulkan viewport have Y increasing downward,
	// so the conversion is straightforward: screen [0,size] -> NDC [-1,+1]
	const float ndc_x0 = 2.0f * dx / sw - 1.0f;
	const float ndc_y0 = 2.0f * dy / sh - 1.0f;
	const float ndc_w  = 2.0f * dw / sw;   // NDC width of target rect
	const float ndc_h  = 2.0f * dh / sh;   // NDC height of target rect

	// Center of target rect in NDC space
	const float cx = ndc_x0 + ndc_w * 0.5f;
	const float cy = ndc_y0 + ndc_h * 0.5f;

	// The quad spans [-1,+1] (width=2, height=2). Scale it to fit the target rect,
	// then translate to center. Also flip Y because vertex data is Y-up but Vulkan viewport is Y-down.
	QMatrix4x4 m;
	m.setToIdentity();
	m(0, 0) = ndc_w * 0.5f;    // scaleX: quad width 2 -> target width ndc_w
	m(1, 1) = ndc_h * 0.5f;   // scaleY + Y-flip for Vulkan viewport
	m(0, 3) = cx;              // translateX (column-major)
	m(1, 3) = cy;              // translateY (column-major)
	return m;
}

void VulkanTextureRenderer::startNextFrame()
{
	VkCommandBuffer cb = m_window->currentCommandBuffer();
	const QSize sz = m_window->swapChainImageSize();

	VkClearColorValue clearColor = {{ 0, 0, 0, 1 }};
	VkClearDepthStencilValue clearDS = { 1, 0 };
	VkClearValue clearValues[3];
	memset(clearValues, 0, sizeof(clearValues));
	clearValues[0].color = clearValues[2].color = clearColor;
	clearValues[1].depthStencil = clearDS;

	VkRenderPassBeginInfo rpBeginInfo;
	memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
	rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBeginInfo.renderPass = m_window->defaultRenderPass();
	rpBeginInfo.framebuffer = m_window->currentFramebuffer();
	rpBeginInfo.renderArea.extent.width = sz.width();
	rpBeginInfo.renderArea.extent.height = sz.height();
	rpBeginInfo.clearValueCount = m_window->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
	rpBeginInfo.pClearValues = clearValues;
	VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
	m_devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	quint8 *p;
	VkDeviceSize offset = m_uniformBufInfo[m_window->currentFrame()].offset;
	VkResult err = m_bufMem->Map(offset, UNIFORM_DATA_SIZE, 0, reinterpret_cast<void **>(&p));
	if (err != VK_SUCCESS)
		qFatal("Failed to map memory: %d", err);

	UniformData uniforms{};
	memset(&uniforms, 0, sizeof(uniforms));

	QMatrix4x4 proj = calculateProjection(sz);
	memcpy(uniforms.m, proj.constData(), 16 * sizeof(float));
	uniforms.isBt2020 = m_isBt2020 ? 1.f : 0.f;
	memcpy(p, &uniforms, sizeof(uniforms));
	m_bufMem->Unmap();

	m_devFuncs->vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
	m_devFuncs->vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
							   &m_descSet[m_window->currentFrame()], 0, nullptr);
	VkDeviceSize vbOffset = 0;
	VkBuffer bufHandle = static_cast<VkBuffer>(*m_buf);
	m_devFuncs->vkCmdBindVertexBuffers(cb, 0, 1, &bufHandle, &vbOffset);

	VkViewport viewport;
	viewport.x = viewport.y = 0;
	viewport.width = sz.width();
	viewport.height = sz.height();
	viewport.minDepth = 0;
	viewport.maxDepth = 1;
	m_devFuncs->vkCmdSetViewport(cb, 0, 1, &viewport);

	VkRect2D scissor;
	scissor.offset.x = scissor.offset.y = 0;
	scissor.extent.width = viewport.width;
	scissor.extent.height = viewport.height;
	m_devFuncs->vkCmdSetScissor(cb, 0, 1, &scissor);

	m_devFuncs->vkCmdDraw(cb, 6, 1, 0, 0);

	m_devFuncs->vkCmdEndRenderPass(cmdBuf);

	m_window->frameReady();
	m_window->requestUpdate(); // render continuously, throttled by the presentation rate
}
