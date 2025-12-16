#include "menu_system.h"
#include "simple_json.h"
#include "simple_logger.h"
#include "gfc_input.h"
#include "gf2d_font.h"
#include "gf2d_sprite.h"
#include <string.h>
#include <SDL.h>

#define MAX_MENUS 10

typedef struct {
    char name[64];
    MenuScreen screen;
} NamedMenu;

static struct {
    NamedMenu menus[MAX_MENUS];
    int menuCount;
    MenuSystem state;
    int initialized;
} menu_manager = { 0 };

// Helper: Convert action string to enum
MenuAction menu_action_from_string(const char* actionStr) {
    if (!actionStr) return MENU_ACTION_NONE;
    if (strcmp(actionStr, "start_game") == 0) return MENU_ACTION_START_GAME;
    if (strcmp(actionStr, "level_select") == 0) return MENU_ACTION_LEVEL_SELECT;
    if (strcmp(actionStr, "load_level_1") == 0) return MENU_ACTION_LOAD_LEVEL_1;
    if (strcmp(actionStr, "load_level_2") == 0) return MENU_ACTION_LOAD_LEVEL_2;
    if (strcmp(actionStr, "load_level_3") == 0) return MENU_ACTION_LOAD_LEVEL_3;
    if (strcmp(actionStr, "options") == 0) return MENU_ACTION_OPTIONS;
    if (strcmp(actionStr, "achievements") == 0) return MENU_ACTION_ACHIEVEMENTS;
    if (strcmp(actionStr, "level_editor") == 0) return MENU_ACTION_LEVEL_EDITOR;
    if (strcmp(actionStr, "view_stats") == 0) return MENU_ACTION_VIEW_STATS;  
    if (strcmp(actionStr, "back") == 0) return MENU_ACTION_BACK;
    if (strcmp(actionStr, "quit") == 0) return MENU_ACTION_QUIT;
    return MENU_ACTION_NONE;
}

// Parse a single menu from JSON
int parse_menu(const char* menuName, SJson* menuObj, MenuScreen* screen) {
    if (!menuObj || !screen) return 0;

    // Parse title
    const char* title = sj_object_get_string(menuObj, "title");
    if (title) {
        strncpy(screen->title, title, 127);
        screen->title[127] = '\0';
    }

    // Parse subtitle
    const char* subtitle = sj_object_get_string(menuObj, "subtitle");
    if (subtitle) {
        strncpy(screen->subtitle, subtitle, 255);
        screen->subtitle[255] = '\0';
    }

    // Parse items
    SJson* itemsArray = sj_object_get_value(menuObj, "items");
    if (!itemsArray || !sj_is_array(itemsArray)) return 0;

    screen->itemCount = sj_array_get_count(itemsArray);
    screen->items = (MenuItem*)malloc(sizeof(MenuItem) * screen->itemCount);
    if (!screen->items) return 0;

    for (int i = 0; i < screen->itemCount; i++) {
        SJson* itemObj = sj_array_get_nth(itemsArray, i);
        if (!itemObj) continue;

        const char* text = sj_object_get_string(itemObj, "text");
        if (text) {
            strncpy(screen->items[i].text, text, 127);
            screen->items[i].text[127] = '\0';
        }

        const char* action = sj_object_get_string(itemObj, "action");
        screen->items[i].action = menu_action_from_string(action);
    }

    screen->selectedIndex = 0;
    return 1;
}

int menu_system_init(const char* menuFile) {
    slog("Initializing menu system from %s", menuFile);

    memset(&menu_manager, 0, sizeof(menu_manager));

    SJson* root = sj_load(menuFile);
    if (!root) {
        slog("ERROR: Failed to load menu file: %s", menuFile);
        return 0;
    }

    if (!sj_is_object(root)) {
        slog("ERROR: Menu file root is not an object");
        sj_free(root);
        return 0;
    }

    // Parse each menu
    const char* menuNames[] = { "main", "level_select", "options", "achievements", "level_editor" };
    int numMenus = sizeof(menuNames) / sizeof(menuNames[0]);

    for (int i = 0; i < numMenus && menu_manager.menuCount < MAX_MENUS; i++) {
        SJson* menuObj = sj_object_get_value(root, menuNames[i]);
        if (!menuObj) continue;

        strncpy(menu_manager.menus[menu_manager.menuCount].name, menuNames[i], 63);
        if (parse_menu(menuNames[i], menuObj, &menu_manager.menus[menu_manager.menuCount].screen)) {
            slog("Loaded menu: %s", menuNames[i]);
            menu_manager.menuCount++;
        }
    }

    sj_free(root);

    // Set initial menu to "main"
    menu_system_goto("main");
    menu_manager.state.isActive = 1;
    menu_manager.initialized = 1;

    slog("Menu system initialized with %d menus", menu_manager.menuCount);
    return 1;
}

