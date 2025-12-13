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

    //game init
    srand(SDL_GetTicks());
    slog_sync();
    gf2d_mouse_load("actors/mouse.actor");

    gfc_matrix4_identity(id);

    Mesh* skyMesh = gf3d_mesh_load("models/sky/sky.obj");
    Texture* skyTexture = gf3d_texture_load("models/sky/sky.png");

    //World
    World* world = world_load("defs/terrain/terrain.def.txt");
    gfc_matrix4_multiply_scalar(terrainMat, id, 5);

    // Spawn player
    Entity* player = plane_spawn(spawnPos, GFC_COLOR_WHITE);
    if (!player) {
        slog("Failed to spawn player!");
        _done = 1;
    }

    // Spawn enemies
    GFC_Vector3D enemy_spawns[] = {
       { -256.03f, 236.84f, 8.71f },
       { -94.62f, -374.55f, 8.71f },
       { 262.17f, 86.17f, 219.06f },
       { -38.25f, 331.75f, 350.19f },
       { -384.89f, -114.42f, 453.39f }
    };

    enemy_spawn(enemy_spawns[0], ENEMY_LIGHT_TURRET, player);
    enemy_spawn(enemy_spawns[1], ENEMY_HEAVY_TURRET, player);
    enemy_spawn(enemy_spawns[2], ENEMY_FIGHTER, player);
    enemy_spawn(enemy_spawns[3], ENEMY_BOMBER, player);
    enemy_spawn(enemy_spawns[4], ENEMY_INTERCEPTOR, player);

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
        world_draw(world);
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
        }

        // Controls
        gf2d_font_draw_line_tag("WASD: Pitch/Roll | QE: Yaw | Arrows: Speed | Space: Fire",
            FT_H3, GFC_COLOR_WHITE, gfc_vector2d(10, 700));

        // Crosshair
        gf2d_font_draw_line_tag("+", FT_H1, GFC_COLOR_RED, gfc_vector2d(640, 360));

        gf2d_mouse_draw();
        gf3d_vgraphics_render_end();

        if (gfc_input_command_down("exit")) _done = 1;
        game_frame_delay();
    }

    vkDeviceWaitIdle(gf3d_vgraphics_get_default_logical_device());
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