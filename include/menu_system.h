#ifndef __MENU_SYSTEM_H__
#define __MENU_SYSTEM_H__

#include "gfc_types.h"
#include "gfc_vector.h"
#include "gfc_color.h"

/**
 * @brief Menu action types
 */
typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_START_GAME,
    MENU_ACTION_LEVEL_SELECT,
    MENU_ACTION_LOAD_LEVEL_1,
    MENU_ACTION_LOAD_LEVEL_2,
    MENU_ACTION_LOAD_LEVEL_3,
    MENU_ACTION_OPTIONS,
    MENU_ACTION_ACHIEVEMENTS,
    MENU_ACTION_LEVEL_EDITOR,
    MENU_ACTION_VIEW_STATS,
    MENU_ACTION_BACK,
    MENU_ACTION_QUIT
} MenuAction;

/**
 * @brief Single menu item
 */
typedef struct {
    char text[128];
    MenuAction action;
} MenuItem;

/**
 * @brief Menu screen
 */
typedef struct {
    char title[128];
    char subtitle[256];
    MenuItem* items;
    int itemCount;
    int selectedIndex;
} MenuScreen;

/**
 * @brief Menu system state
 */
typedef struct {
    MenuScreen* currentScreen;
    MenuScreen* previousScreen;
    MenuAction lastAction;
    int isActive;
} MenuSystem;

/**
 * @brief Initialize menu system
 * @param menuFile Path to menu JSON file
 * @return 1 on success, 0 on failure
 */
int menu_system_init(const char* menuFile);

/**
 * @brief Close menu system and free resources
 */
void menu_system_close();

/**
 * @brief Update menu (handle input)
 * @return Current menu action
 */
MenuAction menu_system_update();

/**
 * @brief Draw the current menu
 */
void menu_system_draw();

/**
 * @brief Navigate to a specific menu by name
 * @param menuName Name of menu (e.g., "main", "level_select")
 */
void menu_system_goto(const char* menuName);

/**
 * @brief Go back to previous menu
 */
void menu_system_back();

/**
 * @brief Check if menu system is active
 */
int menu_system_is_active();

/**
 * @brief Set menu system active state
 */
void menu_system_set_active(int active);

#endif