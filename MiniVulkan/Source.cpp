#define MINIVULKAN_SHORTREF
#define QOI_IMPLEMENTATION
#include "./MiniVK.hpp"
#include "./QuiteOkImageFormat.h"
using namespace mvk;

#define DEFAULT_VERTEX_SHADER "./sample_vert.spv"
#define DEFAULT_FRAGMENT_SHADER "./sample_frag.spv"

int MINIVULKAN_WINDOWMAIN {
    try {
        #pragma region INITIALIZATION_VARIABLES

        const std::vector<VkPhysicalDeviceType> renderDeviceTypes = { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU };
        const MiniVkBufferingMode bufferingMode = MiniVkBufferingMode::TRIPLE;
        const std::tuple<VkShaderStageFlagBits, std::string> vertexShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, DEFAULT_VERTEX_SHADER };
        const std::tuple<VkShaderStageFlagBits, std::string> fragmentShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, DEFAULT_FRAGMENT_SHADER };
        const MiniVkVertexDescription vertexDescription = MiniVkVertex::GetVertexDescription();
        const std::vector<VkDescriptorSetLayoutBinding>& descriptorBindings = { MiniVkDynamicPipeline::SelectPushDescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT) };
        const std::vector<VkPushConstantRange>& pushConstantRanges = { MiniVkDynamicPipeline::SelectPushConstantRange(sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT) };
        
        #pragma endregion
        #pragma region MINIVULKAN_INITIALIZATION

        MiniVkWindow window("MINIVK WINDOW", 1920, 1080, true);
        MiniVkWindowInputEvents inputs(window);

        MiniVkInstance instance(MiniVkWindow::QueryRequiredExtensions(MVK_VALIDATION_LAYERS), "MINIVK");
        MiniVkRenderDevice renderDevice(instance, window.CreateWindowSurface(instance.instance), renderDeviceTypes);
        MiniVkVMAllocator vmAlloc(instance, renderDevice);

        MiniVkSwapChain swapChain(renderDevice, window, MiniVkSurfaceSupporter(), bufferingMode);
        MiniVkCommandPool cmdSwapPool(renderDevice, static_cast<size_t>(bufferingMode));
        MiniVkShaderStages shaders(renderDevice, { vertexShader, fragmentShader });

        MiniVkDynamicPipeline dySwapChainPipe(renderDevice, swapChain.imageFormat, shaders, vertexDescription, descriptorBindings, pushConstantRanges, true, true, VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);
        MiniVkSwapChainRenderer dyRender(renderDevice, vmAlloc, cmdSwapPool, swapChain, dySwapChainPipe);

        MiniVkDynamicPipeline dyImagePipe(renderDevice, swapChain.imageFormat, shaders, vertexDescription, descriptorBindings, pushConstantRanges, true, true, VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);
        MiniVkCommandPool cmdRenderPool(renderDevice, static_cast<size_t>(bufferingMode));
        MiniVkCmdPoolQueue cmdRenderQueue(cmdRenderPool);
        MiniVkImage renderSurface(renderDevice, dySwapChainPipe, cmdRenderPool, vmAlloc, window.GetWidth(), window.GetHeight(), false, VK_FORMAT_B8G8R8A8_SRGB, MINIVK_SHADER_READONLY_OPTIMAL);
        MiniVkImageRenderer imageRenderer(renderDevice, cmdRenderQueue, vmAlloc, &renderSurface, dyImagePipe);
        
        #pragma endregion
        #pragma region IMAGE_QUAD_LOADING_COPY_TO_GPU
        std::vector<MiniVkVertex> quad1 = MiniVkQuad::CreateWithOffset({480.0,270.0}, {960.0,540.0,0.0}, {1.0,1.0,1.0,0.75});
        std::vector<MiniVkVertex> quad2 = MiniVkQuad::CreateWithOffset({128.0,128.0}, {960.0,540.0,0.0}, {1.0,1.0,1.0,0.75});

        //constexpr glm::float32 angle = 45.0f * (glm::pi<glm::float32>() / 180.0f);
        //constexpr glm::float32 scale = 1.0;
        //glm::vec3 origin = quad2[0].position;
        //MiniVkQuad::RotateScaleFromOrigin(quad2, origin, -angle * 0.5f, scale);
        //
        //std::vector<MiniVkVertex> triangle;
        //triangle.insert(triangle.end(), quad2.begin(), quad2.end());

        std::vector<uint32_t> indices = MiniVkPolygon::TriangulatePointList(quad2);

        MiniVkBuffer vbuffer(renderDevice, vmAlloc, quad2.size() * sizeof(MiniVkVertex), MiniVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
        vbuffer.StageBufferData(dyImagePipe.graphicsQueue, cmdRenderPool.GetPool(), quad2.data(), quad2.size() * sizeof(MiniVkVertex), 0, 0);
        MiniVkBuffer ibuffer(renderDevice, vmAlloc, indices.size() * sizeof(indices[0]), MiniVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
        ibuffer.StageBufferData(dyImagePipe.graphicsQueue, cmdRenderPool.GetPool(), indices.data(), indices.size() * sizeof(indices[0]), 0, 0);
        
        FILE* test = fopen("./Screeny.qoi", "rb");
        if (test == NULL) { std::cout << "NO QOI IMAGE" << std::endl; } else { fclose(test); }
        qoi_desc qoidesc;
        void* qoiPixels = qoi_read("./Screeny.qoi", &qoidesc, 4);
        VkDeviceSize dataSize = qoidesc.width * qoidesc.height * qoidesc.channels;
        MiniVkImage image = MiniVkImage(renderDevice, dyImagePipe, cmdRenderPool, vmAlloc, qoidesc.width, qoidesc.height, false, VK_FORMAT_R8G8B8A8_SRGB, MINIVK_SHADER_READONLY_OPTIMAL);
        image.StageImageData(qoiPixels, dataSize);
        QOI_FREE(qoiPixels);
        #pragma endregion
        #pragma region IMAGE_RENDERER_TESTING

        VkClearValue clearColor{ .color = { 1.0, 0.0, 0.0, 1.0 } };
        VkClearValue depthStencil{ .depthStencil = { 1.0f, 0 } };

        int32_t rentBufferIndex;
        VkCommandBuffer renderTargetBuffer = cmdRenderQueue.RentBuffer(rentBufferIndex);
        imageRenderer.BeginRecordCmdBuffer(renderTargetBuffer, VkExtent2D {.width = (uint32_t)renderSurface.width, .height = (uint32_t)renderSurface.height}, clearColor, depthStencil);
            
            glm::mat4 projection = MiniVkMath::Project2D(window.GetWidth(), window.GetHeight(), 0.0, 0.0, -1.0, 1.0);
            imageRenderer.PushConstants(renderTargetBuffer, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), &projection);
            
            auto image_descriptor = image.GetImageDescriptor();
            VkWriteDescriptorSet writeDescriptorSets = MiniVkDynamicPipeline::SelectWriteImageDescriptor(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_descriptor);
            imageRenderer.PushDescriptorSet(renderTargetBuffer, { writeDescriptorSets });

            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(renderTargetBuffer, 0, 1, &vbuffer.buffer, offsets);
            vkCmdBindIndexBuffer(renderTargetBuffer, ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(renderTargetBuffer, static_cast<uint32_t>(ibuffer.size) / sizeof(uint32_t), 1, 0, 0, 0);
            
        imageRenderer.EndRecordCmdBuffer(renderTargetBuffer, VkExtent2D{ .width = (uint32_t)renderSurface.width, .height = (uint32_t)renderSurface.height }, clearColor, depthStencil);
        imageRenderer.RenderExecute(renderTargetBuffer);
        
        renderDevice.WaitIdle();
        cmdRenderQueue.ReturnBuffer(rentBufferIndex);
        
        #pragma endregion
        #pragma region COPY_RENDERED_IMAGE_TO_GPU

        std::vector<MiniVkVertex> sw_triangles = MiniVkQuad::Create(glm::vec3(1920.0, 1080.0, -0.5));
        std::vector<uint32_t> sw_indices = { 0, 1, 2, 2, 3, 0 };
        MiniVkBuffer sw_vbuffer(renderDevice, vmAlloc, sw_triangles.size() * sizeof(MiniVkVertex), MiniVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
        sw_vbuffer.StageBufferData(dyImagePipe.graphicsQueue, cmdSwapPool.GetPool(), sw_triangles.data(), sw_triangles.size() * sizeof(MiniVkVertex), 0, 0);
        MiniVkBuffer sw_ibuffer(renderDevice, vmAlloc, sw_indices.size() * sizeof(sw_indices[0]), MiniVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
        sw_ibuffer.StageBufferData(dyImagePipe.graphicsQueue, cmdSwapPool.GetPool(), sw_indices.data(), sw_indices.size() * sizeof(sw_indices[0]), 0, 0);

        #pragma endregion
        #pragma region SWAPCHAIN_RENDERER_TESTING

        size_t frame = 0;
        bool swap = false;
        dyRender.onRenderEvents.hook(callback<VkCommandBuffer>([&swap, &frame, &instance, &window, &swapChain, &dyRender, &dySwapChainPipe, &renderSurface, &sw_ibuffer, &sw_vbuffer](VkCommandBuffer commandBuffer) {
            VkClearValue clearColor{ .color = { 0.25, 0.25, 0.25, 1.0 } };
            VkClearValue depthStencil{ .depthStencil = { 1.0f, 0 } };
        
            dyRender.BeginRecordCmdBuffer(commandBuffer, swapChain.imageExtent, clearColor, depthStencil);
            
            glm::mat4 projection = MiniVkMath::Project2D(window.GetWidth(), window.GetHeight(), 120.0F * (double) swap, 0.0, -1.0, 1.0);
            dyRender.PushConstants(commandBuffer, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), &projection);
            
            auto image_descriptor = renderSurface.GetImageDescriptor();
            VkWriteDescriptorSet writeDescriptorSets = MiniVkDynamicPipeline::SelectWriteImageDescriptor(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_descriptor);
            dyRender.PushDescriptorSet(commandBuffer, { writeDescriptorSets });
            
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sw_vbuffer.buffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, sw_ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sw_ibuffer.size) / sizeof(uint32_t), 1, 0, 0, 0);
            
            dyRender.EndRecordCmdBuffer(commandBuffer, swapChain.imageExtent, clearColor, depthStencil);

            frame ++;

            if (frame > 120) {
                frame = 0;
                swap = (swap == true)? false : true;
            }
        }));
        
        #pragma endregion
        #pragma region EXECUTE_MAIN_CLEANUP
        
        std::thread mythread([&window, &dyRender]() { while (!window.ShouldClose()) { dyRender.RenderExecute(); } });
        window.WhileMain();
        mythread.join();
        
        disposable::DisposeOrdered({ &instance, &window, &renderDevice, &vmAlloc, &swapChain, &cmdSwapPool, &shaders, &dySwapChainPipe, &dyRender,
            &dyImagePipe, &cmdRenderPool, &cmdRenderQueue, &renderSurface, &imageRenderer, &vbuffer, &ibuffer, &image, &sw_vbuffer, &sw_ibuffer }, true);
        
        #pragma endregion
    } catch (std::runtime_error err) { std::cout << err.what() << std::endl; }
    return VK_SUCCESS;
}

/// TODO LIST:
///     1. Abstract Class for Loading QOI/PNG/BMP Images from memory and staging them as MiniVkImages.
///     2. Make MiniVkPolygon a non-static class that can hold 2 vectors: one for vertices, one for indices.
///     3. Create Draw Call functions which auto-sort by depth/z-component.
///     4. 