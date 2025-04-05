#include <core/logger.h>
#include <core/asserts.h>
//NOTE: Test
#include <core/application.h>

int main(void){
    //Application config
    application_config config;
    config.start_pos_x = 100;
    config.start_pos_y = 100;
    config.start_width = 1280;
    config.start_height = 720;
    config.name = "Fater Engine";
    application_create(&config);
    application_run();
    return 0;
}