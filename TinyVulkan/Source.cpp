#define TINYVK_ALLOWS_POLLING_GAMEPADS
#define TINYVK_AUTO_PRESENT_EXTENSIONS
#include "./TinyVK.hpp"
using namespace tinyvk;

#define QOI_IMPLEMENTATION
#include "./images_qoi.h"

#define DEFAULT_VERTEX_SHADER "./sample_vert.spv"
#define DEFAULT_FRAGMENT_SHADER "./sample_frag.spv"
#define DEFAULT_COMMAND_POOLSIZE 10

const std::vector<VkPhysicalDeviceType> rdeviceTypes = { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU };
const TinyVkBufferingMode bufferingMode = TinyVkBufferingMode::TRIPLE;
const std::tuple<VkShaderStageFlagBits, std::string> vertexShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, DEFAULT_VERTEX_SHADER };
const std::tuple<VkShaderStageFlagBits, std::string> fragmentShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, DEFAULT_FRAGMENT_SHADER };

const TinyVkVertexDescription vertexDescription = TinyVkVertex::GetVertexDescription();
const std::vector<VkDescriptorSetLayoutBinding>& descriptorBindings = { TinyVkDynamicPipeline::SelectPushDescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT) };
const std::vector<VkPushConstantRange>& pushConstantRanges = { TinyVkDynamicPipeline::SelectPushConstantRange(sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT) };

void* image_get(std::string fpath, qoi_desc& qoidesc) {
    FILE* test = fopen(fpath.c_str(), "rb");
    if (test == NULL) throw std::runtime_error("TinyVulkan: Cannot load QOI image! " + fpath);
    return qoi_read("./Screeny.qoi", &qoidesc, 4);
}
void image_free(void* imgPixels) { QOI_FREE(imgPixels); }

