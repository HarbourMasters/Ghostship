#ifndef OBJECT_LIST_PROCESSOR_H
#define OBJECT_LIST_PROCESSOR_H

#include <libultra/types.h>

#include "area.h"
#include "macros.h"
#include "types.h"

/**
 * Flags for gTimeStopState. These control which objects are processed each frame
 * and also track some miscellaneous info.
 */
#define TIME_STOP_UNKNOWN_0         (1 << 0)
#define TIME_STOP_ENABLED           (1 << 1)
#define TIME_STOP_DIALOG            (1 << 2)
#define TIME_STOP_MARIO_AND_DOORS   (1 << 3)
#define TIME_STOP_ALL_OBJECTS       (1 << 4)
#define TIME_STOP_MARIO_OPENED_DOOR (1 << 5)
#define TIME_STOP_ACTIVE            (1 << 6)


/**
 * The maximum number of objects that can be loaded at once.
 */
#define OBJECT_POOL_CAPACITY 1024

/**
 * Every object is categorized into an object list, which controls the order
 * they are processed and which objects they can collide with.
 */
enum ObjectList {
    OBJ_LIST_PLAYER,      //  (0) Mario
    OBJ_LIST_UNUSED_1,    //  (1) (unused)
    OBJ_LIST_DESTRUCTIVE, //  (2) things that can be used to destroy other objects, like
                          //      bob-ombs and corkboxes
    OBJ_LIST_UNUSED_3,    //  (3) (unused)
    OBJ_LIST_GENACTOR,    //  (4) general actors. most normal 'enemies' or actors are
                          //      on this list. (MIPS, bullet bill, bully, etc)
    OBJ_LIST_PUSHABLE,    //  (5) pushable actors. This is a group of objects which
                          //      can push each other around as well as their parent
                          //      objects. (goombas, koopas, spinies)
    OBJ_LIST_LEVEL,       //  (6) level objects. general level objects such as heart, star
    OBJ_LIST_UNUSED_7,    //  (7) (unused)
    OBJ_LIST_DEFAULT,     //  (8) default objects. objects that didnt start with a 00
                          //      command are put here, so this is treated as a default.
    OBJ_LIST_SURFACE,     //  (9) surface objects. objects that specifically have surface
                          //      collision and not object collision. (thwomp, whomp, etc)
    OBJ_LIST_POLELIKE,    // (10) polelike objects. objects that attract or otherwise
                          //      "cling" Mario similar to a pole action. (hoot,
                          //      whirlpool, trees/poles, etc)
    OBJ_LIST_SPAWNER,     // (11) spawners
    OBJ_LIST_UNIMPORTANT, // (12) unimportant objects. objects that will not load
                          //      if there are not enough object slots: they will also
                          //      be manually unloaded to make room for slots if the list
                          //      gets exhausted.
    NUM_OBJ_LISTS
};

struct NumTimesCalled {
    /*0x00*/ s16 floor;
    /*0x02*/ s16 ceil;
    /*0x04*/ s16 wall;
};

extern_s struct ObjectNode gObjectListArray[];

extern_s s32 gDebugInfoFlags;
extern_s s32 gNumFindFloorMisses;
extern_s UNUSED s32 unused_8033BEF8;
extern_s s32 gUnknownWallCount;
extern_s u32 gObjectCounter;

extern_s struct NumTimesCalled gNumCalls;

extern_s s16 gDebugInfo[][8];
extern_s s16 gDebugInfoOverwrite[][8];

extern_s u32 gTimeStopState;
extern_s struct Object gObjectPool[];
extern_s struct Object gMacroObjectDefaultParent;
extern_s struct ObjectNode *gObjectLists;
extern_s struct ObjectNode gFreeObjectList;

extern_s struct Object *gMarioObject;
extern_s struct Object *gLuigiObject;
extern_s struct Object *gCurrentObject;

extern_s const BehaviorScript *gCurBhvCommand;
extern_s s16 gPrevFrameObjectCount;

extern_s s32 gSurfaceNodesAllocated;
extern_s s32 gSurfacesAllocated;
extern_s s32 gNumStaticSurfaceNodes;
extern_s s32 gNumStaticSurfaces;

extern_s struct MemoryPool *gObjectMemoryPool;

extern_s s16 gCheckingSurfaceCollisionsForCamera;
extern_s s16 gFindFloorIncludeSurfaceIntangible;
extern_s s16 *gEnvironmentRegions;
extern_s s32 gEnvironmentLevels[20];
extern_s s8 gDoorAdjacentRooms[60][2];
extern_s s16 gMarioCurrentRoom;
extern_s s16 D_8035FEE2;
extern_s s16 D_8035FEE4;
extern_s s16 gTHIWaterDrained;
extern_s s16 gTTCSpeedSetting;
extern_s s16 gMarioShotFromCannon;
extern_s s16 gCCMEnteredSlide;
extern_s s16 gNumRoomedObjectsInMarioRoom;
extern_s s16 gNumRoomedObjectsNotInMarioRoom;
extern_s s16 gWDWWaterLevelChanging;
extern_s s16 gMarioOnMerryGoRound;


extern_s void bhv_mario_update(void);
extern_s void set_object_respawn_info_bits(struct Object *obj, u8 bits);
extern_s void unload_objects_from_area(UNUSED s32 unused, s32 areaIndex);
extern_s void spawn_objects_from_info(UNUSED s32 unused, struct SpawnInfo *spawnInfo);
extern_s void clear_objects(void);
extern_s void update_objects(UNUSED s32 unused);

#endif // OBJECT_LIST_PROCESSOR_H
