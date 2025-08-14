#pragma once
#include "core/application.h"
#include "core/logger.h"
#include "game_types.h"
#include "core/fmemory.h"

//Externally-defined function to create a game
extern b8 create_game(game* out_game);
//Main entry point of the application
int main(void){
    initialize_memory();
    //Request the game instance from the application
    game game_inst;
    if(!create_game(&game_inst))
    {
        FATAL("Couldn't create game!!!");
        return -1;
    }
    //Check if function pointers exist
    if(!game_inst.render || !game_inst.update || !game_inst.initialize || !game_inst.on_resize)
    {
        FATAL("Game's function pointers must be assigned!!!")
        return -2;
    }
    //Initialization
    if(!application_create(&game_inst))
    {
        INFO("Failed to create application");
        return 1;
    }
    //Begin the game loop
    if(!application_run())
    {
        INFO("Application didn't shutdown gracefully");
        return 2;
    }
    shutdown_memory();
    application_config config;
    return 0;
}