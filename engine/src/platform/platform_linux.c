#include "platform.h"

#include <xcb/xcb.h>

typedef struct internal_state
{
    Display *display;
    xcb_connection_t *connection;
    xcb_window_t window;
    xcb_screen_t *screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
} internal_state; 

b8 platform_startup(platform_state* plat_state, const char* application_name, i32 x, i32 y, i32 width, i32 height)
{

}

void plarform_shutdown(platform_state* plat_state)
{

}

b8 platform_pump_messages(platform_state* plat_state)
{

}

void* platform_allocate(u64 size, u8 aligned)
{

}

void platform_free(void* block, u8 aligned)
{

}

void* platform_zero_memory(void* block, u64 size)
{

}

void* platform_copy_memory(void* dest, const void* source, u64 size)
{

}

void* platform_set_memory(void* dest, i32 value, u64 size)
{

}

void platform_console_write(const char* message, u8 color)
{

}

void platform_console_write_error(const char* message, u8 color)
{

}

f64 platform_get_absolute_time()
{

}

void platform_sleep(u64 ms)
{

}