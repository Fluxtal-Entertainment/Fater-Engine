#include "vulkan_device.h"
#include "core/logger.h"
#include "core/f_string.h"
#include "core/f_memory.h"
#include "containers/dynamic_array.h"

typedef struct vulkan_physical_device_requirements
{
    b8 graphics;
    b8 present;
    b8 compute;
    b8 transfer;
    //dynamic_array
    const char** device_extension_names;
    b8 sampler_anisotropy;
    b8 discrete_gpu;
}vulkan_physical_device_requirements;

typedef struct vulkan_physical_device_queue_family_info
{
    u32 graphics_family_index;
    u32 present_family_index;
    u32 compute_family_index;
    u32 transfer_family_index;
}vulkan_physical_device_queue_family_info;

b8 select_physical_device(vulkan_context* context);

b8 physical_device_meets_requirements(VkPhysicalDevice device, VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties, const VkPhysicalDeviceFeatures* features,
    const vulkan_physical_device_requirements* requirements, vulkan_physical_device_queue_family_info* out_queue_info,
    vulkan_swapchain_support_info* out_swapchain_support);

b8 vulkan_device_create(vulkan_context* context)
{
    if(!select_physical_device(context))
    {
        return false;
    }

    INFO_LOG("Creating logical device...");
    //NOTE: Skip creating additional queues for shared indices
    b8 present_shares_graphics_queue = context->device.graphics_queue_index == context->device.present_queue_index;
    b8 transfer_shares_graphics_queue = context->device.graphics_queue_index == context->device.transfer_queue_index;
    u32 index_count = 1;
    if(!present_shares_graphics_queue)
    {
        index_count++;
    }
    if(!transfer_shares_graphics_queue)
    {
        index_count++;
    }
    u32 indices[index_count];
    u8 index = 0;
    indices[index++] = context->device.graphics_queue_index;
    if(!present_shares_graphics_queue)
    {
        indices[index++] = context->device.present_queue_index;
    }
    if(!transfer_shares_graphics_queue)
    {
        indices[index++] = context->device.transfer_queue_index;
    }

    VkDeviceQueueCreateInfo queue_create_infos[index_count];
    for(u32 i = 0; i < index_count; ++i)
    {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = indices[i];
        queue_create_infos[i].queueCount = 1;
        if(indices[i] == context->device.graphics_queue_index)
        {
            queue_create_infos[i].queueCount = 2;
        }
        queue_create_infos[i].flags = 0;
        queue_create_infos[i].pNext = 0;
        f32 queue_priority = 1.0f;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
    }

    //Request device features (TODO: should be config driven)
    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE; //Request anisotropy

    VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = 1;
    const char* extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    device_create_info.ppEnabledExtensionNames = &extension_names;

    //Deprecated and ignored. Passing nothing just for sure 
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;

    //Creating the device
    VK_CHECK(vkCreateDevice(context->device.physical_device, &device_create_info, context->allocator, &context->device.logical_device));
    INFO_LOG("Logical device created");
    
    //Get queues
    vkGetDeviceQueue(context->device.logical_device, context->device.graphics_queue_index, 0, &context->device.graphics_queue);
    vkGetDeviceQueue(context->device.logical_device, context->device.present_queue_index, 0, &context->device.present_queue);
    vkGetDeviceQueue(context->device.logical_device, context->device.transfer_queue_index, 0, &context->device.transfer_queue);
    INFO_LOG("Queues obtained");

    return true;
}

void vulkan_device_destroy(vulkan_context* context)
{
    //Unset queues
    context->device.graphics_queue = 0;
    context->device.present_queue = 0;
    context->device.transfer_queue = 0;

    //Destroy logical device
    INFO_LOG("Destroying logical device...");
    if(context->device.logical_device)
    {
        vkDestroyDevice(context->device.logical_device, context->allocator);
        context->device.logical_device = 0;
    }

    //Releasing Phy. Dev. Resources because Phy. Dev cannot be destroyed
    INFO_LOG("Releasing physical device resources...");
    context->device.physical_device = 0;

    if(context->device.swapchain_support.formats)
    {
        mem_free(context->device.swapchain_support.formats, sizeof(VkSurfaceFormatKHR) * context->device.swapchain_support.format_count, MEMORY_TAG_RENDERER);
        context->device.swapchain_support.formats = 0;
        context->device.swapchain_support.format_count = 0;
    }

    if(context->device.swapchain_support.present_modes)
    {
        mem_free(context->device.swapchain_support.present_modes, sizeof(VkPresentModeKHR) * context->device.swapchain_support.present_mode_count, MEMORY_TAG_RENDERER);
        context->device.swapchain_support.present_modes = 0;
        context->device.swapchain_support.present_mode_count = 0;
    }

    mem_zero(&context->device.swapchain_support.capabilities, sizeof(context->device.swapchain_support.capabilities));
    context->device.graphics_queue_index = -1;
    context->device.present_queue_index = -1;
    context->device.transfer_queue_index = -1;
}

