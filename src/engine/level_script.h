#ifndef LEVEL_SCRIPT_H
#define LEVEL_SCRIPT_H

#include <libultra/types.h>

struct LevelCommand {
    /*00*/ uint8_t type;
    /*01*/ uint8_t size;
    /*02*/ // variable sized argument data
};

extern_s const LevelScript level_script_entry[];
extern_s struct AllocOnlyPool *sLevelPool;

extern_s struct LevelCommand *level_script_execute(struct LevelCommand *cmd);
extern_s void level_script_reset(void);

#endif // LEVEL_SCRIPT_H
