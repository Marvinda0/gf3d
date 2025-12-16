#include "data_definitions.h"
#include "simple_json.h"
#include "simple_logger.h"
#include <string.h>

// Global storage 
EnemyDefinition enemyDefs[ENEMY_TYPE_COUNT];

WeaponDefinition weaponDefs[WEAPON_COUNT];

LoadoutDefinition loadoutDefs[LOADOUT_COUNT];

ItemDefinition itemDefs[ITEM_TYPE_COUNT];

/**
 * @brief Parse a color array from JSON [r, g, b, a]
 */
GFC_Color parse_color_array(SJson* colorArray) {
    GFC_Color color;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;


    if (!colorArray || !sj_is_array(colorArray)) {
        slog("Warning: Invalid color array, using white");
    }

    int count = sj_array_get_count(colorArray);
    if (count >= 3) {
        SJson* rVal = sj_array_get_nth(colorArray, 0);
        SJson* gVal = sj_array_get_nth(colorArray, 1);
        SJson* bVal = sj_array_get_nth(colorArray, 2);

        sj_get_float_value(rVal, &r);
        sj_get_float_value(gVal, &g);
        sj_get_float_value(bVal, &b);

        if (count >= 4) {
            SJson* aVal = sj_array_get_nth(colorArray, 3);
            sj_get_float_value(aVal, &a);
        }
        else {
            color.a = 1.0f;
        }
    }
    color = gfc_color(r, g, b, a);
    return color;
}

/**
 * @brief Parse a scale array from JSON [x, y, z]
 */
GFC_Vector3D parse_scale_array(SJson* scaleArray) {
    GFC_Vector3D scale = gfc_vector3d(1.0f, 1.0f, 1.0f);

    if (!scaleArray || !sj_is_array(scaleArray)) {
        slog("Warning: Invalid scale array, using 1,1,1");
        return scale;
    }

    int count = sj_array_get_count(scaleArray);
    if (count >= 3) {
        SJson* x = sj_array_get_nth(scaleArray, 0);
        SJson* y = sj_array_get_nth(scaleArray, 1);
        SJson* z = sj_array_get_nth(scaleArray, 2);

        sj_get_float_value(x, &scale.x);
        sj_get_float_value(y, &scale.y);
        sj_get_float_value(z, &scale.z);
    }

    return scale;
}

void parse_enemy_definition(SJson* enemyObj, EnemyDefinition* def) {
    const char* name = sj_object_get_string(enemyObj, "name");
    if (name) {
        strncpy(def->name, name, 63);
        def->name[63] = '\0';
    }

    sj_object_get_int(enemyObj, "health", &def->health);
    sj_object_get_float(enemyObj, "speed", &def->speed);
    sj_object_get_float(enemyObj, "fireRate", &def->fireRate);

    const char* weapon = sj_object_get_string(enemyObj, "weapon");
    if (weapon) {
        strncpy(def->weaponType, weapon, 31);
        def->weaponType[31] = '\0';
    }

    const char* model = sj_object_get_string(enemyObj, "model");
    if (model) {
        strncpy(def->modelPath, model, 127);
        def->modelPath[127] = '\0';
    }

    // Parse arrays
    SJson* scaleArray = sj_object_get_value(enemyObj, "scale");
    if (scaleArray) {
        def->scale = parse_scale_array(scaleArray);
    }

    SJson* colorArray = sj_object_get_value(enemyObj, "color");
    if (colorArray) {
        def->color = parse_color_array(colorArray);
    }
}

