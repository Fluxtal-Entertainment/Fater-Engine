#include "platform/platform.h"

//Windows platform layer.
#if PLATFORM_WINDOWS

#include "core/logger.h"
#include "core/input.h"
#include "core/event.h"
#include "containers/dynamic_array.h"
#include <windows.h>
#include <windowsx.h> //param input extraction
#include <stdlib.h>

//Only for surface creation
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "renderer/vulkan/vk_types.inl"

typedef struct internal_state
{
    HINSTANCE h_instance;
    HWND hwnd;
    VkSurfaceKHR surface;
} internal_state;

//Clock
static f64 clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

b8 platform_startup(platform_state *plat_state, const char *application_name, i32 x, i32 y, i32 width, i32 height)
{
    plat_state -> internal_state = malloc(sizeof(internal_state));
    internal_state *state = (internal_state *)plat_state -> internal_state;
    state->h_instance  = GetModuleHandleA(0);

    //Setup and register window class
    HICON icon = LoadIcon(state -> h_instance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS; //Get double-clicks
    wc.lpfnWndProc = win32_process_message;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state -> h_instance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); //NULL;  Manage the cursor manually
    wc.hbrBackground = NULL;    //transparent
    wc.lpszClassName = "Fater_Engine_window_class";

    if(!RegisterClassA(&wc))
    {
        MessageBoxA(0, "Window registration failed!", "ERROR", MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    //Window Creation
    u32 client_x = x;
    u32 client_y = y;
    u32 client_width = width;
    u32 client_height = height;
    u32 window_x = client_x;
    u32 window_y = client_y;
    u32 window_width = client_width;
    u32 window_height = client_height;
    u32 window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
    u32 window_ex_style = WS_EX_APPWINDOW;

    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;

    //Size of border
    RECT border_rect = {0, 0, 0, 0};
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    //In this case the border is negative
    window_x += border_rect.left;
    window_y += border_rect.top;

    //Increase size of the OS border
    window_width += border_rect.right - border_rect.left;
    window_height += border_rect.bottom - border_rect.top;

    HWND handle = CreateWindowExA(window_ex_style, "Fater_Engine_window_class", application_name, window_style, window_x, window_y, window_width, window_height, 0, 0, state->h_instance, 0);
    if(handle == 0)
    {
        MessageBoxA(NULL, "Window Creation FAILED!", "ERROR!!!", MB_ICONEXCLAMATION | MB_OK);
        FATAL_LOG("Window Creation FAILED!");
        return FALSE;
    }
    else
    {
        state -> hwnd = handle;
    }

    //Show window
    b32 should_activate = 1;  //TODO: If window shouldn't accept input, this should be false.
    i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
    //If start maximized use "SW_SHOWMAXIMIZED : SW_MAXIMIZE" , if start minimized use "SW_MINIMIZE : SW_SHOWMINNOACTIVE"
    ShowWindow(state -> hwnd, show_window_command_flags);

    //Clock setup
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (f64)frequency.QuadPart;
    QueryPerformanceCounter(&start_time);

    return TRUE;
}

void platform_shutdown(platform_state *plat_state)
{
    //Cold-cast to known type
    internal_state *state = (internal_state *)plat_state -> internal_state;

    if(state -> hwnd)
    {
        DestroyWindow(state -> hwnd);
        state -> hwnd = 0;
    }    
}

b8 platform_pump_messages(platform_state* plat_state)
{
    MSG message;
    while(PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return TRUE;
}

void *platform_allocate(u64 size, b8 aligned)
{
    return malloc(size);
}

void platform_free(void* block, b8 aligned)
{
    free(block);
}

void *platform_zero_memory(void *block, u64 size)
{
    return memset(block, 0, size);
}

void *platform_copy_memory(void *dest, const void *source, u64 size)
{
    return memcpy(dest, source, size);
}

void *platform_set_memory(void *dest, i32 value, u64 size)
{
    return memset(dest, value, size);
}

void platform_console_write(const char* message, u8 color)
{
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};    //Fatal, Error, Warn, Info, Debug, Trace
    SetConsoleTextAttribute(console_handle, levels[color]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_written, 0);
}

void platform_console_write_error(const char* message, u8 color)
{
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};    //Fatal, Error, Warn, Info, Debug, Trace
    SetConsoleTextAttribute(console_handle, levels[color]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, number_written, 0);
}

f64 platform_get_absolute_time()
{
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64)now_time.QuadPart * clock_frequency;
}

void platform_sleep(u64 ms)
{
    Sleep(ms);
}

void platform_get_required_extension_names(const char*** names_dynamic_array)
{
    dynamic_array_push(*names_dynamic_array, &"VK_KHR_win32_surface");
}

//Surface creation for Vulkan
b8 platform_create_vulkan_surface(platform_state* plat_state, vulkan_context* context)
{
    //Cold-casting to the known type
    internal_state *state = (internal_state *)plat_state->internal_state;

    VkWin32SurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    create_info.hinstance = state->h_instance;
    create_info.hwnd = state->hwnd;

    VkResult result = vkCreateWin32SurfaceKHR(context->instance, &create_info, context->allocator, &state->surface);
    if(result != VK_SUCCESS)
    {
        FATAL_LOG("Vulkan surface creation has failed!!!");
        return false;
    }
    context->surface = state->surface;
    return true;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param)
{
    switch(msg)
    {
        case WM_ERASEBKGND:
            //Preventing from flickering by letting the OS know that erasing will be handled by application
            return 1;

        case WM_CLOSE:
            event_context data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
            return true;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
        {
            //Get updated size
            /*RECT r;
            GetClientRect(hwnd, &r);
            u32 width = r.right - r.left;
            u32 height = r.bottom - r.top;*/

            //TODO: Fire an event for window resizing
        } break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            //Key pressed or released
            b8 pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            keys key = (u16)w_param;
            //pass to input subsystem
            input_process_key(key, pressed);
        }break;

        case WM_MOUSEMOVE:
        {
            //Mouse move
            i32 x_pos = GET_X_LPARAM(l_param);
            i32 y_pos = GET_Y_LPARAM(l_param);
            //pass to input subsystem
            input_process_mouse_move(x_pos, y_pos);
        }break;

        case WM_MOUSEHWHEEL:
        {
            i32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
            if(z_delta != 0)
            {
                //Flatten input to an OS independent (-1,1)
                z_delta = (z_delta < 0) ? -1 : 1;
                //Input processing
                input_process_mouse_wheel(z_delta);
            }
        }break;

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            b8 pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
            //Input processing
            buttons mouse_button = BUTTON_MAX_BUTTONS;
            switch(msg)
            {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                {
                    mouse_button = LEFT_BUTTON;
                }break;

                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                {
                    mouse_button = MIDDLE_BUTTON;
                }break;

                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                {
                    mouse_button = RIGHT_BUTTON;
                }break;
            }
            if(mouse_button != BUTTON_MAX_BUTTONS)
            {
                input_process_button(mouse_button, pressed);
            }      
        }break;
    }
    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

#endif //PLARFORM_WINDOWS