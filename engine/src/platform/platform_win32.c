#include "platform.h"

//Windows platform layer.
#if KPLATFORM_WINDOWS

#include <windows.h>
#include <windowsx.h> //param input extraction

typedef struct internal_state
{
    HINSTANCE h_instance;
    HWND hwnd;
} internal_state;

b8 platform_startup(platform_state *plat_state, const char *application_name, i32 x, i32 y, i32 width, i32 height)
{
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state *state = (internal_state *)plat_state->internal_state;
    state->h_instance  = GetModuleHandleA(0);

    //Setup and register window class
    HICON icon = LoadIcon(state->h_instance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS; //Get double-clicks
    wc.lpfnWndProc = win32_process_message;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state->h_instance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); //NULL;  Manage the cursor manually
    wc.hbrBackground = NULL;    //transparent
    wc.lpszClassName = "TfujstarySX3VX_window_class";
}

#endif