void data_load_enemy_definitions() {
    slog("Loading enemy definitions from defs/enemies.def...");

    // Initialize all definitions to defaults
    memset(enemyDefs, 0, sizeof(enemyDefs));

    // Load JSON file
    SJson* root = sj_load("defs/enemies.def.txt");
    if (!root) {
        slog("ERROR: Failed to load defs/enemies.def");
        return;
    }

    if (!sj_is_object(root)) {
        slog("ERROR: enemies.def root is not an object");
        sj_free(root);
        return;
    }

    // Parse each enemy type
    SJson* lightTurret = sj_object_get_value(root, "light_turret");
    if (lightTurret) {
        parse_enemy_definition(lightTurret, &enemyDefs[ENEMY_LIGHT_TURRET]);
        slog("Loaded: %s", enemyDefs[ENEMY_LIGHT_TURRET].name);
    }

    SJson* heavyTurret = sj_object_get_value(root, "heavy_turret");
    if (heavyTurret) {
        parse_enemy_definition(heavyTurret, &enemyDefs[ENEMY_HEAVY_TURRET]);
        slog("Loaded: %s", enemyDefs[ENEMY_HEAVY_TURRET].name);
    }

    SJson* fighter = sj_object_get_value(root, "fighter");
    if (fighter) {
        parse_enemy_definition(fighter, &enemyDefs[ENEMY_FIGHTER]);
        slog("Loaded: %s", enemyDefs[ENEMY_FIGHTER].name);
    }

    SJson* bomber = sj_object_get_value(root, "bomber");
    if (bomber) {
        parse_enemy_definition(bomber, &enemyDefs[ENEMY_BOMBER]);
        slog("Loaded: %s", enemyDefs[ENEMY_BOMBER].name);
    }

    SJson* interceptor = sj_object_get_value(root, "interceptor");
    if (interceptor) {
        parse_enemy_definition(interceptor, &enemyDefs[ENEMY_INTERCEPTOR]);
        slog("Loaded: %s", enemyDefs[ENEMY_INTERCEPTOR].name);
    }

    sj_free(root);
    slog("Enemy definitions loaded successfully!");
}

EnemyDefinition* data_get_enemy_def(EnemyType type) {
    if (type < 0 || type >= ENEMY_TYPE_COUNT) {
        slog("ERROR: Invalid enemy type %d", type);
        return NULL;
    }
    return &enemyDefs[type];
}

/**
 * @brief Parse a single weapon definition from JSON object
 */
void parse_weapon_definition(SJson* weaponObj, WeaponDefinition* def) {
    const char* name = sj_object_get_string(weaponObj, "name");
    if (name) {
        strncpy(def->name, name, 63);
        def->name[63] = '\0';
    }

    sj_object_get_float(weaponObj, "speed", &def->speed);
    sj_object_get_int(weaponObj, "damage", &def->damage);
    sj_object_get_float(weaponObj, "lifetime", &def->lifetime);
    sj_object_get_float(weaponObj, "fireRate", &def->fireRate);
    sj_object_get_float(weaponObj, "turnRate", &def->turnRate);

    // Parse boolean
    short int homing = 0;
    sj_object_get_bool(weaponObj, "homing", &homing);
    def->homing = homing ? 1 : 0;

    const char* model = sj_object_get_string(weaponObj, "model");
    if (model) {
        strncpy(def->modelPath, model, 127);
        def->modelPath[127] = '\0';
    }

    SJson* colorArray = sj_object_get_value(weaponObj, "color");
    if (colorArray) {
        def->color = parse_color_array(colorArray);
    }
}

void data_load_weapon_definitions() {
    slog("Loading weapon definitions from defs/weapons.def...");

    memset(weaponDefs, 0, sizeof(weaponDefs));

    SJson* root = sj_load("defs/weapons.def.txt");
    if (!root) {
        slog("ERROR: Failed to load defs/weapons.def");
        return;
    }

    if (!sj_is_object(root)) {
        slog("ERROR: weapons.def root is not an object");
        sj_free(root);
        return;
    }

    // Parse each weapon type
    SJson* machineGun = sj_object_get_value(root, "machine_gun");
    if (machineGun) {
        parse_weapon_definition(machineGun, &weaponDefs[WEAPON_MACHINE_GUN]);
        slog("Loaded: %s", weaponDefs[WEAPON_MACHINE_GUN].name);
    }

    SJson* missile = sj_object_get_value(root, "missile");
    if (missile) {
        parse_weapon_definition(missile, &weaponDefs[WEAPON_MISSILE]);
        slog("Loaded: %s", weaponDefs[WEAPON_MISSILE].name);
    }

    SJson* homing = sj_object_get_value(root, "homing");
    if (homing) {
        parse_weapon_definition(homing, &weaponDefs[WEAPON_HOMING]);
        slog("Loaded: %s", weaponDefs[WEAPON_HOMING].name);
    }

    sj_free(root);
    slog("Weapon definitions loaded successfully!");
}

WeaponDefinition* data_get_weapon_def(WeaponType type) {
    if (type < 0 || type >= WEAPON_COUNT) {
        slog("ERROR: Invalid weapon type %d", type);
        return NULL;
    }
    return &weaponDefs[type];
}