void vulkan_device_query_swapchain_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface, vulkan_swapchain_support_info* out_support_info)
{
    //Surface capabilities
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &out_support_info->capabilities));

    //Surface formats
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, 0));

    if(out_support_info->format_count != 0)
    {
        if(!out_support_info->formats)
        {
            out_support_info->formats = mem_allocate(sizeof(VkSurfaceFormatKHR) * out_support_info->format_count, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, out_support_info->formats));
    }

    //present modes
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count, 0));
    if(out_support_info->present_mode_count != 0)
    {
        if(!out_support_info->present_modes)
        {
            out_support_info->present_modes = mem_allocate(sizeof(VkPresentModeKHR) * out_support_info->present_mode_count, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count, out_support_info->present_modes));
    }
}

b8 select_physical_device(vulkan_context* context)
{
    u32 physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));
    if(physical_device_count == 0)
    {
        FATAL_LOG("No Vulkan supporting devices were found!!!");
        return false;
    }

    VkPhysicalDevice physical_devices[physical_device_count];
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices));
    for(u32 i = 0; i < physical_device_count; ++i)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

        //NOTE: These requirements should probably be driven by engine configuration
        vulkan_physical_device_requirements requirements = {};
        requirements.graphics = true;
        requirements.present = true;
        requirements.transfer = true;
        //NOTE:Enable this if compute will be required
        //requirements.compute = true;
        requirements.sampler_anisotropy = true;
        requirements.discrete_gpu = true;
        requirements.device_extension_names = dynamic_array_create(const char*);
        dynamic_array_push(requirements.device_extension_names, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        vulkan_physical_device_queue_family_info queue_info = {};
        b8 result = physical_device_meets_requirements(physical_devices[i], context->surface, &properties, &features, &requirements, &queue_info, &context->device.swapchain_support);

        if(result)
        {
            INFO_LOG("Selected device: '%s'", properties.deviceName);
            //GPU type, etc.
            switch (properties.deviceType)
            {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    INFO_LOG("Unknown type of GPU");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    INFO_LOG("Integrated type GPU");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    INFO_LOG("Discrete type GPU");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    INFO_LOG("Virtual type GPU");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    INFO_LOG("CPU type GPU");
                    break;
            }

            //Logs installed driver version
            INFO_LOG("GPU Driver Version: %d.%d.%d", VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion), VK_VERSION_PATCH(properties.driverVersion));
            //Logs installed Vulkan API version
            INFO_LOG("Vulkan API Version: %d.%d.%d", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion));

            //Memory information
            for(u32 j = 0; j < memory.memoryHeapCount; ++j)
            {
                f32 memory_size_gib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if(memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    INFO_LOG("Local GPU Memory: %.2f GiB", memory_size_gib);
                }
                else
                {
                    INFO_LOG("Shared System Memory: %.2f GiB", memory_size_gib);
                }
            }

            context->device.physical_device = physical_devices[i];
            context->device.graphics_queue_index = queue_info.graphics_family_index;
            context->device.present_queue_index = queue_info.present_family_index;
            context->device.transfer_queue_index = queue_info.transfer_family_index;
            //NOTE: Set compute index here if needed

            //Keep a copy of properties, features and memory info for later use
            context->device.properties = properties;
            context->device.features = features;
            context->device.memory = memory;

            break;
        }
    }

    //Ensure a device was selected
    if(!context->device.physical_device)
    {
        ERROR_LOG("No physical device were found witch meet the requirements!!!")
        return false;
    }

    INFO_LOG("Physical device has been selected");
    return true;
}

