#include "input.h"
#include "event.h"
#include "fmemory.h"
#include "logger.h"

typedef struct keyboard_state
{
    b8 keys[256];
} keyboard_state;

typedef struct mouse_state
{
    i16 x;
    i16 y;
    u8 buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct input_state
{
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

//Internal input state
static b8 initialized = false;
static input_state state = {};

void input_initialize()
{
    mem_zero(&state, sizeof(input_state));
    initialized = true;
    INFO("Input subsystem have been initialized");
}

void input_shutdown()
{
    //TODO:Add shutdown routines if needed
    initialized = false;
}

void input_update(f64 delta_time)
{
    if(!initialized)
    {
        return;
    }
    //Copy current states to previous states
    mem_copy(&state.keyboard_previous, &state.keyboard_current, sizeof(keyboard_state));
    mem_copy(&state.mouse_previous, &state.mouse_current, sizeof(mouse_state));
}

void input_process_key(keys key, b8 pressed)
{
    //Only if state changed
    if(state.keyboard_current.keys[key] != pressed)
    {
        //Update internal state
        state.keyboard_current.keys[key] = pressed;
        //Fire off an event for immediate processing
        event_context context;
        context.data.u16[0] = key;
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);  
    }
}

void input_process_button(buttons button, b8 pressed)
{
    //Fire an event if the statement had changed
    if(state.mouse_current.buttons[button] != pressed)
    {
        state.mouse_current.buttons[button] = pressed;
        //Fire the event
        event_context context;
        context.data.u16[0] = button;
        event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void input_process_mouse_move(i16 x, i16 y)
{
    //Only process if actually different
    if(state.mouse_current.x != x || state.mouse_current.y != y)
    {
        //NOTE:Line below only for debug purpose
        //DEBUG("Mouse pos: %i, %i", x, y);

        //Updates internal state
        state.mouse_current.x = x;
        state.mouse_current.y = y;
        //Fires the event
        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void input_process_mouse_wheel(i8 z_delta)
{
    //NOTE:No internal state to update
    //Fires the event
    event_context context;
    context.data.u8[0] = z_delta;
    event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

//Keyboard input
b8 input_is_key_down(keys key)
{
    if(!initialized)
    {
        return false;
    }
    return state.keyboard_current.keys[key] == true;
}

b8 input_is_key_up(keys key)
{
    if(!initialized)
    {
        return true;
    }
    return state.keyboard_current.keys[key] == false;
}

b8 input_was_key_down(keys key)
{
    if(!initialized)
    {
        return false;
    }
    return state.keyboard_previous.keys[key] == true;
}

b8 input_was_key_up(keys key)
{
    if(!initialized)
    {
        return true;
    }
    return state.keyboard_previous.keys[key] == false;
}

//Mouse input
b8 input_is_button_down(buttons button)
{
    if(!initialized)
    {
        return false;
    }
    return state.mouse_current.buttons[button] == true;
}

b8 input_is_button_up(buttons button)
{
    if(!initialized)
    {
        return true;
    }
    return state.mouse_current.buttons[buttony] == false;
}

b8 input_was_button_down(buttons button)
{
    if(!initialized)
    {
        return false;
    }
    return state.mouse_previous.buttons[button] == true;
}

b8 input_was_button_up(buttons button)
{
    if(!initialized)
    {
        return true;
    }
    return state.mouse_previous.buttons[button] == false;
}

void input_get_mouse_pos(i32* x, i32* y)
{
    if(!initialized)
    {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}

void input_get_mouse_prev_pos(i32* x, i32* y)
{
    if(!initialized)
    {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}