LevelDefinition* data_load_level(const char* filepath) {
    slog("Loading level from %s...", filepath);

    SJson* root = sj_load(filepath);
    if (!root) {
        slog("ERROR: Failed to load %s", filepath);
        return NULL;
    }

    if (!sj_is_object(root)) {
        slog("ERROR: Level file root is not an object");
        sj_free(root);
        return NULL;
    }

    // Allocate level
    LevelDefinition* level = (LevelDefinition*)malloc(sizeof(LevelDefinition));
    if (!level) {
        slog("ERROR: Failed to allocate level");
        sj_free(root);
        return NULL;
    }
    memset(level, 0, sizeof(LevelDefinition));

    // Parse basic info
    const char* name = sj_object_get_string(root, "name");
    if (name) {
        strncpy(level->name, name, 63);
        level->name[63] = '\0';
    }

    const char* title = sj_object_get_string(root, "title");
    if (title) {
        strncpy(level->title, title, 127);
        level->title[127] = '\0';
    }

    const char* desc = sj_object_get_string(root, "description");
    if (desc) {
        strncpy(level->description, desc, 511);
        level->description[511] = '\0';
    }

    const char* terrain = sj_object_get_string(root, "terrain");
    if (terrain) {
        strncpy(level->terrainPath, terrain, 127);
        level->terrainPath[127] = '\0';
    }

    const char* missionType = sj_object_get_string(root, "missionType");
    if (missionType) {
        strncpy(level->missionType, missionType, 31);
        level->missionType[31] = '\0';
    }

    // Parse player spawn
    SJson* playerSpawn = sj_object_get_value(root, "playerSpawn");
    if (playerSpawn && sj_is_array(playerSpawn)) {
        level->playerSpawn = parse_scale_array(playerSpawn); // Reuse parse function
    }
    else {
        level->playerSpawn = gfc_vector3d(0, 0, 100);
    }

    // Parse enemies array
    SJson* enemiesArray = sj_object_get_value(root, "enemies");
    if (enemiesArray && sj_is_array(enemiesArray)) {
        level->enemyCount = sj_array_get_count(enemiesArray);

        if (level->enemyCount > 0) {
            level->enemies = (EnemySpawn*)malloc(sizeof(EnemySpawn) * level->enemyCount);
            if (!level->enemies) {
                slog("ERROR: Failed to allocate enemy array");
                free(level);
                sj_free(root);
                return NULL;
            }

            // Parse each enemy
            for (int i = 0; i < level->enemyCount; i++) {
                SJson* enemyObj = sj_array_get_nth(enemiesArray, i);
                if (!enemyObj) continue;

                const char* type = sj_object_get_string(enemyObj, "type");
                if (type) {
                    strncpy(level->enemies[i].enemyType, type, 31);
                    level->enemies[i].enemyType[31] = '\0';
                }

                SJson* posArray = sj_object_get_value(enemyObj, "position");
                if (posArray && sj_is_array(posArray)) {
                    level->enemies[i].position = parse_scale_array(posArray);
                }
            }
        }
    }
    SJson* itemsArray = sj_object_get_value(root, "items");
    if (itemsArray && sj_is_array(itemsArray)) {
        level->itemCount = sj_array_get_count(itemsArray);

        if (level->itemCount > 0) {
            level->items = (ItemSpawn*)malloc(sizeof(ItemSpawn) * level->itemCount);
            if (!level->items) {
                slog("ERROR: Failed to allocate items array");
                if (level->enemies) free(level->enemies);
                free(level);
                sj_free(root);
                return NULL;
            }

            // Parse each item
            for (int i = 0; i < level->itemCount; i++) {
                SJson* itemObj = sj_array_get_nth(itemsArray, i);
                if (!itemObj) continue;

                const char* type = sj_object_get_string(itemObj, "type");
                if (type) {
                    strncpy(level->items[i].itemType, type, 31);
                    level->items[i].itemType[31] = '\0';
                }

                SJson* posArray = sj_object_get_value(itemObj, "position");
                if (posArray && sj_is_array(posArray)) {
                    level->items[i].position = parse_scale_array(posArray);
                }
            }
            slog("Loaded %d items", level->itemCount);
        }
    }
    else {
        level->itemCount = 0;
        level->items = NULL;
    }

    sj_free(root);
    slog("Level '%s' loaded: %d enemies, %d items", level->name, level->enemyCount, level->itemCount);

    return level;
}

