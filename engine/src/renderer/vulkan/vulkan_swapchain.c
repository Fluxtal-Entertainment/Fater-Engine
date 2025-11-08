#include "vulkan_swapchain.h"
#include "core/logger.h"
#include "core/f_memory.h"
#include "vulkan_device.h"

void create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain);
void destroy(vulkan_context* context, vulkan_swapchain* swapchain);

void vulkan_swapchain_create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* out_swapchain)
{
    //Simply create a new one
    create(context, width, height, out_swapchain);
}

void vulkan_swapchain_recreate(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain)
{
    //Destroys old one and creates a new one
    destroy(context, swapchain);
    create(context, width, height, swapchain);
}

void vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain)
{
    destroy(context, swapchain);
}

b8 vulkan_swapchain_aquire_next_image_index(vulkan_context* context, vulkan_swapchain* swapchain, u64 timeout_ns, VkSemaphore image_available_semaphore, VkFence fence, u32* out_image_index)
{
    VkResult result = vkAcquireNextImageKHR(context->device.logical_device, swapchain->handle, timeout_ns, image_available_semaphore, fence, out_image_index);

    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        //Trigger swapchain recreation, then boot out of the render loop
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
        return false;
    }
    else if(result != VK_SUCCESS && result !+ VK_SUBOPTIMAL_KHR)
    {
        FATAL_LOG("Failed to acquire swapchain image!!!");
        return false;
    }

    return true;
}

void vulkan_swapchain_present(vulkan_context* context, vulkan_swapchain* swapchain, VkQueue graphics_queue, VkQueue present_queue, VkSemaphore render_complete_semaphore, u32 present_image_index)
{
    //Return image to swapchain for presentation
    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_complete_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = &present_image_index;
    present_info.pResults = 0;

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        //Swapchain is out of date, suboptimal or a framebuffer resize has occured. Trigger swapchain recreation
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
    }
    else if(result != VK_SUCCESS)
    {
        FATAL_LOG("Failed to present swapchain image!!!");
    }
    
}