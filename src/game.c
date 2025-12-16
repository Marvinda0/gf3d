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
#include "item_entity.h"
#include <SDL_mixer.h>  
#include "gfc_audio.h"   

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

//helper fucntion for loading screen:
void show_mission_briefing(LevelDefinition* level)
{
    if (!level) return;

    Sprite* briefingBG = gf2d_sprite_load_image("images/briefing_background.png");

    int done = 0;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE ||
                    event.key.keysym.sym == SDLK_RETURN) {
                    done = 1;
                }
            }
        }

        gfc_input_update();

        // Render briefing
        gf3d_vgraphics_render_start();

        if (briefingBG) {
            gf2d_sprite_draw_image(briefingBG, gfc_vector2d(0, 0));
        }

        // Mission title
        gf2d_font_draw_line_tag(level->title, FT_H1,
            GFC_COLOR_RED,  
            gfc_vector2d(400, 150));


        // Description 
        char line1[128], line2[128], line3[128], line4[128];
        strncpy(line1, level->description, 80);
        line1[80] = '\0';

        strncpy(line2, level->description + 80, 80);
        line2[80] = '\0';

        strncpy(line3, level->description + 160, 80);
        line3[80] = '\0';

        strncpy(line4, level->description + 240, 80);
        line4[80] = '\0';

        // Draw each line
        int startY = 300;
        int lineSpacing = 35;

        gf2d_font_draw_line_tag(line1, FT_H3, GFC_COLOR_WHITE, gfc_vector2d(150, startY));
        gf2d_font_draw_line_tag(line2, FT_H3, GFC_COLOR_WHITE, gfc_vector2d(150, startY + lineSpacing));
        gf2d_font_draw_line_tag(line3, FT_H3, GFC_COLOR_WHITE, gfc_vector2d(150, startY + lineSpacing * 2));
        gf2d_font_draw_line_tag(line4, FT_H3, GFC_COLOR_WHITE, gfc_vector2d(150, startY + lineSpacing * 3));

        // Prompt
        gf2d_font_draw_line_tag("Press SPACE to begin mission", FT_H3,
            GFC_COLOR_GREEN,
            gfc_vector2d(450, 600));

        gf3d_vgraphics_render_end();
        SDL_Delay(16);
    }
    if (briefingBG) {
        gf2d_sprite_free(briefingBG);
    }
}

//helper function menu
void show_stats_screen()
{
    PlayerStats* stats = data_get_player_stats();

    int done = 0;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE ||
                    event.key.keysym.sym == SDLK_SPACE) {
                    done = 1;
                }
            }
        }

        gfc_input_update();
        gf3d_vgraphics_render_start();

        // Title (centered)
        gf2d_font_draw_line_tag("=== PLAYER STATISTICS ===", FT_H1,
            GFC_COLOR_YELLOW, gfc_vector2d(350, 80));

        // Stats display (centered)
        char statText[128];
        int yPos = 180;
        int lineSpacing = 50;  // Increased from 40

        sprintf(statText, "Missions Completed: %d", stats->missionsCompleted);
        gf2d_font_draw_line_tag(statText, FT_H2, GFC_COLOR_WHITE, gfc_vector2d(400, yPos));
        yPos += lineSpacing;

        sprintf(statText, "Total Deaths: %d", stats->totalDeaths);
        gf2d_font_draw_line_tag(statText, FT_H2, GFC_COLOR_WHITE, gfc_vector2d(450, yPos));
        yPos += lineSpacing;

        sprintf(statText, "Total Kills: %d", stats->totalKills);
        gf2d_font_draw_line_tag(statText, FT_H2, GFC_COLOR_GREEN, gfc_vector2d(460, yPos));
        yPos += lineSpacing + 10;  // Extra space before breakdown

        // Enemy breakdown title (centered)
        gf2d_font_draw_line_tag("Enemy Kills Breakdown:", FT_H3,
            GFC_COLOR_CYAN, gfc_vector2d(420, yPos));
        yPos += 35;

        sprintf(statText, "Light Turrets: %d", stats->lightTurretKills);
        gf2d_font_draw_line_tag(statText, FT_H3, GFC_COLOR_WHITE, gfc_vector2d(460, yPos));
        yPos += 30;

        sprintf(statText, "Heavy Turrets: %d", stats->heavyTurretKills);
        gf2d_font_draw_line_tag(statText, FT_H3, GFC_COLOR_WHITE, gfc_vector2d(460, yPos));
        yPos += 30;

        sprintf(statText, "Fighters: %d", stats->fighterKills);
        gf2d_font_draw_line_tag(statText, FT_H3, GFC_COLOR_WHITE, gfc_vector2d(500, yPos));
        yPos += 30;

        sprintf(statText, "Bombers: %d", stats->bomberKills);
        gf2d_font_draw_line_tag(statText, FT_H3, GFC_COLOR_WHITE, gfc_vector2d(500, yPos));
        yPos += 30;

        sprintf(statText, "Interceptors: %d", stats->interceptorKills);
        gf2d_font_draw_line_tag(statText, FT_H3, GFC_COLOR_WHITE, gfc_vector2d(470, yPos));
        yPos += 50;  // Extra space after breakdown

        sprintf(statText, "Items Collected: %d", stats->itemsCollected);
        gf2d_font_draw_line_tag(statText, FT_H2, GFC_COLOR_WHITE, gfc_vector2d(420, yPos));
        yPos += lineSpacing;

        // Playtime (convert seconds to hours:minutes:seconds)
        int hours = (int)(stats->totalPlaytime / 3600.0f);
        int minutes = (int)((stats->totalPlaytime - hours * 3600) / 60.0f);
        int seconds = (int)(stats->totalPlaytime) % 60;
        sprintf(statText, "Total Playtime: %02d:%02d:%02d", hours, minutes, seconds);
        gf2d_font_draw_line_tag(statText, FT_H2, GFC_COLOR_WHITE, gfc_vector2d(400, yPos));

        // Back prompt (centered at bottom)
        gf2d_font_draw_line_tag("Press ESC or SPACE to return to menu", FT_H3,
            GFC_COLOR_GREEN, gfc_vector2d(350, 680));

        gf3d_vgraphics_render_end();
        SDL_Delay(16);
    }
}