void data_free_level(LevelDefinition* level) {
    if (!level) return;

    if (level->enemies) {
        free(level->enemies);
    }
    if(level->items) {
        free(level->items);
    }

    free(level);
}

EnemyType enemy_type_from_string(const char* typeStr) {
    if (strcmp(typeStr, "light_turret") == 0) return ENEMY_LIGHT_TURRET;
    if (strcmp(typeStr, "heavy_turret") == 0) return ENEMY_HEAVY_TURRET;
    if (strcmp(typeStr, "fighter") == 0) return ENEMY_FIGHTER;
    if (strcmp(typeStr, "bomber") == 0) return ENEMY_BOMBER;
    if (strcmp(typeStr, "interceptor") == 0) return ENEMY_INTERCEPTOR;

    slog("WARNING: Unknown enemy type '%s', defaulting to light_turret", typeStr);
    return ENEMY_LIGHT_TURRET;
}

void parse_loadout_definition(SJson* loadoutObj, LoadoutDefinition* def) {
    const char* name = sj_object_get_string(loadoutObj, "name");
    if (name) {
        strncpy(def->name, name, 63);
        def->name[63] = '\0';
    }

    const char* description = sj_object_get_string(loadoutObj, "description");
    if (description) {
        strncpy(def->description, description, 127);
        def->description[127] = '\0';
    }

    sj_object_get_int(loadoutObj, "health", &def->health);
    sj_object_get_float(loadoutObj, "maxSpeed", &def->maxSpeed);
    sj_object_get_float(loadoutObj, "minSpeed", &def->minSpeed);
    sj_object_get_float(loadoutObj, "acceleration", &def->acceleration);
    sj_object_get_float(loadoutObj, "pitchSensitivity", &def->pitchSensitivity);
    sj_object_get_float(loadoutObj, "yawSensitivity", &def->yawSensitivity);
    sj_object_get_float(loadoutObj, "rollSensitivity", &def->rollSensitivity);

    const char* model = sj_object_get_string(loadoutObj, "model");
    if (model) {
        strncpy(def->modelPath, model, 127);
        def->modelPath[127] = '\0';
    }

    // Parse arrays
    SJson* scaleArray = sj_object_get_value(loadoutObj, "scale");
    if (scaleArray) {
        def->scale = parse_scale_array(scaleArray);
    }

    SJson* colorArray = sj_object_get_value(loadoutObj, "color");
    if (colorArray) {
        def->color = parse_color_array(colorArray);
    }

    // Parse weapons array
    SJson* weaponsArray = sj_object_get_value(loadoutObj, "weapons");
    if (weaponsArray && sj_is_array(weaponsArray)) {
        def->weaponCount = sj_array_get_count(weaponsArray);
        if (def->weaponCount > 3) def->weaponCount = 3;

        for (int i = 0; i < def->weaponCount; i++) {
            SJson* weaponStr = sj_array_get_nth(weaponsArray, i);
            if (weaponStr && sj_is_string(weaponStr)) {
                const char* weaponName = sj_get_string_value(weaponStr);
                if (weaponName) {
                    strncpy(def->weaponTypes[i], weaponName, 31);
                    def->weaponTypes[i][31] = '\0';
                }
            }
        }
    }
}

void data_load_loadout_definitions() {
    slog("Loading loadout definitions from defs/loadouts.json...");

    memset(loadoutDefs, 0, sizeof(loadoutDefs));

    SJson* root = sj_load("defs/loadouts.def.txt");
    if (!root) {
        slog("ERROR: Failed to load defs/loadouts.json");
        return;
    }

    if (!sj_is_object(root)) {
        slog("ERROR: loadouts.json root is not an object");
        sj_free(root);
        return;
    }

    // Parse each loadout type
    SJson* scout = sj_object_get_value(root, "scout");
    if (scout) {
        parse_loadout_definition(scout, &loadoutDefs[LOADOUT_SCOUT]);
        slog("Loaded loadout: %s - %s", loadoutDefs[LOADOUT_SCOUT].name, loadoutDefs[LOADOUT_SCOUT].description);
    }

    SJson* tank = sj_object_get_value(root, "tank");
    if (tank) {
        parse_loadout_definition(tank, &loadoutDefs[LOADOUT_TANK]);
        slog("Loaded loadout: %s - %s", loadoutDefs[LOADOUT_TANK].name, loadoutDefs[LOADOUT_TANK].description);
    }

    SJson* balanced = sj_object_get_value(root, "balanced");
    if (balanced) {
        parse_loadout_definition(balanced, &loadoutDefs[LOADOUT_BALANCED]);
        slog("Loaded loadout: %s - %s", loadoutDefs[LOADOUT_BALANCED].name, loadoutDefs[LOADOUT_BALANCED].description);
    }

    sj_free(root);
    slog("Loadout definitions loaded successfully!");
}

