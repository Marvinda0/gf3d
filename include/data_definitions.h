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
    char terrainPath[128];            // Terrain model path
    char missionType[32];             // "destroy_all", "destroy_objectives", "collect"
    GFC_Vector3D playerSpawn;         // Where player starts
    EnemySpawn* enemies;              // Array of enemy spawns
    int enemyCount;                   // Number of enemies
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
#endif