int main(int argc, char* argv[])
{
    //local variables
    GFC_Matrix4 id, terrainMat;
    GFC_Vector3D spawnPos = gfc_vector3d(0, 0, 100);

    Uint32 sessionStartTime = 0;
    Uint32 sessionEndTime = 0;

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

    gfc_sound_init_config("config/audio.cfg");

    //entity system init
    entity_system_init(1000);
    data_load_enemy_definitions();  
    data_load_weapon_definitions(); 
    data_load_loadout_definitions();  
    data_load_item_definitions();  
    data_load_player_stats();   

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
    // === LOAD AND PLAY MENU MUSIC ===
    Mix_Music* menuMusic = Mix_LoadMUS("sounds/menu_song.mp3");
    if (menuMusic) {
        slog("Playing menu music");
        Mix_PlayMusic(menuMusic, -1);  // Loop forever
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
        else if (menuAction == MENU_ACTION_VIEW_STATS) {
            show_stats_screen();
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

    if (menuMusic) {
        Mix_HaltMusic();
        Mix_FreeMusic(menuMusic);
        slog("Menu music stopped");
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

    if (!currentLevel) {
        slog("Failed to load level!");
        _done = 1;
        return 0;
    }

    show_mission_briefing(currentLevel);    


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

    sessionStartTime = SDL_GetTicks();


    for (int i = 0; i < currentLevel->enemyCount; i++) {
        EnemyType type = enemy_type_from_string(currentLevel->enemies[i].enemyType);
        enemy_spawn(currentLevel->enemies[i].position, type, player);
    }
    for (int i = 0; i < currentLevel->itemCount; i++) {
        ItemType type = item_type_from_string(currentLevel->items[i].itemType);
        item_spawn(currentLevel->items[i].position, type);
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
        PlaneData* pdata = NULL;
        if (player && player->data) {
            pdata = (PlaneData*)player->data;

            if (pdata->health <= 0) {
                gf2d_font_draw_line_tag("GAME OVER", FT_H1, GFC_COLOR_RED, gfc_vector2d(500, 300));
                gf2d_font_draw_line_tag("Press ESC to quit", FT_H2, GFC_COLOR_WHITE, gfc_vector2d(450, 350));
            }
            else {
                // === LEFT SIDE - PLAYER STATUS ===

                // Health display
                sprintf(healthText, "HP: %d / %d", pdata->health, pdata->maxHealth);
                GFC_Color healthColor = pdata->health > 50 ? GFC_COLOR_GREEN :
                    pdata->health > 20 ? GFC_COLOR_YELLOW : GFC_COLOR_RED;
                gf2d_font_draw_line_tag(healthText, FT_H1, healthColor, gfc_vector2d(10, 10));

                // Shield display (blue, below health)
                if (pdata->shieldHP > 0) {
                    char shieldText[64];
                    sprintf(shieldText, "SHIELD: %.0f", pdata->shieldHP);
                    gf2d_font_draw_line_tag(shieldText, FT_H2,
                        gfc_color(0.0f, 0.7f, 1.0f, 1.0f),
                        gfc_vector2d(10, 45));
                }

                // === ACTIVE POWERUPS (fixed positions) ===
                if (pdata->speedBoostTimer > 0) {
                    char boostText[64];
                    sprintf(boostText, "SPEED: %.1fs", pdata->speedBoostTimer);
                    gf2d_font_draw_line_tag(boostText, FT_H3,
                        gfc_color(0.0f, 1.0f, 1.0f, 1.0f),
                        gfc_vector2d(10, 80));
                }

                if (pdata->invincibilityTimer > 0) {
                    char invText[64];
                    sprintf(invText, "INVINCIBLE: %.1fs", pdata->invincibilityTimer);
                    gf2d_font_draw_line_tag(invText, FT_H3,
                        gfc_color(1.0f, 1.0f, 0.0f, 1.0f),
                        gfc_vector2d(10, 105));
                }

                if (pdata->shrinkTimer > 0) {
                    char shrinkText[64];
                    sprintf(shrinkText, "TINY: %.1fs", pdata->shrinkTimer);
                    gf2d_font_draw_line_tag(shrinkText, FT_H3,
                        gfc_color(1.0f, 0.0f, 1.0f, 1.0f),
                        gfc_vector2d(10, 130));
                }

                if (pdata->shieldTimer > 0) {
                    char shieldTimeText[64];
                    sprintf(shieldTimeText, "SHIELD TIME: %.1fs", pdata->shieldTimer);
                    gf2d_font_draw_line_tag(shieldTimeText, FT_H3,
                        gfc_color(0.0f, 1.0f, 0.5f, 1.0f),
                        gfc_vector2d(10, 155));
                }

                // === RIGHT SIDE - MISSION OBJECTIVES ===

                // Mission 1: DESTROY ALL
                int enemiesLeft = 0;
                for (int i = 0; i < entity_get_max_count(); i++) {
                    Entity* e = entity_get_by_index(i);
                    if (!e) continue;
                    if (strcmp(e->name, "Light Turret") == 0 ||
                        strcmp(e->name, "Heavy Turret") == 0 ||
                        strcmp(e->name, "Fighter") == 0 ||
                        strcmp(e->name, "Bomber") == 0 ||
                        strcmp(e->name, "Interceptor") == 0) {
                        enemiesLeft++;
                    }
                }
                char enemyText[64];
                sprintf(enemyText, "ENEMIES: %d", enemiesLeft);
                gf2d_font_draw_line_tag(enemyText, FT_H1, GFC_COLOR_RED, gfc_vector2d(900, 10));

                // Mission 2: COLLECT OBJECTIVES
                int totalObjectives = 0;
                for (int i = 0; i < currentLevel->itemCount; i++) {
                    if (strcmp(currentLevel->items[i].itemType, "objective") == 0) {
                        totalObjectives++;
                    }
                }
                char objText[64];
                sprintf(objText, "OBJECTIVES: %d/%d", pdata->objectivesCollected, totalObjectives);
                gf2d_font_draw_line_tag(objText, FT_H1, GFC_COLOR_YELLOW, gfc_vector2d(850, 45));

                // Mission 3: SURVIVE
                static float survivalTime = 0.0f;
                float targetTime = 60.0f;
                survivalTime += 1.0f / 60.0f;
                float timeLeft = targetTime - survivalTime;

                char timeText[64];
                if (timeLeft > 0) {
                    sprintf(timeText, "SURVIVE: %.1fs", timeLeft);
                    gf2d_font_draw_line_tag(timeText, FT_H1, GFC_COLOR_CYAN, gfc_vector2d(870, 80));
                }
                else {
                    sprintf(timeText, "SURVIVED!");
                    gf2d_font_draw_line_tag(timeText, FT_H1, GFC_COLOR_GREEN, gfc_vector2d(870, 80));
                }

                // === CENTER - WIN CONDITIONS ===
                int allMissionsComplete = (enemiesLeft == 0) &&
                    (pdata->objectivesCollected >= totalObjectives && totalObjectives > 0) &&
                    (timeLeft <= 0);

                if (allMissionsComplete) {
                    stats_add_mission_complete();
                    gf2d_font_draw_line_tag("=== ALL MISSIONS COMPLETE ===", FT_H1,
                        GFC_COLOR_GREEN, gfc_vector2d(300, 300));
                }

                // === OUT OF BOUNDS WARNING ===
                if (pdata->isOutOfBounds) {
                    gf2d_font_draw_line_tag("!!! TURN AROUND - EDGE OF MAP !!!",
                        FT_H1, GFC_COLOR_RED, gfc_vector2d(300, 250));
                }
            }
        }   

        // === BOTTOM - CONTROLS ===
        gf2d_font_draw_line_tag("WS: PITCH | ARROWS: ROLL | A/D: YAW | SPACE: FIRE",
            FT_H3, GFC_COLOR_WHITE, gfc_vector2d(10, 690));

        // === CENTER - CROSSHAIR ===
        gf2d_font_draw_line_tag("+", FT_H1, GFC_COLOR_RED, gfc_vector2d(640, 360));

        gf2d_mouse_draw();
        gf3d_vgraphics_render_end();

        if (gfc_input_command_down("exit")) _done = 1;
        game_frame_delay();
    }

    sessionEndTime = SDL_GetTicks();
    float sessionTime = (sessionEndTime - sessionStartTime) / 1000.0f; // Convert to seconds
    PlayerStats* stats = data_get_player_stats();
    stats->totalPlaytime += sessionTime;
    data_save_player_stats();

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