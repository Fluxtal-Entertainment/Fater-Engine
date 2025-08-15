#include "application.h"
#include "game_types.h"
#include "logger.h"
#include "platform/platform.h"
#include "fmemory.h"
#include "event.h"
#include "input.h"

typedef struct application_state
{
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    i16 width;
    i16 height;
    f64 last_time;
} application_state;
static b8 initialized = FALSE;
static application_state app_state;

b8 application_create(game* game_inst)
{
    if(initialized)
    {
        ERROR("[application_create()] called more than once!!!")
        return FALSE;
    }
    app_state.game_inst = game_inst;
    //Initialize subsystems
    input_initialize();
    //NOTE: Only for tests. This'll be removed later.
    FATAL("A test message: %f", 3.14f);
    ERROR("A test message: %f", 3.14f);
    WARN("A test message: %f", 3.14f);
    INFO("A test message: %f", 3.14f);
    DEBUG("A test message: %f", 3.14f);
    TRACE("A test message: %f", 3.14f);
    app_state.is_running = TRUE;
    app_state.is_suspended = FALSE;
    if(!event_initialize())
    {
        ERROR("Event system failed during initialization. Application can't continue!!!");
        return false;
    }
    if(!platform_startup(&app_state.platform, game_inst->app_config.name, game_inst->app_config.start_pos_x, game_inst->app_config.start_pos_y, game_inst->app_config.start_width, game_inst->app_config.start_height))
    {
        return FALSE;
    }
    //Game initialization
    if(!app_state.game_inst->initialize(app_state.game_inst))
    {
        FATAL("FAILED to initialize the game!!!");
        return FALSE;
    }
    app_state.game_inst->on_resize(app_state.game_inst, app_state.width, app_state.height);
    initialized = TRUE;
    return TRUE;
}

b8 application_run()
{
    INFO(get_memory_usage_string());
    while(app_state.is_running)
    {
       if(!platform_pump_messages(&app_state.platform))
       {
        app_state.is_running = FALSE;
       }
       if(!app_state.is_suspended)
       {
            if(!app_state.game_inst->update(app_state.game_inst, (f32)0))
            {
                FATAL("Game update failed!!! Shutting down.");
                app_state.is_running = FALSE;
                break;
            }
            if(!app_state.game_inst->render(app_state.game_inst, (f32)0))
            {
                FATAL("Game render failed!!! Shutting down.");
                app_state.is_running = FALSE;
                break;
            }
            //NOTE: Input update or state copying should always be handled after any input should be recorded; I.E. before this line.
            //As a safety, input is the last thing to be updated before this frame ends
            input_update(0);
       }
    }
    app_state.is_running = FALSE;
    event_shutdown();
    input_shutdown();
    platform_shutdown(&app_state.platform);
    return TRUE;
}