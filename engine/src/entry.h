#pragma once
#include "core/application.h"
#include "core/logger.h"
#include "game_types.h"

//Externally-defined function to create a game
extern b8 create_game(game* out_game);
//Main entry point of the application
int main(void){
    //Request the game instance from the application
    game game_inst;
    if(!create_game(&game_inst))
    {
        FFATAL("Couldn't create game!!!");
        return -1;
    }
    //Check if function pointers exist
    if(!game_inst.render || !game_inst.update || !game_inst.initialize || !game_inst.on_resize)
    {
        FFATAL("Game's function pointers must be assigned!!!")
        return -2;
    }
    //Initialization
    if(!application_create(&game_inst))
    {
        FINFO("Failed to create application");
        return 1;
    }
    //Begin the game loop
    if(!application_run())
    {
        FINFO("Application didn't shutdown gracefully");
        return 2;
    }
    application_config config;
    return 0;
}