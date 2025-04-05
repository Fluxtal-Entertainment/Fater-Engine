#include "game.h"
#include <entry.h>
//NOTE:To remove
#include <platform/platform.h>

//Define the function to create a game
b8 create_game(game* out_game)
{
    //Application config
    out_game->app_config.start_pos_x = 100;
    out_game->app_config.start_pos_y = 100;
    out_game->app_config.start_width = 1280;
    out_game->app_config.start_height = 720;
    out_game->app_config.name = "Fater Engine";
    out_game->update = game_update;
    out_game->render = game_render;
    out_game->initialize = game_initialize;
    out_game->on_resize = game_on_resize;
    //Create game state
    out_game->state =  platform_allocate(sizeof(game_state), FALSE);
    return TRUE;
}