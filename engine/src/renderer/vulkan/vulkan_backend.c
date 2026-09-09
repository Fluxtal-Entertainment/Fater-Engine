#include "vulkan_backend.h"
#include "vulkan_types.inl"
#include "vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_fence.h"

#include "core/logger.h"
#include "core/f_string.h"
#include "core/f_memory.h"
#include "core/application.h"
#include "containers/dynamic_array.h"
#include "platform/platform.h"

//static Vulkan context
static vulkan_context context;
static u32 cached_framebuffer_width = 0;
static u32 cached_framebuffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_types, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);

i32 find_memory_index(u32 type_filter, u32 property_flags);
void create_command_buffers(renderer_backend* backend);
void regenerate_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass);

b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name, struct platform_state* plat_state)
{
    //Function pointers
    context.find_memory_index = find_memory_index;

    //NOTE:Temporary
    context.allocator = 0;

    application_get_framebuffer_size(&cached_framebuffer_width, &cached_framebuffer_height);
    context.framebuffer_width = (cached_framebuffer_width != 0) ? cached_framebuffer_width : 800;
    context.framebuffer_height = (cached_framebuffer_height != 0) ? cached_framebuffer_height : 600;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    //Setup Vulkan instance
    VkApplicationInfo app_info =  {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_3;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Fater Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;

    //Obtain a list of required extensions
    const char** required_extensions = dynamic_array_create(const char*);
    dynamic_array_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);//Generic surface extension
    platform_get_required_extension_names(&required_extensions); //Platform specific extension(s)

#if defined(_DEBUG)
//Debug utilities
    dynamic_array_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    DEBUG_LOG("Required extensions:");
    u32 length = dynamic_array_length(required_extensions);
    for(u32 i = 0 ; i < length; i++)
    {
        DEBUG_LOG(required_extensions[i]);
    }
#endif
    create_info.enabledExtensionCount = dynamic_array_length(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;

    //Validation layers
    const char** required_validation_layer_names = 0;
    u32 required_validation_layer_count = 0;

//If validation should be done, get a list of the required validation layers names
//and make sure they exist. Validation layers should only be enabled on non-release builds
#if defined(_DEBUG)
    INFO_LOG("Validation layers enabled. Enumerating...");

    //The list of validation layers required
    required_validation_layer_names = dynamic_array_create(const char*);
    dynamic_array_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");
    required_validation_layer_count = dynamic_array_length(required_validation_layer_names);

    //Obtain a list of available validation layers
    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
    VkLayerProperties* available_layers = dynamic_array_reserve(VkLayerProperties, available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers)); 

    //Verify if all required layers are available
    for(u32 i = 0; i < required_validation_layer_count; i++)
    {
        INFO_LOG("Searching for layer: %s...", required_validation_layer_names[i]);
        b8 found = false;
        for(u32 j = 0; j < available_layer_count; j++)
        {
            if(string_equal(required_validation_layer_names[i], available_layers[j].layerName))
            {
                found = true;
                INFO_LOG("Layer found");
                break;
            }
        }
        if(!found)
        {
            FATAL_LOG("Required validation layer is missing: %s", required_validation_layer_names[i]);
            return false;
        }
    }
    INFO_LOG("All required validation layers are present");
#endif

    create_info.enabledLayerCount = required_validation_layer_count;
    create_info.ppEnabledLayerNames = required_validation_layer_names;

    VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));
    INFO_LOG("Vulkan Instance created");

//Debugger
#if defined(_DEBUG)
    DEBUG_LOG("Creating Vulkan debugger...");
    u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;//| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_create_info.pfnUserCallback = vk_debug_callback;

    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    ASSERT_MSG(func, "Failed to create debug messenger!!!");
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messanger));
    DEBUG_LOG("Vulkan debugger created");