LoadoutDefinition* data_get_loadout_def(LoadoutType type) {
    if (type < 0 || type >= LOADOUT_COUNT) {
        slog("ERROR: Invalid loadout type %d", type);
        return NULL;
    }
    return &loadoutDefs[type];
}

WeaponType weapon_type_from_string(const char* typeStr) {
    if (strcmp(typeStr, "machine_gun") == 0) return WEAPON_MACHINE_GUN;
    if (strcmp(typeStr, "missile") == 0) return WEAPON_MISSILE;
    if (strcmp(typeStr, "homing") == 0) return WEAPON_HOMING;

    slog("WARNING: Unknown weapon type '%s', returning WEAPON_NONE", typeStr);
    return WEAPON_NONE;  
}
void data_load_item_definitions() {
    SJson* root = sj_load("defs/items.def.txt");
    if (!root) {
        slog("Failed to load items.def.txt");
        return;
    }

    const char* itemNames[] = {
        "health_pack",
        "speed_boost",
        "weapon_upgrade",
        "shield",
        "invincibility",
        "objective"
    };

    for (int i = 0; i < ITEM_TYPE_COUNT; i++) {
        SJson* itemObj = sj_object_get_value(root, itemNames[i]);
        if (!itemObj) {
            slog("Warning: Item '%s' not found in JSON", itemNames[i]);
            continue;
        }

        ItemDefinition* def = &itemDefs[i];

        const char* name = sj_object_get_string(itemObj, "name");
        if (name) gfc_line_cpy(def->name, name);

        const char* type = sj_object_get_string(itemObj, "type");
        if (type) strncpy(def->type, type, 31);

        const char* effect = sj_object_get_string(itemObj, "effect");
        if (effect) strncpy(def->effect, effect, 31);

        sj_get_integer_value(sj_object_get_value(itemObj, "value"), &def->value);
        sj_get_float_value(sj_object_get_value(itemObj, "duration"), &def->duration);

        const char* model = sj_object_get_string(itemObj, "model");
        if (model) gfc_line_cpy(def->modelPath, model);

        sj_object_get_vector3d(itemObj, "scale", &def->scale);

        SJson* colorArray = sj_object_get_value(itemObj, "color");
        def->color = parse_color_array(colorArray);

        sj_get_float_value(sj_object_get_value(itemObj, "rotationSpeed"), &def->rotationSpeed);
        sj_get_float_value(sj_object_get_value(itemObj, "bobHeight"), &def->bobHeight);
        sj_get_float_value(sj_object_get_value(itemObj, "bobSpeed"), &def->bobSpeed);

        slog("Loaded item: %s", def->name);
    }

    sj_free(root);
}

ItemDefinition* data_get_item_def(ItemType type) {
    if (type < 0 || type >= ITEM_TYPE_COUNT) return NULL;
    return &itemDefs[type];
}

ItemType item_type_from_string(const char* str) {
    if (!str) return ITEM_HEALTH_PACK;
    if (strcmp(str, "health_pack") == 0) return ITEM_HEALTH_PACK;
    if (strcmp(str, "speed_boost") == 0) return ITEM_SPEED_BOOST;
    if (strcmp(str, "weapon_upgrade") == 0) return ITEM_WEAPON_UPGRADE;
    if (strcmp(str, "shield") == 0) return ITEM_SHIELD;
    if (strcmp(str, "invincibility") == 0) return ITEM_INVINCIBILITY;  // ADD THIS LINE
    if (strcmp(str, "objective") == 0) return ITEM_OBJECTIVE;
    return ITEM_HEALTH_PACK;
}