#pragma once
#ifndef TINYVK_TINYVGRAPHICSPIPELINE
#define TINYVK_TINYVGRAPHICSPIPELINE
	#include "./TinyVK.hpp"

	namespace TINYVULKAN_NAMESPACE {
		#define VKCOMP_RGBA VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		#define VKCOMP_BGRA VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT

		enum TinyVkDescriptorTypes {
			TINYVK_DESCRIPTOR_IMAGE_SAMPLER = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			TINYVK_DESCRIPTOR_STORAGE_IMAGE = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			TINYVK_DESCRIPTOR_UNIFORM_BUFFER = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			TINYVK_DESCRIPTOR_STORAGE_BUFFER = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		};

		/// <summary>Represents the Vertex shader layout data passing through the graphics pipeline.</summary>
		struct TinyVkVertexDescription {
			const VkVertexInputBindingDescription binding;
			const std::vector<VkVertexInputAttributeDescription> attributes;

			TinyVkVertexDescription(VkVertexInputBindingDescription binding, const std::vector<VkVertexInputAttributeDescription> attributes) : binding(binding), attributes(attributes) {}
		};

		/// <summary>Vulkan Graphics Pipeline using Dynamic Viewports/Scissors, Push Descriptors/Constants.</summary>
		class TinyVkGraphicsPipeline : public disposable {
		private:
			void CreateGraphicsPipeline() {
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				/////////// This section specifies that MvkVertex provides the vertex layout description ///////////
				const VkVertexInputBindingDescription bindingDescription = vertexDescription.binding;
				const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions = vertexDescription.attributes;

				VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
				vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputInfo.vertexBindingDescriptionCount = 1;
				vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
				vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
				vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				
				pipelineLayoutInfo.pushConstantRangeCount = 0;
				uint32_t pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
				if (pushConstantRangeCount > 0) {
					pipelineLayoutInfo.pushConstantRangeCount = pushConstantRangeCount;
					pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
				}

				if (descriptorBindings.size() > 0) {
					VkDescriptorSetLayoutCreateInfo descriptorCreateInfo{};
					descriptorCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					descriptorCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
					descriptorCreateInfo.bindingCount = static_cast<uint32_t>(descriptorBindings.size());
					descriptorCreateInfo.pBindings = descriptorBindings.data();

					if (vkCreateDescriptorSetLayout(renderDevice.logicalDevice, &descriptorCreateInfo, nullptr, &descriptorLayout) != VK_SUCCESS)
						throw std::runtime_error("TinyVulkan: Failed to create push descriptor bindings! ");

					pipelineLayoutInfo.setLayoutCount = 1;
					pipelineLayoutInfo.pSetLayouts = &descriptorLayout;
				}

				if (vkCreatePipelineLayout(renderDevice.logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to create graphics pipeline layout!");
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
				inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssembly.topology = vertexTopology;
				inputAssembly.primitiveRestartEnable = VK_FALSE;

				VkPipelineViewportStateCreateInfo viewportState{};
				viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportState.viewportCount = 1;
				viewportState.scissorCount = 1;
				viewportState.flags = 0;

				VkPipelineRasterizationStateCreateInfo rasterizer{};
				rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterizer.depthClampEnable = VK_FALSE;
				rasterizer.rasterizerDiscardEnable = VK_FALSE;
				rasterizer.polygonMode = polgyonTopology;
				rasterizer.lineWidth = 1.0f;
				rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
				rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
				rasterizer.depthBiasEnable = VK_FALSE;

				VkPipelineMultisampleStateCreateInfo multisampling{};
				multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampling.sampleShadingEnable = VK_FALSE;
				multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

				VkPipelineColorBlendStateCreateInfo colorBlending{};
				colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				colorBlending.logicOpEnable = VK_FALSE;
				colorBlending.logicOp = VK_LOGIC_OP_COPY;
				colorBlending.attachmentCount = 1;

				VkPipelineColorBlendAttachmentState blendDescription = colorBlendState;
				colorBlending.pAttachments = &blendDescription;
				colorBlending.blendConstants[0] = 0.0f;
				colorBlending.blendConstants[1] = 0.0f;
				colorBlending.blendConstants[2] = 0.0f;
				colorBlending.blendConstants[3] = 0.0f;

				const std::array<VkDynamicState, 2> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicState.flags = 0;
				dynamicState.pDynamicStates = dynamicStateEnables.data();
				dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
				dynamicState.pNext = nullptr;

				VkPipelineRenderingCreateInfoKHR renderingCreateInfo{};
				renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
				renderingCreateInfo.colorAttachmentCount = 1;
				renderingCreateInfo.pColorAttachmentFormats = &imageFormat;
				renderingCreateInfo.depthAttachmentFormat = QueryDepthFormat();

				VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
				depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				depthStencilInfo.depthTestEnable = enableDepthTesting;
				depthStencilInfo.depthWriteEnable = enableDepthTesting;
				depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
				depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
				depthStencilInfo.minDepthBounds = 0.0f; // Optional
				depthStencilInfo.maxDepthBounds = 1.0f; // Optional
				depthStencilInfo.stencilTestEnable = VK_FALSE;
				depthStencilInfo.front = {}; // Optional
				depthStencilInfo.back = {}; // Optional

				///////////////////////////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				VkGraphicsPipelineCreateInfo pipelineInfo{};
				pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.shaderCreateInfo.size());
				pipelineInfo.pStages = shaderStages.shaderCreateInfo.data();
				pipelineInfo.pVertexInputState = &vertexInputInfo;
				pipelineInfo.pInputAssemblyState = &inputAssembly;
				pipelineInfo.pViewportState = &viewportState;
				pipelineInfo.pRasterizationState = &rasterizer;
				pipelineInfo.pMultisampleState = &multisampling;
				pipelineInfo.pColorBlendState = &colorBlending;
				pipelineInfo.pDepthStencilState = &depthStencilInfo;

				pipelineInfo.pDynamicState = &dynamicState;
				pipelineInfo.pNext = &renderingCreateInfo;

				pipelineInfo.layout = pipelineLayout;
				pipelineInfo.renderPass = nullptr;
				pipelineInfo.subpass = 0;
				pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
				pipelineInfo.basePipelineIndex = -1; // Optional

				if (vkCreateGraphicsPipelines(renderDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to create graphics pipeline!");
			}
			
		public:
			TinyVkRenderDevice& renderDevice;
			TinyVkShaderStages& shaderStages;

			VkDescriptorSetLayout descriptorLayout;
			std::vector<VkDescriptorSetLayoutBinding> descriptorBindings;
			std::vector<VkPushConstantRange> pushConstantRanges;

			VkPipelineDynamicStateCreateInfo dynamicState;
			VkPipelineLayout pipelineLayout;
			VkPipeline graphicsPipeline;
			
			VkFormat imageFormat;
			VkColorComponentFlags colorComponentFlags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			VkPipelineColorBlendAttachmentState colorBlendState;

			TinyVkVertexDescription vertexDescription;
			VkPrimitiveTopology vertexTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			VkPolygonMode polgyonTopology = VK_POLYGON_MODE_FILL;
			
			bool enableBlending;
			bool enableDepthTesting;
			VkQueue graphicsQueue;
			VkQueue presentQueue;

			~TinyVkGraphicsPipeline() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice);

				vkDestroyDescriptorSetLayout(renderDevice.logicalDevice, descriptorLayout, nullptr);
				vkDestroyPipeline(renderDevice.logicalDevice, graphicsPipeline, nullptr);
				vkDestroyPipelineLayout(renderDevice.logicalDevice, pipelineLayout, nullptr);
			}

			TinyVkGraphicsPipeline(TinyVkRenderDevice& renderDevice, VkFormat imageFormat, TinyVkShaderStages& shaderStages, TinyVkVertexDescription vertexDescription, const std::vector<VkDescriptorSetLayoutBinding>& descriptorBindings, const std::vector<VkPushConstantRange>& pushConstantRanges, bool enableDepthTesting, VkPipelineColorBlendAttachmentState colorBlendState = GetBlendDescription(true), VkColorComponentFlags colorComponentFlags = VKCOMP_RGBA, VkPrimitiveTopology vertexTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkPolygonMode polgyonTopology = VK_POLYGON_MODE_FILL)
			: renderDevice(renderDevice), imageFormat(imageFormat), shaderStages(shaderStages), vertexDescription(vertexDescription), descriptorBindings(descriptorBindings), pushConstantRanges(pushConstantRanges), colorComponentFlags(colorComponentFlags), colorBlendState(colorBlendState), vertexTopology(vertexTopology), polgyonTopology(polgyonTopology) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));

				this->enableBlending = colorBlendState.blendEnable;
				this->enableDepthTesting = enableDepthTesting;

				TinyVkQueueFamily indices = TinyVkQueueFamily::FindQueueFamilies(renderDevice.physicalDevice, renderDevice.presentationSurface);
				vkGetDeviceQueue(renderDevice.logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
				
				if (renderDevice.presentationSurface != nullptr)
					vkGetDeviceQueue(renderDevice.logicalDevice, indices.presentFamily.value(), 0, &presentQueue);

				CreateGraphicsPipeline();
			}

			TinyVkGraphicsPipeline operator=(const TinyVkGraphicsPipeline& pipeline) = delete;

			/// <summary>Returns the optimal VkFormat for the desired depth image format.</summary>
			VkFormat QueryDepthFormat(VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL) {
				const std::vector<VkFormat>& candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
				for (VkFormat format : candidates) {
					VkFormatProperties props;
					vkGetPhysicalDeviceFormatProperties(renderDevice.physicalDevice, format, &props);

					VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
					if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
						return format;
					} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
						return format;
					}
				}

				throw std::runtime_error("TinyVulkan: Failed to find supported format!");
			}

			/// <summary>Returns true/false whether alpha blending is enabled on the graphics pipeline.</summary>
			bool BlendingIsEnabled() { return enableBlending; }

			/// <summary>Returns true/false whether depthj fragment testing is enabled on the graphics pipeline.</summary>
			bool DepthTestingIsEnabled() { return enableDepthTesting; }

			/// <summary>Gets a generic Normal Blending Mode for creating a GraphicsPipeline with.</summary>
			inline static const VkPipelineColorBlendAttachmentState GetBlendDescription(bool isBlendingEnabled = true) {
				VkPipelineColorBlendAttachmentState colorBlendAttachment{};
				colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				colorBlendAttachment.blendEnable = isBlendingEnabled;

				colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

				colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
				return colorBlendAttachment;
			}

			/// <summary>Returns the push constant range info of a given size applied to the given shader stages.</summary>
			inline static VkPushConstantRange SelectPushConstantRange(uint32_t pushConstantRangeSize = 0, VkShaderStageFlags shaderStages = VK_SHADER_STAGE_ALL_GRAPHICS) {
				VkPushConstantRange pushConstantRange{};
				pushConstantRange.stageFlags = shaderStages;
				pushConstantRange.offset = 0;
				pushConstantRange.size = pushConstantRangeSize;
				return pushConstantRange;
			}

			/// <summary>Creates a layout description for how a descriptor should be bound to the graphics pipeline at hwta binding and shader stages./summary>
			inline static VkDescriptorSetLayoutBinding SelectPushDescriptorLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1) {
				VkDescriptorSetLayoutBinding descriptorLayoutBinding {};
				descriptorLayoutBinding.binding = binding;
				descriptorLayoutBinding.descriptorCount = descriptorCount;
				descriptorLayoutBinding.descriptorType = descriptorType;
				descriptorLayoutBinding.pImmutableSamplers = nullptr;
				descriptorLayoutBinding.stageFlags = stageFlags;
				return descriptorLayoutBinding;
			}

			/// <summary>Creates a generic write descriptor to represent data passed to the GPU when rendering (on myrenderer.PushDescriptorSet).</summary>
			inline static VkWriteDescriptorSet SelectWriteDescriptor(uint32_t binding, uint32_t descriptorCount, VkDescriptorType descriptorType, const VkDescriptorImageInfo* imageInfo, const VkDescriptorBufferInfo* bufferInfo) {
				VkWriteDescriptorSet writeDescriptorSets{};
				writeDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets.dstSet = 0;
				writeDescriptorSets.dstBinding = binding;
				writeDescriptorSets.descriptorCount = descriptorCount;
				writeDescriptorSets.descriptorType = descriptorType;
				writeDescriptorSets.pImageInfo = imageInfo;
				writeDescriptorSets.pBufferInfo = bufferInfo;
				return writeDescriptorSets;
			}

			/// <summary>Creates a write image descriptor (Combined Image Sampler) for passing images to the GPU (on myrenderer.PushDescriptorSet).</summary>
			inline static VkWriteDescriptorSet SelectWriteImageDescriptor(uint32_t binding, uint32_t descriptorCount, const VkDescriptorImageInfo* imageInfo) {
				VkWriteDescriptorSet writeDescriptorSets{};
				writeDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets.dstSet = 0;
				writeDescriptorSets.dstBinding = binding;
				writeDescriptorSets.descriptorCount = descriptorCount;
				writeDescriptorSets.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets.pImageInfo = imageInfo;
				return writeDescriptorSets;
			}

			/// <summary>Creates a write buffer descriptor (any of VK_DESCRIPTOR_TYPE_*_BUFFER) for passing buffers to the GPU (on myrenderer.PushDescriptorSet).</summary>
			inline static VkWriteDescriptorSet SelectWriteBufferDescriptor(uint32_t binding, uint32_t descriptorCount, const VkDescriptorBufferInfo* bufferInfo) {
				VkWriteDescriptorSet writeDescriptorSets{};
				writeDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets.dstSet = 0;
				writeDescriptorSets.dstBinding = binding;
				writeDescriptorSets.descriptorCount = descriptorCount;
				writeDescriptorSets.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets.pBufferInfo = bufferInfo;
				return writeDescriptorSets;
			}
		
			/// <summary>Creates a layout description for how a descriptor should be bound to the graphics pipeline at hwta binding and shader stages./summary>
			inline static VkDescriptorSetLayoutBinding SelectPushDescriptorLayoutBinding(uint32_t binding, TinyVkDescriptorTypes descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1) {
				VkDescriptorSetLayoutBinding descriptorLayoutBinding {};
				descriptorLayoutBinding.binding = binding;
				descriptorLayoutBinding.descriptorCount = descriptorCount;
				descriptorLayoutBinding.descriptorType = static_cast<VkDescriptorType>(descriptorType);
				descriptorLayoutBinding.pImmutableSamplers = nullptr;
				descriptorLayoutBinding.stageFlags = stageFlags;
				return descriptorLayoutBinding;
			}

			/// <summary>Creates a generic write descriptor to represent data passed to the GPU when rendering (on myrenderer.PushDescriptorSet).</summary>
			inline static VkWriteDescriptorSet SelectWriteDescriptor(uint32_t binding, uint32_t descriptorCount, TinyVkDescriptorTypes descriptorType, const VkDescriptorImageInfo* imageInfo, const VkDescriptorBufferInfo* bufferInfo) {
				VkWriteDescriptorSet writeDescriptorSets{};
				writeDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets.dstSet = 0;
				writeDescriptorSets.dstBinding = binding;
				writeDescriptorSets.descriptorCount = descriptorCount;
				writeDescriptorSets.descriptorType = static_cast<VkDescriptorType>(descriptorType);
				writeDescriptorSets.pImageInfo = imageInfo;
				writeDescriptorSets.pBufferInfo = bufferInfo;
				return writeDescriptorSets;
			}
		};
	}
#endif