b8 physical_device_meets_requirements(VkPhysicalDevice device, VkSurfaceKHR surface, const VkPhysicalDeviceProperties* properties, const VkPhysicalDeviceFeatures* features, const vulkan_physical_device_requirements* requirements, vulkan_physical_device_queue_family_info* out_queue_info, vulkan_swapchain_support_info* out_swapchain_support)
{
    //Evaluate device properties to determine if it meets the needs of our application
    out_queue_info->graphics_family_index = -1;
    out_queue_info->present_family_index = -1;
    out_queue_info->compute_family_index = -1;
    out_queue_info->transfer_family_index = -1;

    //Is Descrete GPU?
    if(requirements->discrete_gpu)
    {
        if(properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            INFO_LOG("Device is not a discrete GPU and one is required. Skipping...");
            return false; 
        }
    }

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    //Look at each queue and see what types of queue it supports
    INFO_LOG("Graphics | Present  | Compute  | Transfer  | Name");
    u8 min_transfer_score = 255;
    for(u32 i = 0; i < queue_family_count; ++i)
    {
        u8 current_transfer_score = 0;

        //graphics queue?
        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            out_queue_info->graphics_family_index = i;
            ++current_transfer_score;
        }

        //compute queue?
        if(queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            out_queue_info->compute_family_index = i;
            ++current_transfer_score;
        }

        //transfer queue?
        if(queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            //Take the index if it is the current lowest. This increases the liklihood that it it a dedicated trandfer queue
            if(current_transfer_score <= min_transfer_score)
            {
                min_transfer_score = current_transfer_score;
                out_queue_info->transfer_family_index = i;
            }
        }

        //present queue?
        VkBool32 supports_present = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
        if(supports_present)
        {
            out_queue_info->present_family_index = i;
        }
    }

    //Prints out info about the device
    INFO_LOG("       %d |        %d |        %d |         %d | %s",
        out_queue_info->graphics_family_index != -1,
        out_queue_info->present_family_index != -1,
        out_queue_info->compute_family_index != -1,
        out_queue_info->transfer_family_index != -1,
        properties->deviceName);

    if ((!requirements->graphics || (requirements->graphics && out_queue_info->graphics_family_index != -1)) &&
        (!requirements->present || (requirements->present && out_queue_info->present_family_index != -1)) &&
        (!requirements->compute || (requirements->compute && out_queue_info->compute_family_index != -1)) &&
        (!requirements->transfer || (requirements->transfer && out_queue_info->transfer_family_index != -1)))
    {
        INFO_LOG("Device meets queue requirements");
        TRACE_LOG("Graphics Family Index: %i", out_queue_info->graphics_family_index);
        TRACE_LOG("Present Family Index: %i", out_queue_info->present_family_index);
        TRACE_LOG("Transfer Family Index: %i", out_queue_info->transfer_family_index);
        TRACE_LOG("Compute Family Index: %i", out_queue_info->compute_family_index);

        //Query swapchain support
        vulkan_device_query_swapchain_support(device, surface, out_swapchain_support);
        if(out_swapchain_support->format_count < 1 || out_swapchain_support ->present_mode_count < 1)
        {
            if(out_swapchain_support->formats)
            {
                mem_free(out_swapchain_support->formats, sizeof(VkSurfaceFormatKHR) * out_swapchain_support->format_count, MEMORY_TAG_RENDERER);
            }
            if(out_swapchain_support->present_modes)
            {
                mem_free(out_swapchain_support->present_modes, sizeof(VkPresentModeKHR) * out_swapchain_support->present_mode_count, MEMORY_TAG_RENDERER);
            }
            INFO_LOG("Required swapchain support not present, skipping device...");
            return false;
        }

        //Device extensions
        if(requirements->device_extension_names)
        {
            u32 avaliable_extension_count = 0;
            VkExtensionProperties* avaliable_extensions = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &avaliable_extension_count, 0));
            if(avaliable_extension_count != 0)
            {
                avaliable_extensions = mem_allocate(sizeof(VkExtensionProperties) * avaliable_extension_count, MEMORY_TAG_RENDERER);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &avaliable_extension_count, avaliable_extensions));

                u32 required_extension_count = dynamic_array_length(requirements->device_extension_names);
                for(u32 i = 0; i < required_extension_count; ++i)
                {
                    b8 found = false;
                    for(u32 j = 0; j < avaliable_extension_count; ++j)
                    {
                        if(string_equal(requirements->device_extension_names[i], avaliable_extensions[j].extensionName))
                        {
                            found = true;
                            break;
                        }
                    }

                    if(!found)
                    {
                        INFO_LOG("Required extension not found: '%s', skipping device...", requirements->device_extension_names[i]);
                        mem_free(avaliable_extensions, sizeof(VkExtensionProperties) * avaliable_extension_count, MEMORY_TAG_RENDERER);
                        return false;
                    }
                }
            }
            mem_free(avaliable_extensions, sizeof(VkExtensionProperties) * avaliable_extension_count, MEMORY_TAG_RENDERER);
        }

        //Sampler anisotropy
        if(requirements->sampler_anisotropy && !features->samplerAnisotropy)
        {
            INFO_LOG("Device doesn't support samplerAnisotropy, skipping...");
            return false;
        }

        //Device meets all rerquirements
        return true;
    }
    return false;
}