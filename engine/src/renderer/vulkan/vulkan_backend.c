#include "vulkan_backend.h"
#include "vulkan_types.inl"
#include "vulkan_platform.h"
#include "core/logger.h"
#include "core/f_string.h"
#include "containers/dynamic_array.h"
#include "platform/platform.h"
#include "vulkan_device.h"

//static Vulkan context
static vulkan_context context;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_types, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);

b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name, struct platform_state* plat_state)
{
    //NOTE:Temporary
    context.allocator = 0;

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
    INFO_LOG("Vulkan renderer initialized succesfully");
    return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend)
{
    //Destroy in the opposite order of creation
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