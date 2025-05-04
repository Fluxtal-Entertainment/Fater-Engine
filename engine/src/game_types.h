#pragma once
#include "core/application.h"

//Represents the basic game state in game. Called for creation by application.
typedef struct game
{
   //Application config
   application_config app_config;
   //Function pointer to game's initialize function
   b8 (*initialize)(struct game* game_inst);
   //Function pointer to game's update function
   b8 (*update)(struct game* game_inst, f32 delta_time);
   //Function to game's render function
   b8 (*render)(struct game* game_inst, f32 delta_time);
   //Function pointer to handle resizes
   void (*on_resize)(struct game* game_inst, u32 width, u32 height);
   //Game-specific state. Created and managed by the game
   void* state;
}game;