int32_t TINYVULKAN_WINDOWMAIN {
	TinyVkWindow window("TINYVK WINDOW", 1920, 1080, true);
	TinyVkInstance instance({}, "TINYVK");
	TinyVkRenderDevice rdevice(instance, window.CreateWindowSurface(instance.GetInstance()), rdeviceTypes);
	TinyVkVMAllocator vmAlloc(instance, rdevice);
	TinyVkCommandPool commandPool(rdevice, static_cast<size_t>(bufferingMode) + DEFAULT_COMMAND_POOLSIZE);
	TinyVkSwapChain swapChain(rdevice, window, bufferingMode /* 3 default optional args used*/);
	TinyVkShaderStages shaders(rdevice, { vertexShader, fragmentShader });
	TinyVkDynamicPipeline renderPipe(rdevice, swapChain.imageFormat, shaders, vertexDescription, descriptorBindings, pushConstantRanges, true, true, VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);
	TinyVkSwapChainRenderer swapRenderer(rdevice, vmAlloc, commandPool, swapChain, renderPipe);
	
    // You can have multiple graphics pipelines if your pipelines are different:
    //TinyVkDynamicPipeline imagePipe(rdevice, swapChain.imageFormat, shaders, vertexDescription, descriptorBindings, pushConstantRanges, true, true, VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);
    TinyVkImage rsurface(rdevice, renderPipe, commandPool, vmAlloc, window.GetWidth(), window.GetHeight(), false, VK_FORMAT_B8G8R8A8_SRGB, TINYVK_SHADER_READONLY_OPTIMAL);
	TinyVkImageRenderer imageRenderer(rdevice, commandPool, vmAlloc, &rsurface, renderPipe);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /*
        Create the render background properties: Clear Color and Clear Depth.
        Create an image quad (array/vector of vertices) of arbitrary size (whatever you want)
        and triangulate the quad vertices as an array of vertex mapped indices for rendering.
            * You don't have to use indices by manually defining your own triangulated quad
            * negating the use of index buffers entirely if necessary.
        Load an image in QOI format from disc into CPU memory using TinyVkImage.

        Create vertex/index buffers for the image quad data in GPU memory (TinyVkBuffer).
        Stage the QOI image, Vertex/Index buffers into GPU memory to read for rendering.
        Finally free out the QOI image that was CPU memory allocated as it is now staged to GPU.
    */

    VkClearValue clearColor{ .color = { 1.0, 0.0, 0.0, 1.0 } };
    VkClearValue depthStencil{ .depthStencil = { 1.0f, 0 } };

	std::vector<TinyVkVertex> quad2 = TinyVkQuad::CreateWithOffset({ 128.0,128.0 }, { 960.0,540.0,0.0 }, { 1.0,1.0,1.0,0.75 });
    std::vector<uint32_t> indices = TinyVkPolygon::TriangulatePointList(quad2);

    TinyVkBuffer vbuffer(rdevice, vmAlloc, quad2.size() * sizeof(TinyVkVertex), TinyVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
    vbuffer.StageBufferData(renderPipe.graphicsQueue, commandPool.GetPool(), quad2.data(), quad2.size() * sizeof(TinyVkVertex), 0, 0);
    TinyVkBuffer ibuffer(rdevice, vmAlloc, indices.size() * sizeof(indices[0]), TinyVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
    ibuffer.StageBufferData(renderPipe.graphicsQueue, commandPool.GetPool(), indices.data(), indices.size() * sizeof(indices[0]), 0, 0);

    qoi_desc qoidesc;
    void* qoiPixels = image_get("./Screeny.qoi", qoidesc);
    TinyVkImage image = TinyVkImage(rdevice, renderPipe, commandPool, vmAlloc, qoidesc.width, qoidesc.height, false, VK_FORMAT_R8G8B8A8_SRGB, TINYVK_SHADER_READONLY_OPTIMAL);
    image.StageImageData(qoiPixels, qoidesc.width* qoidesc.height* qoidesc.channels);
    image_free(qoiPixels);
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /*
        Using the Render-To-Image renderer (VkImageRenderer), render a QOI image onto a VkImage with the
        VkImageRenderer's default built-in command buffer and onRenderEvent (callback<VkCommandBuffer>).
    */

    imageRenderer.onRenderEvent.hook(callback<VkCommandBuffer>([&window, &imageRenderer, &rsurface, &clearColor, &depthStencil, &image, &vbuffer, &ibuffer](VkCommandBuffer renderBuffer){
        imageRenderer.BeginRecordCmdBuffer(VkExtent2D{ .width = (uint32_t)rsurface.width, .height = (uint32_t)rsurface.height }, clearColor, depthStencil, renderBuffer);

        glm::mat4 projection = TinyVkMath::Project2D(window.GetWidth(), window.GetHeight(), 0.0, 0.0, -1.0, 1.0);
        imageRenderer.PushConstants(renderBuffer, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), &projection);

        auto image_descriptor = image.GetImageDescriptor();
        VkWriteDescriptorSet writeDescriptorSets = TinyVkDynamicPipeline::SelectWriteImageDescriptor(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_descriptor);
        imageRenderer.PushDescriptorSet(renderBuffer, { writeDescriptorSets });

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(renderBuffer, 0, 1, &vbuffer.buffer, offsets);
        vkCmdBindIndexBuffer(renderBuffer, ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(renderBuffer, static_cast<uint32_t>(ibuffer.size) / sizeof(uint32_t), 1, 0, 0, 0);

        imageRenderer.EndRecordCmdBuffer(VkExtent2D{ .width = (uint32_t)rsurface.width, .height = (uint32_t)rsurface.height }, clearColor, depthStencil, renderBuffer);
    }));
    imageRenderer.RenderExecute();
    
    vbuffer.Dispose();
    ibuffer.Dispose();
    image.Dispose();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /*
        Create an image quad from the previous rendered image for rendering to the swap chain (window).
        Render that image onto the swap chain (window) with a camera projection offset based on the current frame.
        Use the swap chain renderer's built-in onRender event and command buffers for rendering.
    */

    std::vector<TinyVkVertex> sw_triangles = TinyVkQuad::Create(glm::vec3(1920.0, 1080.0, -0.5));
    std::vector<uint32_t> sw_indices = { 0, 1, 2, 2, 3, 0 };
    TinyVkBuffer sw_vbuffer(rdevice, vmAlloc, sw_triangles.size() * sizeof(TinyVkVertex), TinyVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
    sw_vbuffer.StageBufferData(renderPipe.graphicsQueue, commandPool.GetPool(), sw_triangles.data(), sw_triangles.size() * sizeof(TinyVkVertex), 0, 0);
    TinyVkBuffer sw_ibuffer(rdevice, vmAlloc, sw_indices.size() * sizeof(sw_indices[0]), TinyVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
    sw_ibuffer.StageBufferData(renderPipe.graphicsQueue, commandPool.GetPool(), sw_indices.data(), sw_indices.size() * sizeof(sw_indices[0]), 0, 0);

    size_t frame = 0;
    bool swap = false;
    swapRenderer.onRenderEvents.hook(callback<VkCommandBuffer>([&swap, &frame, &instance, &window, &swapChain, &swapRenderer, &renderPipe, &rsurface, &sw_ibuffer, &sw_vbuffer](VkCommandBuffer commandBuffer) {
        VkClearValue clearColor{ .color = { 0.25, 0.25, 0.25, 1.0 } };
        VkClearValue depthStencil{ .depthStencil = { 1.0f, 0 } };

        swapRenderer.BeginRecordCmdBuffer(commandBuffer, swapChain.imageExtent, clearColor, depthStencil);

        glm::mat4 projection = TinyVkMath::Project2D(window.GetWidth(), window.GetHeight(), 120.0F * (double)swap, 0.0, -1.0, 1.0);
        swapRenderer.PushConstants(commandBuffer, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), &projection);

        auto image_descriptor = rsurface.GetImageDescriptor();
        VkWriteDescriptorSet writeDescriptorSets = TinyVkDynamicPipeline::SelectWriteImageDescriptor(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_descriptor);
        swapRenderer.PushDescriptorSet(commandBuffer, { writeDescriptorSets });

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sw_vbuffer.buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, sw_ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sw_ibuffer.size) / sizeof(uint32_t), 1, 0, 0, 0);

        swapRenderer.EndRecordCmdBuffer(commandBuffer, swapChain.imageExtent, clearColor, depthStencil);

        if (frame++ > 120) {
            frame = 0;
            swap = (swap == true) ? false : true;
        }
    }));

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /*
        Finally execute the swap chain (per-frame) render events on a secondary thread to avoid
        being blocked by the GLFW main thread--allows resizing, avoids handing on window move, etc.
        Then clean up the render thread and TinyVk remaining allocated resources.
    */

    std::thread mythread([&window, &swapRenderer]() { while (!window.ShouldClose()) { swapRenderer.RenderExecute(); } });
    window.WhileMain();
    mythread.join();

	disposable::DisposeOrdered({ &window, &instance, &rdevice, &vmAlloc, &commandPool, &swapChain, &shaders, &renderPipe, &swapRenderer, &rsurface, /*&imagePipe,*/ &imageRenderer, &sw_vbuffer, &sw_ibuffer}, true);
	return VK_SUCCESS;
};