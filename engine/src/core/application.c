#include "application.h"
#include "game_types.h"
#include "logger.h"
#include "platform/platform.h"

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
        FERROR("[application_create] called more than once!!!")
        return FALSE;
    }
    app_state.game_inst = game_inst;
    //Initialize subsystems
    initialize_logging();
    //NOTE: Only for tests. This'll be removed later.
    FFATAL("A test message: %f", 3.14f);
    FERROR("A test message: %f", 3.14f);
    FWARN("A test message: %f", 3.14f);
    FINFO("A test message: %f", 3.14f);
    FDEBUG("A test message: %f", 3.14f);
    FTRACE("A test message: %f", 3.14f);
    app_state.is_running = TRUE;
    app_state.is_suspended = FALSE;
    if(!platform_startup(&app_state.platform, game_inst->app_config.name, game_inst->app_config.start_pos_x, game_inst->app_config.start_pos_y, game_inst->app_config.start_width, game_inst->app_config.start_height))
    {
        return FALSE;
    }
    //Game initialization
    if(!app_state.game_inst->initialize(app_state.game_inst))
    {
        FFATAL("FAILED to initialize the game!!!");
        return FALSE;
    }
    app_state.game_inst->on_resize(app_state.game_inst, app_state.width, app_state.height);
    initialized = TRUE;
    return TRUE;
}
b8 application_run()
{
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
                FFATAL("Game update failed!!! Shutting down.");
                app_state.is_running = FALSE;
                break;
            }
            if(!app_state.game_inst->render(app_state.game_inst, (f32)0))
            {
                FFATAL("Game render failed!!! Shutting down.");
                app_state.is_running = FALSE;
                break;
            }
       }
    }
    app_state.is_running = FALSE;
    platform_shutdown(&app_state.platform);
    return TRUE;
}