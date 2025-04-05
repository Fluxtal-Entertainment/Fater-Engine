#include "application.h"

typedef struct application_state
{
    b8 is_running;
    b8 is_suspended;
    platform_state* platform;
    i16 width;
    i16 height;
    f64 last_time;
}
static b8 initialized = FALSE;
static application_state app_state;
b8 application_create(application_config* config)
{
    if(initialized)
    {
        FERROR("Application_create called more than once!!!")
        return FALSE;
    }
}
b8 application_run()
{

}