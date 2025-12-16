#include <SDL.h>            

#include "simple_json.h"
#include "simple_logger.h"

#include "gfc_input.h"
#include "gfc_config_def.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "gfc_audio.h"
#include "gfc_string.h"
#include "gfc_actions.h"

#include "gf2d_sprite.h"
#include "gf2d_font.h"
#include "gf2d_actor.h"
#include "gf2d_mouse.h"

#include "gf3d_vgraphics.h"
#include "gf3d_pipeline.h"
#include "gf3d_swapchain.h"
#include "gf3d_camera.h"
#include "gf3d_mesh.h"
#include "gf3d_entity.h"
#include "plane_entity.h"
#include "world.h"
#include "enemy_entity.h"
#include "data_definitions.h"
#include "menu_system.h"


extern int __DEBUG;

static int _done = 0;
static Uint32 frame_delay = 33;
static float fps = 0;

void parse_arguments(int argc, char* argv[]);
void game_frame_delay();

void exitGame()
{
    _done = 1;
}

int main(int argc, char* argv[])
{
    //local variables
    GFC_Matrix4 id, terrainMat;
    GFC_Vector3D spawnPos = gfc_vector3d(0, 0, 100);
    LevelDefinition* currentLevel = NULL;

    //initializtion    
    parse_arguments(argc, argv);
    init_logger("gf3d.log", 0);
    slog("gf3d begin");

    //gfc init
    gfc_input_init("config/input.cfg");
    gfc_config_def_init();
    gfc_action_init(1024);

    //gf3d init
    gf3d_vgraphics_init("config/setup.cfg");
    gf2d_font_init("config/font.cfg");
    gf2d_actor_init(100);

    //entity system init
    entity_system_init(1000);
    data_load_enemy_definitions();  
    data_load_weapon_definitions(); 
    data_load_loadout_definitions();    


    //game init
    srand(SDL_GetTicks());
    slog_sync();
    gf2d_mouse_load("actors/mouse.actor");

    gfc_matrix4_identity(id);

    Mesh* skyMesh = gf3d_mesh_load("models/sky/sky.obj");
    Texture* skyTexture = gf3d_texture_load("models/sky/sky.png");

    //World
    World* world = world_load("defs/terrain/terrain.def.txt");
    gfc_matrix4_multiply_scalar(terrainMat, id, 1);

    // Initialize menu system
    if (!menu_system_init("defs/menu/main_menu.def.txt")) {
        slog("Failed to initialize menu system!");
        _done = 1;
        return 0;
    }

    // Menu loop
    MenuAction menuAction = MENU_ACTION_NONE;
    int selectedLevel = 1; // Default to level 1
    LoadoutType selectedLoadout = LOADOUT_BALANCED;

    while (menu_system_is_active() && !_done) {
        // Clear screen
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                _done = 1;
                break;
            }
        }

        gfc_input_update();

        // Update menu and get action
        menuAction = menu_system_update();

        // Handle menu actions
        if (menuAction == MENU_ACTION_START_GAME) {
            // Start game with default level and loadout selection
            menu_system_set_active(0);
            break;
        }
        else if (menuAction == MENU_ACTION_LOAD_LEVEL_1) {
            selectedLevel = 1;
            menu_system_set_active(0);
            break;
        }
        else if (menuAction == MENU_ACTION_LOAD_LEVEL_2) {
            selectedLevel = 2;
            menu_system_set_active(0);
            break;
        }
        else if (menuAction == MENU_ACTION_LOAD_LEVEL_3) {
            selectedLevel = 3;
            menu_system_set_active(0);
            break;
        }
        else if (menuAction == MENU_ACTION_QUIT) {
            _done = 1;
            break;
        }

        // Draw menu
        gf3d_vgraphics_render_start();
        menu_system_draw();
        gf3d_vgraphics_render_end();

        SDL_Delay(16);
    }

    // Cleanup menu
    menu_system_close();

    if (_done) {
        slog("Exiting from menu.");
        return 0;
    }

    // Now load the selected level
    char levelPath[256];
    sprintf(levelPath, "defs/levels/level%d.def.txt", selectedLevel);
    currentLevel = data_load_level(levelPath);

    // Then continue with loadout selection (keep your existing code)
    slog("=== SELECT LOADOUT ===");
    for (int i = 0; i < LOADOUT_COUNT; i++) {
        LoadoutDefinition* l = data_get_loadout_def(i);
        if (l) {
            slog("  [%d] %s - %s (HP: %d, Speed: %.1f)",
                i + 1,
                l->name ? l->name : "UNKNOWN",  
                l->description ? l->description : "UNKNOWN",  
                l->health,
                l->maxSpeed);
        }
    }   
    slog("Press 1/2/3 to select loadout...");

    int loadoutSelected = 0;
    while (!loadoutSelected && !_done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                _done = 1;
                break;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_1:
                    selectedLoadout = LOADOUT_SCOUT;
                    loadoutSelected = 1;
                    slog("Selected: Scout");
                    break;
                case SDLK_2:
                    selectedLoadout = LOADOUT_TANK;
                    loadoutSelected = 1;
                    slog("Selected: Tank");
                    break;
                case SDLK_3:
                    selectedLoadout = LOADOUT_BALANCED;
                    loadoutSelected = 1;
                    slog("Selected: Balanced");
                    break;
                case SDLK_ESCAPE:
                    _done = 1;
                    break;
                }
            }
        }
        SDL_Delay(16);
    }

    if (_done) {
        slog("Exiting before game start.");
        return 0;
    }

    if (!currentLevel) {
        slog("Failed to load level!");
        _done = 1;
        return 0;
    }
    spawnPos = currentLevel->playerSpawn;

    Entity* player = plane_spawn_with_loadout(spawnPos, selectedLoadout);
    if (!player) {  
        slog("Failed to spawn player!");    
        _done = 1;  
    }

    for (int i = 0; i < currentLevel->enemyCount; i++) {
        EnemyType type = enemy_type_from_string(currentLevel->enemies[i].enemyType);
        enemy_spawn(currentLevel->enemies[i].position, type, player);
    }

    char healthText[64];

    // Main game loop
    while (!_done)
    {
        gfc_input_update();
        gf2d_mouse_update();
        gf2d_font_update();

        //entity updates
        entity_think_all();
        entity_update_all();

        gf3d_vgraphics_render_start();

        //3D draws  
        world_draw(world, terrainMat);
        gf3d_sky_draw(skyMesh, id, GFC_COLOR_WHITE, skyTexture);
        entity_draw_all();

        //2D draws - HUD
        // Display player health 
        PlaneData* pdata = NULL;
        if (player && player->data) {
            pdata = (PlaneData*)player->data;

            // Health display
            sprintf(healthText, "HP: %d / %d", pdata->health, pdata->maxHealth);
            GFC_Color healthColor = pdata->health > 50 ? GFC_COLOR_GREEN :
                pdata->health > 20 ? GFC_COLOR_YELLOW : GFC_COLOR_RED;
            gf2d_font_draw_line_tag(healthText, FT_H1, healthColor, gfc_vector2d(10, 10));

            // Out of bounds warning - ADD THIS
            if (pdata->isOutOfBounds) {
                gf2d_font_draw_line_tag("!!! TURN AROUND - EDGE OF MAP !!!",
                    FT_H1, GFC_COLOR_RED, gfc_vector2d(300, 100));
            }
        }
        else if (!player) {
            // Player is dead - show game over
            gf2d_font_draw_line_tag("GAME OVER", FT_H1, GFC_COLOR_RED, gfc_vector2d(500, 300));
            gf2d_font_draw_line_tag("Press ESC to quit", FT_H2, GFC_COLOR_WHITE, gfc_vector2d(450, 350));
        }


        // Controls
        gf2d_font_draw_line_tag("WS: Pitch| Left/Right Arrows: Roll | A/D: Yaw | Space: Fire",
            FT_H3, GFC_COLOR_WHITE, gfc_vector2d(10, 650));

        // Crosshair
        gf2d_font_draw_line_tag("+", FT_H1, GFC_COLOR_RED, gfc_vector2d(640, 360));

        gf2d_mouse_draw();
        gf3d_vgraphics_render_end();

        if (gfc_input_command_down("exit")) _done = 1;
        game_frame_delay();
    }

    vkDeviceWaitIdle(gf3d_vgraphics_get_default_logical_device());
    data_free_level(currentLevel);

    //cleanup
    slog("gf3d program end");
    exit(0);
    slog_sync();
    return 0;
}

void parse_arguments(int argc, char* argv[])
{
    int a;
    for (a = 1; a < argc; a++)
    {
        if (strcmp(argv[a], "--debug") == 0)
        {
            __DEBUG = 1;
        }
    }
}

void game_frame_delay()
{
    Uint32 diff;
    static Uint32 now;
    static Uint32 then;
    then = now;
    slog_sync();
    now = SDL_GetTicks();
    diff = (now - then);
    if (diff < frame_delay)
    {
        SDL_Delay(frame_delay - diff);
    }
    fps = 1000.0 / MAX(SDL_GetTicks() - then, 0.001);
}
/*eol@eof*/