#endif

    //Vulkan surface creation
    DEBUG_LOG("Creating Vulkan surface...")
    if(!platform_create_vulkan_surface(plat_state, &context))
    {
        ERROR_LOG("Failed to create platform surface!!!");
        return false;
    }
    DEBUG_LOG("Vulkan surface created")

    //Vulkan device creation
    if(!vulkan_device_create(&context))
    {
        ERROR_LOG("Failed to create Vulkan device!!!");
        return false;
    }

    //Swapchain creation
    vulkan_swapchain_create(&context, context.framebuffer_width, context.framebuffer_height, &context.swapchain);

    //Renderpass creation
    vulkan_renderpass_create(&context, &context.main_renderpass, 0, 0, context.framebuffer_width, context.framebuffer_height, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0);

    //Swapchain framebuffers
    context.swapchain.framebuffers = dynamic_array_reserve(vulkan_framebuffer, context.swapchain.image_count);
    regenerate_framebuffers(backend, &context.swapchain, &context.main_renderpass);

    //Command buffer creation
    create_command_buffers(backend);

    //Create sync objects
    context.image_available_semaphores = dynamic_array_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.queue_complete_semaphores = dynamic_array_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.in_flight_fences = dynamic_array_reserve(vulkan_fence, context.swapchain.max_frames_in_flight);

    for(u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]);
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]);

        /**
         * Create the fence in a signaled state, indicating that the first frame has already been "rendered"
         * This will prevent the application from waiting indefinetly for the first frame to render since it
         * cannot be rendered until a frame is "rendered" before it
         */
        vulkan_fence_create(&context, true, &context.in_flight_fences[i]);
    }

    /**
     * In flight fences shouldn't yet exist at this point, so clear the list
     * These are stored in pointers because the initial state should be 0, and will stay 0 when not in use
     * Actual fences aren't owned by this list
     */
    context.images_in_flight = dynamic_array_reserve(vulkan_fence, context.swapchain.image_count);
    for(u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        context.images_in_flight[i] = 0;
    }

    INFO_LOG("Vulkan renderer initialized succesfully");
    return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend)
{
    vkDeviceWaitIdle(context.device.logical_device);
    //Destroy in the opposite order of creation

    //Sync objects
    for(u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i)
    {
        if(context.image_available_semaphores[i])
        {
            vkDestroySemaphore(context.device.logical_device, context.image_available_semaphores[i], context.allocator);
            context.image_available_semaphores[i] = 0;
        }
        if(context.queue_complete_semaphores[i])
        {
            vkDestroySemaphore(context.device.logical_device, context.queue_complete_semaphores[i], context.allocator);
            context.queue_complete_semaphores[i] = 0;
        }
        vulkan_fence_destroy(&context, &context.in_flight_fences[i]);
    }

    dynamic_array_destroy(context.image_available_semaphores);
    context.image_available_semaphores = 0;
    dynamic_array_destroy(context.queue_complete_semaphores);
    context.queue_complete_semaphores = 0;
    dynamic_array_destroy(context.in_flight_fences);
    context.in_flight_fences = 0;
    dynamic_array_destroy(context.images_in_flight);
    context.images_in_flight = 0;

    //Command buffers
    for(u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        if(context.graphics_command_buffers[i].handle)
        {
            vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
            context.graphics_command_buffers[i].handle = 0;
        }
    }
    dynamic_array_destroy(context.graphics_command_buffers);
    context.graphics_command_buffers = 0;

    //Framebuffers
    for(u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
    }

    //Renderpass
    vulkan_renderpass_destroy(&context, &context.main_renderpass);

    //Swapchain
    vulkan_swapchain_destroy(&context, &context.swapchain);

    DEBUG_LOG("Destroying Vulkan device...");
    vulkan_device_destroy(&context);

    DEBUG_LOG("Destroying Vulkan surface...");
    if(context.surface)
    {
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = 0;
    }

    DEBUG_LOG("Destroying Vulkan debugger...");
    if(context.debug_messanger)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(context.instance, context.debug_messanger, context.allocator);
    }

    DEBUG_LOG("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_rederer_backend_on_resized(renderer_backend* backend, u16 width, u16 height)
{

}

b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time)
{
    return true;
}

b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time)
{
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_types, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    switch(message_severity)
    {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        {
            ERROR_LOG(callback_data->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        {
            WARN_LOG(callback_data->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        {
            INFO_LOG(callback_data->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        {
            TRACE_LOG(callback_data->pMessage);
            break;
        }
    }
    return VK_FALSE;
}

i32 find_memory_index(u32 type_filter, u32 property_flags)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context.device.physical_device, &memory_properties);

    for(u32 i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        //Check each memory type to see if its bit is set to 1
        if(type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags)
        {
            return i;
        }
    }

    WARN_LOG("Unable to find suitable memory type!");
    return -1;
}

void create_command_buffers(renderer_backend* backend)
{
    if(!context.graphics_command_buffers)
    {
        context.graphics_command_buffers = dynamic_array_reserve(vulkan_command_buffer, context.swapchain.image_count);
        for(u32 i = 0; i < context.swapchain.image_count; ++i)
        {
            mem_zero(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
        }
    }

    for(u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        if(context.graphics_command_buffers[i].handle)
        {
            vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
        }
        mem_zero(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
        vulkan_command_buffer_allocate(&context, context.device.graphics_command_pool, true, &context.graphics_command_buffers[i]);
    }
    INFO_LOG("Vulkan command buffers created");
}

void regenerate_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass)
{
    for(u32 i = 0; i < swapchain->image_count; ++i)
    {
        //TODO: Make this dynamic based on the currently configured attachments
        u32 attachment_count = 2;
        VkImageView attachments[] = {swapchain->views[i], swapchain->depth_attachment.view};
        vulkan_framebuffer_create(&context, renderpass, context.framebuffer_width, context.framebuffer_height, attachment_count, attachments, &context.swapchain.framebuffers[i]);
    }
}