void menu_system_close() {
    for (int i = 0; i < menu_manager.menuCount; i++) {
        if (menu_manager.menus[i].screen.items) {
            free(menu_manager.menus[i].screen.items);
        }
    }
    memset(&menu_manager, 0, sizeof(menu_manager));
    slog("Menu system closed");
}

void menu_system_goto(const char* menuName) {
    if (!menuName) return;

    for (int i = 0; i < menu_manager.menuCount; i++) {
        if (strcmp(menu_manager.menus[i].name, menuName) == 0) {
            menu_manager.state.previousScreen = menu_manager.state.currentScreen;
            menu_manager.state.currentScreen = &menu_manager.menus[i].screen;
            menu_manager.state.currentScreen->selectedIndex = 0;
            slog("Navigated to menu: %s", menuName);
            return;
        }
    }
    slog("WARNING: Menu '%s' not found", menuName);
}

void menu_system_back() {
    if (menu_manager.state.previousScreen) {
        MenuScreen* temp = menu_manager.state.currentScreen;
        menu_manager.state.currentScreen = menu_manager.state.previousScreen;
        menu_manager.state.previousScreen = temp;
    }
    else {
        // No previous menu, go to main
        menu_system_goto("main");
    }
}

MenuAction menu_system_update() {
    if (!menu_manager.state.isActive || !menu_manager.state.currentScreen) {
        return MENU_ACTION_NONE;
    }

    MenuScreen* screen = menu_manager.state.currentScreen;
    MenuAction action = MENU_ACTION_NONE;

    // Handle input
    if (gfc_input_command_pressed("pitch_up")) {
        screen->selectedIndex--;
        if (screen->selectedIndex < 0) {
            screen->selectedIndex = screen->itemCount - 1;
        }
    }

    if (gfc_input_command_pressed("pitch_down")) {
        screen->selectedIndex++;
        if (screen->selectedIndex >= screen->itemCount) {
            screen->selectedIndex = 0;
        }
    }

    if (gfc_input_command_pressed("enter") || gfc_input_command_pressed("space")) {
        action = screen->items[screen->selectedIndex].action;
        menu_manager.state.lastAction = action;

        // Handle navigation actions
        switch (action) {
        case MENU_ACTION_LEVEL_SELECT:
            menu_system_goto("level_select");
            return MENU_ACTION_NONE; // Don't return yet, stay in menu
        case MENU_ACTION_OPTIONS:
            menu_system_goto("options");
            return MENU_ACTION_NONE;
        case MENU_ACTION_ACHIEVEMENTS:
            menu_system_goto("achievements");
            return MENU_ACTION_NONE;
        case MENU_ACTION_LEVEL_EDITOR:
            menu_system_goto("level_editor");
            return MENU_ACTION_NONE;
        case MENU_ACTION_BACK:
            menu_system_back();
            return MENU_ACTION_NONE;
        default:
            break;
        }

        return action; // Return the action for game.c to handle
    }

    if (gfc_input_command_pressed("exit")) {
        menu_system_back();
    }

    return MENU_ACTION_NONE;
}

void menu_system_draw() {
    if (!menu_manager.state.isActive || !menu_manager.state.currentScreen) return;

    static Sprite* bgSprite = NULL;
    if (!bgSprite) {
        bgSprite = gf2d_sprite_load("images/bg_flat.png", 1280, 720, 1);
    }
    if (bgSprite) {
        gf2d_sprite_draw_image(bgSprite, gfc_vector2d(0, 0));
    }


    MenuScreen* screen = menu_manager.state.currentScreen;

    // Draw title
    gf2d_font_draw_line_tag(screen->title, FT_H1, GFC_COLOR_BLACK,
        gfc_vector2d(400, 100));

    // Draw subtitle
    gf2d_font_draw_line_tag(screen->subtitle, FT_H3, GFC_COLOR_YELLOW,
        gfc_vector2d(350, 150));

    // Draw menu items
    for (int i = 0; i < screen->itemCount; i++) {
        GFC_Color color = (i == screen->selectedIndex) ? GFC_COLOR_RED : GFC_COLOR_WHITE;
        float y = 250 + (i * 50);

        // Draw selection indicator
        if (i == screen->selectedIndex) {
            gf2d_font_draw_line_tag(">", FT_H2, GFC_COLOR_RED,
                gfc_vector2d(400, y));
        }

        // Draw item text
        gf2d_font_draw_line_tag(screen->items[i].text, FT_H2, color,
            gfc_vector2d(450, y));
    }
}

int menu_system_is_active() {
    return menu_manager.state.isActive;
}

void menu_system_set_active(int active) {
    menu_manager.state.isActive = active;
}