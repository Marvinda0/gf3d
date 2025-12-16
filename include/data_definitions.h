#ifndef __DATA_DEFINITIONS_H__
#define __DATA_DEFINITIONS_H__

#include "gfc_vector.h"
#include "gfc_color.h"
#include "enemy_entity.h"


typedef struct {
    char name[64];           // Display name
    int health;              // Starting health
    float speed;             // Movement speed (0 for stationary)
    float fireRate;          // Seconds between shots
    char weaponType[32];     // Weapon identifier (e.g., "machine_gun")
    char modelPath[128];     // Path to 3D model
    GFC_Vector3D scale;      // Model scale
    GFC_Color color;         // Tint color
} EnemyDefinition;



typedef struct {
    char name[64];           // Display name
    float speed;             // Projectile speed
    int damage;              // Damage per hit
    float lifetime;          // How long bullet lives (seconds)
    float fireRate;          // Cooldown between shots (seconds)
    int homing;              // 1 if homing, 0 otherwise
    float turnRate;          // Turn rate for homing (radians/sec)
    char modelPath[128];     // Path to projectile model
    GFC_Color color;         // Bullet color
} WeaponDefinition;


typedef struct {
    char enemyType[32];      // Enemy type name (e.g., "light_turret")
    GFC_Vector3D position;   // Spawn position
} EnemySpawn;

typedef enum {
    LOADOUT_SCOUT = 0,
    LOADOUT_TANK,
    LOADOUT_BALANCED,
    LOADOUT_COUNT
} LoadoutType;

typedef struct {
    char name[64];              // Display name (e.g., "Scout")
    char description[128];      // Description
    int health;                 // Starting health
    float maxSpeed;             // Maximum speed
    float minSpeed;             // Minimum speed
    float acceleration;         // Acceleration rate
    float pitchSensitivity;     // Pitch control sensitivity
    float yawSensitivity;       // Yaw control sensitivity
    float rollSensitivity;      // Roll control sensitivity
    char weaponTypes[3][32];    // Up to 3 weapon type names
    int weaponCount;            // Number of weapons
    char modelPath[128];        // Plane model path
    GFC_Vector3D scale;         // Model scale
    GFC_Color color;            // Plane color
} LoadoutDefinition;

typedef enum {
    ITEM_HEALTH_PACK = 0,
    ITEM_SPEED_BOOST,
    ITEM_WEAPON_UPGRADE,
    ITEM_SHIELD,
    ITEM_INVINCIBILITY,   
    ITEM_OBJECTIVE,
    ITEM_TYPE_COUNT
} ItemType;

// Item definition
typedef struct {
    char name[64];
    char type[32];        // "powerup" or "objective"
    char effect[32];      // "restore_health", "increase_speed", etc.
    int value;            // Amount to restore/add
    float duration;       // For temporary effects
    char modelPath[256];
    GFC_Vector3D scale;
    GFC_Color color;
    float rotationSpeed;
    float bobHeight;      // How high it bobs up/down
    float bobSpeed;       // How fast it bobs
} ItemDefinition;

typedef struct {
    char itemType[32];
    GFC_Vector3D position;
} ItemSpawn;


/**
 * @brief Load all enemy definitions from defs/enemies.json
 * Must be called during game initialization before spawning enemies
 */
void data_load_enemy_definitions();

/**
 * @brief Get enemy definition by type
 * @param type The enemy type enum
 * @return Pointer to enemy definition, or NULL if invalid type
 */
EnemyDefinition* data_get_enemy_def(EnemyType type);

/**
 * @brief Load all weapon definitions from defs/weapons.json
 */
void data_load_weapon_definitions();

/**
 * @brief Get weapon definition by type
 * @param type The weapon type enum
 * @return Pointer to weapon definition, or NULL if invalid
 */
WeaponDefinition* data_get_weapon_def(WeaponType type);

/**
 * @brief Complete level definition
 */
typedef struct {
    char name[64];                    // Level display name
    char title[128];                  // Level title
    char description[512];            // Level description
    char terrainPath[128];            // Terrain model path
    char missionType[32];             // "destroy_all", "destroy_objectives", "collect"
    GFC_Vector3D playerSpawn;         // Where player starts
    EnemySpawn* enemies;              // Array of enemy spawns
    int enemyCount;                   // Number of enemies
    ItemSpawn* items;                 
    int itemCount;                    
} LevelDefinition;

/**
 * @brief Load a level definition from file
 * @param filepath Path to level JSON file (e.g., "defs/levels/level1.json")
 * @return Pointer to loaded level, or NULL on failure. Must be freed with data_free_level()
 */
LevelDefinition* data_load_level(const char* filepath);

/**
 * @brief Free a loaded level definition
 * @param level The level to free
 */
void data_free_level(LevelDefinition* level);

/**
 * @brief Convert enemy type string to enum
 */
EnemyType enemy_type_from_string(const char* typeStr);

/**
 * @brief Load all loadout definitions from defs/loadouts.json
 */
void data_load_loadout_definitions();

/**
 * @brief Get loadout definition by type
 * @param type The loadout type enum
 * @return Pointer to loadout definition, or NULL if invalid
 */
LoadoutDefinition* data_get_loadout_def(LoadoutType type);

/**
 * @brief Load all item definitions from defs/items.def.txt
 */
void data_load_item_definitions();

/**
 * @brief Get item definition by type
 * @param type The item type enum
 * @return Pointer to item definition, or NULL if invalid
 */
ItemDefinition* data_get_item_def(ItemType type);

/**
 * @brief Convert item type string to enum
 * @param str String like "health_pack", "speed_boost", etc.
 * @return ItemType enum value
 */
ItemType item_type_from_string(const char* str);

/**
 * @brief Player statistics tracked across sessions
 */
typedef struct {
    int missionsCompleted;
    int totalDeaths;
    int lightTurretKills;
    int heavyTurretKills;
    int fighterKills;
    int bomberKills;
    int interceptorKills;
    int totalKills;
    int itemsCollected;
    float totalPlaytime;
} PlayerStats;

/**
 * @brief Load player stats from file
 */
void data_load_player_stats();

/**
 * @brief Save player stats to file
 */
void data_save_player_stats();

/**
 * @brief Get current player stats
 */
PlayerStats* data_get_player_stats();

/**
 * @brief Increment enemy kill count
 */
void stats_add_enemy_kill(EnemyType type);

/**
 * @brief Increment death count
 */
void stats_add_death();

/**
 * @brief Increment mission completed count
 */
void stats_add_mission_complete();

/**
 * @brief Increment items collected
 */
void stats_add_item_collected();
#endif