#pragma once
#include "defines.h"

//Application config
typedef struct application_config
{
    //Window start position x axis
    i16 start_pos_x;
    //Window start position y axis
    i16 start_pos_y;
    //Window start width
    i16 start_width;
    //Window start height
    i16 start_height;
    //Application name
    char* name;
}application_config;

FAPI b8 application_create(application_config* config);
FAPI b8 application_run();